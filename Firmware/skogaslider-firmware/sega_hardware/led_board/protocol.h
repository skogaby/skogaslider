/**
 * @file protocol.h
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-16
 * @copyright Copyright (c) skogaby 2022
 */

#pragma once

#include "pico.h"

/** First byte of every packet sent to and from the LED boards to the host application */
#define LED_PACKET_BEGIN 0xE0
/** Byte used to escape any reserved bytes in an LED board packet */
#define LED_PACKET_ESCAPE 0xD0

/**
 * @brief This is an enumeration of the LED 15093-06 board command IDs we wish to implement.
 */
enum LedCommandId {
    /** Resets the LED board */
    LED_RESET = 0x10,
    /** Sets the timeout value for the board (not actually used for this firmware, but we need to respond to it) */
    SET_TIMEOUT = 0x11,
    /** Disables LED responses from the specified board */
    SET_DISABLE_RESPONSE = 0x14,
    /** Request to set the LEDs */
    SET_LED = 0x82,
    /** Gets the information about the board (model, etc.) */
    BOARD_INFO = 0xF0,
    /** Gets the current status of the board */
    BOARD_STATUS = 0xF1,
    /** Gets the checksum of the board's firmware */
    FW_SUM = 0xF2,
    /** Gets the protocol version the board suports */
    PROTOCOL_VER = 0xF3,
    /**  Custom command that isn't in the official protocol, but is a utility to see which side (0 for left, 1 for right) this board is for */
    BOARD_SIDE = 0x27
};

/**
 * @brief Structure which handles request packets from the host to the LED board.
 */
struct LedRequestPacket {
    /** Command ID for the packet, enumerated above */
    uint8_t command;
    /** Length of the packet data */
    uint8_t length;
    /** Data for this request */
    uint8_t* data;
};

/**
 * @brief Structure which handles response packets from the LED board to the host.
 */
struct LedResponsePacket {
    /** Status for the response */
    uint8_t status;
    /** Command ID for the response, enumerated above */
    uint8_t command;
    /** Report value, not sure what this signifies, seems to always be 1 */
    uint8_t report;
    /** Length of the packet data */
    uint8_t length;
    /** Data for the response */
    uint8_t* payload;
};
