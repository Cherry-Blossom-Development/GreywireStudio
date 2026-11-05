/* Panel helpers shared by the Linework pedals (Flashover, Tap Changer): the endless knobs'
 * speed-up, the 11-LED rings, LED brightness, and ramping a digital pot. Plain C. */
#ifndef LW_UI_H
#define LW_UI_H

#include <stdint.h>

#define LW_RING 11            /* LEDs per ring, about 7 o'clock to 5 o'clock, LED 6 at 12 */
#define LW_LIGHT_FULL 256u    /* "light" values run 0-256 per LED before brightness */

/* Steps for one knob click: 1 when turned slowly, 2 for clicks under 80 ms apart, 4 under 30 ms.
 * 128 steps is about 5 turns slowly, a little over 1 turn spun (24 clicks per turn). */
int lw_knob_step(uint32_t *last_click, uint32_t now);
int lw_knob_add(uint8_t v, int clicks, int step);       /* clamped to 0-127 */

/* A value 0-127 on a ring: a bar from LED 1 (always lit), or a single dot. The end of the bar
 * and the dot fade across two LEDs, so the ring moves smoothly. */
void lw_ring_bar(unsigned light[LW_RING], uint8_t v);
void lw_ring_dot(unsigned light[LW_RING], uint8_t v);

/* Light 0-256 to the LED driver's PWM 0-255, with brightness 1-255 and a rough gamma. */
uint8_t lw_led_pwm(unsigned light, unsigned bright);

/* One ramp step: from cur toward target by at most step. */
uint8_t lw_ramp(uint8_t cur, uint8_t target, int step);

#endif
