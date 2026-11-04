/* HT16K33 4-digit 7-segment display, wired as on Adafruit's backpack (product 878):
 * digits on COM0, COM1, COM3, COM4 (COM2 is the backpack's colon), segments A-G and DP on ROW0-7. */
#ifndef HT16K33_H
#define HT16K33_H

#include <stdbool.h>
#include <stdint.h>
#include "hardware/i2c.h"

bool ht16k33_init(i2c_inst_t *i2c, uint8_t addr, uint8_t brightness);   /* brightness 0-15 */
void ht16k33_show(const char text[4], uint8_t dp);                         /* '0'-'9', '-', ' ' */

#endif
