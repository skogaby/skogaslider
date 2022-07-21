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
    board_info_payload {
        0x31, 0x35, 0x30, 0x39, 0x33, 0x2D, 0x30, 0x36,
        0x0A, 0x36, 0x37, 0x31, 0x30, 0x20, 0xFF, 0x90
    },
    response_payload { 0x00 },
    led_data_index { 50 * 3, 60 * 3 },
    response_enabled { true }
{
    // Initialize response packet constants
    response_packet = new LedResponsePacket();
    response_packet->report = 1;
    response_packet->status = 1;
    response_packet->payload = &response_payload[0];
}

/**
 * @brief Processes a request packet from the host, sending a response if necessary.
 */
void SegaLedBoard::process_packet(LedRequestPacket* request, uint8_t addr) {
    LedResponsePacket* response;

    switch (request->command) {
        case LED_RESET:
            response = handle_reset(addr);
            break;
        case SET_TIMEOUT:
            response = handle_set_timeout(request);
            break;
        case SET_DISABLE_RESPONSE:
            response = handle_set_disable_response(request, addr);
            break;
        case SET_LED:
            response = handle_set_led(request, addr);
            break;
        case BOARD_INFO:
            response = handle_board_info();
            break;
        case BOARD_STATUS:
            response = handle_board_status();
            break;
        case FW_SUM:
            response = handle_fw_sum();
            break;
        case PROTOCOL_VER:
            response = handle_protocol_ver();
            break;
    }

    // We shouldn't send responses for LED packets if the responses are disabled; otherwise, every other packet
    // expects a response in return
    if (request->command != SET_LED || response_enabled[addr]) {
        send_packet(response, addr);
    }
}

/**
 * @brief Handles a request to reset the board. Enables responses.
 */
LedResponsePacket* SegaLedBoard::handle_reset(uint8_t addr) {
    response_enabled[addr] = true;
    response_packet->command = LED_RESET;
    response_packet->length = 0;
    
    return response_packet;
}

/**
 * @brief Handles a request to set the timeout value. Currently we just ACK this.
 */
LedResponsePacket* SegaLedBoard::handle_set_timeout(LedRequestPacket* request) {
    response_packet->command = SET_TIMEOUT;
    response_packet->length = 2;
    response_packet->payload[0] = request->data[0];
    response_packet->payload[1] = request->data[1];

    return response_packet;
}

/**
 * @brief Handles a request to enable or disable responses from the board.
 */
LedResponsePacket* SegaLedBoard::handle_set_disable_response(LedRequestPacket* request, uint8_t addr) {
    response_enabled[addr] = !request->data[0];
    response_packet->command = SET_DISABLE_RESPONSE;
    response_packet->length = 1;
    response_packet->payload[0] = request->data[0];

    return response_packet;
}

/**
 * @brief Handles a request from the host to get the board information (hardcoded).
 */
LedResponsePacket* SegaLedBoard::handle_board_info() {
    response_packet->command = BOARD_INFO;
    response_packet->length = 16;
    memcpy(response_packet->payload, board_info_payload, 16);

    return response_packet;
}

/**
 * @brief Handles a command to get the board status (hardcoded).
 */
LedResponsePacket* SegaLedBoard::handle_board_status() {
    response_packet->command = BOARD_STATUS;
    response_packet->length = 4;
    response_packet->payload[0] = 0;
    response_packet->payload[1] = 0;
    response_packet->payload[2] = 0;
    response_packet->payload[3] = 0;

    return response_packet;
}

/**
 * @brief Handles a request for the checksum of the firmware (hardcoded).
 */
LedResponsePacket* SegaLedBoard::handle_fw_sum() {
    response_packet->command = FW_SUM;
    response_packet->length = 2;
    response_packet->payload[0] = 0xAD;
    response_packet->payload[1] = 0xF7;

    return response_packet;
}

/**
 * @brief Returns the (hardcoded) protocol version this board supports.
 */
LedResponsePacket* SegaLedBoard::handle_protocol_ver() {
    response_packet->command = PROTOCOL_VER;
    response_packet->length = 3;
    response_packet->payload[0] = 0x01;
    response_packet->payload[1] = 0x01;
    response_packet->payload[2] = 0x04;

    return response_packet;
}

/**
 * @brief Handles a request to set the actual LED data for the board.
 */
LedResponsePacket* SegaLedBoard::handle_set_led(LedRequestPacket* request, uint8_t addr) {
    // Read the correct index to skip over the billboard LED data in the request payload
    uint8_t index = led_data_index[addr];

    for (uint8_t i = 0; i < 3; i++) {
        uint8_t blue = request->data[index + (3 * i)];
        uint8_t red = request->data[index + (3 * i) + 1];
        uint8_t green = request->data[index + (3 * i) + 2];

        // Set the appropriate tower light
        led_strip->set_tower(addr, i, red, green, blue);
    }

    // Send the response to the host
    response_packet->command = SET_LED;
    response_packet->length = 0;
    return response_packet;
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

