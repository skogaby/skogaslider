/**
 * @file sega_serial_reader.cpp
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-17
 * @copyright Copyright (c) skogaby 2022
 */

#include "sega_serial_reader.h"

/**
 * @brief Construct a new SegaSerialReader::SegaSerialReader object.
 */
SegaSerialReader::SegaSerialReader():
    serial_buf_slider { 0 },
    serial_buf_led_1 { 0 },
    serial_buf_led_2 { 0 },
    last_byte_escape { false },
    sync { 0 },
    data_length { -1 },
    bytes_read { 0 },
    checksum { -1 },
    next_byte { -1 },
    packet_in_progress { false },
    led_dst_addr { 0 },
    led_src_addr { 0 },
    slider_command_id { 0 }
{
}

/**
 * @brief Reads a single slider packet from serial, if one is avaiable. If data is available, it will be read into
 * the provided packet and true is returned. If a whole packet is not available yet, false is returned and nothing
 * is modified in the provided packet.
 */
bool SegaSerialReader::read_slider_packet(SliderPacket* dst) {
    bool packet_available = false;

    // Check if any serial packets are available. Since we can't make any timing
    // guarantees about when we'll read and process data relative to when each byte
    // arrives, this simple state machine processes packets one byte at a time
    // and tracks partial packets between loop iterations, processing them once
    // they fully arrive

    // If we're at the beginning of a packet, we need to read bytes without unescaping
    if (sync[0] == 0) {
        next_byte[0] = read_serial_byte(ITF_SLIDER);
    } else {
        next_byte[0] = read_unescaped_serial_byte(ITF_SLIDER, SLIDER_PACKET_ESCAPE, 0);
    }

    // There is at least 1 byte available, process it
    while (next_byte[0] != -1) {
        if (sync[0] == 0) {
            // We haven't read the next packet begin yet
            if (next_byte[0] == SLIDER_PACKET_BEGIN) {
                packet_in_progress[0] = true;
                sync[0] = next_byte[0];
                next_byte[0] = read_unescaped_serial_byte(ITF_SLIDER, SLIDER_PACKET_ESCAPE, 0);
            } else {
                next_byte[0] = read_serial_byte(ITF_SLIDER);
            }
        } else if (slider_command_id == 0) {
            // We've read the SYNC byte, haven't read a command ID yet
            slider_command_id = next_byte[0];
            next_byte[0] = read_unescaped_serial_byte(ITF_SLIDER, SLIDER_PACKET_ESCAPE, 0);
        } else if (data_length[0] == -1) {
            // We've read the command ID, haven't read the data length yet
            data_length[0] = next_byte[0];
            next_byte[0] = read_unescaped_serial_byte(ITF_SLIDER, SLIDER_PACKET_ESCAPE, 0);
        } else if (bytes_read[0] != data_length[0]) {
            // We're inside the body of a packet, read bytes until we've
            // read them all
            serial_buf_slider[bytes_read[0]] = next_byte[0];
            bytes_read[0] = bytes_read[0] + 1;
            next_byte[0] = read_unescaped_serial_byte(ITF_SLIDER, SLIDER_PACKET_ESCAPE, 0);
        } else if (checksum[0] == -1) {
            // We've finished the packet body, read the checksum and then return the packet
            checksum[0] = next_byte[0];

            // Construct the request packet
            dst->command_id = slider_command_id;
            dst->data = &serial_buf_slider[0];
            dst->length = data_length[0];
            dst->checksum = checksum[0];
            packet_available = true;

            // Reset the packet states for the next read
            sync[0] = 0;
            slider_command_id = 0;
            data_length[0] = -1;
            bytes_read[0] = 0;
            checksum[0] = -1;
            next_byte[0] = -1;
            packet_in_progress[0] = false;
        }
    }

    return packet_available;
}

/**
 * @brief Reads a single LED board packet from serial, if one is avaiable. If data is available, it will be read into
 * the provided packet and true is returned. If a whole packet is not available yet, false is returned and nothing
 * is modified in the provided packet. The address is either 0 or 1, depending on which board you wish to read from.
 */
bool SegaSerialReader::read_led_packet(SliderPacket* dst, uint8_t addr) {
    bool packet_available = false;

    // TODO    

    return packet_available;
}

/**
 * @brief Says whether or not a slider packet is currently in progress of being read.
 */
bool SegaSerialReader::slider_packet_in_progress() {
    return packet_in_progress[0];
}

/**
 * @brief Reads a single byte from serial for the given interface, or -1
 * if no bytes are available. While reading bytes, if the given escape
 * byte is encountered, it is skipped and the next byte + 1 is returned
 * instead.
 */
int SegaSerialReader::read_unescaped_serial_byte(uint8_t itf, uint8_t escape_byte, uint8_t board) {
    int return_value = -1;

    // Make sure any data is available
    if (tud_cdc_n_available(itf)) {
        uint8_t val = tud_cdc_n_read_char(itf);

        if (val != escape_byte) {
            if (last_byte_escape[board]) {
                return_value = val + 1;
                last_byte_escape[board] = false;
            } else {
                return_value = val;
            }
        } else {
            // If we read the escape byte, just set the flag and then call recursively
            // so the next call will unescape the next byte
            last_byte_escape[board] = true;
            return_value = read_unescaped_serial_byte(itf, escape_byte, board);
        }
    }

    return return_value;
}

/**
 * @brief Reads a single byte from serial for the given interface, or -1
 * if no bytes are available.
 */
int SegaSerialReader::read_serial_byte(uint8_t itf) {
    int return_value = -1;

    if (tud_cdc_n_available(itf)) {
        return_value = tud_cdc_n_read_char(itf);
    }

    return return_value;
}
