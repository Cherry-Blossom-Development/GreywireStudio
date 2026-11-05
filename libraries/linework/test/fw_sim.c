/* PC simulation of a firmware update over the Linework loop: the fake phone app sends a new
 * firmware through Switchyard to Flashover (commands 30-32), Flashover restarts, and the loader
 * logic installs it. The pedal's whole 2 MB flash is simulated, power cuts included.
 * Uses the real Switchyard, Flashover and shared Linework code.
 *
 * Build and run:  make -C test test */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "switchyard.h"
#include "flashover.h"
#include "lw_fw.h"

static int fails, checks;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { fails++; printf("  FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static uint32_t T;
static uint32_t now_ms(void *c) { (void)c; return T; }

#define QSZ 16384
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

/* ---------------------------------------------------------------- Switchyard and the app */
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

#define MAXLOG 8192
typedef struct { uint8_t b[160]; int n; uint32_t t; } logged;
static logged app_log[MAXLOG];
static int napp;
static lw_parser app_parse;
static void app_msg(void *c, const uint8_t *m, int n)
{
    (void)c;
    if (napp >= MAXLOG) {                            /* keep the newest */
        memmove(app_log, app_log + MAXLOG / 2, sizeof(logged) * (MAXLOG / 2));
        napp = MAXLOG / 2;
    }
    if (n <= 160) {
        memcpy(app_log[napp].b, m, n);
        app_log[napp].n = n;
        app_log[napp++].t = T;
    }
}
static void app_send(void *c, const uint8_t *m, int n) { (void)c; for (int i = 0; i < n; i++) lw_parse(&app_parse, m[i]); }
static switchyard SY;
static sy_hal syhal = {
    .now_ms = now_ms, .loop_send = sy_loop_send, .other_send = other_send, .app_send = app_send,
    .app_connected = yes, .power_9v = yes, .store_read = sst_read, .store_write = sst_write,
    .store_erase = sst_erase, .display = sy_disp,
    .serial = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x01, 0x02, 0x03},
};
static lw_pedal SB;
static void sb_send(void *c, const uint8_t *m, int n) { (void)c; cable_put(&link_[2], m, n); }

/* ---------------------------------------------------------------- Flashover's chip */
#define CHIP (2u * 1024u * 1024u)
static uint8_t chip[CHIP];
static bool powered = true;
static int tear = -1;            /* >= 0: the power goes after this many more bytes are written */
static int erases;
static void ch_read(void *c, uint32_t off, void *buf, uint32_t n) { memcpy(buf, &chip[(uintptr_t)c + off], n); }
static void ch_write(void *c, uint32_t off, const void *buf, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        if (!powered)
            return;
        if (tear == 0) {
            powered = false;
            return;
        }
        if (tear > 0)
            tear--;
        chip[(uintptr_t)c + off + i] &= ((const uint8_t *)buf)[i];
    }
}
static void ch_erase(void *c, uint32_t off, uint32_t n)
{
    if (!powered)
        return;
    if (((uintptr_t)c + off) % 4096 || n % 4096)
        printf("  bad erase %u %u\n", off, n);
    memset(&chip[(uintptr_t)c + off], 0xFF, n);
    erases++;
}
static const lw_flash whole = {(void *)0, ch_read, ch_write, ch_erase};
static const lw_flash settings = {(void *)(uintptr_t)(CHIP - 64 * 1024), ch_read, ch_write, ch_erase};

/* the update hooks, as the RP2040 firmware and the loader provide them */
static lw_fw_rx rx;
static bool restart_asked;
static int u_begin(void *c, uint32_t size, uint32_t crc) { (void)c; return lw_fw_begin(&rx, &whole, size, crc); }
static bool u_erase(void *c) { (void)c; return lw_fw_erase_step(&rx); }
static int u_block(void *c, uint32_t n, const uint8_t *d) { (void)c; return lw_fw_block(&rx, n, d); }
static int u_finish(void *c) { (void)c; return lw_fw_finish(&rx); }
static void u_restart(void *c) { (void)c; restart_asked = true; }
static const pd_fw_hal upd = {NULL, u_begin, u_erase, u_block, u_finish, u_restart};

