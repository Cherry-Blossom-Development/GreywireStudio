/* The simulated loop shared by sim.c (the checks) and bridge.c (for the test tool):
 * Switchyard and NPED stand-in pedals joined by MIDI cables at the real 31,250 baud. */
#ifndef WORLD_H
#define WORLD_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "switchyard.h"
#include "pedal.h"
#include "lw_fw.h"

/* ---------------------------------------------------------------- simulated world */

static uint32_t T;                                  /* ms since start */
static uint32_t now_ms(void *c) { (void)c; return T; }

#define NPED 3
#define QSZ 8192
typedef struct { uint8_t q[QSZ]; int head, tail; bool cut; } cable;
static cable link_[NPED + 1];                       /* 0: Switchyard -> P1, k: Pk -> Pk+1, NPED: last -> Switchyard */

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

static uint8_t store[SY_STORE_SIZE];
static void st_read(void *c, uint32_t off, void *buf, uint32_t n) { (void)c; memcpy(buf, &store[off], n); }
static void st_write(void *c, uint32_t off, const void *buf, uint32_t n)
{
    (void)c;
    const uint8_t *b = buf;
    for (uint32_t i = 0; i < n; i++)
        store[off + i] &= b[i];                     /* NOR flash: writes can only clear bits */
}
static void st_erase(void *c, uint32_t off, uint32_t n) { (void)c; memset(&store[off], 0xFF, n); }

static char disp[5];
static uint8_t disp_dp;
static void display(void *c, const char t[4], uint8_t dp)
{
    (void)c;
    memcpy(disp, t, 4);
    disp_dp = dp;
}

/* messages seen by the app and on the other-brands out */
#define MAXLOG 4096
typedef struct { uint8_t b[160]; int n; uint32_t t; } logged;
static logged app_log[MAXLOG], other_log[64];
static int napp, nother;
static lw_parser app_parse;
static void app_msg(void *c, const uint8_t *m, int n)
{
    (void)c;
    if (napp < MAXLOG && n <= 160) {
        memcpy(app_log[napp].b, m, n);
        app_log[napp].n = n;
        app_log[napp].t = T;
        napp++;
    }
}
static void app_send_hal(void *c, const uint8_t *m, int n)
{
    (void)c;
    for (int i = 0; i < n; i++)
        lw_parse(&app_parse, m[i]);
}
static void other_send_hal(void *c, const uint8_t *m, int n)
{
    (void)c;
    if (nother < 64) {
        memcpy(other_log[nother].b, m, n);
        other_log[nother++].n = n;
    }
}
static bool app_conn(void *c) { (void)c; return true; }
static bool p9v(void *c) { (void)c; return true; }
static void sy_loop_send(void *c, const uint8_t *m, int n) { (void)c; cable_put(&link_[0], m, n); }

static switchyard SY;
static sy_hal syhal = {
    .now_ms = now_ms, .loop_send = sy_loop_send, .other_send = other_send_hal, .app_send = app_send_hal,
    .app_connected = app_conn, .power_9v = p9v, .store_read = st_read, .store_write = st_write,
    .store_erase = st_erase, .display = display,
    .serial = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x01, 0x02, 0x03},
};

static lw_pedal P[NPED];
static void ped_send(void *c, const uint8_t *m, int n) { cable_put(&link_[(intptr_t)c + 1], m, n); }

/* Each stand-in has its own 2 MB flash chip, laid out like a real pedal's (lw_fw.h): the
 * firmware, the download area, the update record, and its settings in the last 64 KB. So it
 * keeps its settings and takes firmware updates through the real code. A simulated install
 * raises the last number of its firmware version, 0.1.0 -> 0.1.1 and so on. */
#define CHIP (2u * 1024u * 1024u)
static uint8_t chip[NPED][CHIP];
static void ch_read(void *c, uint32_t off, void *buf, uint32_t n) { memcpy(buf, (uint8_t *)c + off, n); }
static void ch_write(void *c, uint32_t off, const void *buf, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
        ((uint8_t *)c)[off + i] &= ((const uint8_t *)buf)[i];
}
static void ch_erase(void *c, uint32_t off, uint32_t n) { memset((uint8_t *)c + off, 0xFF, n); }
static lw_flash whole[NPED], settings[NPED];
static lw_fw_rx rx[NPED];
static bool restart_asked[NPED];
static uint8_t fw_patch[NPED];
static int u_begin(void *c, uint32_t size, uint32_t crc) { int k = (int)(intptr_t)c; return lw_fw_begin(&rx[k], &whole[k], size, crc); }
static bool u_erase(void *c) { return lw_fw_erase_step(&rx[(intptr_t)c]); }
static int u_block(void *c, uint32_t n, const uint8_t *d) { return lw_fw_block(&rx[(intptr_t)c], n, d); }
static int u_finish(void *c) { return lw_fw_finish(&rx[(intptr_t)c]); }
static void u_restart(void *c) { restart_asked[(intptr_t)c] = true; }
static pd_fw_hal upd[NPED];

static void make_pedal(int k)
{
    if (!whole[k].read) {                            /* first power-up: a blank chip */
        memset(chip[k], 0xFF, CHIP);
        whole[k] = (lw_flash){chip[k], ch_read, ch_write, ch_erase};
        settings[k] = (lw_flash){chip[k] + CHIP - 64 * 1024, ch_read, ch_write, ch_erase};
        upd[k] = (pd_fw_hal){(void *)(intptr_t)k, u_begin, u_erase, u_block, u_finish, u_restart};
    }
    pd_hal h = {
        .ctx = (void *)(intptr_t)k, .now_ms = now_ms, .send = ped_send,
        .update = &upd[k], .flash = &settings[k],
        .model = {0x7F, 0x7F}, .fw = {0, 1, fw_patch[k]},
        .ncontrols = 4, .cc = {20, 21, 22, 80}, .value = {64, 64, 64, 0},
    };
    lw_serial_from_u64(0x1234567800000000ull + (uint64_t)(k + 1) * 0x10101ull, h.serial);
    pd_init(&P[k], &h);
}

/* the watchdog restart after an update: the loader installs it, then the firmware starts */
static void restart_pedal(int k)
{
    restart_asked[k] = false;
    int did = FW_DID_NOTHING;
    lw_fw_boot(&whole[k], false, &did);
    if (did == FW_DID_INSTALL)
        fw_patch[k]++;
    make_pedal(k);
    if (lw_fw_take_announce(&whole[k]))
        pd_announce(&P[k], 1000);
}

static void tick(void)
{
    T++;
    for (int k = 0; k <= NPED; k++)
        for (int i = 0; i < 3; i++) {                /* 31,250 baud is about 3 bytes per ms */
            int b = cable_get(&link_[k]);
            if (b < 0)
                break;
            if (k < NPED)
                pd_byte(&P[k], (uint8_t)b);
            else
                sy_loop_byte(&SY, (uint8_t)b);
        }
    for (int k = 0; k < NPED; k++) {
        pd_poll(&P[k]);
        if (restart_asked[k])
            restart_pedal(k);
    }
    sy_poll(&SY);
}

static void run(int ms)
{
    while (ms--)
        tick();
}

#endif
