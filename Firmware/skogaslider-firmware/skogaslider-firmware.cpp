/**
 * @file skogaslider-firmware.cpp
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-03
 * @copyright Copyright (c) skogaby 2022
 */

#include <PicoLed.hpp>
#include <stdio.h>
#include "pico/stdlib.h"
#include "config.h"
#include "slider/TouchSlider.h"
#include "leds/LedController.h"

/**
 * @brief Initializes the GPIO pins, sets up I2C, etc.
 * 
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
    stdio_init_all();
    setup_gpio();

    // Initialize touch slider
    auto touchSlider = TouchSlider();

    // Initialize LED strip
    auto ledStrip = LedController(100);

    for (int i = 0; i < 15; i++) {
        ledStrip.setKey(i, YELLOW);
        ledStrip.setDivider(i, PURPLE);
    }

    ledStrip.setKey(15, YELLOW);
    ledStrip.update();

    // Keep track of the polling rate and log it each second
    uint32_t timeNow = to_ms_since_boot(get_absolute_time());
    uint32_t timeLog = timeNow + 1000;
    uint32_t pollCount = 0;

    // Test loop to read inputs and set lights accordingly
    while (true) {
        // Scan the touch keys
        touchSlider.scanKeys();

        // Set the LEDs according to sensor state
        for (int i = 0; i < 16; i++) {
            bool keyPressed = touchSlider.isKeyPressed(i);

            if (keyPressed) {
                ledStrip.setKey(i, BLUE);
            } else {
                ledStrip.setKey(i, YELLOW);
            }
        }

        if (touchSlider.didStateChange()) {
            ledStrip.update();
        }

        pollCount++;
        timeNow = to_ms_since_boot(get_absolute_time());

        // Log the current poll rate
        if (timeNow > timeLog) {
            printf("Current poll count: %i\n", pollCount);
            timeLog = timeNow + 1000;
            pollCount = 0;
        }
    }

    return 0;
}
