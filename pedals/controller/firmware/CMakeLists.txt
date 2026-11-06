# Switchyard firmware: two Pico targets.
#   switchyard  - the controller (stage 1 breadboard, and the board later)
#   standin     - the stand-in pedal: a second Pico that acts like a Linework pedal on the loop
#
# Build (needs the Pico SDK in lib/pico-sdk, CMake, Ninja and the Arm GNU toolchain):
#   cmake -S . -B build -G Ninja
#   cmake --build build
# Output (drag onto the Pico in BOOTSEL mode):
#   build/switchyard_full.uf2  Switchyard's loader + firmware, for a blank chip or a new Pico
#   build/switchyard.uf2       the firmware alone, once the loader is on; also what the app sends
#                              to update Switchyard over USB
#   build/standin.uf2          the stand-in pedal
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
include(${CMAKE_CURRENT_LIST_DIR}/../../../libraries/linework/pico/linework.cmake)   # sets LW
set(CORE ${LW}/lw_proto.c)

add_executable(switchyard
    ${CORE} core/switchyard.c core/sy_update.c
    pico/switchyard_main.c pico/flash_store.c pico/ht16k33.c pico/usb_descriptors.c)
target_include_directories(switchyard PRIVATE core pico pico/usb ${LW} ${LW}/pico)   # pico/usb: TinyUSB settings, Switchyard only
if(SWITCHYARD_BOARD STREQUAL "pcb")
    target_compile_definitions(switchyard PRIVATE BOARD_HAS_SENSE_9V=1)
endif()
target_link_libraries(switchyard PRIVATE pico_stdlib pico_unique_id hardware_uart hardware_i2c
    hardware_flash hardware_sync hardware_adc tinyusb_device tinyusb_board)
target_compile_definitions(switchyard PRIVATE LW_LAYOUT_SWITCHYARD=1)   # flash layout: lw_fw.h
linework_firmware(switchyard)                # runs at 32 KB, after the loader; takes updates
pico_add_extra_outputs(switchyard)

# Switchyard's loader (pico/sy_loader.c): installs updates; recovery mode over USB when both
# footswitches are held at power-up
add_executable(switchyard_loader pico/sy_loader.c core/sy_update.c pico/usb_descriptors.c
    ${CORE} ${LW}/lw_fw.c ${LW}/pico/lw_update.c)
target_include_directories(switchyard_loader PRIVATE core pico pico/usb ${LW} ${LW}/pico)
target_compile_options(switchyard_loader PRIVATE -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Os)
target_compile_definitions(switchyard_loader PRIVATE LW_LAYOUT_SWITCHYARD=1 SY_RECOVERY=1)
target_link_libraries(switchyard_loader PRIVATE pico_stdlib pico_unique_id hardware_flash hardware_sync
    hardware_watchdog tinyusb_device tinyusb_board)
pico_add_linker_script_override_path(switchyard_loader ${LW}/pico/link_loader FILES pico_flash_region.ld)
pico_add_extra_outputs(switchyard_loader)
linework_full(switchyard switchyard_loader)

add_executable(standin
    ${CORE} ${LW}/pedal.c ${LW}/pd_store.c ${LW}/lw_fw.c
    pico/standin_main.c ${LW}/pico/pedal_flash.c)
target_include_directories(standin PRIVATE core pico ${LW} ${LW}/pico)
target_link_libraries(standin PRIVATE pico_stdlib pico_unique_id hardware_uart hardware_flash hardware_sync)
pico_enable_stdio_usb(standin 1)      # debug messages over the Pico's USB serial port
pico_enable_stdio_uart(standin 0)     # UART0 is the MIDI loop
pico_add_extra_outputs(standin)
