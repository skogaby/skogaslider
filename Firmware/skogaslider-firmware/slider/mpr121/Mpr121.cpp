/**
 * @file mpr121.cpp
 * @author skogaby <skogabyskogaby@gmail.com>
 * @date 2022-07-02
 * @copyright Copyright (c) skogaby 2022
 */

#include "mpr121.h"

// MPR121 register map
const uint8_t MPR121_TOUCH_STATUS = 0x00;
const uint8_t MPR121_ELECTRODE_FILTERED_DATA = 0x04;
const uint8_t MPR121_BASELINE_VALUE = 0x1E;
const uint8_t MPR121_MAX_HALF_DELTA_RISING = 0x2B;
const uint8_t MPR121_NOISE_HALF_DELTA_RISING = 0x2C;
const uint8_t MPR121_NOISE_COUNT_LIMIT_RISING = 0x2D;
const uint8_t MPR121_FILTER_DELAY_COUNT_RISING = 0x2E;
const uint8_t MPR121_MAX_HALF_DELTA_FALLING = 0x2F;
const uint8_t MPR121_NOISE_HALF_DELTA_FALLING = 0x30;
const uint8_t MPR121_NOISE_COUNT_LIMIT_FALLING = 0x31;
const uint8_t MPR121_FILTER_DELAY_COUNT_FALLING = 0x32;
const uint8_t MPR121_NOISE_HALF_DELTA_TOUCHED = 0x33;
const uint8_t MPR121_NOISE_COUNT_LIMIT_TOUCHED = 0x34;
const uint8_t MPR121_FILTER_DELAY_COUNT_TOUCHED = 0x35;
const uint8_t MPR121_TOUCH_THRESHOLD = 0x41;
const uint8_t MPR121_RELEASE_THRESHOLD = 0x42;
const uint8_t MPR121_DEBOUNCE = 0x5B;
const uint8_t MPR121_CONFIG1 = 0x5C;
const uint8_t MPR121_CONFIG2 = 0x5D;
const uint8_t MPR121_ELECTRODE_CONFIG = 0x5E;
const uint8_t MPR121_SOFT_RESET = 0x80;

/**
 * @brief Construct a new MPR121::MPR121 object with the default values, no reset.
 */
MPR121::MPR121() {
    this->i2c_port = i2c0;
    this->i2c_addr = 0x5A;
}

/**
 * @brief Construct a new MPR121::MPR121 object and reset it to its default state.
 * @param i2c_addr The address to use for I2C communication for this sensor
 */
MPR121::MPR121(i2c_inst_t *i2c_port, uint8_t i2c_addr) {
    this->i2c_port = i2c_port;
    this->i2c_addr = i2c_addr;
    reset();
}

/**
 * @brief Writes a single byte to the given register.
 * @param reg The register to write to
 * @param val The value to write
 */
void MPR121::write_8(uint8_t reg, uint8_t val) {
    uint8_t buf[] = { reg, val };
    i2c_write_blocking(this->i2c_port, this->i2c_addr, buf, 2, false);
}

/**
 * @brief Reads a single byte from the given register.
 * @param reg The register to read from
 * @return uint8_t The value of the register
 */
uint8_t MPR121::read_8(uint8_t reg) {
    uint8_t val;
    i2c_write_blocking(this->i2c_port, this->i2c_addr, &reg, 1, true);
    i2c_read_blocking(this->i2c_port, this->i2c_addr, &val, 1, false);
    return val;
}

/**
 * @brief Reads a 16-byte value from the given register.
 * @param reg The register to read from
 * @return uint8_t The value of the register
 */
uint16_t MPR121::read_16(uint8_t reg) {
    uint8_t vals[2];
    i2c_write_blocking(this->i2c_port, this->i2c_addr, &reg, 1, true);
    i2c_read_blocking(this->i2c_port, this->i2c_addr, vals, 2, false);
    return vals[1] << 8 | vals[0];
}

/**
 * @brief Resets the state of the MPR121 sensor.
 */
