#include "lw_ui.h"
#include <string.h>

int lw_knob_step(uint32_t *last_click, uint32_t now)
{
    uint32_t gap = now - *last_click;
    *last_click = now;
    return gap < 30 ? 4 : gap < 80 ? 2 : 1;
}

int lw_knob_add(uint8_t v, int clicks, int step)
{
    int n = v + clicks * step;
    return n < 0 ? 0 : n > 127 ? 127 : n;
}

/* the position on the ring in 1/256ths of an LED step, 0 to 10 x 256 */
static unsigned ring_x(uint8_t v) { return ((unsigned)(v & 0x7F) * 10u * 256u + 63u) / 127u; }

void lw_ring_bar(unsigned light[LW_RING], uint8_t v)
{
    unsigned x = ring_x(v);
    for (unsigned k = 0; k < LW_RING; k++) {
        unsigned at = k * 256u;
        light[k] = x >= at ? 256u : (x + 256u > at ? x + 256u - at : 0u);
    }
}

void lw_ring_dot(unsigned light[LW_RING], uint8_t v)
{
    unsigned x = ring_x(v), k = x / 256u, frac = x % 256u;
    memset(light, 0, LW_RING * sizeof *light);
    light[k] = 256u - frac;
    if (frac && k + 1 < LW_RING)
        light[k + 1] = frac;
}

uint8_t lw_led_pwm(unsigned light, unsigned bright)
{
    unsigned g = (light * light) >> 8;
    unsigned v = (g * bright) >> 8;      /* a sliver of the next LED rounds to off */
    return (uint8_t)(v > 255 ? 255 : v);
}

uint8_t lw_ramp(uint8_t cur, uint8_t target, int step)
{
    if (cur < target)
        return (uint8_t)(target - cur > step ? cur + step : target);
    return (uint8_t)(cur - target > step ? cur - step : target);
}
