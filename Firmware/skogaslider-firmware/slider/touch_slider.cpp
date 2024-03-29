/**
 * @file TouchSlider.cpp
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-03
 * @copyright Copyright (c) 2022
 */

#include "touch_slider.h"

/**
 * @brief Construct a new TouchSlider::TouchSlider object.
 */
TouchSlider::TouchSlider(): 
    touch_sensors { 
        MPR121(i2c0, I2C_ADDR_MPR121_0),
        MPR121(i2c0, I2C_ADDR_MPR121_1),
        MPR121(i2c0, I2C_ADDR_MPR121_2)
    },
    states { false }
{
}

/**
 * @brief Does a scan across all the MPR121s and updates the internal boolean touch state of each sensor,
 * primarily used for keyboard mode, since we don't manually calculate thresholds and leave it to the
 * MPR121 in that mode.
 * @return bool* The boolean touch state of each sensor
 */
bool* TouchSlider::scan_touch_states() {
    uint8_t curr_state_index = 0;

    // Loop over the 3 MPR121s and read every key
    for (uint8_t sensor_index = 0; sensor_index < 3; sensor_index++) {
        MPR121 mpr121 = touch_sensors[sensor_index];
        uint16_t touched = mpr121.get_all_touched();
        uint8_t lower_bound;
        
        // The 3rd MPR121 only contains 8 keys, so we make sure not to read the last 4 electrodes
        // on this sensor.
        if (sensor_index == 2) {
            lower_bound = 4;
        } else {
            lower_bound = 0;
        }

        for (int i = 11; i >= lower_bound; i--) {
            states[curr_state_index++] = bit_read(touched, i);
        }
    }

    return states;
}

/**
 * @brief Does a scan across all the MPR121s and updates the internal uint16_t touch values for each key.
 * @return uint16_t* The touch readout values of each of the 32 sensors.
 */
uint16_t* TouchSlider::scan_touch_readouts() {
    uint8_t curr_state_index = 0;

    // Loop over the 3 MPR121s and read every key
    for (uint8_t sensor_index = 0; sensor_index < 3; sensor_index++) {
        MPR121 mpr121 = touch_sensors[sensor_index];
        uint16_t* touch_values = mpr121.get_all_electrode_values();
        uint8_t lower_bound;
        
        // The 3rd MPR121 only contains 8 keys, so we make sure not to read the first 4 electrodes
        // on this sensor
        if (sensor_index == 2) {
            lower_bound = 4;
        } else {
            lower_bound = 0;
        }

        for (int i = 11; i >= lower_bound; i--) {
            touch_readouts[curr_state_index++] = touch_values[i];
        }
    }

    return touch_readouts;
}

/**
 * @brief Returns the pressed status of the given key, checking both sensors for the key.
 * @param key The key to read
 * @return true If either or both sensors on the key is pressed
 * @return false If neither sensor on the key is pressed
 */
bool TouchSlider::is_key_pressed(uint8_t key) {
    return states[key * 2] | states[key * 2 + 1];
}
