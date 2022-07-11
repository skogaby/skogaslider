/**
 * @file LedController.h
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-03
 * @copyright Copyright (c) skogaby 2022
 */

#pragma once

#include <PicoLed.hpp>
#include "pico/stdlib.h"
#include "../config.h"

// The number of RGB LEDs (2 for each slider key, 1 for each slider divider, 9 for each air tower)
#define NUM_RGB_LEDS ((16 * 2) + 15) + (2 * 9)

// Constants for lights, used during reactive lighting mode, calibration sequences, etc.
#define BLUE 0, 0, 255
#define YELLOW 255, 100, 0
#define PURPLE 160, 32, 240

/**
 * @brief This is a low-level controller for the LEDs, which manages the mapping of setting a specific key, divider, or air tower light
 * without needing to know the indices in the overall LED chain. Logically, the slider has 16 keys with 15 dividers between them, but each
 * key has 2 LEDs to assist with physical lighting.
 */
class LedController {
    private:
        PicoLed::PicoLedController led_strip;
    public:
        LedController(uint8_t brightness);
        void set_all(uint8_t red, uint8_t green, uint8_t blue);
        void set_key(uint8_t key, uint8_t red, uint8_t green, uint8_t blue);
        void set_divider(uint8_t divider, uint8_t red, uint8_t green, uint8_t blue);
        void set_brightness(uint8_t brightness);
        void update();
};