/* what's running on the simulated pedal: the firmware (Flashover) or the loader in recovery */
static flashover FO;
static lw_pedal LOADER;
static bool in_recovery;
static bool r_on;
static void hw_pot(void *c, int p, uint8_t v) { (void)c; (void)p; (void)v; }
static void hw_relays(void *c, bool hard, bool on) { (void)c; (void)hard; r_on = on; }
static void hw_leds(void *c, const uint8_t *p) { (void)c; (void)p; }
static void hw_link(void *c, bool on) { (void)c; (void)on; }
static void fo_send(void *c, const uint8_t *m, int n) { (void)c; if (powered) cable_put(&link_[1], m, n); }
static const uint8_t SERIAL[LW_SERIAL_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};

/* power-up: the loader decides, then starts the firmware or stays in recovery */
static int boot(bool recovery_held, int *did)
{
    powered = true;
    restart_asked = false;
    r_on = false;
    int r = lw_fw_boot(&whole, recovery_held, did);
    if (!powered)
        return -1;                                   /* power lost during the install */
    in_recovery = r == FW_BOOT_RECOVERY;
    if (in_recovery) {
        /* the loader is a minimal Linework pedal: it keeps the loop going and takes an update */
        pd_hal h = {.now_ms = now_ms, .send = fo_send, .update = &upd, .flash = &settings,
                    .model = {FO_MODEL[0], FO_MODEL[1]}, .fw = {0, 0, 0}, .ncontrols = 0};
        memcpy(h.serial, SERIAL, LW_SERIAL_LEN);
        pd_init(&LOADER, &h);
    } else {
        fo_hal h = {.now_ms = now_ms, .midi_send = fo_send, .set_pot = hw_pot, .set_relays = hw_relays,
                    .set_leds = hw_leds, .set_link_led = hw_link, .flash = &settings, .fw = &upd};
        memcpy(h.serial, SERIAL, LW_SERIAL_LEN);
        fo_init(&FO, &h, false);
        if (lw_fw_take_announce(&whole))
            pd_announce(&FO.pd, 500);
    }
    return r;
}

static void tick(void)
{
    T++;
    for (int k = 0; k < 3; k++)
        for (int i = 0; i < 3; i++) {
            int b = cable_get(&link_[k]);
            if (b < 0)
                break;
            if (k == 0) {
                if (powered)
                    in_recovery ? pd_byte(&LOADER, (uint8_t)b) : fo_midi_byte(&FO, (uint8_t)b);
            } else if (k == 1) {
                pd_byte(&SB, (uint8_t)b);
            } else {
                sy_loop_byte(&SY, (uint8_t)b);
            }
        }
    if (powered)
        in_recovery ? pd_poll(&LOADER) : fo_poll(&FO);
    if (powered && restart_asked)
        boot(false, NULL);                           /* the watchdog restart */
    pd_poll(&SB);
    sy_poll(&SY);
}
static void run(int ms) { while (ms--) tick(); }

/* ---------------------------------------------------------------- the app's side */
static void app(const uint8_t *m, int n) { for (int i = 0; i < n; i++) sy_app_byte(&SY, m[i]); }
static uint8_t QQ = 0x40;

static int ack_from(int mark, uint8_t qq, uint8_t cmd, unsigned *block)
{
    for (int i = mark; i < napp; i++) {
        uint8_t *b = app_log[i].b;
        if (lw_is_linework(b, app_log[i].n) && LW_CMD(b) == LW_ACK && LW_QQ(b) == qq && LW_BODY(b)[1] == cmd) {
            if (block)
                *block = app_log[i].n >= LW_HDR_LEN + 8 ? lw_get14(&LW_BODY(b)[3]) : 0xFFFF;
            return LW_BODY(b)[2];
        }
    }
    return -1;
}

/* send and wait for the acknowledgement: its status, or -1 after timeout ms */
static int ask(lw_msg *m, uint8_t cmd, int timeout, unsigned *block)
{
    uint8_t qq = LW_QQ(m->b);
    int mark = napp;
    app(m->b, m->n);
    for (int t = 0; t < timeout; t++) {
        tick();
        int st = ack_from(mark, qq, cmd, block);
        if (st >= 0)
            return st;
    }
    return -1;
}

static uint8_t next_qq(void) { QQ = (uint8_t)(QQ == 0x7F ? 0x40 : QQ + 1); return QQ; }

static int fw_begin(const uint8_t *serial, uint32_t size, uint32_t crc, int timeout)
{
    lw_msg m;
    lw_begin(&m, LW_FW_BEGIN, next_qq());
    lw_put_n(&m, serial, LW_SERIAL_LEN);
    lw_put(&m, size & 0x7F);
    lw_put(&m, (size >> 7) & 0x7F);
    lw_put(&m, (size >> 14) & 0x7F);
    lw_put32(&m, crc);
    lw_end(&m);
    return ask(&m, LW_FW_BEGIN, timeout, NULL);
}

