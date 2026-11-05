# Flashover firmware for the RP2040 on the main board (a Pico on a breadboard uses the same pins).
#
# Build (needs CMake, Ninja, the Arm GNU toolchain and the Pico SDK, which is the git submodule
# in pedals/controller/firmware/lib/pico-sdk, shared with Switchyard):
#   cmake -S . -B build -G Ninja
#   cmake --build build
# Output: build/flashover.uf2, and build/flashover_debug.uf2 for the breadboard
cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/../../controller/firmware/lib/pico-sdk)
endif()
set(PICO_BOARD pico CACHE STRING "Board type")   # same 12 MHz crystal and 2 MB flash as the board
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(flashover_firmware C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

set(LW ${CMAKE_CURRENT_LIST_DIR}/../../../libraries/linework)

set(SRC core/flashover.c
    ${LW}/lw_proto.c ${LW}/pedal.c ${LW}/pd_store.c ${LW}/pico/pedal_flash.c
    pico/flashover_main.c pico/chips.c)

# flashover: the pedal.  flashover_debug: the same, plus reports on the Pico's USB serial port
# (for the breadboard; the board has no USB).
foreach(T flashover flashover_debug)
    add_executable(${T} ${SRC})
    target_include_directories(${T} PRIVATE core pico ${LW} ${LW}/pico)
    target_compile_options(${T} PRIVATE -Wall -Wextra -Wno-unused-function -Wno-unused-parameter)
    target_link_libraries(${T} PRIVATE pico_stdlib pico_unique_id hardware_uart hardware_i2c
        hardware_flash hardware_sync hardware_irq)
    pico_enable_stdio_uart(${T} 0)          # UART0 is MIDI
    pico_add_extra_outputs(${T})
endforeach()
target_compile_definitions(flashover_debug PRIVATE FO_DEBUG_USB=1)
pico_enable_stdio_usb(flashover_debug 1)
pico_enable_stdio_usb(flashover 0)
