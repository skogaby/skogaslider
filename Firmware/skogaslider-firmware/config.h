/**
 * @file config.h
 * @author skogaby <skogabyskogaby@gmail.com>
 * @copyright Copyright (c) skogaby 2022
 * @brief This file serves as a top-level hardware configuration for the controller firmware. This file defines
 * things such as pin numbers and other primitive constants.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

// Helper function to read a single bit from a number
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)

// Pin for RGB out. The air towers are part of the same logical strip as the slider.
#define PIN_RGB_LED 2
// Pin to detect whether a secondary power source is plugged into the 2nd USB port
#define PIN_AUX_POWER_DETECT 3
// I2C pin configuration, for the MPR121s and EEPROM
#define PIN_SDA 4
#define PIN_SCL 5
// Pins for the functional buttons
#define PIN_SW_TEST 6
#define PIN_SW_SERVICE 7
#define PIN_SW_FUNCTION 8
// Pins for the air tower IR LED outputs
#define PIN_AIR_LED_0 11
#define PIN_AIR_LED_1 14
#define PIN_AIR_LED_2 10
#define PIN_AIR_LED_3 13 
#define PIN_AIR_LED_4 9
#define PIN_AIR_LED_5 12
// Pins for the multiplexer address selection pins
#define PIN_MUX_0 16
#define PIN_MUX_1 17
#define PIN_MUX_2 18
// Pin for the output from the multiplexer (IR sensors)
#define PIN_AIR_SENSOR_IN 26

// I2C configuration, ports and addresses
#define I2C_PORT i2c0
#define I2C_FREQUENCY 100000

#endif
