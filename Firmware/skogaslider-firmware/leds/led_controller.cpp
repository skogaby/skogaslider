/**
 * @file LedController.cpp
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-03
 * @copyright Copyright (c) skogaby 2022
 */

#include "led_controller.h"

/**
 * @brief Construct a new LedController::LedController object
 * @param brightness How bright the strip should be, out of 255
 */
LedController::LedController(uint8_t brightness) {
    led_strip = PicoLed::addLeds<PicoLed::WS2812B>(pio0, 0, PIN_RGB_LED, NUM_RGB_LEDS, PicoLed::FORMAT_GRB);
    led_strip.setBrightness(brightness);

    // Set the initial colors for the slider
    for (int i = 0; i < 15; i++) {
        set_key(i, YELLOW);
        set_divider(i, PURPLE);
    }

    set_key(15, YELLOW);
    led_strip.show();
}

/**
 * @brief Sets all the LEDs in the strip the given color.
 */
void LedController::set_all(uint8_t red, uint8_t green, uint8_t blue) {
    led_strip.fill(PicoLed::RGB(red, green, blue));
}

/**
 * @brief Sets the color of the LEDs for the given slider key.
 */
void LedController::set_key(uint8_t key, uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t led_index = (key * 2) + key;
    auto color = PicoLed::RGB(red, green, blue);
    led_strip.setPixelColor(led_index, color);
    led_strip.setPixelColor(led_index + 1, color);
}

/**
 * @brief Sets the color of the LEDs for the given divider.
 */
void LedController::set_divider(uint8_t divider, uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t led_index = (2 * (divider + 1)) + divider;
    auto color = PicoLed::RGB(red, green, blue);
    led_strip.setPixelColor(led_index, color);
}

/**
 * @brief Sets the color of the LEDs for a particular group of LEDs for the air towers. Tower
 * 0 is the left tower, 1 is the right tower. Each tower has 3 groups of 3 LEDs each, with
 * group 0 being on the bottom and group 2 being on the top.
 */
void LedController::set_tower(uint8_t tower, uint8_t group, uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t led_index = 0;
    auto color = PicoLed::RGB(red, green, blue);

    // To make logic easier, the left tower is tower 0. However, the PCB itself is wired up with right tower
    // first, so if tower is 0, offset the LED index by 9 to skip the right tower LEDs
    if (tower == 0) {
        led_index = 56 + (3 * group);
    } else {
        led_index = 47 + (3 * group);
    }

    led_strip.fill(color, led_index, 3);
}

/**
 * @brief Changes the brightness of the LED strip to the given value.
 */
void LedController::set_brightness(uint8_t brightness) {
    led_strip.setBrightness(brightness);
}

/**
 * @brief Updates the physical LED strip to show the latest colors set in memory.
 */
void LedController::update() {
    led_strip.show();
}
