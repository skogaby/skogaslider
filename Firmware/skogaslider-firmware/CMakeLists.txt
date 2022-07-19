cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(skogaslider-firmware LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_CXX_ENABLE_EXCEPTIONS 1)
set(PICO_CXX_ENABLE_RTTI 1)
set(PICO_SDK_PATH "/usr/local/share/pico-sdk")

pico_sdk_init()

include("lib/PicoLED/PicoLed.cmake")

add_compile_definitions(PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)

add_executable(skogaslider-firmware main.cpp)

pico_set_program_name(skogaslider-firmware "skogaslider-firmware")
pico_set_program_version(skogaslider-firmware "0.1")

pico_enable_stdio_usb(skogaslider-firmware 1)

target_include_directories(skogaslider-firmware PUBLIC ${CMAKE_CURRENT_LIST_DIR}/tinyusb)

target_link_libraries(skogaslider-firmware
        pico_stdlib
        pico_multicore
        tinyusb_device
        hardware_i2c
        hardware_pio
        PicoLed
)

target_sources(skogaslider-firmware PRIVATE
        leds/led_controller.cpp
        sega_hardware/led_board/sega_led_board.cpp
        sega_hardware/slider/sega_slider.cpp
        sega_hardware/serial/sega_serial_reader.cpp
        slider/touch_slider.cpp
        slider/mpr121/mpr121.cpp
        tinyusb/usb_descriptors.c
        usb_output/usb_output.cpp
)

target_compile_options(skogaslider-firmware PRIVATE -fpermissive)

pico_add_extra_outputs(skogaslider-firmware)
