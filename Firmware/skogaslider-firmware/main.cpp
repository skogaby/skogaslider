/**
 * @file skogaslider-firmware.cpp
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

/** Manages handling touch events and updating touch state */
TouchSlider* touch_slider;
/** Manages the LED strip and abstracts away LED indices from key and divider indices */
LedController* led_strip;
/** Handles sending keyboard outputs to the host computer */
UsbOutput* usb_output;
/** This keeps track of the touch states of the keys for reactive lighting updates (combines each key's sensors into one ORed state) */
bool key_states[16] = { false };
/** This flag indicates that the light state has been updated and the lights should be refreshed */
bool update_lights = false;

// Function prototypes
void main_core_1();

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
    led_strip = new LedController(50);
    usb_output = new UsbOutput();

    // Launch the input code on the second core
    multicore_launch_core1(main_core_1);

    // Keep track of the output rate and log it each second
    uint32_t time_now = to_ms_since_boot(get_absolute_time());
    uint32_t time_log = time_now + 1000;
    uint32_t output_count = 0;
    uint32_t lights_update_count = 0;

    // Limit how often we update lights, relative to how often we send USB updates
    uint32_t lights_update_limiter = 0;

    while (true) {
        // tinyusb device task, required to call this frequently since we're
        // not using a RTOS
        tud_task();

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

        // Log the current keyboard output rate once per second
        time_now = to_ms_since_boot(get_absolute_time());

        if (time_now > time_log) {
            printf("[Core 0] Keyboard output rate: %i Hz | Lights update rate: %i Hz\n",
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
    uint32_t time_now = to_ms_since_boot(get_absolute_time());
    uint32_t time_log = time_now + 1000;
    uint32_t scan_count = 0;

    // Infinite loop to read all the input data from various sources
    while (true) {
        // Scan the touch keys
        touch_slider->scan_keys();

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

        // Log the current keyboard output rate once per second
        time_now = to_ms_since_boot(get_absolute_time());

        if (time_now > time_log) {
            printf("[Core 1] input scan rate: %i Hz\n", scan_count);
            time_log = time_now + 1000;
            scan_count = 0;
        }
    }
}
