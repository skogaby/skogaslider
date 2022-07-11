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

touch_slider* touch_slider;
LedController* led_strip;
bool touch_states[16] = { false };
bool update_lights = false;

const uint8_t key_codes[16] = {
    HID_KEY_A, HID_KEY_B, HID_KEY_C, HID_KEY_D, HID_KEY_E, HID_KEY_F, HID_KEY_G, HID_KEY_H,
    HID_KEY_I, HID_KEY_J, HID_KEY_K, HID_KEY_L, HID_KEY_M, HID_KEY_N, HID_KEY_O, HID_KEY_P
};

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

        // Set the slider LEDslklmlkjklmlkjihijklmnijklmnijklmlkll according to touch sensor states,
        // but let core 0 handle the actual call to *show* the lights
        for (int i = 0; i < 16; i++) {
            bool key_pressed = touch_slider->is_key_pressed(i);

            if (key_pressed != touch_states[i]) {
                if (key_pressed) {
                    led_strip->set_key(i, PURPLE);
                } else {
                    led_strip->set_key(i, YELLOW);
                }

                update_lights = true;
            }

            touch_states[i] = key_pressed;
        }

        scan_count++;

        // Log the current keyboard output rate once per second
        time_now = to_ms_since_boot(get_absolute_time());

        if (time_now > time_log) {
            printf("Core 1 input scan rate: %i Hz\n", scan_count);
            time_log = time_now + 1000;
            scan_count = 0;
        }
    }
}

/**
 * @brief Main firmware entrypoint.
 */
int main() {
    tusb_init();
    stdio_init_all();
    setup_gpio();

    // Initialize touch slider
    touch_slider = new TouchSlider();

    // Initialize LED strip
    led_strip = new LedController(51);

    // Set the initial colors for the slider
    for (int i = 0; i < 15; i++) {
        led_strip->set_key(i, YELLOW);
        led_strip->set_divider(i, PURPLE);
    }

    led_strip->set_key(15, YELLOW);
    led_strip->update();

    // Launch the input code on the second core
    multicore_launch_core1(main_core_1);

    // Keep track of the output rate and log it each second
    uint32_t time_now = to_ms_since_boot(get_absolute_time());
    uint32_t time_log = time_now + 1000;
    uint32_t output_count = 0;

    // Test loop to set outputs according to inputs being read on core1.
    while (true) {
        // tinyusb device task
        tud_task();

        // Send the keyboard outputs
        if (tud_hid_ready()) {
            uint8_t nkro_report[32] = { 0 };

            for (int i = 0; i < 16; i++) {
                if (touch_states[i]) {
                    uint8_t bit = key_codes[i] % 8;
                    uint8_t byte = (key_codes[i] / 8) + 1;

                    if (key_codes[i] >= 240 && key_codes[i] <= 247) {
                        nkro_report[0] |= (1 << bit);
                    } else if (byte > 0 && byte <= 31) {
                        nkro_report[byte] |= (1 << bit);
                    }
                }
            }

            tud_hid_n_report(0x00, REPORT_ID_KEYBOARD, &nkro_report, sizeof(nkro_report));

            // Update the lights
            if (update_lights) {
                led_strip->update();
                update_lights = false;
            }

            output_count++;
        }

        // Log the current keyboard output rate once per second
        time_now = to_ms_since_boot(get_absolute_time());

        if (time_now > time_log) {
            printf("Core 0 keyboard output rate: %i Hz\n", output_count);
            time_log = time_now + 1000;
            output_count = 0;
        }
    }

    return 0;
}
