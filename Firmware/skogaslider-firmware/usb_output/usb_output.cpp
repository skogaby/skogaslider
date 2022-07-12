#include "usb_output.h"

/**
 * @brief Construct a new UsbOutput::UsbOutput object.
 */
UsbOutput::UsbOutput():
    nkro_report { 0 }
{
}

/**
 * @brief Sets the given key code to be pressed on the keyboard output report.
 * @param key_code The key code to press
 */
void UsbOutput::set_keycode_pressed(uint8_t key_code) {
    uint8_t bit = key_code % 8;
    uint8_t byte = (key_code / 8) + 1;

    if (key_code >= 240 && key_code <= 247) {
        nkro_report[0] |= (1 << bit);
    } else if (byte > 0 && byte <= 31) {
        nkro_report[byte] |= (1 << bit);
    }
}

/**
 * @brief Sets the states for all of the touch slider sensors in the USB report.
 * @param states The states of all 32 touch sensors.
 */
void UsbOutput::set_slider_sensors(bool states[32]) {
    for (int i = 0; i < 32; i++) {
        if (states[i]) {
            set_keycode_pressed(slider_key_codes[i]);
        }
    }
}

/**
 * @brief Sets the states for all of the air tower sensors in the USB report.
 * @param states The states of all 6 air sensors.
 */
void UsbOutput::set_air_sensors(bool states[6]) {
    for (int i = 0; i < 6; i++) {
        if (states[i]) {
            set_keycode_pressed(air_key_codes[i]);
        }
    }
}

/**
 * @brief Sends the keyboard output to the computer.
 */
void UsbOutput::send_update() {
    tud_hid_n_report(0x00, REPORT_ID_KEYBOARD, &nkro_report, sizeof(nkro_report));
    memset(&nkro_report, 0, sizeof(nkro_report));
}
