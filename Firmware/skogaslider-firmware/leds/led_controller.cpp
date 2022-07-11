/**
 * @file LedController.cpp
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-03
 * @copyright Copyright (c) skogaby 2022
 */

#include "LedController.h"

LedController::LedController(uint8_t brightness) {
    led_strip = PicoLed::addLeds<PicoLed::WS2812B>(pio0, 0, PIN_RGB_LED, NUM_RGB_LEDS, PicoLed::FORMAT_GRB);
    led_strip.setBrightness(brightness);
}

void LedController::set_all(uint8_t red, uint8_t green, uint8_t blue) {
    led_strip.fill(PicoLed::RGB(red, green, blue));
}

void LedController::set_key(uint8_t key, uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t led_index = (key * 2) + key;
    auto color = PicoLed::RGB(red, green, blue);
    led_strip.setPixelColor(led_index, color);
    led_strip.setPixelColor(led_index + 1, color);
}

void LedController::set_divider(uint8_t divider, uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t led_index = (2 * (divider + 1)) + divider;
    auto color = PicoLed::RGB(red, green, blue);
    led_strip.setPixelColor(led_index, color);
}

void LedController::set_brightness(uint8_t brightness) {
    led_strip.setBrightness(brightness);
}

void LedController::update() {
    led_strip.show();
}
