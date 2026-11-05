/* Firmware updates on the RP2040: the whole flash chip as an lw_flash device (offsets from the
 * start of flash), the update hooks for the pedal code (pd_fw_hal), and starting the firmware
 * from the loader. See lw_fw.h for the flash layout. */
#ifndef LW_UPDATE_H
#define LW_UPDATE_H

#include "pedal.h"
#include "lw_fw.h"

extern const lw_flash lw_flash_whole;
extern const pd_fw_hal lw_update_hal;       /* restart = watchdog reboot into the loader */

void lw_start_app(void);                    /* the loader: jump to the firmware; doesn't return */

#endif
