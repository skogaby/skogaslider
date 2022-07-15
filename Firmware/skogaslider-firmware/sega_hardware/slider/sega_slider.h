/**
 * @file sega_slider.h
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-12
 * @copyright Copyright (c) skogaby 2022
 */

#pragma once

#include "protocol.h"
#include "../../slider/touch_slider.h"
#include "../../leds/led_controller.h"

// Comment this out if you wish to send the actual touch values back to the game,
// as opposed to faking out the values based on whether the keys are touched
// or not based on the MPR121's internal touch state registers.
#define FAKE_SLIDER_REPORT_VALUES

/**
 * @brief Class that implements the SEGA slider's request and response protocol.
 */
class SegaSlider {
    private:
        // These are packets that can be re-used across calls which should be able to handle
        // any response without having to create new structs for every packet
        SliderPacket slider_report_packet;
        SliderPacket hw_info_packet;
        SliderPacket empty_body_packet;

        TouchSlider* touch_slider;
        LedController* led_strip;
        bool auto_send_reports;
        uint8_t slider_response_data[32];
        uint8_t hw_info_response_data[16];

        uint8_t map_touch_to_byte(uint16_t value);
        SliderPacket generate_slider_report();
        SliderPacket handle_slider_report();
        void handle_led_report(SliderPacket request);
        void handle_enable_slider_report();
        SliderPacket handle_disable_slider_report();
        SliderPacket handle_reset();
        SliderPacket handle_get_hw_info();
    public:
        SegaSlider(TouchSlider* _slider, LedController* _led_strip);
        SliderPacket process_packet(SliderPacket request);
        uint8_t calculate_checksum(SliderPacket packet);
};
