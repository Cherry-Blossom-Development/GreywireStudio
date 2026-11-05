/* The I2C chips: three MCP45HV51 digital pots and the IS31FL3236A LED driver. */
#ifndef CHIPS_H
#define CHIPS_H

#include <stdint.h>
#include <stdbool.h>

void pots_init(void);
bool pot_write(int pot, uint8_t code);              /* 0 Gain, 1 Tone, 2 Level */
bool leds_init(void);
void leds_write(const uint8_t pwm[36]);
bool pot_answered(int pot);                         /* the last write to it was acknowledged */
bool leds_answered(void);

#endif
