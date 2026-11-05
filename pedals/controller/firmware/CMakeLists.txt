# Switchyard firmware: two Pico targets.
#   switchyard  - the controller (stage 1 breadboard, and the board later)
#   standin     - the stand-in pedal: a second Pico that acts like a Linework pedal on the loop
#
# Build (needs the Pico SDK in lib/pico-sdk, CMake, Ninja and the Arm GNU toolchain):
#   cmake -S . -B build -G Ninja
#   cmake --build build
# Output: build/switchyard.uf2 and build/standin.uf2 (drag onto the Pico in BOOTSEL mode).
cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/pico-sdk)
endif()
set(PICO_BOARD pico CACHE STRING "Board type")
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(switchyard_firmware C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

# breadboard (stage 1): no SENSE_9V divider, so 9V is reported as present
set(SWITCHYARD_BOARD "breadboard" CACHE STRING "breadboard or pcb")

# shared Linework code (libraries/linework): protocol, pedal behaviour, settings in flash
set(LW ${CMAKE_CURRENT_LIST_DIR}/../../../libraries/linework)
set(CORE ${LW}/lw_proto.c)

add_executable(switchyard
    ${CORE} core/switchyard.c
    pico/switchyard_main.c pico/flash_store.c pico/ht16k33.c pico/usb_descriptors.c)
target_include_directories(switchyard PRIVATE core pico pico/usb ${LW} ${LW}/pico)   # pico/usb: TinyUSB settings, Switchyard only
if(SWITCHYARD_BOARD STREQUAL "pcb")
    target_compile_definitions(switchyard PRIVATE BOARD_HAS_SENSE_9V=1)
endif()
target_link_libraries(switchyard PRIVATE pico_stdlib pico_unique_id hardware_uart hardware_i2c
    hardware_flash hardware_sync hardware_adc tinyusb_device tinyusb_board)
pico_add_extra_outputs(switchyard)

add_executable(standin
    ${CORE} ${LW}/pedal.c ${LW}/pd_store.c
    pico/standin_main.c ${LW}/pico/pedal_flash.c)
target_include_directories(standin PRIVATE core pico ${LW} ${LW}/pico)
target_link_libraries(standin PRIVATE pico_stdlib pico_unique_id hardware_uart hardware_flash hardware_sync)
pico_enable_stdio_usb(standin 1)      # debug messages over the Pico's USB serial port
pico_enable_stdio_uart(standin 0)     # UART0 is the MIDI loop
pico_add_extra_outputs(standin)
