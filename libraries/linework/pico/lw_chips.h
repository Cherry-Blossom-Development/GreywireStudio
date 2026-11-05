/* The I2C chips the Linework pedals share: MCP45HV51 digital pots and the IS31FL3236A LED
 * driver (each on its own bus, since their addresses overlap). */
#ifndef LW_CHIPS_H
#define LW_CHIPS_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

void lw_i2c_init(i2c_inst_t *i2c, unsigned sda, unsigned scl);      /* 400 kHz; pull-ups on the board */

/* MCP45HV51 (datasheet DS20005304A, section 7): command byte = address AD3:AD0 (wiper 0 = 0),
 * command C1:C0 (00 = write data), D9:D8 (unused), then the data byte. WLAT is tied low on the
 * boards, so the new value takes effect at once. Code 00 = wiper at terminal B, FF = at A. */
bool lw_pot_write(i2c_inst_t *i2c, uint8_t addr, uint8_t code);

/* IS31FL3236A (Lumissil datasheet rev. I): 00h shutdown (1 = normal), 01h-24h PWM, 25h update,
 * 26h-49h LED control (bit 0 on, bits 2:1 current: 00 = full), 4Ah global (0 = normal), 4Bh PWM
 * frequency (1 = 22 kHz, above hearing), 4Fh reset. en_pin drives SDB (high = on). */
bool lw_leds_init(i2c_inst_t *i2c, uint8_t addr, unsigned en_pin);
void lw_leds_write(const uint8_t pwm[36]);          /* does nothing if init failed */
bool lw_leds_answered(void);

#endif