static int fw_block(uint8_t ch, unsigned n, const uint8_t *data, bool spoil, unsigned *echo)
{
    uint8_t packed[FW_PACKED];
    lw_pack7(data, FW_BLOCK, packed);
    uint8_t sum = lw_fw_checksum(packed, FW_PACKED);
    if (spoil)
        packed[10] ^= 0x01;                          /* a byte damaged on the way */
    lw_msg m;
    lw_begin(&m, LW_FW_DATA, next_qq());
    lw_put(&m, ch);
    lw_put14(&m, n);
    lw_put_n(&m, packed, FW_PACKED);
    lw_put(&m, sum);
    lw_end(&m);
    return ask(&m, LW_FW_DATA, 500, echo);
}

static int fw_finish(uint8_t ch)
{
    lw_msg m;
    lw_begin(&m, LW_FW_FINISH, next_qq());
    lw_put(&m, ch);
    lw_end(&m);
    return ask(&m, LW_FW_FINISH, 3000, NULL);
}

/* a whole update, as the app would do it; returns the Finish status (or the first failure) */
static int retries;
static int update(const uint8_t *img, uint32_t size, uint32_t crc_sent, int stop_after_blocks)
{
    uint8_t ch = in_recovery ? LOADER.channel : FO.pd.channel;
    int st = fw_begin(SERIAL, size, crc_sent, 5000);
    if (st != ST_OK)
        return 100 + st;
    unsigned blocks = (size + FW_BLOCK - 1) / FW_BLOCK;
    for (unsigned n = 0; n < blocks; n++) {
        if ((int)n == stop_after_blocks)
            return -2;
        uint8_t d[FW_BLOCK];
        memset(d, 0xFF, sizeof d);
        memcpy(d, &img[n * FW_BLOCK], size - n * FW_BLOCK < FW_BLOCK ? size - n * FW_BLOCK : FW_BLOCK);
        unsigned echo = 0;
        int r = fw_block(ch, n, d, false, &echo);
        while (r == ST_CHECKSUM || r == ST_BUSY || r < 0) {   /* resend, as Switchyard would */
            retries++;
            r = fw_block(ch, n, d, false, &echo);
        }
        if (r != ST_OK || echo != n)
            return 200 + r;
    }
    return fw_finish(ch);
}

/* a made-up firmware image: random bytes with a vector table the loader accepts */
static void make_image(uint8_t *img, uint32_t size, unsigned seed)
{
    srand(seed);
    for (uint32_t i = 0; i < size; i++)
        img[i] = (uint8_t)rand();
    uint32_t sp = 0x20042000u, reset = 0x10000000u + FW_APP_OFF + 0x1C1u;
    memcpy(&img[FW_VECTORS], &sp, 4);
    memcpy(&img[FW_VECTORS + 4], &reset, 4);
}

static bool app_is(const uint8_t *img, uint32_t size) { return memcmp(&chip[FW_APP_OFF], img, size) == 0; }

static int identities_since(int mark)
{
    int k = 0;
    for (int i = mark; i < napp; i++)
        if (app_log[i].n > 5 && app_log[i].b[0] == 0xF0 && app_log[i].b[1] == 0x7E && app_log[i].b[4] == 0x02
            && app_log[i].b[8] == FO_MODEL[0] && app_log[i].b[9] == FO_MODEL[1])
            k++;
    return k;
}