void MPR121::reset() {
    // Soft reset
    write_8(MPR121_SOFT_RESET, 0x63);

    // Reset electrode configuration to defaults - enter stop mode
    // Config registers are read-only unless in stop mode
    write_8(MPR121_ELECTRODE_CONFIG, 0x00);

    // Check CDT, SFI, ESI configuration is at defaults
    // A soft reset puts CONFIG2 (0x5D) at 0x24
    // Charge Discharge Time, CDT=1 (0.5us charge time)
    // Second Filter Iterations, SFI=0 (4x samples taken)
    // Electrode Sample Interval, ESI=4 (16ms period)
    if (read_8(MPR121_CONFIG2) != 0x24) {
        return;
    }

    // Set touch and release trip thresholds
    for (int i = 0; i < 12; i++) {
        set_threshold(15, 7, i);
    }

    // Configure electrode filtered data and baseline registers
    write_8(MPR121_MAX_HALF_DELTA_RISING, 0x01);
    write_8(MPR121_MAX_HALF_DELTA_FALLING, 0x01);
    write_8(MPR121_NOISE_HALF_DELTA_RISING, 0x01);
    write_8(MPR121_NOISE_HALF_DELTA_FALLING, 0x05);
    write_8(MPR121_NOISE_HALF_DELTA_TOUCHED, 0x00);
    write_8(MPR121_NOISE_COUNT_LIMIT_RISING, 0x0E);
    write_8(MPR121_NOISE_COUNT_LIMIT_FALLING, 0x01);
    write_8(MPR121_NOISE_COUNT_LIMIT_TOUCHED, 0x00);
    write_8(MPR121_FILTER_DELAY_COUNT_RISING, 0x00);
    write_8(MPR121_FILTER_DELAY_COUNT_FALLING, 0x00);
    write_8(MPR121_FILTER_DELAY_COUNT_TOUCHED, 0x00);

    // Set config registers
    // Debounce Touch, DT=0 (increase up to 7 to reduce noise)
    // Debounce Release, DR=0 (increase up to 7 to reduce noise)
    write_8(MPR121_DEBOUNCE, 0x00);
    // First Filter Iterations, FFI=0 (6x samples taken)
    // Charge Discharge Current, CDC=16 (16uA)
    write_8(MPR121_CONFIG1, 0x10);
    // Charge Discharge Time, CDT=1 (0.5us charge time)
    // Second Filter Iterations, SFI=0 (4x samples taken)
    // Electrode Sample Interval, ESI=0 (1ms period)
    write_8(MPR121_CONFIG2, 0x20);

    // Enable all electrodes - enter run mode
    // Calibration Lock, CL=10 (baseline tracking enabled, initial value 5 high bits)
    // Proximity Enable, ELEPROX_EN=0 (proximity detection disabled)
    // Electrode Enable, ELE_EN=15 (enter run mode for 12 electrodes)
    write_8(MPR121_ELECTRODE_CONFIG, 0x8F);
}

/**
 * @brief Sets the thresholds for a single sensor.
 * 
 * @param touch The touch threshold
 * @param release The release threshold
 * @param sensor Which sensor/electrode these values are for
 */
void MPR121::set_threshold(uint8_t touch, uint8_t release, uint8_t sensor) {
    // You can only modify thresholds when in stop mode
    uint8_t config = read_8(MPR121_ELECTRODE_CONFIG);
    if (config != 0) { write_8(MPR121_ELECTRODE_CONFIG, 0); }

    write_8(MPR121_TOUCH_THRESHOLD + sensor * 2, touch);
    write_8(MPR121_RELEASE_THRESHOLD + sensor * 2, release);

    // Return to previous mode if temporarily entered stop mode
    if (config != 0) { write_8(MPR121_ELECTRODE_CONFIG, config); }
}

/**
 * @brief Gets the filtered data for the given electrode.
 * 
 * @param electrode The electrode to read
 * @return uint16_t The filtered data from the given electrode
 */
uint16_t MPR121::filtered_data(uint8_t electrode) {
    return read_16(MPR121_ELECTRODE_FILTERED_DATA + electrode * 2);
}

/**
 * @brief Gets the baseline data for the given electrode.
 * 
 * @param electrode The electrode to read
 * @return uint8_t The baseline data from the given electrode
 */
uint8_t MPR121::baseline_data(uint8_t electrode) {
    return read_8(MPR121_BASELINE_VALUE + electrode) << 2;
}

/**
 * @brief Gets a 16-bit bitfield whose lower 12 bits represent the touch state for
 * the electrodes of this sensor.
 * @return uint16_t The bitfield representing touch states
 */
uint16_t MPR121::get_all_touched() {
    return read_16(MPR121_TOUCH_STATUS);
}

/**
 * @brief Returns whether the given electrode is currently being touched.
 * @param electrode Which electrode to read from
 * @return true If the electrode is being touched
 * @return false If the electrode is not being touched
 */
bool MPR121::is_electrode_touched(uint8_t electrode) {
    uint16_t t = get_all_touched();
    return (t & (1 << electrode)) != 0;
}
