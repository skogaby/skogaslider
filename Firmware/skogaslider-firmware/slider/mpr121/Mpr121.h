/**
 * @file mpr121.h
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-02
 * @copyright Copyright (c) skogaby 2022
 */

#ifndef _MPR121_H_
#define _MPR121_H_

#include "pico.h"
#include "hardware/i2c.h"

/**
 * @brief A small library to communicate with the MPR121 chip. Ported from https://github.com/mcauser/micropython-mpr121 because
 * the library I did find for the Pico already was inadequate and contained errors, this is better suited for our simple usecase.
 */
class MPR121 {
    private:
        i2c_inst_t *i2cPort;
        uint8_t i2cAddr;

        void write8(uint8_t reg, uint8_t val);
        uint8_t read8(uint8_t reg);
        uint16_t read16(uint8_t reg);

    public:
        MPR121();
        MPR121(i2c_inst_t *i2cPort, uint8_t i2cAddr);
        void reset();
        void setThreshold(uint8_t touch, uint8_t release, uint8_t sensor);
        uint16_t filteredData(uint8_t electrode);
        uint8_t baselineData(uint8_t electrode);
        uint16_t getAllTouched();
        bool isElectrodeTouched(uint8_t electrode);
};

#endif
