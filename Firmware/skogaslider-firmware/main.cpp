/**
 * @file main.cpp
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-03
 * @copyright Copyright (c) skogaby 2022
 */

#include <PicoLed.hpp>
#include <stdio.h>
#include "tusb.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "config.h"
#include "sega_hardware/slider/sega_slider.h"
#include "leds/led_controller.h"
#include "slider/touch_slider.h"
#include "tinyusb/usb_descriptors.h"
#include "usb_output/usb_output.h"

/** 
 * This divisor is used for limiting the lights output rate, by dividing the USB output rate (1000Hz) by this divisor and tying
 * the lights output to the USB output, if a counter reaches (LIGHTS_UPDATE_DIVISOR - 1). This effectively divides 1000 by 
 * LIGHTS_UPDATE_DIVISOR and this is the update rate of the lights.
 */
#define LIGHTS_UPDATE_DIVISOR 4

/**
 * Uncomment this if you want to use keyboard output and reactive lights, instead of the arcade slider protocol. This will eventually
 * be driven by a button press at runtime or something.
 */
// #define USE_KEYBOARD_OUTPUT

/** Manages handling touch events and updating touch state */
TouchSlider* touch_slider;
/** Manages the LED strip and abstracts away LED indices from key and divider indices */
LedController* led_strip;
/** Handles sending keyboard outputs to the host computer */
UsbOutput* usb_output;
/** Handles packet processing for adhering to the SEGA slider protocol */
SegaSlider* sega_slider;
/** This keeps track of the touch states of the keys for reactive lighting updates (combines each key's sensors into one ORed state) */
bool key_states[16] = { false };
/** This flag indicates that the light state has been updated and the lights should be refreshed (not for arcade protocol mode) */
bool update_lights = false;
/** Used during serial reads, to indicate if the last byte read was an escape byte, in case the next byte is not available yet */
bool last_byte_escape = false;

void main_core_1();
int read_unescaped_slider_byte();
int read_slider_byte();

/**
 * @brief Initializes the GPIO pins, sets up I2C, etc.
 */
void setup_gpio() {
    // Initialise I2C
    i2c_init(I2C_PORT, I2C_FREQUENCY);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);
}

/**
 * @brief Main firmware entrypoint.
 */
int main() {
    tusb_init();
    stdio_init_all();
    setup_gpio();

    // Initialize inputs and outputs
    touch_slider = new TouchSlider();
    led_strip = new LedController(100);
    usb_output = new UsbOutput();
    sega_slider = new SegaSlider(touch_slider, led_strip);

    // Launch the input code on the second core
    multicore_launch_core1(main_core_1);

    // Keep track of the output rate and log it each second
    uint32_t time_now = to_ms_since_boot(get_absolute_time());
    uint32_t time_log = time_now + 1000;
    uint32_t output_count = 0;
    uint32_t lights_update_count = 0;

#ifdef USE_KEYBOARD_OUTPUT
    // Limit how often we update lights in keyboard mode, relative to how often we send USB updates
    uint32_t lights_update_limiter = 0;
#else
    // Buffer to contain incoming serial packets
    uint8_t serial_buffer[256] = { 0 };
    // These help us track in-progress packet reads
    uint8_t sync = 0;
    uint8_t command_id = 0;
    int data_length = -1;
    int bytes_read = 0;
    int checksum = -1;
    int next_byte = -1;
    bool packet_in_progress = false;

#endif

    while (true) {
        // tinyusb device task, required to call this frequently since we're
        // not using a RTOS
        tud_task();

#ifdef USE_KEYBOARD_OUTPUT
        // Check if the host is ready to receive another USB packet
        if (tud_hid_ready()) {
            // Send the keyboard updates
            usb_output->set_slider_sensors(touch_slider->states);
            usb_output->send_update();

            // Update the lights if necessary, based on how many USB frames
            // to skip before updating the lights
            if (lights_update_limiter == (LIGHTS_UPDATE_DIVISOR - 1)) {
                if (update_lights) {
                    led_strip->update();
                    update_lights = false;
                }

                lights_update_limiter = 0;
                lights_update_count++;
            } else {
                lights_update_limiter++;
            }

            output_count++;
        }
#else
        // Check if any serial packets are available. Since we can't make any timing
        // guarantees about when we'll read and process data relative to when each byte
        // arrives, this simple state machine processes packets one byte at a time
        // and tracks partial packets between loop iterations, processing them once
        // they fully arrive

        // If we're at the beginning of a packet, we need to read bytes without unescaping
        if (sync == 0) {
            next_byte = read_slider_byte();
        } else {
            next_byte = read_unescaped_slider_byte();
        }

        // There is at least 1 byte available, process it
        while (next_byte != -1) {
            if (sync == 0) {
                // We haven't read the next packet begin yet
                if (next_byte == PACKET_BEGIN) {
                    packet_in_progress = true;
                    sync = next_byte;
                    next_byte = read_unescaped_slider_byte();
                } else {
                    next_byte = read_slider_byte();
                }
            } else if (command_id == 0) {
                // We've read the SYNC byte, haven't read a command ID yet
                command_id = next_byte;
                next_byte = read_unescaped_slider_byte();
            } else if (data_length == -1) {
                // We've read the command ID, haven't read the data length yet
                data_length = next_byte;
                next_byte = read_unescaped_slider_byte();
            } else if (bytes_read != data_length) {
                // We're inside the body of a packet, read bytes until we've
                // read them all
                serial_buffer[bytes_read++] = next_byte;
                next_byte = read_unescaped_slider_byte();
            } else if (checksum == -1) {
                // We've finished the packet body, read the checksum and then process
                // the packet
                checksum = next_byte;

                // Construct the request packet
                SliderPacket request;
                request.command_id = command_id;
                request.data = &serial_buffer[0];
                request.length = data_length;
                request.checksum = checksum;

                // printf("Received packet, command ID 0x%X, data length %d\n", command_id, data_length);

                // Process the request packet. If a response needs to be sent,
                // SegaSlider itself handles the sending, escaping, and checksumming
                sega_slider->process_packet(request);

                // Reset the packet states for the next read
                sync = 0;
                command_id = 0;
                data_length = -1;
                bytes_read = 0;
                checksum = -1;
                next_byte = -1;
                packet_in_progress = false;
            }
        }

        // Send a slider packet to the host, if auto-reporting is enabled
        if (!packet_in_progress && sega_slider->auto_send_reports) {
            sega_slider->send_slider_report();
            output_count++;
        }
#endif
        // Log the current output rate once per second
        time_now = to_ms_since_boot(get_absolute_time());

        if (time_now > time_log) {
            printf("[Core 0] Output rate: %i Hz | Lights update rate: %i Hz\n",
                output_count, lights_update_count);
            time_log = time_now + 1000;
            output_count = 0;
            lights_update_count = 0;
        }
    }

    return 0;
}