int main(void)
{
    static uint8_t A[30000], B[40000], C[52000];
    make_image(A, sizeof A, 1);
    make_image(B, sizeof B, 2);
    make_image(C, sizeof C, 3);
    lw_parser_init(&app_parse, app_msg, NULL);
    memset(sy_store, 0xFF, sizeof sy_store);
    memset(chip, 0xFF, sizeof chip);
    memcpy(&chip[FW_APP_OFF], A, sizeof A);          /* loaded over USB: no update record yet */
    T = 1000;

    printf("1. First power-up (firmware loaded over USB, no update record)\n");
    int did;
    CHECK(boot(false, &did) == FW_BOOT_APP && did == FW_DID_NOTHING, "the loader starts the firmware");
    pd_hal sbh = {.now_ms = now_ms, .send = sb_send, .model = {0x7F, 0x7F}, .fw = {0, 1, 0},
                  .ncontrols = 1, .cc = {20}, .kind = {0}, .value = {64}};
    lw_serial_from_u64(0x99, sbh.serial);
    pd_init(&SB, &sbh);
    sy_init(&SY, &syhal);
    run(2500);
    CHECK(SY.npedals == 2 && FO.pd.channel == 0, "Switchyard counts 2 pedals; Flashover on channel 1");
    lw_msg m;
    lw_begin(&m, 0, 0);
    uint8_t onmsg[3] = {0xB0, 80, 127};
    app(onmsg, 3);
    run(100);
    CHECK(r_on, "effect on before the update");

    printf("2. Messages that must be refused\n");
    uint8_t other[LW_SERIAL_LEN] = {9, 9, 9, 9, 9, 9, 9, 9, 9, 9};
    CHECK(fw_begin(other, 1000, 0, 300) == -1, "Begin for another serial: no answer (it is passed on)");
    CHECK(fw_begin(SERIAL, FW_APP_SIZE + 1, 0, 300) == ST_RANGE, "too big: value out of range");
    uint8_t blk[FW_BLOCK] = {0};
    unsigned echo;
    CHECK(fw_block(0, 0, blk, false, &echo) == ST_RANGE, "data before Begin: refused");
    CHECK(r_on && !pd_updating(&FO.pd), "still on, not updating");

    printf("3. An update, block by block\n");
    uint32_t crcB = lw_crc32(0, B, sizeof B);
    int mark = napp;
    erases = 0;
    lw_begin(&m, LW_FW_BEGIN, next_qq());
    lw_put_n(&m, SERIAL, LW_SERIAL_LEN);
    lw_put(&m, sizeof B & 0x7F);
    lw_put(&m, (sizeof B >> 7) & 0x7F);
    lw_put(&m, (sizeof B >> 14) & 0x7F);
    lw_put32(&m, crcB);
    lw_end(&m);
    uint8_t bq = LW_QQ(m.b);
    app(m.b, m.n);                                   /* Begin, and a block straight behind it */
    CHECK(fw_block(0, 0, B, false, &echo) == ST_BUSY, "a block during the erase: busy");
    int ib = -1, ik = -1;
    for (int i = mark; i < napp; i++) {
        uint8_t *b = app_log[i].b;
        if (lw_is_linework(b, app_log[i].n) && LW_CMD(b) == LW_ACK && LW_BODY(b)[1] == LW_FW_DATA && ik < 0)
            ik = i;
        if (lw_is_linework(b, app_log[i].n) && LW_CMD(b) == LW_ACK && LW_QQ(b) == bq && ib < 0)
            ib = i;
    }
    CHECK(pd_updating(&FO.pd) && (ib < 0 || ib > ik), "the Begin ack waits for the erase");
    run(200);
    CHECK(!r_on, "bypassed during the update");
    for (int t = 0; t < 3000 && ack_from(mark, bq, LW_FW_BEGIN, NULL) < 0; t++)
        tick();
    CHECK(ack_from(mark, bq, LW_FW_BEGIN, NULL) == ST_OK, "Begin acknowledged once erased");
    CHECK(erases == 10, "erased only what's needed: %d sectors for 40,000 bytes", erases);
    uint32_t t0 = T;
    int bad = 0, nblocks = (sizeof B + 63) / 64;
    for (int n = 0; n < nblocks; n++) {
        uint8_t d[FW_BLOCK];
        memset(d, 0xFF, sizeof d);
        memcpy(d, &B[n * 64], sizeof B - n * 64 < 64 ? sizeof B - n * 64 : 64);
        int r = fw_block(0, n, d, n == 100, &echo);  /* block 100 arrives damaged */
        if (n == 100) {
            CHECK(r == ST_CHECKSUM && echo == 100, "damaged block: checksum failed (block %u)", echo);
            r = fw_block(0, n, d, false, &echo);
        }
        if (r != ST_OK || echo != (unsigned)n)
            bad++;
    }
    CHECK(bad == 0, "every block acknowledged with its number (%d bad)", bad);
    CHECK(app_is(A, sizeof A), "the running firmware is untouched so far");
    printf("  625 blocks (40,000 bytes) took %.1f s, waiting for each acknowledgement\n", (T - t0) / 1000.0);
    mark = napp;
    CHECK(fw_finish(0) == ST_OK, "Finish: CRC matches");
    run(1500);
    CHECK(app_is(B, sizeof B), "after the restart the loader has installed it");
    CHECK(identities_since(mark) == 1, "the new version announces itself once");
    CHECK(FO.pd.channel == 0 && r_on, "settings kept: channel 1, effect on again");
    mark = napp;
    boot(false, &did);
    run(1500);
    CHECK(did == FW_DID_NOTHING && identities_since(mark) == 0 && app_is(B, sizeof B), "next power-up: no reinstall, no announcement");

    printf("4. An update with the wrong CRC\n");
    CHECK(update(C, sizeof C, 0x12345678, -1) == ST_FW_FAIL, "Finish: firmware check failed");
    run(1500);
    CHECK(app_is(B, sizeof B) && !pd_updating(&FO.pd) && r_on, "the old firmware carries on, effect on again");
    boot(false, &did);
    CHECK(did == FW_DID_NOTHING && app_is(B, sizeof B), "and nothing is installed at the next power-up");

    printf("5. Power cut halfway through a download\n");
    uint32_t crcC = lw_crc32(0, C, sizeof C);
    CHECK(update(C, sizeof C, crcC, 300) == -2, "stopped after 300 blocks");
    powered = false;
    run(100);
    CHECK(boot(false, &did) == FW_BOOT_APP && did == FW_DID_NOTHING && app_is(B, sizeof B), "comes back on the old firmware");
    run(2000);

    printf("6. Power cut while the loader is installing\n");
    CHECK(update(C, sizeof C, crcC, -1) == ST_OK, "full download, Finish OK");
    /* the restart: this time the power goes partway through the copy */
    tear = 20000;
    restart_asked = false;
    CHECK(boot(false, &did) == -1, "power lost during the install");
    tear = -1;
    CHECK(!app_is(C, sizeof C) && !app_is(B, sizeof B), "the firmware area is half written");
    CHECK(boot(false, &did) == FW_BOOT_APP && did == FW_DID_INSTALL && app_is(C, sizeof C), "next power-up finishes the install");
    run(2000);

    printf("7. A damaged firmware area\n");
    chip[FW_APP_OFF + 5000] ^= 0x40;
    CHECK(boot(false, &did) == FW_BOOT_APP && did == FW_DID_REPAIR && app_is(C, sizeof C), "repaired from the download area");
    chip[FW_APP_OFF + 5000] ^= 0x40;
    chip[FW_DL_OFF + 5000] ^= 0x40;
    chip[FW_APP_OFF + 7000] ^= 0x01;
    powered = false;                                 /* unplugged, then plugged back in */
    run(1500);
    CHECK(boot(false, &did) == FW_BOOT_RECOVERY, "both damaged: recovery mode");

    printf("8. Recovery mode\n");
    run(3000);
    CHECK(!SY.broken && LOADER.position == 1 && LOADER.channel == 0, "the loader keeps the loop going, counted at position 1");
    CHECK(SY.pedal[0].model[0] == FO_MODEL[0] && SY.pedal[0].fw[0] == 0 && SY.pedal[0].fw[1] == 0 && SY.pedal[0].fw[2] == 0,
          "Switchyard lists it as Flashover, firmware 0.0.0 (so the app knows to send an update)");
    CHECK(update(B, sizeof B, crcB, -1) == ST_OK, "takes an update");
    run(1500);
    CHECK(!in_recovery && app_is(B, sizeof B), "and starts it");
    CHECK(boot(true, &did) == FW_BOOT_RECOVERY, "recovery buttons held at power-up: recovery mode");
    CHECK(boot(false, &did) == FW_BOOT_APP, "not held: the firmware");
    run(2000);

    printf("9. A blank chip\n");
    memset(chip, 0xFF, sizeof chip);
    CHECK(boot(false, &did) == FW_BOOT_RECOVERY, "nothing to start: recovery mode");

    printf("10. 7-bit packing\n");
    uint8_t in[64], pk[74], outb[64];
    for (int i = 0; i < 64; i++)
        in[i] = (uint8_t)(i * 37 + 200);
    lw_pack7(in, 64, pk);
    int high = 0;
    for (int i = 0; i < 74; i++)
        high |= pk[i] & 0x80;
    lw_unpack7(pk, 64, outb);
    CHECK(!high && memcmp(in, outb, 64) == 0, "64 bytes travel as 74 MIDI data bytes and come back intact");
    printf("  (resends during the updates: %d)\n", retries);

    printf("\n%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
