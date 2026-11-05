#include "flashover.h"
#include "lw_ui.h"
#include <string.h>

#define DEFAULT_BRIGHTNESS 128

static uint32_t now(flashover *f) { return f->hal.now_ms(f->hal.ctx); }

/* ---------------------------------------------------------------- pots */

/* Knob value 0-127 to a wiper code 0-255. The MCP45HV51 has 256 positions: code 00 puts the
 * wiper at terminal B, FF at terminal A. As wired (Flashover schematic):
 *  - Gain: A to R4, wiper and B to C3, so the resistance is A to wiper. FF = 0 ohms = most gain.
 *  - Tone: A on the low-pass (dark) side, B on the high-pass (bright) side. Knob 0 is darkest,
 *    so the code runs backwards.
 *  - Level: A from the amp, B at VB. FF = full level. In Hard mode it is cut to 38% (97/255)
 *    to match Soft's loudness.
 * The mapping is linear, like the B-taper pots on the stage 1 breadboard. */
uint8_t fo_pot_code(int pot, uint8_t value, bool hard)
{
    unsigned c = ((unsigned)(value & 0x7F) * 255u + 63u) / 127u;
    switch (pot) {
    case FO_POT_TONE: return (uint8_t)(255u - c);
    case FO_POT_LEVEL: return (uint8_t)(hard ? (c * 97u + 127u) / 255u : c);
    default: return (uint8_t)c;
    }
}

static uint8_t target(flashover *f, int pot)
{
    if (pot == FO_POT_LEVEL && f->mute)
        return 0;                            /* wiper at VB: silence */
    return fo_pot_code(pot, f->pd.value[pot], f->hard);   /* the relay in use, not the value */
}

static void write_pot(flashover *f, int pot, uint8_t code)
{
    f->pot[pot] = code;
    f->hal.set_pot(f->hal.ctx, pot, code);
}

/* moves each pot toward its target by FO_RAMP codes per elapsed ms */
static void ramp(flashover *f, uint32_t ms)
{
    int step = (int)(ms * FO_RAMP);
    for (int k = 0; k < 3; k++) {
        uint8_t t = target(f, k);
        if (f->pot[k] != t)
            write_pot(f, k, lw_ramp(f->pot[k], t, step));
    }
}

/* ---------------------------------------------------------------- relays */

static bool want_hard(const flashover *f) { return f->pd.value[FO_MODE] != 0; }
static bool want_on(const flashover *f) { return f->pd.value[FO_ON] != 0; }

static void set_relays(flashover *f)
{
    f->hard = want_hard(f);
    f->on = want_on(f);
    f->hal.set_relays(f->hal.ctx, f->hard, f->on);
}

/* A relay switch: Level ramps down to 0, the relays switch, they settle, Level ramps back up.
 * A change that arrives meanwhile is picked up before Level comes back. */
static void relays_poll(flashover *f, uint32_t t)
{
    bool differ = f->hard != want_hard(f) || f->on != want_on(f);
    switch (f->seq) {
    case 0:
        if (differ) {
            f->mute = true;
            f->seq = 1;
        }
        break;
    case 1:
        if (f->pot[FO_POT_LEVEL] == 0) {
            set_relays(f);
            f->seq = 2;
            f->seq_t = t;
        }
        break;
    default:
        if (t - f->seq_t >= FO_SETTLE_MS) {
            if (differ) {
                set_relays(f);
                f->seq_t = t;
            } else {
                f->mute = false;
                f->seq = 0;
            }
        }
        break;
    }
}

/* ---------------------------------------------------------------- LEDs */

void fo_render(const flashover *f, uint8_t out[FO_LEDS], uint32_t t)
{
    unsigned light[FO_LEDS];
    memset(light, 0, sizeof light);
    unsigned bright = f->hal.brightness ? f->hal.brightness : DEFAULT_BRIGHTNESS;
    if (f->setup) {
        /* channel 1-11: that LED on the Gain ring; 12-16: Gain LED 11 plus Tone LED 1-5 */
        unsigned ch = f->setup_ch + 1u;
        light[(ch < 11 ? ch : 11) - 1] = 256;
        if (ch > 11)
            light[FO_RING + ch - 12] = 256;
        bool phase = (t / 250u) % 2u;        /* Soft and Hard blink in turn: setup mode */
        light[FO_LED_SOFT] = phase ? 256 : 0;
        light[FO_LED_HARD] = phase ? 0 : 256;
    } else {
        lw_ring_bar(&light[0], f->pd.value[FO_GAIN]);
        lw_ring_dot(&light[FO_RING], f->pd.value[FO_TONE]);
        lw_ring_bar(&light[2 * FO_RING], f->pd.value[FO_LEVEL]);
        /* the LEDs show what the relays are doing, or about to do */
        light[want_hard(f) ? FO_LED_HARD : FO_LED_SOFT] = 256;
        light[FO_LED_ON] = want_on(f) ? 256 : 0;
    }
    for (int i = 0; i < FO_LEDS; i++)
        out[i] = lw_led_pwm(light[i], bright);
}

