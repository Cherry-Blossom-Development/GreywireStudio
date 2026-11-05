#include "tapchanger.h"
#include <math.h>
#include <string.h>

#define DEFAULT_BRIGHTNESS 128

/* The Boost stage (Tap Changer schematic, Audio sheet): gain = 1 + (R + Rw) / R4, where R is the
 * digital pot between terminal A (at the op-amp's - input) and its wiper, which is joined to
 * terminal B at the output. R = Rab x (255 - code) / 255, so code FF is the least boost. */
#define RAB 10000.0           /* MCP45HV51-103 */
#define RW 150.0              /* wiper resistance at about 9V (Product Document) */
#define R4 1100.0

static uint8_t code_of[128];  /* knob value -> wiper code, worked out once */

double tc_gain_db(uint8_t code)
{
    double r = RAB * (255 - code) / 255.0;
    return 20.0 * log10(1.0 + (r + RW) / R4);
}

static void make_table(void)
{
    double lo = tc_gain_db(255), hi = tc_gain_db(0);     /* about +1.1 dB and +20.2 dB */
    for (int v = 0; v < 128; v++) {
        double db = lo + (hi - lo) * v / 127.0;
        double r = (pow(10.0, db / 20.0) - 1.0) * R4 - RW;
        long c = 255 - lround(r * 255.0 / RAB);
        code_of[v] = (uint8_t)(c < 0 ? 0 : c > 255 ? 255 : c);
    }
}

uint8_t tc_pot_code(uint8_t value)
{
    if (!code_of[0])
        make_table();                        /* code_of[0] is 255 once made */
    return code_of[value & 0x7F];
}

static uint32_t now(tapchanger *t) { return t->hal.now_ms(t->hal.ctx); }
/* on, unless a firmware update is under way: then the pedal stays in bypass */
static bool want_on(const tapchanger *t) { return t->pd.value[TC_ON] != 0 && !pd_updating(&t->pd); }

static uint8_t target(tapchanger *t) { return t->duck ? 255 : tc_pot_code(t->pd.value[TC_BOOST]); }

static void write_pot(tapchanger *t, uint8_t code)
{
    t->pot = code;
    t->hal.set_pot(t->hal.ctx, code);
}

static void set_relay(tapchanger *t)
{
    t->on = want_on(t);
    t->hal.set_relay(t->hal.ctx, t->on);
}

/* A relay switch: Boost ramps to its minimum, the relay switches, settles, and Boost ramps back.
 * A change that arrives meanwhile is picked up before Boost comes back. */
static void relay_poll(tapchanger *t, uint32_t ms)
{
    bool differ = t->on != want_on(t);
    switch (t->seq) {
    case 0:
        if (differ) {
            t->duck = true;
            t->seq = 1;
        }
        break;
    case 1:
        if (t->pot == 255) {
            set_relay(t);
            t->seq = 2;
            t->seq_t = ms;
        }
        break;
    default:
        if (ms - t->seq_t >= TC_SETTLE_MS) {
            if (differ) {
                set_relay(t);
                t->seq_t = ms;
            } else {
                t->duck = false;
                t->seq = 0;
            }
        }
        break;
    }
}

void tc_render(const tapchanger *t, uint8_t out[TC_LEDS], uint32_t ms)
{
    unsigned light[TC_LEDS];
    memset(light, 0, sizeof light);
    unsigned bright = t->hal.brightness ? t->hal.brightness : DEFAULT_BRIGHTNESS;
    if (t->setup) {
        /* channel 1-11: that LED; 12-16: LED 11 and LED 1-5 together ("11 plus") */
        unsigned ch = t->setup_ch + 1u;
        light[(ch < 11 ? ch : 11) - 1] = LW_LIGHT_FULL;
        if (ch > 11)
            light[ch - 12] = LW_LIGHT_FULL;
        light[TC_LED_ON] = (ms / 250u) % 2u ? LW_LIGHT_FULL : 0;   /* On blinks: setup mode */
    } else {
        lw_ring_bar(light, t->pd.value[TC_BOOST]);
        light[TC_LED_ON] = want_on(t) ? LW_LIGHT_FULL : 0;
    }
    for (int i = 0; i < TC_LEDS; i++)
        out[i] = lw_led_pwm(light[i], bright);
}

