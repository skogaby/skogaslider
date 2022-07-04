/**
 * @file TouchSlider.h
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-03
 * @copyright Copyright (c) 2022
 */

#ifndef _TOUCH_SLIDER_H_
#define _TOUCH_SLIDER_H_

#include "../config.h"
#include "mpr121/Mpr121.h"

#define I2C_ADDR_MPR121_0 0x5A
#define I2C_ADDR_MPR121_1 0x5C
#define I2C_ADDR_MPR121_2 0x5D

/**
 * @brief This class handles the functionality of the touch slider on the controller. The hardware implementation of the
 * MPR121s is abstracted away, and this class provides simple functionality to scan the current state of the keys,
 * get the state of any individual key, and get a flag that says whether any of the key states changed on the previous
 * scan, which is useful for lighting updates during reactive mode, etc.
 */
class TouchSlider {
    private:
        MPR121 touchSensors[3];
        bool states[32];

    public:
        TouchSlider();
        void scanKeys();
        bool isKeyPressed(uint8_t key);
};

#endif