static void leds_poll(flashover *f, uint32_t t, bool force)
{
    if (!force && t - f->last_render < 10)
        return;
    f->last_render = t;
    uint8_t p[FO_LEDS];
    fo_render(f, p, t);
    if (force || memcmp(p, f->leds, sizeof p) != 0) {
        memcpy(f->leds, p, sizeof p);
        f->hal.set_leds(f->hal.ctx, p);
    }
    bool link = pd_linked(&f->pd);
    if (force || link != f->link) {
        f->link = link;
        f->hal.set_link_led(f->hal.ctx, link);
    }
}

/* ---------------------------------------------------------------- public */

static uint32_t pd_now(void *c) { flashover *f = c; return now(f); }
static void pd_send(void *c, const uint8_t *m, int n) { flashover *f = c; f->hal.midi_send(f->hal.ctx, m, n); }

void fo_init(flashover *f, const fo_hal *hal, bool setup)
{
    memset(f, 0, sizeof *f);
    f->hal = *hal;
    pd_hal ph = {
        .ctx = f, .now_ms = pd_now, .send = pd_send, .flash = hal->flash,
        .ncontrols = FO_NCONTROLS,
        .cc = {20, 21, 22, 81, 80},
        .kind = {PD_KNOB, PD_KNOB, PD_KNOB, PD_SWITCH, PD_ONOFF},
        .value = {64, 64, 64, 0, 0},         /* a new pedal: knobs at noon, Soft, bypassed */
    };
    memcpy(ph.model, FO_MODEL, 2);
    memcpy(ph.fw, FO_FW, 3);
    memcpy(ph.serial, hal->serial, LW_SERIAL_LEN);
    pd_init(&f->pd, &ph);                    /* loads the saved channel and sound */

    /* power-up: Level silent, relays and the other pots straight to the saved setting, then
     * Level ramps up once the relays have settled */
    uint32_t t = now(f);
    f->mute = true;
    write_pot(f, FO_POT_LEVEL, 0);
    write_pot(f, FO_POT_GAIN, target(f, FO_POT_GAIN));
    write_pot(f, FO_POT_TONE, target(f, FO_POT_TONE));
    set_relays(f);
    f->seq = 2;
    f->seq_t = f->last_ms = t;

    f->setup = setup;
    f->setup_ch = f->pd.channel <= 0x0F ? f->pd.channel : 0;
    leds_poll(f, t, true);
}

void fo_midi_byte(flashover *f, uint8_t b) { pd_byte(&f->pd, b); }

void fo_turn(flashover *f, int knob, int detents)
{
    if (knob < FO_GAIN || knob > FO_LEVEL || !detents)
        return;
    if (f->setup) {
        if (knob == FO_GAIN) {
            int c = f->setup_ch + detents;
            f->setup_ch = (uint8_t)(c < 0 ? 0 : c > 15 ? 15 : c);
        }
        return;
    }
    int step = lw_knob_step(&f->last_detent[knob], now(f));
    pd_set_control(&f->pd, knob, (uint8_t)lw_knob_add(f->pd.value[knob], detents, step));
}

void fo_press(flashover *f, int button)
{
    if (f->setup) {
        if (button == FO_BTN_FOOT) {         /* save the channel and start normally */
            pd_set_channel(&f->pd, f->setup_ch);
            f->setup = false;
        }
        return;
    }
    switch (button) {
    case FO_BTN_FOOT:
        pd_set_control(&f->pd, FO_ON, want_on(f) ? 0 : 127);
        break;
    case FO_BTN_MODE:
        pd_set_control(&f->pd, FO_MODE, want_hard(f) ? 0 : 1);
        break;
    default:
        break;                               /* knob pushes: not used yet (Product Document, open) */
    }
}

void fo_poll(flashover *f)
{
    uint32_t t = now(f);
    pd_poll(&f->pd);
    uint32_t ms = t - f->last_ms;
    if (ms) {
        f->last_ms = t;
        relays_poll(f, t);
        ramp(f, ms > 8 ? 8 : ms);
    }
    leds_poll(f, t, false);
}
