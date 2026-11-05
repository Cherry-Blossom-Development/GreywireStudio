/* PC simulation of Flashover on a Linework loop: Switchyard (the real controller code), then
 * Flashover, then a stand-in pedal, joined by MIDI cables at 31,250 baud, with a fake phone
 * app on Switchyard's USB. Flashover's hardware (pots, relays, LEDs, flash) is simulated and
 * logged, so the checks can see what the chips would be told and in what order.
 *
 * Build and run:  make -C test test */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "switchyard.h"
#include "flashover.h"

static int fails, checks;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { fails++; printf("  FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ---------------------------------------------------------------- the loop */

static uint32_t T;
static uint32_t now_ms(void *c) { (void)c; return T; }

#define QSZ 8192
typedef struct { uint8_t q[QSZ]; int head, tail; bool cut; } cable;
static cable link_[3];          /* 0: Switchyard -> Flashover, 1: Flashover -> stand-in, 2: stand-in -> Switchyard */

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

/* the stand-in pedal after Flashover */
static lw_pedal SB;
static void sb_send(void *c, const uint8_t *m, int n) { (void)c; cable_put(&link_[2], m, n); }

/* Flashover's simulated hardware */
static flashover FO;
static uint8_t pot[3];
static bool r_hard, r_on, link_led;
static uint8_t leds[FO_LEDS];
typedef struct { uint32_t t; char what; uint8_t a, b; } event;     /* 'P' pot a = b, 'R' relays */
static event ev[20000];
static int nev;
static void hw_pot(void *c, int p, uint8_t code)
{
    (void)c;
    pot[p] = code;
    if (nev < 20000)
        ev[nev++] = (event){T, 'P', (uint8_t)p, code};
}
static void hw_relays(void *c, bool hard, bool on)
{
    (void)c;
    r_hard = hard;
    r_on = on;
    if (nev < 20000)
        ev[nev++] = (event){T, 'R', hard, on};
}
static void hw_leds(void *c, const uint8_t p[FO_LEDS]) { (void)c; memcpy(leds, p, FO_LEDS); }
static void hw_link(void *c, bool on) { (void)c; link_led = on; }
static void fo_send(void *c, const uint8_t *m, int n) { (void)c; cable_put(&link_[1], m, n); }

static bool fo_powered = true, others_on;   /* others_on: Switchyard and the stand-in are running */
static uint8_t fl[PS_SIZE];
static int erases, tear_after = -1;    /* tear_after >= 0: power is lost after that many more bytes */
static void fl_read(void *c, uint32_t off, void *buf, uint32_t n) { (void)c; memcpy(buf, &fl[off], n); }
static void fl_write(void *c, uint32_t off, const void *buf, uint32_t n)
{
    (void)c;
    for (uint32_t i = 0; i < n; i++) {
        if (!fo_powered)
            return;
        if (tear_after == 0) {
            fo_powered = false;                         /* the power has gone: nothing more happens */
            return;
        }
        if (tear_after > 0)
            tear_after--;
        fl[off + i] &= ((const uint8_t *)buf)[i];     /* NOR flash: writing only clears bits */
    }
}
static void fl_erase(void *c, uint32_t off, uint32_t n)
{
    (void)c;
    if (off % PS_SECTOR || n % PS_SECTOR)
        printf("  bad erase %u %u\n", off, n);
    if (!fo_powered)
        return;
    memset(&fl[off], 0xFF, n);
    erases++;
}
static const lw_flash flash = {NULL, fl_read, fl_write, fl_erase};

static const fo_hal fohal = {
    .now_ms = now_ms, .midi_send = fo_send, .set_pot = hw_pot, .set_relays = hw_relays,
    .set_leds = hw_leds, .set_link_led = hw_link, .flash = &flash,
    .serial = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A},
};

static void tick(void)
{
    T++;
    for (int k = 0; k < 3; k++)
        for (int i = 0; i < 3; i++) {                /* about 3 bytes per ms */
            if (k && !others_on)
                break;
            int b = cable_get(&link_[k]);
            if (b < 0)
                break;
            if (k == 0) {
                if (fo_powered)
                    fo_midi_byte(&FO, (uint8_t)b);
            } else if (k == 1) {
                pd_byte(&SB, (uint8_t)b);
            } else {
                sy_loop_byte(&SY, (uint8_t)b);
            }
        }
    if (fo_powered)
        fo_poll(&FO);
    if (others_on) {
        pd_poll(&SB);
        sy_poll(&SY);
    }
}

static void run(int ms)
{
    while (ms--)
        tick();
}

static void power_up_flashover(bool setup)
{
    fo_powered = true;
    memset(pot, 0x7F, sizeof pot);                   /* the pots' own power-up value: mid-scale */
    r_hard = r_on = false;
    fo_init(&FO, &fohal, setup);
}

/* ---------------------------------------------------------------- app helpers */

static void app(const uint8_t *m, int n)
{
    for (int i = 0; i < n; i++)
        sy_app_byte(&SY, m[i]);
}

static void cc(uint8_t ch, uint8_t num, uint8_t v)
{
    uint8_t m[3] = {(uint8_t)(0xB0 | ch), num, v};
    app(m, 3);
}

static logged *find_lw(int mark, uint8_t cmd, int qq)
{
    for (int i = mark; i < napp; i++)
        if (lw_is_linework(app_log[i].b, app_log[i].n) && LW_CMD(app_log[i].b) == cmd
            && (qq < 0 || LW_QQ(app_log[i].b) == qq))
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

/* control changes the app saw from mark on, for this channel and number: count and last value */
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

static int lit(int first, int n)                     /* LEDs on in a range */
{
    int k = 0;
    for (int i = first; i < first + n; i++)
        k += leds[i] != 0;
    return k;
}

static int brightest(int first)
{
    int b = 0;
    for (int i = 1; i < FO_RING; i++)
        if (leds[first + i] > leds[first + b])
            b = i;
    return b;
}

/* ---------------------------------------------------------------- tests */

int main(void)
{
    lw_parser_init(&app_parse, app_msg, NULL);
    memset(sy_store, 0xFF, sizeof sy_store);
    memset(fl, 0xFF, sizeof fl);
    T = 1000;

    printf("1. A new Flashover powers up\n");
    power_up_flashover(false);
    CHECK(pot[FO_POT_LEVEL] == 0 && !r_on && !r_hard, "power-up: Level silent, bypassed, Soft");
    run(30);
    CHECK(pot[FO_POT_GAIN] == 129 && pot[FO_POT_TONE] == 126 && pot[FO_POT_LEVEL] == 129,
          "knobs at noon: pots %d %d %d", pot[0], pot[1], pot[2]);
    CHECK(lit(0, FO_RING) == 6 && brightest(FO_RING) == 5 && lit(2 * FO_RING, FO_RING) == 6,
          "rings: Gain bar %d, Tone dot at LED %d", lit(0, FO_RING), brightest(FO_RING) + 1);
    CHECK(leds[FO_LED_SOFT] && !leds[FO_LED_HARD] && !leds[FO_LED_ON] && !link_led, "Soft lit, On and Link off");

    printf("2. Switchyard finds it\n");
    pd_hal sbh = {
        .now_ms = now_ms, .send = sb_send, .model = {0x7F, 0x7F}, .fw = {0, 1, 0},
        .ncontrols = 4, .cc = {20, 21, 22, 80}, .kind = {0, 0, 0, PD_ONOFF}, .value = {64, 64, 64, 0},
    };
    lw_serial_from_u64(0x99, sbh.serial);
    pd_init(&SB, &sbh);
    sy_init(&SY, &syhal);
    others_on = true;
    run(2000);
    CHECK(SY.npedals == 2 && FO.pd.position == 1 && FO.pd.channel == 0 && SB.channel == 1,
          "2 pedals; Flashover position %d channel %d", FO.pd.position, FO.pd.channel);
    CHECK(SY.pedal[0].model[0] == 0x01 && SY.pedal[0].model[1] == 0x00 && SY.pedal[0].spec == 2, "Flashover model 01 00");
    CHECK(link_led, "Link LED on once linked");

    printf("3. The app sets the knobs\n");
    int mark = napp, e3 = nev;
    uint8_t lastc[3];
    memcpy(lastc, pot, 3);
    cc(0, 20, 127);
    cc(0, 21, 0);
    cc(0, 22, 100);
    run(30);
    CHECK(pot[FO_POT_GAIN] == 255 && pot[FO_POT_TONE] == 255 && pot[FO_POT_LEVEL] == 201,
          "pots %d %d %d (want 255 255 201)", pot[0], pot[1], pot[2]);
    CHECK(lit(0, FO_RING) == 11 && brightest(FO_RING) == 0, "Gain ring full, Tone dot at LED 1");
    CHECK(ccs_seen(mark, 0, 20, NULL) == 1, "the app sees its own CC once (passed round the loop, not echoed)");
    /* ramp: no pot jumps more than FO_RAMP per ms */
    int worst = 0;
    for (int i = e3; i < nev; i++)
        if (ev[i].what == 'P') {
            int d = abs((int)ev[i].b - lastc[ev[i].a]);
            if (d > worst)
                worst = d;
            lastc[ev[i].a] = ev[i].b;
        }
    CHECK(worst <= FO_RAMP, "pots ramp: biggest step %d", worst);

    printf("4. Hard mode and on/off, without clicks\n");
    int e0 = nev;
    cc(0, 81, 127);                                  /* other brands send 127 for a switch */
    run(40);
    CHECK(r_hard && FO.pd.value[FO_MODE] == 1, "CC 81 127 means Hard");
    CHECK(pot[FO_POT_LEVEL] == 76, "Level cut to 38%% in Hard: %d (want 201 x 97/255 = 76)", pot[FO_POT_LEVEL]);
    bool order_ok = false;
    for (int i = e0; i < nev; i++)
        if (ev[i].what == 'R') {
            uint8_t lv = 201;
            for (int j = e0; j < i; j++)
                if (ev[j].what == 'P' && ev[j].a == FO_POT_LEVEL)
                    lv = ev[j].b;
            order_ok = lv == 0;
            break;
        }
    CHECK(order_ok, "Level at 0 before the relay switches");
    CHECK(leds[FO_LED_HARD] && !leds[FO_LED_SOFT], "Hard LED");
    cc(0, 80, 64);
    run(40);
    CHECK(r_on && leds[FO_LED_ON], "CC 80 64: effect on");
    cc(0, 80, 63);
    run(40);
    CHECK(!r_on && !leds[FO_LED_ON], "CC 80 63: bypass");
    cc(0, 80, 127);
    run(1);
    cc(0, 81, 0);                                    /* a second change while the first is switching */
    run(60);
    CHECK(r_on && !r_hard && pot[FO_POT_LEVEL] == 201 && FO.seq == 0, "two quick changes both land: on, Soft, Level %d",
          pot[FO_POT_LEVEL]);
    cc(1, 22, 5);                                    /* the stand-in's channel */
    run(20);
    CHECK(FO.pd.value[FO_LEVEL] == 100 && SB.value[2] == 5, "another channel's CC is passed on and ignored");

    printf("5. Report settings\n");
    mark = napp;
    lw_msg m;
    lw_begin(&m, LW_REPORT_REQ, 0x45);
    lw_put(&m, 0);
    lw_end(&m);
    app(m.b, m.n);
    run(30);
    logged *l = find_lw(mark, LW_REPORT, 0x45);
    const uint8_t pairs[] = {0, 0x14, 127, 0x15, 0, 0x16, 100, 0x51, 0, 0x50, 127, 0xF7};
    CHECK(l && l->n == LW_HDR_LEN + 2 + (int)sizeof pairs && memcmp(LW_BODY(l->b), pairs, sizeof pairs) == 0,
          "pairs 14 gg 15 tt 16 ll 51 mm 50 oo");

    printf("6. Knobs, button and footswitch by hand\n");
    mark = napp;
    fo_turn(&FO, FO_GAIN, -3);                       /* slow: one step a click */
    run(100);
    int last = -1;
    CHECK(FO.pd.value[FO_GAIN] == 124 && ccs_seen(mark, 0, 20, &last) == 1 && last == 124, "Gain -3 -> 124, reported");
    mark = napp;
    for (int i = 0; i < 10; i++) {                   /* spun fast: 10 clicks, 5 ms apart */
        fo_turn(&FO, FO_LEVEL, -1);
        run(5);
    }
    run(50);
    int n = ccs_seen(mark, 0, 22, &last);
    CHECK(FO.pd.value[FO_LEVEL] < 100 - 10 && last == FO.pd.value[FO_LEVEL], "fast turn accelerates: Level %d",
          FO.pd.value[FO_LEVEL]);
    CHECK(n >= 2 && n <= 4, "reports limited to one per 20 ms: %d for 10 clicks in 50 ms", n);
    mark = napp;
    fo_press(&FO, FO_BTN_MODE);
    fo_press(&FO, FO_BTN_FOOT);
    run(60);
    CHECK(r_hard && !r_on && ccs_seen(mark, 0, 81, &last) == 1 && last == 1 && ccs_seen(mark, 0, 80, &last) == 1 && last == 0,
          "button -> Hard (CC 81 1), footswitch -> bypass (CC 80 0)");
    fo_turn(&FO, FO_LEVEL, -100);
    run(50);
    CHECK(FO.pd.value[FO_LEVEL] == 0 && lit(2 * FO_RING, FO_RING) == 1, "Level stops at 0; its ring shows LED 1 only");
    fo_press(&FO, FO_BTN_FOOT);
    fo_press(&FO, FO_BTN_MODE);
    fo_turn(&FO, FO_LEVEL, 80);
    run(100);

    printf("7. Presets\n");
    uint8_t before[FO_NCONTROLS];
    memcpy(before, FO.pd.value, sizeof before);
    mark = napp;
    lw_begin(&m, LW_STORE, 0x46);                    /* store the current sound in slot 3 */
    lw_put(&m, 0);
    lw_put(&m, 3);
    lw_put(&m, 1);
    lw_end(&m);
    app(m.b, m.n);
    lw_begin(&m, LW_STORE, 0x47);                    /* and slot 9 from pairs */
    lw_put(&m, 0);
    lw_put(&m, 9);
    lw_put(&m, 0);
    const uint8_t p9[] = {20, 10, 21, 20, 22, 30, 81, 1, 80, 100};
    lw_put_n(&m, p9, sizeof p9);
    lw_end(&m);
    app(m.b, m.n);
    run(40);
    CHECK(ack_st(mark, 0x46, LW_STORE) == ST_OK && ack_st(mark, 0x47, LW_STORE) == ST_OK, "both stores acknowledged");
    CHECK(memcmp(FO.pd.preset[3], before, FO_NCONTROLS) == 0 && FO.pd.preset[9][4] == 127, "slot 3 = current, slot 9 on = 127");
    mark = napp;
    const uint8_t pc9[] = {0xC0, 9};
    app(pc9, 2);
    run(60);
    CHECK(FO.pd.value[FO_GAIN] == 10 && r_hard && r_on, "program change 9 recalls slot 9");
    CHECK(pot[FO_POT_GAIN] == 20 && pot[FO_POT_TONE] == 255 - 40 && pot[FO_POT_LEVEL] == (60 * 97 + 127) / 255,
          "pots follow: %d %d %d", pot[0], pot[1], pot[2]);
    CHECK(ccs_seen(mark, 0, 20, NULL) == 1 && ccs_seen(mark, 0, 21, NULL) == 1 && ccs_seen(mark, 0, 22, NULL) == 1
          && ccs_seen(mark, 0, 81, NULL) == 1 && ccs_seen(mark, 0, 80, NULL) == 1, "five control changes back");
    const uint8_t pc3[] = {0xC0, 3};
    app(pc3, 2);
    run(60);
    CHECK(memcmp(FO.pd.value, before, FO_NCONTROLS) == 0, "program change 3 back to the stored sound");

    printf("8. Power off and on\n");
    run(2500);                                       /* past the 2 s save delay */
    uint8_t saved[FO_NCONTROLS];
    memcpy(saved, FO.pd.value, sizeof saved);
    fo_turn(&FO, FO_TONE, 5);                        /* then a change, and the power goes within 2 s */
    run(500);
    fo_powered = false;
    run(1500);
    CHECK(SY.broken, "Switchyard sees the loop broken while Flashover is off");
    power_up_flashover(false);
    CHECK(FO.pd.channel == 0 && memcmp(FO.pd.value, saved, sizeof saved) == 0, "channel and sound as saved");
    CHECK(memcmp(FO.pd.preset[9], (uint8_t[]){10, 20, 30, 1, 127}, 5) == 0 && memcmp(FO.pd.preset[3], before, 5) == 0,
          "presets kept");
    CHECK(pot[FO_POT_LEVEL] == 0, "comes up silent");
    run(40);
    CHECK(pot[FO_POT_LEVEL] == fo_pot_code(FO_POT_LEVEL, saved[FO_LEVEL], saved[FO_MODE]) && r_on == (saved[FO_ON] != 0),
          "then Level ramps to the saved setting");
    run(3000);
    CHECK(!SY.broken && link_led, "loop working again, Link LED on");

    printf("9. Flash wear: many changes\n");
    int w0 = FO.pd.flash_writes;
    erases = 0;
    for (int i = 0; i < 1500; i++) {                 /* 1500 sounds, each held long enough to be saved */
        cc(0, 20, (uint8_t)(i % 128));
        run(2100);
    }
    CHECK(FO.pd.flash_writes - w0 >= 1500, "each change saved (%d writes)", FO.pd.flash_writes - w0);
    CHECK(erases >= 4 && erases <= 12, "erases: %d (one per ~200 changes)", erases);
    printf("  %d flash writes, %d erases\n", FO.pd.flash_writes - w0, erases);
    CHECK(FO.pd.flash_erases == erases, "all erases were the quiet-time kind");
    memcpy(saved, FO.pd.value, sizeof saved);
    power_up_flashover(false);
    CHECK(memcmp(FO.pd.value, saved, sizeof saved) == 0 && FO.pd.channel == 0, "restored after compactions");
    CHECK(memcmp(FO.pd.preset[9], (uint8_t[]){10, 20, 30, 1, 127}, 5) == 0, "presets survive compaction");

    printf("10. Power lost in the middle of a write\n");
    memcpy(saved, FO.pd.value, sizeof saved);
    cc(0, 21, 77);
    run(1990);
    tear_after = 5;                                  /* the next record is cut off after 5 bytes */
    run(50);
    CHECK(!fo_powered, "the power cut happened");
    tear_after = -1;
    power_up_flashover(false);
    CHECK(memcmp(FO.pd.value, saved, sizeof saved) == 0, "the cut-off record is skipped; the one before it counts");
    cc(0, 21, 78);
    run(2100);
    power_up_flashover(false);
    CHECK(FO.pd.value[FO_TONE] == 78, "and the log carries on after it");
    /* a compaction cut off before its header is written: the old copy still counts */
    memcpy(saved, FO.pd.value, sizeof saved);
    FO.pd.log.next = PS_RECORDS;                     /* pretend the sector is full */
    tear_after = 20;                                 /* inside the compacted records, before the header */
    cc(0, 22, 11);
    run(2100);
    CHECK(!fo_powered, "the power cut happened");
    tear_after = -1;
    power_up_flashover(false);
    CHECK(memcmp(FO.pd.value, saved, sizeof saved) == 0, "half-done compaction ignored: %d %d %d %d %d / %d %d %d %d %d",
          FO.pd.value[0], FO.pd.value[1], FO.pd.value[2], FO.pd.value[3], FO.pd.value[4], saved[0], saved[1], saved[2], saved[3], saved[4]);
    run(10);

    printf("11. Setup mode (footswitch held at power-up)\n");
    power_up_flashover(true);
    run(20);
    CHECK(FO.setup && FO.setup_ch == 0 && lit(0, FO_RING) == 1 && leds[0], "shows channel 1 on Gain LED 1");
    fo_turn(&FO, FO_GAIN, 4);
    run(20);
    CHECK(FO.setup_ch == 4 && leds[4] && lit(0, FO_RING) == 1, "Gain picks channel 5");
    fo_turn(&FO, FO_GAIN, 9);
    run(20);
    CHECK(FO.setup_ch == 13 && leds[10] && leds[FO_RING + 2] && lit(0, 3 * FO_RING) == 2, "channel 14: Gain LED 11 + Tone LED 3");
    fo_turn(&FO, FO_GAIN, 50);
    run(300);
    CHECK(FO.setup_ch == 15, "stops at 16");
    bool blink = leds[FO_LED_SOFT] != 0;
    run(250);
    CHECK((leds[FO_LED_SOFT] != 0) != blink, "Soft/Hard blink in setup mode");
    fo_turn(&FO, FO_GAIN, -2);
    fo_press(&FO, FO_BTN_FOOT);
    run(20);
    CHECK(!FO.setup && FO.pd.channel == 13, "footswitch saves channel 14");
    power_up_flashover(false);
    CHECK(FO.pd.channel == 13, "kept through a power cycle");
    fo_powered = false;                              /* unplugged for a moment: Switchyard rescans */
    run(1500);
    power_up_flashover(false);
    run(3000);
    CHECK(SY.npedals == 2 && SY.pedal[0].ch == 13 && FO.pd.channel == 13 && SB.channel == 1,
          "Switchyard's scan keeps channel 14 (table %d)", SY.pedal[0].ch);

    printf("12. Loop break before Flashover\n");
    link_[0].cut = true;
    run(1500);
    CHECK(!link_led, "Link LED off when MIDI in goes quiet");
    link_[0].cut = false;
    run(3000);
    CHECK(link_led && !SY.broken, "back on after repair");

    printf("\n%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
