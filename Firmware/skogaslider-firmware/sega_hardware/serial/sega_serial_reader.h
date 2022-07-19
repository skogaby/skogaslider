/**
 * @file sega_serial_reader.h
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-17
 * @copyright Copyright (c) skogaby 2022
 */

#pragma once

#include "pico.h"
#include "tusb.h"
#include "../slider/protocol.h"
#include "../led_board/protocol.h"

/** Serial interface for the slider device */
#define ITF_SLIDER 1
/** Serial interface for LED board 0 */
#define ITF_LED_0 2
/** Serial interface for LED board 1 */
#define ITF_LED_1 3

/**
 * @brief Class to manage reading serial packets for any of the 3 serial devices we present to the host that emulate SEGA
 * hardware devices (slider, or LED boards). This class abstracts away the state machines which manage being able to
 * read in-progress streams of data from serial and construct packets, while also supporting the streams ending mid-packet
 * and having to resume again later. Once packets are available from any of the 3 endpoints, they are returned to be
 * processed elsewhere.
 */
class SegaSerialReader {
    public:
        SegaSerialReader();
        bool read_slider_packet(SliderPacket* dst);
        bool read_led_packet(LedRequestPacket* dst, uint8_t addr);
        bool slider_packet_in_progress();

    private:
        /** Buffer to hold in-progress packet data for slider packets */
        uint8_t serial_buf_slider[256];
        /** Buffer to hold in-progress packet data for LED board 1 packets */
        uint8_t serial_buf_led_1[256];
        /** Buffer to hold in-progress packet data for LED board 2 packets */
        uint8_t serial_buf_led_2[256];
        /** Flag for whether the last byte read was an escape byte for the 3 packet types */
        bool last_byte_escape[3];
        /** Buffers to hold the last sync bytes read for all 3 packet types */
        uint8_t sync[3];
        /** Buffers to hold the last data length bytes read for all 3 packet types */
        int data_length[3];
        /** Keep track of how many bytes of the current packet have been read for all 3 packet types */
        int bytes_read[3];
        /** Keep track of the last checksum read for all 3 packet types */
        int checksum[3];
        /** Track the next byte to be processed, or -1 if there are no available bytes */
        int next_byte[3];
        /** Flag to say whether a packet is still in the process of being read (even if no available bytes yet) */
        bool packet_in_progress[3];
        /** Buffer the last-read destination addresses for LED packets */
        uint8_t led_dst_addr[2];
        /** Buffer the last-read source addresses for LED packets */
        uint8_t led_src_addr[2];
        /** Keeps track of the last slider command ID read */
        uint8_t slider_command_id;

        int read_unescaped_serial_byte(uint8_t itf, uint8_t escape_byte, uint8_t board);
        int read_serial_byte(uint8_t itf);
};
