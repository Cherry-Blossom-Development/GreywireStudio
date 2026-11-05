# Shared build pieces for the Linework pedals' RP2040 firmware. Include after pico_sdk_init().
#   linework_firmware(<target>)   link <target> to run at 32 KB, after the loader, with updates
#   linework_loader(<name> MODEL0 MODEL1 KEY1 KEY2 LINK_LED)   build <name>_loader
#   linework_full(<target> <loader>)   <target>_full.uf2: loader + firmware, for a blank chip
set(LW ${CMAKE_CURRENT_LIST_DIR}/..)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

function(linework_firmware TARGET)
    target_sources(${TARGET} PRIVATE ${LW}/lw_fw.c ${LW}/pico/lw_update.c)
    target_link_libraries(${TARGET} PRIVATE hardware_watchdog)
    pico_add_linker_script_override_path(${TARGET} ${LW}/pico/link_app FILES pico_flash_region.ld)
endfunction()

function(linework_loader NAME MODEL0 MODEL1 KEY1 KEY2 LINK_LED)
    set(T ${NAME}_loader)
    add_executable(${T} ${LW}/pico/loader.c ${LW}/lw_proto.c ${LW}/pedal.c ${LW}/pd_store.c ${LW}/lw_fw.c
        ${LW}/pico/lw_update.c ${LW}/pico/lw_midi.c ${LW}/pico/pedal_flash.c)
    target_include_directories(${T} PRIVATE ${LW} ${LW}/pico)
    target_compile_options(${T} PRIVATE -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Os)
    target_compile_definitions(${T} PRIVATE LOADER_MODEL0=${MODEL0} LOADER_MODEL1=${MODEL1}
        LOADER_KEY1=${KEY1} LOADER_KEY2=${KEY2} LOADER_LINK_LED=${LINK_LED} LOADER_MIDI_TX=0 LOADER_MIDI_RX=1)
    target_link_libraries(${T} PRIVATE pico_stdlib pico_unique_id hardware_uart hardware_flash
        hardware_sync hardware_irq hardware_watchdog)
    pico_enable_stdio_uart(${T} 0)
    pico_enable_stdio_usb(${T} 0)
    pico_add_linker_script_override_path(${T} ${LW}/pico/link_loader FILES pico_flash_region.ld)
    pico_add_extra_outputs(${T})
endfunction()

function(linework_full TARGET LOADER)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_full.uf2
        COMMAND ${Python3_EXECUTABLE} ${LW}/tools/uf2_merge.py ${TARGET}_full.uf2 ${LOADER}.uf2 ${TARGET}.uf2
        DEPENDS ${TARGET} ${LOADER}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_custom_target(${TARGET}_full ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_full.uf2)
endfunction()