static void leds_poll(tapchanger *t, uint32_t ms, bool force)
{
    if (!force && ms - t->last_render < 10)
        return;
    t->last_render = ms;
    uint8_t p[TC_LEDS];
    tc_render(t, p, ms);
    if (force || memcmp(p, t->leds, sizeof p) != 0) {
        memcpy(t->leds, p, sizeof p);
        t->hal.set_leds(t->hal.ctx, p);
    }
    bool link = pd_linked(&t->pd);
    if (force || link != t->link) {
        t->link = link;
        t->hal.set_link_led(t->hal.ctx, link);
    }
}

static uint32_t pd_now(void *c) { return now(c); }
static void pd_send(void *c, const uint8_t *m, int n) { tapchanger *t = c; t->hal.midi_send(t->hal.ctx, m, n); }

void tc_init(tapchanger *t, const tc_hal *hal, bool setup)
{
    memset(t, 0, sizeof *t);
    t->hal = *hal;
    pd_hal ph = {
        .ctx = t, .now_ms = pd_now, .send = pd_send, .flash = hal->flash, .update = hal->fw,
        .ncontrols = TC_NCONTROLS,
        .cc = {20, 80},
        .kind = {PD_KNOB, PD_ONOFF},
        .value = {64, 0},                    /* a new pedal: Boost at noon (about +10.6 dB), bypassed */
    };
    memcpy(ph.model, TC_MODEL, 2);
    memcpy(ph.fw, TC_FW, 3);
    memcpy(ph.serial, hal->serial, LW_SERIAL_LEN);
    pd_init(&t->pd, &ph);                    /* loads the saved channel and sound */

    /* power-up: the relay goes straight to its saved position with Boost at its minimum, and
     * Boost ramps up once the relay has settled */
    uint32_t ms = now(t);
    t->duck = true;
    write_pot(t, 255);
    set_relay(t);
    t->seq = 2;
    t->seq_t = t->last_ms = ms;
    t->setup = setup;
    t->setup_ch = t->pd.channel <= 0x0F ? t->pd.channel : 0;
    leds_poll(t, ms, true);
}

void tc_midi_byte(tapchanger *t, uint8_t b) { pd_byte(&t->pd, b); }

void tc_turn(tapchanger *t, int clicks)
{
    if (!clicks)
        return;
    if (t->setup) {
        int c = t->setup_ch + clicks;
        t->setup_ch = (uint8_t)(c < 0 ? 0 : c > 15 ? 15 : c);
        return;
    }
    int step = lw_knob_step(&t->last_click, now(t));
    pd_set_control(&t->pd, TC_BOOST, (uint8_t)lw_knob_add(t->pd.value[TC_BOOST], clicks, step));
}

void tc_press(tapchanger *t, int button)
{
    if (button != TC_BTN_FOOT)
        return;                              /* the knob's push: not used yet (Product Document, open) */
    if (t->setup) {
        pd_set_channel(&t->pd, t->setup_ch);
        t->setup = false;
        return;
    }
    pd_set_control(&t->pd, TC_ON, want_on(t) ? 0 : 127);
}

void tc_poll(tapchanger *t)
{
    uint32_t ms = now(t);
    pd_poll(&t->pd);
    uint32_t d = ms - t->last_ms;
    if (d) {
        t->last_ms = ms;
        relay_poll(t, ms);
        uint8_t tg = target(t);
        if (t->pot != tg)
            write_pot(t, lw_ramp(t->pot, tg, (int)((d > 8 ? 8 : d) * TC_RAMP)));
    }
    leds_poll(t, ms, false);
}
