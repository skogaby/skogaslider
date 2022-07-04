/**
 * @file TouchSlider.cpp
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-03
 * @copyright Copyright (c) 2022
 */

#include "TouchSlider.h"

/**
 * @brief Construct a new TouchSlider::TouchSlider object.
 */
TouchSlider::TouchSlider(): 
    touchSensors { MPR121(i2c0, I2C_ADDR_MPR121_0), MPR121(i2c0, I2C_ADDR_MPR121_1), MPR121(i2c0, I2C_ADDR_MPR121_2) },
    states { false } {
}

/**
 * @brief Does a scan across all the MPR121s and updates the internal states of each key.
 */
void TouchSlider::scanKeys() {
    uint8_t currStateIndex = 0;

    // Loop over the 3 MPR121s and read every key
    for (uint8_t sensorIndex = 0; sensorIndex < 3; sensorIndex++) {
        MPR121 mpr121 = touchSensors[sensorIndex];
        uint16_t touched = mpr121.getAllTouched();
        uint8_t lowerBound;
        
        // The 3rd MPR121 only contains 8 keys, so we make sure not to read the last 4 electrodes
        // on this sensor.
        if (sensorIndex == 2) {
            lowerBound = 4;
        } else {
            lowerBound = 0;
        }

        for (int i = 11; i >= lowerBound; i--) {
            states[currStateIndex++] = bitRead(touched, i);
        }
    }
}

/**
 * @brief Returns the pressed status of the given key.
 * @param key The key to read
 * @return true If either or both sensors on the key is pressed
 * @return false If neither sensor on the key is pressed
 */
bool TouchSlider::isKeyPressed(uint8_t key) {
    return states[key * 2] | states[key * 2 + 1];
}
