/**
 * @file skogaslider-firmware.cpp
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-03
 * @copyright Copyright (c) skogaby 2022
 */

#include <PicoLed.hpp>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "config.h"
#include "leds/LedController.h"
#include "slider/TouchSlider.h"

TouchSlider* touchSlider;
LedController* ledStrip;

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
 * @brief Main firmware entrypoint.
 */
int main() {
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
    bool prevTouchStates[16] = { false };
    bool updateLights = false;

    // Test loop to set outputs according to inputs being read on core1.
    while (true) {
        // Set the slider LEDs according to touch sensor states
        for (int i = 0; i < 16; i++) {
            bool keyPressed = touchSlider->isKeyPressed(i);

            if (keyPressed != prevTouchStates[i]) {
                if (keyPressed) {
                    ledStrip->setKey(i, PURPLE);
                } else {
                    ledStrip->setKey(i, YELLOW);
                }

                updateLights = true;
            }

            prevTouchStates[i] = keyPressed;
        }
        
        if (updateLights) {
            ledStrip->update();
            updateLights = false;
        }

        outputCount++;
        timeNow = to_ms_since_boot(get_absolute_time());

        // Log the current poll rate
        if (timeNow > timeLog) {
            printf("Current output rate: %i Hz\n", outputCount);
            timeLog = timeNow + 1000;
            outputCount = 0;
        }
    }

    return 0;
}
