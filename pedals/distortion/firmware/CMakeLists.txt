# Flashover firmware for the RP2040 on the main board (a Pico on a breadboard uses the same pins).
#
# Build (needs CMake, Ninja, the Arm GNU toolchain and the Pico SDK, which is the git submodule
# in pedals/controller/firmware/lib/pico-sdk, shared with Switchyard):
#   cmake -S . -B build -G Ninja
#   cmake --build build
# Output: build/flashover_full.uf2 (loader + firmware, for a blank chip) and build/flashover.uf2
# (the firmware alone, once the loader is on); flashover_debug*.uf2 for the breadboard
cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/../../controller/firmware/lib/pico-sdk)
endif()
set(PICO_BOARD pico CACHE STRING "Board type")   # same 12 MHz crystal and 2 MB flash as the board
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(flashover_firmware C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

include(${CMAKE_CURRENT_LIST_DIR}/../../../libraries/linework/pico/linework.cmake)   # sets LW

set(SRC core/flashover.c pico/flashover_main.c
    ${LW}/lw_proto.c ${LW}/pedal.c ${LW}/pd_store.c ${LW}/lw_ui.c
    ${LW}/pico/pedal_flash.c ${LW}/pico/lw_midi.c ${LW}/pico/lw_input.c ${LW}/pico/lw_chips.c)

# flashover: the pedal.  flashover_debug: the same, plus reports on the Pico's USB serial port
# (for the breadboard; the board has no USB).
foreach(T flashover flashover_debug)
    add_executable(${T} ${SRC})
    target_include_directories(${T} PRIVATE core pico ${LW} ${LW}/pico)
    target_compile_options(${T} PRIVATE -Wall -Wextra -Wno-unused-function -Wno-unused-parameter)
    target_link_libraries(${T} PRIVATE pico_stdlib pico_unique_id hardware_uart hardware_i2c
        hardware_flash hardware_sync hardware_irq)
    pico_enable_stdio_uart(${T} 0)          # UART0 is MIDI
    linework_firmware(${T})                  # runs at 32 KB, after the loader; takes updates
    pico_add_extra_outputs(${T})
endforeach()
target_compile_definitions(flashover_debug PRIVATE FO_DEBUG_USB=1)
pico_enable_stdio_usb(flashover_debug 1)
pico_enable_stdio_usb(flashover 0)

# the loader (model 01 00; recovery: footswitch + Soft/Hard held; Link LED on GPIO20), and
# loader + firmware in one file for a blank chip
linework_loader(flashover 0x01 0x00 15 16 20)
linework_full(flashover flashover_loader)
linework_full(flashover_debug flashover_loader)
