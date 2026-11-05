/* A Linework pedal's settings area in the RP2040's flash: the 8 KB log of pd_store.h, at the
 * start of the last 64 KB of flash. (The rest of that 64 KB is kept free for later.) */
#ifndef PEDAL_FLASH_H
#define PEDAL_FLASH_H

#include "pd_store.h"

#define PEDAL_FLASH_BASE (PICO_FLASH_SIZE_BYTES - 64u * 1024u)

void pedal_flash_check(void);            /* stops with an error if the firmware runs into it */
extern const lw_flash pedal_flash;

#endif
