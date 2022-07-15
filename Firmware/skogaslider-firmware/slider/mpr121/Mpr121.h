/**
 * @file mpr121.h
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-02
 * @copyright Copyright (c) skogaby 2022
 */

#pragma once

#include "pico.h"
#include "hardware/i2c.h"

/**
 * @brief A small library to communicate with the MPR121 chip. Ported from https://github.com/mcauser/micropython-mpr121 because
 * the library I did find for the Pico already was inadequate and contained errors, this is better suited for our simple usecase.
 */
class MPR121 {
    private:
        i2c_inst_t *i2c_port;
        uint8_t i2c_addr;
        uint16_t electrode_data[12];

        void write_8(uint8_t reg, uint8_t val);
        uint8_t read_8(uint8_t reg);
        uint16_t read_16(uint8_t reg);
        uint8_t* read_bytes(uint8_t reg, size_t length);

    public:
        MPR121();
        MPR121(i2c_inst_t *i2c_port, uint8_t i2c_addr);
        void reset();
        void set_threshold(uint8_t touch, uint8_t release, uint8_t sensor);
        uint16_t filtered_data(uint8_t electrode);
        uint8_t baseline_data(uint8_t electrode);
        uint16_t get_all_touched();
        bool is_electrode_touched(uint8_t electrode);
        uint16_t* get_all_electrode_values();
};
