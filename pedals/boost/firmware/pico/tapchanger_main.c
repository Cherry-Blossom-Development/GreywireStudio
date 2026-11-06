/* Tap Changer on the RP2040: connects the core (core/tapchanger.c) to the hardware, using the
 * shared Linework code in libraries/linework/pico/ for the MIDI port, knob, buttons and chips.
 *
 * Power-up: holding the footswitch while plugging in starts setup mode (pick a channel with the
 * knob, press the footswitch to save).
 *
 * Built as tapchanger_debug (TC_DEBUG_USB=1), it also reports over the Pico's USB serial port. */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "board.h"
#include "lw_midi.h"
#include "lw_input.h"
#include "lw_chips.h"
#include "pedal_flash.h"
#include "lw_update.h"
#include "tapchanger.h"

static tapchanger tc;
static bool pot_ok;

static uint32_t hal_now(void *c) { (void)c; return to_ms_since_boot(get_absolute_time()); }
static void hal_send(void *c, const uint8_t *m, int n) { (void)c; lw_midi_send(m, n); }
static void hal_pot(void *c, uint8_t code) { (void)c; pot_ok = lw_pot_write(i2c0, ADDR_POT, code); }
static void hal_relay(void *c, bool on) { (void)c; gpio_put(PIN_FX_ON, on); }
static void hal_leds(void *c, const uint8_t p[TC_LEDS]) { (void)c; lw_leds_write(p); }
static void hal_link(void *c, bool on) { (void)c; gpio_put(PIN_LINK_LED, on); }

static const uint ENC_A[1] = {PIN_BOOST_A};
static const uint ENC_B[1] = {PIN_BOOST_B};
static lw_button btn[2] = {{PIN_FOOTSW, TC_BTN_FOOT, false, 0}, {PIN_BOOST_SW, TC_BTN_KNOB, false, 0}};
static void on_press(int id) { tc_press(&tc, id); }

#if TC_DEBUG_USB
static void debug_start(bool setup)
{
    sleep_ms(1500);                          /* time for the computer to open the serial port */
    printf("\nTap Changer %u.%u.%u debug%s\n", TC_FW[0], TC_FW[1], TC_FW[2], setup ? ", setup mode" : "");
    printf("  Boost pot (address %02X) %s\n", ADDR_POT, pot_ok ? "answered" : "NOT ANSWERING (check wiring, 9V and 3.3V)");
    printf("  LED driver   %s\n", lw_leds_answered() ? "answered" : "not answering");
}

static void debug_poll(void)
{
    static char last[120];
    static uint32_t at;
    uint32_t t = hal_now(NULL);
    if (t - at < 50)
        return;
    at = t;
    char line[120], ch[4] = "-";
    if (tc.pd.channel <= 15)
        snprintf(ch, sizeof ch, "%u", tc.pd.channel + 1u);
    if (tc.setup)
        snprintf(line, sizeof line, "setup mode: channel %u (press the footswitch to save)", tc.setup_ch + 1u);
    else
        snprintf(line, sizeof line, "boost %3u (%+.1f dB)  %s  | pot %3u | ch %s  pos %u  %s",
                 tc.pd.value[TC_BOOST], tc_gain_db(tc_pot_code(tc.pd.value[TC_BOOST])),
                 tc.pd.value[TC_ON] ? "on " : "off", tc.pot, ch, tc.pd.position, tc.link ? "linked" : "");
    if (strcmp(line, last) != 0) {
        strcpy(last, line);
        printf("%s\n", line);
    }
}
#else
static void debug_start(bool setup) { (void)setup; }
static void debug_poll(void) {}
#endif

static void out_pin(uint pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

int main(void)
{
    pedal_flash_check();
#if TC_DEBUG_USB
    stdio_init_all();
#endif
    out_pin(PIN_FX_ON);                      /* relay off: bypassed */
    out_pin(PIN_LINK_LED);
    lw_buttons_init(btn, 2);
    bool setup = btn[0].down;                /* footswitch held while plugging in */
    lw_midi_init(PIN_MIDI_TX, PIN_MIDI_RX);
    lw_i2c_init(i2c0, PIN_POT_SDA, PIN_POT_SCL);
    lw_i2c_init(i2c1, PIN_LED_SDA, PIN_LED_SCL);
    lw_leds_init(i2c1, ADDR_LEDS, PIN_LED_EN);

    tc_hal hal = {
        .now_ms = hal_now, .midi_send = hal_send, .set_pot = hal_pot, .set_relay = hal_relay,
        .set_leds = hal_leds, .set_link_led = hal_link, .flash = &pedal_flash,
        .fw = &lw_update_hal,
    };
    lw_serial(hal.serial);
    tc_init(&tc, &hal, setup);
    if (lw_fw_take_announce(&lw_flash_whole))
        pd_announce(&tc.pd, 1000);     /* first start of a new version: introduce it */
    debug_start(setup);
    lw_enc_init(1, ENC_A, ENC_B);

    uint32_t last_ms = hal_now(NULL);
    while (true) {
        int b;
        while ((b = lw_midi_get()) >= 0)
            tc_midi_byte(&tc, (uint8_t)b);
        uint32_t t = hal_now(NULL);
        if (t != last_ms) {
            last_ms = t;
            lw_buttons_poll(btn, 2, on_press);
        }
        int n = lw_enc_take(0);
        if (n)
            tc_turn(&tc, ENC_REVERSE ? -n : n);
        tc_poll(&tc);
        debug_poll();
        lw_midi_flush();
    }
}
