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
#include "sega_hardware/serial/sega_serial_reader.h"
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

/** How many milliseconds to wait in AC-mode between slider reports */
#define SLIDER_REPORT_DELAY 4

/** How many milliseconds to wait between logging input and output rates */
#define LOG_DELAY 1000

/**
 * Uncomment this if you want to use keyboard output and reactive lights, instead of the arcade slider protocol. This will eventually
 * be driven by a button press at runtime or something, but arcade protocol is the default behavior for now.
 */
// #define USE_KEYBOARD_OUTPUT

/** Manages handling touch events and updating touch state */
TouchSlider* touch_slider;
/** Manages the LED strip and abstracts away LED indices from key and divider indices */
LedController* led_strip;
/** Handles sending keyboard outputs to the host computer */
UsbOutput* usb_output;
/** Handles reading serial packets for slider and LED board emulation, including unescaping logic */
SegaSerialReader* sega_serial;
/** Handles packet processing for adhering to the SEGA slider protocol */
SegaSlider* sega_slider;
/** Re-usable packet structure for incoming slider packets. */
SliderPacket slider_request;
/** This keeps track of the touch states of the keys for reactive lighting updates (combines each key's sensors into one ORed state) */
bool key_states[16] = { false };
/** This flag indicates that the light state has been updated and the lights should be refreshed (not for arcade protocol mode) */
bool update_lights = false;

void main_core_1();

/**
 * @brief Initializes the GPIO pins, sets up I2C, etc.
 */
void init_gpio() {
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
    init_gpio();

    // Initialize inputs and outputs
    touch_slider = new TouchSlider();
    led_strip = new LedController(100);
    usb_output = new UsbOutput();
    sega_serial = new SegaSerialReader();
    sega_slider = new SegaSlider(touch_slider, led_strip);

    // Launch the input code on the second core
    multicore_launch_core1(main_core_1);

    // Keep track of the output rate and log it each second
    uint32_t time_now = to_ms_since_boot(get_absolute_time());
    uint32_t time_log = time_now + LOG_DELAY;
    uint32_t output_count = 0;
    uint32_t lights_update_count = 0;

#ifdef USE_KEYBOARD_OUTPUT
    // Limit how often we update lights in keyboard mode, relative to how often we send USB updates
    uint32_t lights_update_limiter = 0;
#else
    // Limit how often we send slider touch reports in AC protocol emulation mode
    uint32_t time_send_report = time_now + SLIDER_REPORT_DELAY;
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

        time_now = to_ms_since_boot(get_absolute_time());

#else // USE_KEYBOARD_OUTPUT
        // Check if any serial packets are available for the slider, and process them if so
        if (sega_serial->read_slider_packet(&slider_request)) {
            sega_slider->process_packet(&slider_request);
        }

        time_now = to_ms_since_boot(get_absolute_time());

        // Send a slider packet to the host, if auto-reporting is enabled, every X ms
        if (time_now >= time_send_report
                && sega_slider->auto_send_reports
                && !sega_serial->slider_packet_in_progress()) {
            sega_slider->send_slider_report();
            output_count++;
            time_send_report = time_now + SLIDER_REPORT_DELAY;
        }
#endif // USE_KEYBOARD_OUTPUT

        // Log the current output rate once per second
        if (time_now > time_log) {
            printf("[Core 0] Output rate: %i Hz", output_count * (1000 / LOG_DELAY));

#ifdef USE_KEYBOARD_OUTPUT
            printf(" | Lights update rate: %i Hz\n", lights_update_count * (1000 / LOG_DELAY));
            lights_update_count = 0;
#else
            printf("\n");
#endif

            time_log = time_now + LOG_DELAY;
            output_count = 0;
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
    uint32_t time_now = to_ms_since_boot(get_absolute_time());
    uint32_t time_log = time_now + LOG_DELAY;
    uint32_t scan_count = 0;

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
#endif

        scan_count++;

        // Log the current touch scan rate once per second
        time_now = to_ms_since_boot(get_absolute_time());

        if (time_now > time_log) {
            printf("[Core 1] Input scan rate: %i Hz\n", scan_count * (1000 / LOG_DELAY));
            time_log = time_now + LOG_DELAY;
            scan_count = 0;
        }
    }
}
