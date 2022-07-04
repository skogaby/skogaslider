# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/usr/local/share/pico-sdk")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(skogaslider-firmware LANGUAGES C CXX ASM)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)
set(PICO_CXX_ENABLE_RTTI 1)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

include("lib/PicoLED/PicoLed.cmake")

# Add executable. Default name is the project name, version 0.1

# Add any PICO_CONFIG entries specified in the Advanced settings
add_compile_definitions(PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)

add_executable(skogaslider-firmware main.cpp)

pico_set_program_name(skogaslider-firmware "skogaslider-firmware")
pico_set_program_version(skogaslider-firmware "0.1")

pico_enable_stdio_uart(skogaslider-firmware 0)
pico_enable_stdio_usb(skogaslider-firmware 1)

# Add the standard library to the build
target_link_libraries(skogaslider-firmware pico_stdlib pico_multicore)

# Add any user requested libraries
target_link_libraries(skogaslider-firmware
        hardware_i2c
        hardware_dma
        hardware_pio
        PicoLed
)

target_sources(skogaslider-firmware PRIVATE
        leds/LedController.h
        leds/LedController.cpp
        slider/TouchSlider.h
        slider/TouchSlider.cpp
        slider/mpr121/Mpr121.h
        slider/mpr121/Mpr121.cpp
)

target_compile_options(skogaslider-firmware PRIVATE -fpermissive)

pico_add_extra_outputs(skogaslider-firmware)
