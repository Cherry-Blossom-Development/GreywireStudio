/* PC simulation of Tap Changer on a Linework loop: Switchyard (the real controller code), then
 * Tap Changer, then a stand-in pedal, joined by MIDI cables at 31,250 baud, with a fake phone
 * app on Switchyard's USB. Tap Changer's pot, relay, LEDs and flash are simulated and logged.
 *
 * Build and run:  make -C test test */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "switchyard.h"
#include "tapchanger.h"

static int fails, checks;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { fails++; printf("  FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static uint32_t T;
static uint32_t now_ms(void *c) { (void)c; return T; }

#define QSZ 8192
typedef struct { uint8_t q[QSZ]; int head, tail; bool cut; } cable;
static cable link_[3];          /* 0: Switchyard -> Tap Changer, 1: Tap Changer -> stand-in, 2: stand-in -> Switchyard */
static void cable_put(cable *c, const uint8_t *m, int n)
{
    if (c->cut)
        return;
    for (int i = 0; i < n; i++) {
        c->q[c->head] = m[i];
        c->head = (c->head + 1) % QSZ;
    }
}
static int cable_get(cable *c)
{
    if (c->head == c->tail)
        return -1;
    int b = c->q[c->tail];
    c->tail = (c->tail + 1) % QSZ;
    return b;
}

/* Switchyard */
static uint8_t sy_store[SY_STORE_SIZE];
static void sst_read(void *c, uint32_t off, void *buf, uint32_t n) { (void)c; memcpy(buf, &sy_store[off], n); }
static void sst_write(void *c, uint32_t off, const void *buf, uint32_t n)
{
    (void)c;
    for (uint32_t i = 0; i < n; i++)
        sy_store[off + i] &= ((const uint8_t *)buf)[i];
}
static void sst_erase(void *c, uint32_t off, uint32_t n) { (void)c; memset(&sy_store[off], 0xFF, n); }
static void sy_disp(void *c, const char t[4], uint8_t dp) { (void)c; (void)t; (void)dp; }
static bool yes(void *c) { (void)c; return true; }
static void sy_loop_send(void *c, const uint8_t *m, int n) { (void)c; cable_put(&link_[0], m, n); }
static void other_send(void *c, const uint8_t *m, int n) { (void)c; (void)m; (void)n; }

#define MAXLOG 4096
typedef struct { uint8_t b[160]; int n; } logged;
static logged app_log[MAXLOG];
static int napp;
static lw_parser app_parse;
static void app_msg(void *c, const uint8_t *m, int n)
{
    (void)c;
    if (napp < MAXLOG && n <= 160) {
        memcpy(app_log[napp].b, m, n);
        app_log[napp++].n = n;
    }
}
static void app_send(void *c, const uint8_t *m, int n)
{
    (void)c;
    for (int i = 0; i < n; i++)
        lw_parse(&app_parse, m[i]);
}
static switchyard SY;
static sy_hal syhal = {
    .now_ms = now_ms, .loop_send = sy_loop_send, .other_send = other_send, .app_send = app_send,
    .app_connected = yes, .power_9v = yes, .store_read = sst_read, .store_write = sst_write,
    .store_erase = sst_erase, .display = sy_disp,
    .serial = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x01, 0x02, 0x03},
};

static lw_pedal SB;
static void sb_send(void *c, const uint8_t *m, int n) { (void)c; cable_put(&link_[2], m, n); }

/* Tap Changer's simulated hardware */
static tapchanger TC;
static uint8_t pot = 0x7F;
static bool relay, link_led;
static uint8_t leds[TC_LEDS];
typedef struct { uint32_t t; char what; uint8_t v; } event;    /* 'P' pot = v, 'R' relay = v */
static event ev[20000];
static int nev;
static void hw_pot(void *c, uint8_t code) { (void)c; pot = code; if (nev < 20000) ev[nev++] = (event){T, 'P', code}; }
static void hw_relay(void *c, bool on) { (void)c; relay = on; if (nev < 20000) ev[nev++] = (event){T, 'R', on}; }
static void hw_leds(void *c, const uint8_t p[TC_LEDS]) { (void)c; memcpy(leds, p, TC_LEDS); }
static void hw_link(void *c, bool on) { (void)c; link_led = on; }
static void tc_send(void *c, const uint8_t *m, int n) { (void)c; cable_put(&link_[1], m, n); }

static bool tc_powered = true, others_on;
static uint8_t fl[PS_SIZE];
static void fl_read(void *c, uint32_t off, void *buf, uint32_t n) { (void)c; memcpy(buf, &fl[off], n); }
static void fl_write(void *c, uint32_t off, const void *buf, uint32_t n)
{
    (void)c;
    if (!tc_powered)
        return;
    for (uint32_t i = 0; i < n; i++)
        fl[off + i] &= ((const uint8_t *)buf)[i];
}
static void fl_erase(void *c, uint32_t off, uint32_t n) { (void)c; if (tc_powered) memset(&fl[off], 0xFF, n); }
static const lw_flash flash = {NULL, fl_read, fl_write, fl_erase};
static const tc_hal tchal = {
    .now_ms = now_ms, .midi_send = tc_send, .set_pot = hw_pot, .set_relay = hw_relay,
    .set_leds = hw_leds, .set_link_led = hw_link, .flash = &flash,
    .serial = {0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01},
};

static void tick(void)
{
    T++;
    for (int k = 0; k < 3; k++)
        for (int i = 0; i < 3; i++) {
            if (k && !others_on)
                break;
            int b = cable_get(&link_[k]);
            if (b < 0)
                break;
            if (k == 0) {
                if (tc_powered)
                    tc_midi_byte(&TC, (uint8_t)b);
            } else if (k == 1) {
                pd_byte(&SB, (uint8_t)b);
            } else {
                sy_loop_byte(&SY, (uint8_t)b);
            }
        }
    if (tc_powered)
        tc_poll(&TC);
    if (others_on) {
        pd_poll(&SB);
        sy_poll(&SY);
    }
}
static void run(int ms) { while (ms--) tick(); }

static void power_up(bool setup)
{
    tc_powered = true;
    pot = 0x7F;                                      /* the pot's own power-up value: mid-scale */
    relay = false;
    tc_init(&TC, &tchal, setup);
}

static void app(const uint8_t *m, int n) { for (int i = 0; i < n; i++) sy_app_byte(&SY, m[i]); }
static void cc(uint8_t ch, uint8_t num, uint8_t v) { uint8_t m[3] = {(uint8_t)(0xB0 | ch), num, v}; app(m, 3); }
static logged *find_lw(int mark, uint8_t cmd, int qq)
{
    for (int i = mark; i < napp; i++)
        if (lw_is_linework(app_log[i].b, app_log[i].n) && LW_CMD(app_log[i].b) == cmd && (qq < 0 || LW_QQ(app_log[i].b) == qq))
            return &app_log[i];
    return NULL;
}
static int ack_st(int mark, uint8_t qq, uint8_t cmd)
{
    for (int i = mark; i < napp; i++) {
        uint8_t *b = app_log[i].b;
        if (lw_is_linework(b, app_log[i].n) && LW_CMD(b) == LW_ACK && LW_QQ(b) == qq && LW_BODY(b)[1] == cmd)
            return LW_BODY(b)[2];
    }
    return -1;
}
static int ccs_seen(int mark, uint8_t ch, uint8_t num, int *last)
{
    int k = 0;
    for (int i = mark; i < napp; i++)
        if (app_log[i].n == 3 && app_log[i].b[0] == (0xB0 | ch) && app_log[i].b[1] == num) {
            k++;
            if (last)
                *last = app_log[i].b[2];
        }
    return k;
}
static int lit(int first, int n)
{
    int k = 0;
    for (int i = first; i < first + n; i++)
        k += leds[i] != 0;
    return k;
}

int main(void)
{
    lw_parser_init(&app_parse, app_msg, NULL);
    memset(sy_store, 0xFF, sizeof sy_store);
    memset(fl, 0xFF, sizeof fl);
    T = 1000;

    printf("1. The Boost knob in dB\n");
    double lo = tc_gain_db(tc_pot_code(0)), hi = tc_gain_db(tc_pot_code(127)), mid = tc_gain_db(tc_pot_code(64));
    CHECK(tc_pot_code(0) == 255 && tc_pot_code(127) == 0, "ends: code %d and %d", tc_pot_code(0), tc_pot_code(127));
    CHECK(fabs(lo - 1.1) < 0.1 && fabs(hi - 20.2) < 0.1, "range %.2f to %.2f dB", lo, hi);
    CHECK(fabs(mid - (lo + (hi - lo) * 64 / 127)) < 0.3, "noon is %.2f dB, halfway in dB", mid);
    bool mono = true;
    double worst = 0;
    for (int v = 1; v < 128; v++) {
        if (tc_pot_code(v) > tc_pot_code(v - 1))
            mono = false;
        double step = tc_gain_db(tc_pot_code(v)) - tc_gain_db(tc_pot_code(v - 1));
        if (step > worst)
            worst = step;
    }
    CHECK(mono, "more knob never means less boost");
    CHECK(worst < 0.5, "largest step between knob values %.2f dB", worst);
    printf("  knob 0 / 32 / 64 / 96 / 127: %.1f / %.1f / %.1f / %.1f / %.1f dB\n", lo,
           tc_gain_db(tc_pot_code(32)), mid, tc_gain_db(tc_pot_code(96)), hi);

    printf("2. A new Tap Changer powers up\n");
    power_up(false);
    CHECK(pot == 255 && !relay, "comes up at minimum Boost, bypassed");
    run(40);
    CHECK(pot == tc_pot_code(64), "then ramps to noon: code %d", pot);
    CHECK(lit(0, LW_RING) == 6 && !leds[TC_LED_ON] && !link_led, "ring shows 6 LEDs; On and Link off");

    printf("3. Switchyard finds it\n");
    pd_hal sbh = {.now_ms = now_ms, .send = sb_send, .model = {0x7F, 0x7F}, .fw = {0, 1, 0},
                  .ncontrols = 4, .cc = {20, 21, 22, 80}, .kind = {0, 0, 0, PD_ONOFF}, .value = {64, 64, 64, 0}};
    lw_serial_from_u64(0x99, sbh.serial);
    pd_init(&SB, &sbh);
    sy_init(&SY, &syhal);
    others_on = true;
    run(2000);
    CHECK(SY.npedals == 2 && TC.pd.position == 1 && TC.pd.channel == 0, "position %d channel %d", TC.pd.position, TC.pd.channel);
    CHECK(SY.pedal[0].model[0] == 0x01 && SY.pedal[0].model[1] == 0x01, "model 01 01");
    CHECK(link_led, "Link LED on");

    printf("4. Boost and on/off from the app\n");
    int e0 = nev;
    cc(0, 20, 127);
    run(30);
    CHECK(pot == 0 && lit(0, LW_RING) == 11, "Boost 127: code 0, full ring");
    int big = 0;
    uint8_t lastc = tc_pot_code(64);
    for (int i = e0; i < nev; i++)
        if (ev[i].what == 'P') {
            int d = abs((int)ev[i].v - lastc);
            if (d > big)
                big = d;
            lastc = ev[i].v;
        }
    CHECK(big <= TC_RAMP, "ramped: biggest step %d", big);
    e0 = nev;
    cc(0, 80, 127);
    run(50);
    CHECK(relay && leds[TC_LED_ON], "CC 80 127: on");
    bool order_ok = false;
    for (int i = e0; i < nev; i++)
        if (ev[i].what == 'R') {
            uint8_t pv = 0;
            for (int j = e0; j < i; j++)
                if (ev[j].what == 'P')
                    pv = ev[j].v;
            order_ok = pv == 255;
            break;
        }
    CHECK(order_ok, "Boost at its minimum when the relay switches");
    CHECK(pot == 0 && TC.seq == 0, "Boost back afterwards");

    printf("5. Report settings, presets\n");
    int mark = napp;
    lw_msg m;
    lw_begin(&m, LW_REPORT_REQ, 0x45);
    lw_put(&m, 0);
    lw_end(&m);
    app(m.b, m.n);
    run(30);
    logged *l = find_lw(mark, LW_REPORT, 0x45);
    const uint8_t pairs[] = {0, 0x14, 127, 0x50, 127, 0xF7};
    CHECK(l && l->n == LW_HDR_LEN + 2 + (int)sizeof pairs && memcmp(LW_BODY(l->b), pairs, sizeof pairs) == 0, "pairs 14 bb 50 oo");
    lw_begin(&m, LW_STORE, 0x46);
    lw_put(&m, 0);
    lw_put(&m, 7);
    lw_put(&m, 0);
    const uint8_t p7[] = {20, 30, 80, 0};
    lw_put_n(&m, p7, sizeof p7);
    lw_end(&m);
    app(m.b, m.n);
    run(30);
    CHECK(ack_st(mark, 0x46, LW_STORE) == ST_OK, "store acknowledged");
    mark = napp;
    const uint8_t pc[] = {0xC0, 7};
    app(pc, 2);
    run(60);
    CHECK(TC.pd.value[TC_BOOST] == 30 && !relay && pot == tc_pot_code(30), "program change 7: Boost 30, bypassed");
    CHECK(ccs_seen(mark, 0, 20, NULL) == 1 && ccs_seen(mark, 0, 80, NULL) == 1, "two control changes back");

    printf("6. By hand\n");
    mark = napp;
    int last = -1;
    tc_turn(&TC, 5);
    run(100);
    CHECK(TC.pd.value[TC_BOOST] == 35 && ccs_seen(mark, 0, 20, &last) >= 1 && last == 35, "+5 clicks slowly: 35, reported");
    mark = napp;
    tc_press(&TC, TC_BTN_FOOT);
    run(60);
    CHECK(relay && ccs_seen(mark, 0, 80, &last) == 1 && last == 127, "footswitch: on, CC 80 127");
    tc_press(&TC, TC_BTN_KNOB);
    run(20);
    CHECK(relay, "knob push does nothing yet");

    printf("7. Power off and on\n");
    run(2500);
    uint8_t saved[TC_NCONTROLS];
    memcpy(saved, TC.pd.value, sizeof saved);
    tc_powered = false;
    run(1500);
    power_up(false);
    CHECK(TC.pd.channel == 0 && memcmp(TC.pd.value, saved, sizeof saved) == 0 && TC.pd.preset[7][0] == 30, "channel, sound, preset kept");
    CHECK(pot == 255, "comes up at minimum Boost");
    run(50);
    CHECK(relay && pot == tc_pot_code(saved[TC_BOOST]), "then the saved Boost, on");

    printf("8. Setup mode\n");
    power_up(true);
    run(20);
    CHECK(TC.setup && leds[0] && lit(0, LW_RING) == 1, "channel 1 shown");
    tc_turn(&TC, 12);
    run(20);
    CHECK(TC.setup_ch == 12 && leds[10] && leds[1] && lit(0, LW_RING) == 2, "channel 13: LED 11 + LED 2");
    bool b = leds[TC_LED_ON] != 0;
    run(250);
    CHECK((leds[TC_LED_ON] != 0) != b, "On LED blinks");
    tc_press(&TC, TC_BTN_FOOT);
    run(20);
    CHECK(!TC.setup && TC.pd.channel == 12, "footswitch saves channel 13");
    power_up(false);
    CHECK(TC.pd.channel == 12, "kept through a power cycle");

    printf("\n%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
