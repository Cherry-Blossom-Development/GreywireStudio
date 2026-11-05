/* Flashover's behaviour, independent of the hardware, so it runs unchanged on the RP2040 and in
 * the PC simulation (test/fo_sim.c).
 *
 * The Linework side (the loop, channel, presets, flash) is the shared pedal code in
 * libraries/linework/pedal.c. This file adds what is Flashover's own:
 *  - the three endless knobs, with faster steps when turned quickly
 *  - the digital pots: knob values to wiper codes, Level cut to 38% in Hard mode, and a short
 *    ramp on every change so the pots don't click or "zip"
 *  - the bypass and Soft/Hard relays, switched with Level turned down for a moment (no click)
 *  - the LED rings (bar for Gain and Level, dot for Tone), the Soft, Hard and On LEDs and the
 *    Link LED
 *  - setup mode: hold the footswitch while plugging in power, pick a channel with Gain, press
 *    the footswitch to save it. */
#ifndef FLASHOVER_H
#define FLASHOVER_H

#include "pedal.h"

/* controls, in the order of the settings pairs: 14 gg 15 tt 16 ll 51 mm 50 oo */
enum { FO_GAIN, FO_TONE, FO_LEVEL, FO_MODE, FO_ON, FO_NCONTROLS };
enum { FO_POT_GAIN, FO_POT_TONE, FO_POT_LEVEL };
enum { FO_BTN_FOOT, FO_BTN_MODE, FO_BTN_GAIN, FO_BTN_TONE, FO_BTN_LEVEL };

#define FO_LEDS 36            /* LED driver outputs: Gain ring 1-11, Tone 12-22, Level 23-33, Soft, Hard, On */
#define FO_RING 11
#define FO_LED_SOFT 33        /* zero-based */
#define FO_LED_HARD 34
#define FO_LED_ON 35

#define FO_RAMP 32            /* pot codes per ms: full scale in about 8 ms */
#define FO_SETTLE_MS 10       /* relay operate time (G6K: 3 ms max) plus bounce */

static const uint8_t FO_MODEL[2] = {0x01, 0x00};
static const uint8_t FO_FW[3] = {0, 1, 0};

typedef struct {
    void *ctx;
    uint32_t (*now_ms)(void *ctx);
    void (*midi_send)(void *ctx, const uint8_t *m, int n);
    void (*set_pot)(void *ctx, int pot, uint8_t code);         /* MCP45HV51 wiper, 00 = terminal B */
    void (*set_relays)(void *ctx, bool hard, bool fx_on);
    void (*set_leds)(void *ctx, const uint8_t pwm[FO_LEDS]);
    void (*set_link_led)(void *ctx, bool on);
    const lw_flash *flash;
    const pd_fw_hal *fw;       /* optional: firmware updates over MIDI */
    uint8_t serial[LW_SERIAL_LEN];
    uint8_t brightness;        /* LED brightness 1-255; 0 means the default */
} fo_hal;

typedef struct {
    fo_hal hal;
    lw_pedal pd;
    bool setup;                /* setup mode: picking a channel */
    uint8_t setup_ch;
    uint8_t pot[3];            /* wiper codes last written */
    bool hard, on;             /* relay positions */
    bool mute;                 /* Level held at 0 while a relay switches */
    int seq;                   /* relay switching: 0 idle, 1 turning Level down, 2 settling */
    uint32_t seq_t, last_ms, last_render;
    uint32_t last_detent[3];
    uint8_t leds[FO_LEDS];
    bool link;
} flashover;

void fo_init(flashover *f, const fo_hal *hal, bool setup);
void fo_midi_byte(flashover *f, uint8_t b);
void fo_turn(flashover *f, int knob, int detents);   /* FO_GAIN, FO_TONE or FO_LEVEL; + is clockwise */
void fo_press(flashover *f, int button);              /* a debounced press (not a release) */
void fo_poll(flashover *f);                           /* call often: at least once a millisecond */

/* exposed for the simulation */
uint8_t fo_pot_code(int pot, uint8_t value, bool hard);
void fo_render(const flashover *f, uint8_t pwm[FO_LEDS], uint32_t t);

#endif
