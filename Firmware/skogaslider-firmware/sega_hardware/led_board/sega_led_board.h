/**
 * @file sega_led_board.h
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-19
 * @copyright Copyright (c) skogaby 2022
 */

#pragma once

#include "tusb.h"
#include "protocol.h"
#include "../../leds/led_controller.h"
#include "../serial/sega_serial_reader.h"

#define ADDRESS_HOST 1
#define ADDRESS_BOARD 2

/**
 * @brief Class that implements the 15093-06 LED board's request and response protocol. This board
 * is responsible for the air tower lights (and billboard lights, but we won't worry about this
 * for this implementation).
 */
class SegaLedBoard {
    public:
        SegaLedBoard(LedController* _led_strip);
        void process_packet(LedRequestPacket* request, uint8_t addr);

    private:
        LedController* led_strip;
        LedResponsePacket* response_packet;
        uint8_t response_payload[32];
        uint8_t board_info_payload[16];
        uint8_t led_data_index[2];
        bool response_enabled[2];

        void send_packet(LedResponsePacket* packet, uint8_t addr);
        LedResponsePacket* handle_reset(uint8_t addr);
        LedResponsePacket* handle_set_timeout(LedRequestPacket* request);
        LedResponsePacket* handle_set_disable_response(LedRequestPacket* request, uint8_t addr);
        LedResponsePacket* handle_set_led(LedRequestPacket* request, uint8_t addr);
        LedResponsePacket* handle_board_info();
        LedResponsePacket* handle_board_status();
        LedResponsePacket* handle_fw_sum();
        LedResponsePacket* handle_protocol_ver();
        void send_escaped_byte(uint8_t byte, uint8_t itf);
};

