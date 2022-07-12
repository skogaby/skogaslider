/**
 * @file usb_output.h
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-11
 * @copyright Copyright (c) skogaby 2022
 */

#pragma once

#include <stdlib.h>
#include "tusb.h"
#include "../tinyusb/usb_descriptors.h"

/**
 * @brief These are the keycodes that get output for each of the 32 sensors of the slider. The indices match the
 * sensor numbers, which are as follows:
 * 
 *   0 | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30
 *   1 | 3 | 5 | 7 | 9 | 11 | 13 | 15 | 17 | 19 | 21 | 23 | 25 | 27 | 29 | 31
 */
const uint8_t slider_key_codes[32] = {
    HID_KEY_A, HID_KEY_B, HID_KEY_C, HID_KEY_D, HID_KEY_E, HID_KEY_F, HID_KEY_G, HID_KEY_H,
    HID_KEY_I, HID_KEY_J, HID_KEY_K, HID_KEY_L, HID_KEY_M, HID_KEY_N, HID_KEY_O, HID_KEY_P,
    HID_KEY_Q, HID_KEY_R, HID_KEY_S, HID_KEY_T, HID_KEY_U, HID_KEY_V, HID_KEY_W, HID_KEY_X,
    HID_KEY_Y, HID_KEY_Z, HID_KEY_1, HID_KEY_2, HID_KEY_3, HID_KEY_4, HID_KEY_5, HID_KEY_6,
};

/**
 * @brief These are the keycodes that get output for each of the 6 IR sensors on the air towers. The indices match
 * the sensor numbers, which are 0 to 5, bottom to top.
 */
const uint8_t air_key_codes[6] = {
    HID_KEY_BACKSLASH, HID_KEY_SLASH, HID_KEY_MINUS, HID_KEY_COMMA, HID_KEY_SEMICOLON, HID_KEY_PERIOD
};

/**
 * @brief Class which is responsible for managing sending USB keyboard outputs to the computer based on
 * the touch inputs and air sensor inputs.
 */
class UsbOutput {
    private:
        uint8_t nkro_report[32];

        void set_keycode_pressed(uint8_t key_code);
    public:
        UsbOutput();
        void set_slider_sensors(bool states[32]);
        void set_air_sensors(bool states[6]);
        void send_update();
};
