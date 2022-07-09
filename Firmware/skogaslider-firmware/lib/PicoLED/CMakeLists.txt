# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Initialise the Pico SDK
pico_sdk_init()

# Add library cpp files
add_library(PicoLed INTERFACE)
target_sources(PicoLed INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/PicoLedTarget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/PicoLedController.cpp
    ${CMAKE_CURRENT_LIST_DIR}/PioStrip.cpp
    ${CMAKE_CURRENT_LIST_DIR}/WS2812B.cpp
)

pico_generate_pio_header(PicoLed ${CMAKE_CURRENT_LIST_DIR}/WS2812B.pio)

# Add include directory
target_include_directories(PicoLed INTERFACE ${CMAKE_CURRENT_LIST_DIR})

# Add the standard library to the build
target_link_libraries(PicoLed INTERFACE pico_stdlib hardware_pio)