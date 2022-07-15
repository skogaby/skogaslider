/**
 * @file sega_slider.cpp
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-12
 * @copyright Copyright (c) skogaby 2022
 */

#include "sega_slider.h"

/**
 * @brief Construct a new SegaSlider::SegaSlider object.
 */
SegaSlider::SegaSlider(TouchSlider* _slider, LedController* _led_strip):
    touch_slider { _slider },
    led_strip { _led_strip },
    auto_send_reports { false },
    slider_response_data { 0 },
    hw_info_response_data {
        0x31, 0x35, 0x33, 0x33, 0x30, 0x20, 0x20, 0x20,
        0xA0, 0x30, 0x36, 0x37, 0x31, 0x32, 0xFF, 0x90
    }
{
    // Packet to re-use for slider reports, the address for the data
    // pointer will always be valid since it points to a class member
    slider_report_packet.command_id = SLIDER_REPORT;
    slider_report_packet.data = &slider_response_data[0];
    slider_report_packet.length = 32;

    // Packet for hardware info responses, which contains hardcoded
    // board and model information
    hw_info_packet.command_id = GET_HW_INFO;
    hw_info_packet.data = &hw_info_response_data[0];
    hw_info_packet.length = 16;

    // Packet to re-use for responses whose bodies are empty. Set the
    // command ID as needed
    empty_body_packet.command_id = NO_OP;
    empty_body_packet.length = 0;
}

/**
 * @brief Maps a 10-bit touch value to a uint8_t value.
 * @param value 
 * @return uint8_t 
 */
uint8_t SegaSlider::map_touch_to_byte(uint16_t value) {
    // Maps from (0-1024) to (0-252) so we don't have to
    // worry about escaping bytes on the way out, and 252
    // is still high enough to trigger a press in-game
    return (value / 0x400) * (0xFC);
}

/**
 * @brief Processes an incoming serial packet from the host. Returns a SliderPacket that is either
 * a valid response, or one whose command ID is NO_OP, indicating no response needed.
 * @param request The packet from the host
 * @return SliderPacket A response packet, or a  NO_OP packet.
 */
SliderPacket SegaSlider::process_packet(SliderPacket request) {
    switch (request.command_id) {
        case SLIDER_REPORT:
            return handle_slider_report();
        case LED_REPORT:
            handle_led_report(request);
        case ENABLE_SLIDER_REPORT:
            handle_enable_slider_report();
        case DISABLE_SLIDER_REPORT:
            return handle_disable_slider_report();
        case RESET:
            return handle_reset();
        case GET_HW_INFO:
            return handle_get_hw_info();
    }

    // Anything that didn't explicitly return its own response should just return NO_OP
    empty_body_packet.command_id = NO_OP;
    return empty_body_packet;
}

/**
 * @brief Generates a slider report packet to send to the host, used both for a one-off request for the
 * data, as well as during auto-scan mode.
 * @return SliderPacket A slider report packet containing all sensor readouts
 */
SliderPacket SegaSlider::generate_slider_report() {
    // Re-order the touch states into the right format. Internally, we store them with sensor 0 in the
    // top-left position on the slider, but Sega has it in the top-right position, meaning we can't
    // do a simple reversal here. Also, we need to map the 10-bit touch values into 8-bit values.
    uint8_t response_index = 0;

#ifdef FAKE_SLIDER_REPORT_VALUES
    bool* touched_states = touch_slider->scan_touch_states();
    
    for (int key = 15; key >= 0; key--) {
        for (int j = 0; j < 2; j++) {
            if (touched_states[(key * 2) + j]) {
                // High enough to trigger a press, but not need escaping when it's sent out
                slider_response_data[response_index++] = 0xFC;
            } else {
                slider_response_data[response_index++] = 0x00;
            }
        }
    }
#else
    uint16_t* touch_values = touch_slider->scan_touch_readouts();

    for (int key = 15; key >= 0; key--) {
        slider_response_data[response_index++] = map_touch_to_byte(touch_values[(key * 2)]);
        slider_response_data[response_index++] = map_touch_to_byte(touch_values[(key * 2) + 1]);
    }
#endif

    return slider_report_packet;
}

/**
 * @brief Handles a request for a one-off slider report.
 * @return SliderPacket A slider report packet containing all sensor readouts
 */
SliderPacket SegaSlider::handle_slider_report() {
    generate_slider_report();
    return slider_report_packet;
}

/**
 * @brief Handles a packet from the host to update the LEDs on the slider
 * @param request A SliderPacket that contains an LED update report
 */
void SegaSlider::handle_led_report(SliderPacket request) {
    // The max brightness according to the protocol is 63, map that from 6 bits
    // to 8 bits and set the LED brightness
    uint8_t brightness = (request.data[0] / 0x3F) * 0xFF;
    led_strip->set_brightness(brightness);

    // The index for the LEDs starts at the right-hand side on the last key
    // in the LED reports, and the order of the bytes is BRG
    uint8_t key_index = 15;
    uint8_t divider_index = 14;

    for (uint8_t i = 0; i < 32; i++) {
        uint8_t blue = request.data[(i *  3)];
        uint8_t red = request.data[(i *  3) + 1];
        uint8_t green = request.data[(i *  3) + 2];

        // Alternate between the keys and dividers
        if (i % 2 == 0) {
            led_strip->set_key(key_index--, red, green, blue);
        } else {
            led_strip->set_divider(divider_index--, red, green, blue);
        }
    }

    led_strip->update();
}

/**
 * @brief Handles a request to begin automatically sending slider reports
 * to the host from the slider (arcade does it roughly every 12ms).
 */
void SegaSlider::handle_enable_slider_report() {
    // This flag will be actually processed in a later revision
    auto_send_reports = true;
}

/**
 * @brief Handles a request to disable automatic slider reports to the host.
 * @return SliderPacket An ACK response.
 */
SliderPacket SegaSlider::handle_disable_slider_report() {
    auto_send_reports = false;

    empty_body_packet.command_id = DISABLE_SLIDER_REPORT;
    return empty_body_packet;
}

/**
 * @brief Handles a request to reset the board. For now we'll just ACK this and revisit
 * it later if needed.
 * @return SliderPacket An ACK response.
 */
SliderPacket SegaSlider::handle_reset() {
    empty_body_packet.command_id = RESET;
    return empty_body_packet;
}

/**
 * @brief Handles a request to get the hardware info from the slider.
 * @return SliderPacket A packet containing board info
 */
SliderPacket SegaSlider::handle_get_hw_info() {
    return hw_info_packet;
}

/**
 * @brief Calculate and return the checksum for a given packet
 * @param packet Packet to get the checksum for
 * @return uint8_t Checksum for the packet
 */
uint8_t SegaSlider::calculate_checksum(SliderPacket packet) {
    uint8_t sum = packet.command_id + packet.length;

    for (int i = 0; i < packet.length; i++) {
        sum += packet.data[i];
    }

    return ((sum - 0xFF) & 0xFF);
}