/**
 * @file LedController.cpp
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-03
 * @copyright Copyright (c) skogaby 2022
 */

#include "LedController.h"

LedController::LedController(uint8_t brightness) {
    ledStrip = PicoLed::addLeds<PicoLed::WS2812B>(pio0, 0, PIN_RGB_LED, NUM_RGB_LEDS, PicoLed::FORMAT_GRB);
    ledStrip.setBrightness(brightness);
}

void LedController::setAll(uint8_t red, uint8_t green, uint8_t blue) {
    ledStrip.fill(PicoLed::RGB(red, green, blue));
}

void LedController::setKey(uint8_t key, uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t led_index = (key * 2) + key;
    auto color = PicoLed::RGB(red, green, blue);
    ledStrip.setPixelColor(led_index, color);
    ledStrip.setPixelColor(led_index + 1, color);
}

void LedController::setDivider(uint8_t divider, uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t led_index = (2 * (divider + 1)) + divider;
    auto color = PicoLed::RGB(red, green, blue);
    ledStrip.setPixelColor(led_index, color);
}

void LedController::setBrightness(uint8_t brightness) {
    ledStrip.setBrightness(brightness);
}

void LedController::update() {
    ledStrip.show();
}
