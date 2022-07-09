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
#include "leds/LedController.h"
#include "slider/TouchSlider.h"
#include "usb/usb_descriptors.h"

TouchSlider* touchSlider;
LedController* ledStrip;
bool touchStates[16] = { false };
bool updateLights = false;

const uint8_t keyCodes[16] = {
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
void core1_entry() {
    // Infinite loop to read all the input data from various sources
    while (true) {
        // Scan the touch keys
        touchSlider->scanKeys();
    }
}

/**
 * @brief Update the lights based on the current state of inputs.
 */
void updateLightsOutput() {
    // Set the slider LEDs according to touch sensor states
    for (int i = 0; i < 16; i++) {
        bool keyPressed = touchSlider->isKeyPressed(i);

        if (keyPressed != touchStates[i]) {
            if (keyPressed) {
                ledStrip->setKey(i, PURPLE);
            } else {
                ledStrip->setKey(i, YELLOW);
            }

            updateLights = true;
        }

        touchStates[i] = keyPressed;
    }
    
    if (updateLights) {
        ledStrip->update();
        updateLights = false;
    }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(
    uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen
) {
    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(
    uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize
) {
    return;
}

/**
 * @brief Main firmware entrypoint.
 */
int main() {
    tusb_init();
    stdio_init_all();
    setup_gpio();

    // Initialize touch slider
    touchSlider = new TouchSlider();

    // Initialize LED strip
    ledStrip = new LedController(51);

    // Set the initial colors for the slider
    for (int i = 0; i < 15; i++) {
        ledStrip->setKey(i, YELLOW);
        ledStrip->setDivider(i, PURPLE);
    }

    ledStrip->setKey(15, YELLOW);
    ledStrip->update();

    // Launch the input code on the second core
    multicore_launch_core1(core1_entry);

    // Keep track of the output rate and log it each second
    uint32_t timeNow = to_ms_since_boot(get_absolute_time());
    uint32_t timeLog = timeNow + 1000;
    uint32_t outputCount = 0;

    // Test loop to set outputs according to inputs being read on core1.
    while (true) {
        // tinyusb device task
        tud_task();

        // Update the lights
        updateLightsOutput();

        // Send the keyboard outputs
        if (tud_hid_ready()) {
            uint8_t nkro_report[32] = { 0 };

            for (int i = 0; i < 16; i++) {
                if (touchStates[i]) {
                    uint8_t bit = keyCodes[i] % 8;
                    uint8_t byte = (keyCodes[i] / 8) + 1;

                    if (keyCodes[i] >= 240 && keyCodes[i] <= 247) {
                        nkro_report[0] |= (1 << bit);
                    } else if (byte > 0 && byte <= 31) {
                        nkro_report[byte] |= (1 << bit);
                    }
                }
            }

            tud_hid_n_report(0x00, REPORT_ID_KEYBOARD, &nkro_report, sizeof(nkro_report));
            outputCount++;
        }

        // Log the current keyboard output rate once per second
        timeNow = to_ms_since_boot(get_absolute_time());

        if (timeNow > timeLog) {
            printf("Current output rate: %i Hz\n", outputCount);
            timeLog = timeNow + 1000;
            outputCount = 0;
        }
    }

    return 0;
}