/**
 * @brief Entrypoint for the second core. Currently, core1 is responsible for polling all the inputs,
 * while core0 will be responsible for outputs.
 */
void main_core_1() {
    // Keep track of the scan rate and log it each second
#ifdef USE_KEYBOARD_OUTPUT
    uint32_t time_now = to_ms_since_boot(get_absolute_time());
    uint32_t time_log = time_now + 1000;
    uint32_t scan_count = 0;
#endif

    // Infinite loop to read all the input data from various sources
    while (true) {
        // Scan the touch keys
        touch_slider->scan_touch_states();

#ifdef USE_KEYBOARD_OUTPUT
        // Set the slider LEDs according to touch sensor states,
        // but let core 0 handle the actual call to *show* the lights
        for (int i = 0; i < 16; i++) {
            bool key_pressed = touch_slider->is_key_pressed(i);

            if (key_pressed != key_states[i]) {
                if (key_pressed) {
                    led_strip->set_key(i, BLUE);
                } else {
                    led_strip->set_key(i, YELLOW);
                }

                update_lights = true;
            }

            key_states[i] = key_pressed;
        }

        scan_count++;

        // Log the current touch scan rate once per second
        time_now = to_ms_since_boot(get_absolute_time());

        if (time_now > time_log) {
            printf("[Core 1] Input scan rate: %i Hz\n", scan_count);
            time_log = time_now + 1000;
            scan_count = 0;
        }
#endif
    }
}

/**
 * @brief Reads a byte from the slider serial, and unescapes it if necessary.
 * @return int The next byte from slider serial, unescaped (-1 if no data available)
 */
int read_unescaped_slider_byte() {
    int return_value = -1;

    // Make sure any data is available
    if (tud_cdc_n_available(ITF_SLIDER)) {
        uint8_t val = tud_cdc_n_read_char(ITF_SLIDER);

        // Possible outcomes after reading any given byte:
        // The byte is not the escape byte:
        //    * Check if the previous byte was the escape byte, and if so, add 1 to this byte,
        //      return it and reset the flag
        //    * If previous byte was also not escape byte, then return the value as-read
        // The byte is the escape byte:
        //    * Set the flag and then move onto the next byte
        if (val != PACKET_ESCAPE) {
            if (last_byte_escape) {
                return_value = val + 1;
                last_byte_escape = false;
            } else {
                return_value = val;
            }
        } else {
            last_byte_escape = true;
            return_value = read_unescaped_slider_byte();
        }
    }

    return return_value;
}

/**
 * @brief Reads a byte from the slider serial without unescaping it
 * @return int The next byte from slider serial, or -1 if no data is available
 */
int read_slider_byte() {
    int return_value = -1;

    if (tud_cdc_n_available(ITF_SLIDER)) {
        return_value = tud_cdc_n_read_char(ITF_SLIDER);
    }

    return return_value;
}
