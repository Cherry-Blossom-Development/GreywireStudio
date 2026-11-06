/* Switchyard's own firmware update over USB (Linework Control Specification, Firmware updates):
 * the app sends Firmware: begin addressed to Switchyard's serial number, then Firmware: data and
 * Firmware: finish with ch = 7F. These are handled here and never go into the loop.
 *
 * Plain C, shared by Switchyard's firmware (core/switchyard.c) and its recovery loader
 * (pico/sy_loader.c), and run in the PC simulation. The flash work is done by the same update
 * hooks the pedals use (pd_fw_hal, libraries/linework/pico/lw_update.c on the RP2040). */
#ifndef SY_UPDATE_H
#define SY_UPDATE_H

#include "pedal.h"                 /* pd_fw_hal */

#define SYU_IDLE_MS 10000          /* an update with no message for this long is abandoned */
#define SYU_RESTART_MS 200         /* after the Finish ack: time for it to leave over USB */

typedef struct {
    const pd_fw_hal *hal;          /* NULL: Firmware: begin to Switchyard gets "unknown command" */
    void *ctx;
    uint32_t (*now_ms)(void *ctx);
    void (*reply)(void *ctx, const uint8_t *m, int n);      /* to the app */
    const uint8_t *serial;         /* Switchyard's, LW_SERIAL_LEN bytes */

    uint8_t state;                 /* SYU_* */
    uint8_t begin_qq;
    uint32_t last, restart_at;
    uint32_t blocks, got;          /* blocks in the file, blocks written */
} sy_update;

enum { SYU_OFF, SYU_ERASING, SYU_RECEIVING, SYU_RESTARTING };

void syu_init(sy_update *u);
/* a message from the app: true if it was Switchyard's own update (and so is handled here) */
bool syu_app_msg(sy_update *u, const uint8_t *m, int n);
void syu_poll(sy_update *u);
static inline bool syu_active(const sy_update *u) { return u->state != SYU_OFF; }
int syu_percent(const sy_update *u);    /* -1 while erasing */

#endif
