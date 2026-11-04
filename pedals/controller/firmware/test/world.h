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

static void make_pedal(int k)
{
    pd_hal h = {
        .ctx = (void *)(intptr_t)k, .now_ms = now_ms, .send = ped_send,
        .model = {0x7F, 0x7F}, .fw = {0, 1, 0},
        .ncontrols = 4, .cc = {20, 21, 22, 80}, .value = {64, 64, 64, 0},
    };
    lw_serial_from_u64(0x1234567800000000ull + (uint64_t)(k + 1) * 0x10101ull, h.serial);
    pd_init(&P[k], &h);
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
    for (int k = 0; k < NPED; k++)
        pd_poll(&P[k]);
    sy_poll(&SY);
}

static void run(int ms)
{
    while (ms--)
        tick();
}

#endif
