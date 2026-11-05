# Tap Changer firmware for the RP2040 on the main board (a Pico on a breadboard uses the same pins).
#
# Build (needs CMake, Ninja, the Arm GNU toolchain and the Pico SDK, which is the git submodule
# in pedals/controller/firmware/lib/pico-sdk, shared with Switchyard and Flashover):
#   cmake -S . -B build -G Ninja
#   cmake --build build
# Output: build/tapchanger.uf2, and build/tapchanger_debug.uf2 for a breadboard
cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/../../controller/firmware/lib/pico-sdk)
endif()
set(PICO_BOARD pico CACHE STRING "Board type")   # same 12 MHz crystal and 2 MB flash as the board
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(tapchanger_firmware C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

set(LW ${CMAKE_CURRENT_LIST_DIR}/../../../libraries/linework)
set(SRC core/tapchanger.c pico/tapchanger_main.c
    ${LW}/lw_proto.c ${LW}/pedal.c ${LW}/pd_store.c ${LW}/lw_ui.c
    ${LW}/pico/pedal_flash.c ${LW}/pico/lw_midi.c ${LW}/pico/lw_input.c ${LW}/pico/lw_chips.c)

foreach(T tapchanger tapchanger_debug)
    add_executable(${T} ${SRC})
    target_include_directories(${T} PRIVATE core pico ${LW} ${LW}/pico)
    target_compile_options(${T} PRIVATE -Wall -Wextra -Wno-unused-function -Wno-unused-parameter)
    target_link_libraries(${T} PRIVATE pico_stdlib pico_unique_id hardware_uart hardware_i2c
        hardware_flash hardware_sync hardware_irq)
    pico_enable_stdio_uart(${T} 0)          # UART0 is MIDI
    pico_add_extra_outputs(${T})
endforeach()
target_compile_definitions(tapchanger_debug PRIVATE TC_DEBUG_USB=1)
pico_enable_stdio_usb(tapchanger_debug 1)
pico_enable_stdio_usb(tapchanger 0)
