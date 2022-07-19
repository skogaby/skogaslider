/**
 * @file protocol.h
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-12
 * @brief This file contains structs and constants for implementing the SEGA slider protocol. Implementation reference
 * can be found here: https://gist.github.com/dogtopus/b61992cfc383434deac5fab11a458597 as of writing.
 * @copyright Copyright (c) skogaby 2022
 */

#pragma once

#include "pico.h"

/** First byte of every packet sent to and from the slider to the host application */
#define SLIDER_PACKET_BEGIN 0xFF
/** Byte used to escape any reserved bytes in a packet */
#define SLIDER_PACKET_ESCAPE 0xFD

/**
 * @brief This is an enumeration of the command IDs we care to implement for the purposes of slider emulation.
 */
enum SliderCommandId {
    /** Request from the host for a readout of the sensor pressures */
    SLIDER_REPORT = 0x01,
    /** Packet from the host to set the LEDs to the given state */
    LED_REPORT = 0x02,
    /** Packet from the host to enable the device to start sending slider reports */
    ENABLE_SLIDER_REPORT = 0x03,
    /** Request from the host to disable automatic slider reports */
    DISABLE_SLIDER_REPORT = 0x04,
    /** Request from the host to reset the slider */
    SLIDER_RESET = 0x10,
    /** Request from the host to return the hardware information about the slider */
    GET_HW_INFO = 0xF0
};

/**
 * @brief Represents a single packet sent between the slider and the host device.
 */
struct SliderPacket {
    /** The opcode for the packet */
    uint8_t command_id;
    /** The length of the packet's report body / the length of the data field */
    uint8_t length;
    /** The report body, can be empty */
    uint8_t* data;
    /** The checksum for the packet, should be calculated immediately before sending */
    uint8_t checksum;
};
