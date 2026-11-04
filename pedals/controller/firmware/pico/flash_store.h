/* Switchyard's storage region in the RP2040's flash (setlists and the current part). */
#ifndef FLASH_STORE_H
#define FLASH_STORE_H

#include <stdint.h>

void store_check_fits(void);    /* stops with an error if the firmware runs into the store */
void store_read(void *ctx, uint32_t off, void *buf, uint32_t n);
void store_write(void *ctx, uint32_t off, const void *buf, uint32_t n);
void store_erase(void *ctx, uint32_t off, uint32_t n);

#endif
