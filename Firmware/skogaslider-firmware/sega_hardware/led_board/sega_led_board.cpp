/**
 * @file sega_led_board.cpp
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-19
 * @copyright Copyright (c) skogaby 2022
 */

#include "sega_led_board.h"

/**
 * @brief Construct a new SegaLedBoard::SegaLedBoard object.
 */
SegaLedBoard::SegaLedBoard(LedController* _led_strip):
    led_strip { _led_strip },
    response_enabled { false }
{
}

/**
 * @brief Processes a request packet from the host, sending a response if necessary.
 */
void SegaLedBoard::process_packet(LedRequestPacket* request, uint8_t addr) {
    LedResponsePacket* response;

    switch (request->command) {
        case LED_RESET:
            response = handle_reset(request, addr);
            break;
        case SET_TIMEOUT:
            response = handle_set_timeout(request, addr);
            break;
        case SET_DISABLE_RESPONSE:
            response = handle_set_disable_response(request, addr);
            break;
        case SET_LED:
            response = handle_set_led(request, addr);
            break;
        case BOARD_INFO:
            response = handle_board_info(request, addr);
            break;
        case BOARD_STATUS:
            response = handle_board_status(request, addr);
            break;
        case FW_SUM:
            response = handle_fw_sum(request, addr);
            break;
        case PROTOCOL_VER:
            response = handle_protocol_ver(request, addr);
            break;
    }

    if (response_enabled) {
        send_packet(response, addr);
    }
}

LedResponsePacket* handle_reset(LedRequestPacket* request, uint8_t addr) {
    return NULL;
}

LedResponsePacket* handle_set_timeout(LedRequestPacket* request, uint8_t addr) {
    return NULL;
}

LedResponsePacket* handle_set_disable_response(LedRequestPacket* request, uint8_t addr) {
    return NULL;
}

LedResponsePacket* handle_set_led(LedRequestPacket* request, uint8_t addr) {
    return NULL;
}

LedResponsePacket* handle_board_info(LedRequestPacket* request, uint8_t addr) {
    return NULL;
}

LedResponsePacket* handle_board_status(LedRequestPacket* request, uint8_t addr) {
    return NULL;
}

LedResponsePacket* handle_fw_sum(LedRequestPacket* request, uint8_t addr) {
    return NULL;
}

LedResponsePacket* handle_protocol_ver(LedRequestPacket* request, uint8_t addr) {
    return NULL;
}

/**
 * @brief Sends a response packet to the host, escaping and checksumming as necessary.
 * @param packet The response packet to send
 * @param addr Which tower this packet is for (0 for left, 1 for right)
 */
void SegaLedBoard::send_packet(LedResponsePacket* packet, uint8_t addr) {
    uint8_t checksum = 0;
    uint8_t itf = ITF_LED_0;

    if (addr == 1) {
        itf = ITF_LED_1;
    }

    tud_cdc_n_write_char(itf, LED_PACKET_BEGIN);

    send_escaped_byte(ADDRESS_HOST, itf);
    checksum += ADDRESS_HOST;

    send_escaped_byte(ADDRESS_BOARD, itf);
    checksum += ADDRESS_BOARD;

    send_escaped_byte(packet->length + 3, itf);
    checksum += packet->length + 3;

    send_escaped_byte(packet->status, itf);
    checksum += packet->status;

    send_escaped_byte(packet->command, itf);
    checksum += packet->command;

    send_escaped_byte(packet->report, itf);
    checksum += packet->report;

    for (int i = 0; i < packet->length; i++) {
        send_escaped_byte(packet->payload[i], itf);
        checksum += packet->payload[i];
    }

    send_escaped_byte(checksum, itf);
    tud_cdc_n_write_flush(itf);
}

 /**
  * @brief Sends a byte to the host, escaping it if necessary.
  */
void SegaLedBoard::send_escaped_byte(uint8_t byte, uint8_t itf) {
    if (byte == LED_PACKET_BEGIN || byte == LED_PACKET_ESCAPE) {
        tud_cdc_n_write_char(itf, LED_PACKET_ESCAPE);
        byte -= 1;
    }
    
    tud_cdc_n_write_char(itf, byte);
}

