/* Tap Changer's behaviour, independent of the hardware, so it runs unchanged on the RP2040 and
 * in the PC simulation (test/tc_sim.c).
 *
 * The Linework side (the loop, channel, presets, flash) is the shared pedal code in
 * libraries/linework/pedal.c, and the knob and LED ring helpers are in lw_ui.c. This adds:
 *  - the Boost knob, set in dB: the knob's 128 steps run evenly from +1 dB to +20 dB, which the
 *    firmware turns into the digital pot's resistance (a linear pot would put half its travel
 *    above +15 dB)
 *  - a short ramp on every change, so the pot doesn't click or "zip"
 *  - the bypass relay, switched with Boost turned to its minimum for a moment (there is no
 *    Level pot to mute with, so this softens the jump rather than silencing it)
 *  - the Boost ring (a bar), the On LED and the Link LED
 *  - setup mode: hold the footswitch while plugging in power, pick a channel with the knob,
 *    press the footswitch to save it. */
#ifndef TAPCHANGER_H
#define TAPCHANGER_H

#include "pedal.h"
#include "lw_ui.h"

/* controls, in the order of the settings pairs: 14 bb 50 oo */
enum { TC_BOOST, TC_ON, TC_NCONTROLS };
enum { TC_BTN_FOOT, TC_BTN_KNOB };

#define TC_LEDS 36            /* LED driver outputs: Boost ring 1-11, On = 36 (as Flashover's) */
#define TC_LED_ON 35          /* zero-based */
#define TC_RAMP 32            /* pot codes per ms */
#define TC_SETTLE_MS 10       /* relay operate time plus bounce */

/* Tap Changer's model number, proposed in its Product Document (not yet in the specification) */
static const uint8_t TC_MODEL[2] = {0x01, 0x01};
static const uint8_t TC_FW[3] = {0, 1, 0};

typedef struct {
    void *ctx;
    uint32_t (*now_ms)(void *ctx);
    void (*midi_send)(void *ctx, const uint8_t *m, int n);
    void (*set_pot)(void *ctx, uint8_t code);              /* MCP45HV51 wiper, 00 = terminal B */
    void (*set_relay)(void *ctx, bool fx_on);
    void (*set_leds)(void *ctx, const uint8_t pwm[TC_LEDS]);
    void (*set_link_led)(void *ctx, bool on);
    const lw_flash *flash;
    const pd_fw_hal *fw;       /* optional: firmware updates over MIDI */
    uint8_t serial[LW_SERIAL_LEN];
    uint8_t brightness;        /* LED brightness 1-255; 0 means the default */
} tc_hal;

typedef struct {
    tc_hal hal;
    lw_pedal pd;
    bool setup;
    uint8_t setup_ch;
    uint8_t pot;               /* wiper code last written */
    bool on;                   /* relay position */
    bool duck;                 /* Boost held at its minimum while the relay switches */
    int seq;                   /* 0 idle, 1 turning Boost down, 2 settling */
    uint32_t seq_t, last_ms, last_render, last_click;
    uint8_t leds[TC_LEDS];
    bool link;
} tapchanger;

void tc_init(tapchanger *t, const tc_hal *hal, bool setup);
void tc_midi_byte(tapchanger *t, uint8_t b);
void tc_turn(tapchanger *t, int clicks);               /* + is clockwise */
void tc_press(tapchanger *t, int button);              /* a debounced press */
void tc_poll(tapchanger *t);                           /* call at least once a millisecond */

/* exposed for the simulation */
uint8_t tc_pot_code(uint8_t value);
double tc_gain_db(uint8_t code);                       /* the stage's gain for a wiper code */
void tc_render(const tapchanger *t, uint8_t pwm[TC_LEDS], uint32_t now);

#endif
