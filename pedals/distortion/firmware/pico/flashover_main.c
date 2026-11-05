/* Flashover on the RP2040: connects the core (core/flashover.c) to the hardware. The MIDI port,
 * knobs, buttons and I2C chips use the shared Linework code in libraries/linework/pico/.
 *
 * Power-up: holding the footswitch while plugging in starts setup mode (pick a channel with
 * Gain, press the footswitch to save). Recovery mode (footswitch and Soft/Hard held) needs the
 * update loader, which isn't written yet.
 *
 * Built as flashover_debug (FO_DEBUG_USB=1), it also reports over the Pico's USB serial port:
 * which chips answered at power-up, then a line whenever a setting, the channel or the link
 * changes. For the breadboard, before the LED rings are wired. */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "board.h"
#include "lw_midi.h"
#include "lw_input.h"
#include "lw_chips.h"
#include "pedal_flash.h"
#include "lw_update.h"
#include "flashover.h"

static flashover fo;
static bool pot_ok[3];
static const uint8_t POT_ADDR[3] = {ADDR_POT_GAIN, ADDR_POT_TONE, ADDR_POT_LEVEL};

/* ---------------------------------------------------------------- hal for the core */

static uint32_t hal_now(void *c) { (void)c; return to_ms_since_boot(get_absolute_time()); }
static void hal_send(void *c, const uint8_t *m, int n) { (void)c; lw_midi_send(m, n); }
static void hal_pot(void *c, int pot, uint8_t code) { (void)c; pot_ok[pot] = lw_pot_write(i2c0, POT_ADDR[pot], code); }
static void hal_relays(void *c, bool hard, bool on)
{
    (void)c;
    gpio_put(PIN_HARD_SEL, hard);
    gpio_put(PIN_FX_ON, on);
}
static void hal_leds(void *c, const uint8_t p[FO_LEDS]) { (void)c; lw_leds_write(p); }
static void hal_link(void *c, bool on) { (void)c; gpio_put(PIN_LINK_LED, on); }

/* ---------------------------------------------------------------- knobs and buttons */

static const uint ENC_A[3] = {PIN_GAIN_A, PIN_TONE_A, PIN_LEVEL_A};
static const uint ENC_B[3] = {PIN_GAIN_B, PIN_TONE_B, PIN_LEVEL_B};
static const bool ENC_REV[3] = ENC_REVERSE;
static lw_button btn[5] = {
    {PIN_FOOTSW, FO_BTN_FOOT, false, 0}, {PIN_MODE_BTN, FO_BTN_MODE, false, 0},
    {PIN_GAIN_SW, FO_BTN_GAIN, false, 0}, {PIN_TONE_SW, FO_BTN_TONE, false, 0}, {PIN_LEVEL_SW, FO_BTN_LEVEL, false, 0},
};
static void on_press(int id) { fo_press(&fo, id); }

/* ---------------------------------------------------------------- debug reports (USB serial) */

#if FO_DEBUG_USB
static void debug_start(bool setup)
{
    sleep_ms(1500);                          /* time for the computer to open the serial port */
    printf("\nFlashover %u.%u.%u debug%s\n", FO_FW[0], FO_FW[1], FO_FW[2], setup ? ", setup mode" : "");
    const char *name[3] = {"U7 Gain", "U8 Tone", "U9 Level"};
    for (int k = 0; k < 3; k++)
        printf("  pot %-8s %s\n", name[k], pot_ok[k] ? "answered" : "NOT ANSWERING (check wiring, 9V and 3.3V)");
    printf("  LED driver %s\n", lw_leds_answered() ? "answered" : "not answering (fine until stage 3)");
}

static void debug_poll(void)
{
    static char last[160];
    static uint32_t at;
    uint32_t t = to_ms_since_boot(get_absolute_time());
    if (t - at < 50)
        return;
    at = t;
    char line[160];
    const uint8_t *v = fo.pd.value;
    char ch[4] = "-";
    if (fo.pd.channel <= 15)
        snprintf(ch, sizeof ch, "%u", fo.pd.channel + 1u);
    if (fo.setup)
        snprintf(line, sizeof line, "setup mode: channel %u (press the footswitch to save)", fo.setup_ch + 1u);
    else
        snprintf(line, sizeof line, "gain %3u  tone %3u  level %3u  %s  %s  | pots %3u %3u %3u | ch %s  pos %u  %s",
                 v[FO_GAIN], v[FO_TONE], v[FO_LEVEL], v[FO_MODE] ? "Hard" : "Soft", v[FO_ON] ? "on " : "off",
                 fo.pot[0], fo.pot[1], fo.pot[2], ch, fo.pd.position, fo.link ? "linked" : "");
    if (strcmp(line, last) != 0) {
        strcpy(last, line);
        printf("%s\n", line);
    }
}
#else
static void debug_start(bool setup) { (void)setup; }
static void debug_poll(void) {}
#endif

/* ---------------------------------------------------------------- main */

static void out_pin(uint pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

int main(void)
{
    pedal_flash_check();
#if FO_DEBUG_USB
    stdio_init_all();
#endif
    out_pin(PIN_HARD_SEL);                   /* relays off: Soft, bypassed */
    out_pin(PIN_FX_ON);
    out_pin(PIN_LINK_LED);
    lw_buttons_init(btn, 5);
    bool setup = btn[0].down;                /* footswitch held while plugging in */
    lw_midi_init(PIN_MIDI_TX, PIN_MIDI_RX);
    lw_i2c_init(i2c0, PIN_POT_SDA, PIN_POT_SCL);
    lw_i2c_init(i2c1, PIN_LED_SDA, PIN_LED_SCL);
    lw_leds_init(i2c1, ADDR_LEDS, PIN_LED_EN);

    fo_hal hal = {
        .now_ms = hal_now, .midi_send = hal_send, .set_pot = hal_pot, .set_relays = hal_relays,
        .set_leds = hal_leds, .set_link_led = hal_link, .flash = &pedal_flash,
        .fw = &lw_update_hal,
    };
    lw_serial(hal.serial);
    fo_init(&fo, &hal, setup);
    if (lw_fw_take_announce(&lw_flash_whole))
        pd_announce(&fo.pd, 1000);     /* first start of a new version: introduce it */
    debug_start(setup);
    lw_enc_init(3, ENC_A, ENC_B);

    uint32_t last_ms = hal_now(NULL);
    while (true) {
        int b;
        while ((b = lw_midi_get()) >= 0)
            fo_midi_byte(&fo, (uint8_t)b);
        uint32_t t = hal_now(NULL);
        if (t != last_ms) {
            last_ms = t;
            lw_buttons_poll(btn, 5, on_press);
        }
        for (int k = 0; k < 3; k++) {
            int n = lw_enc_take(k);
            if (n)
                fo_turn(&fo, FO_GAIN + k, ENC_REV[k] ? -n : n);
        }
        fo_poll(&fo);
        debug_poll();
        lw_midi_flush();
    }
}
