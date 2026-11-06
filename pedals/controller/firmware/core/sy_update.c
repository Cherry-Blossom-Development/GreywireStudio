#include "sy_update.h"
#include <string.h>
#include "lw_fw.h"

static uint32_t now(sy_update *u) { return u->now_ms(u->ctx); }

/* acknowledgement from Switchyard itself: ch = 7F; Firmware: data adds the block number */
static void ack(sy_update *u, uint8_t qq, uint8_t cmd, uint8_t st, int block)
{
    lw_msg m;
    lw_begin(&m, LW_ACK, qq);
    lw_put(&m, LW_NO_CHANNEL);
    lw_put(&m, cmd);
    lw_put(&m, st);
    if (block >= 0)
        lw_put14(&m, (unsigned)block);
    lw_end(&m);
    u->reply(u->ctx, m.b, m.n);
}

void syu_init(sy_update *u)
{
    u->state = SYU_OFF;
    u->blocks = u->got = 0;
}

static void begin(sy_update *u, uint8_t qq, const uint8_t *b)
{
    if (!u->hal) {
        ack(u, qq, LW_FW_BEGIN, ST_UNKNOWN, -1);
        return;
    }
    uint32_t size = b[0] | (uint32_t)b[1] << 7 | (uint32_t)b[2] << 14;
    uint32_t crc = lw_get32(&b[3]);
    int st = u->hal->begin(u->hal->ctx, size, crc);
    if (st != ST_OK) {
        u->state = SYU_OFF;
        ack(u, qq, LW_FW_BEGIN, (uint8_t)st, -1);
        return;
    }
    u->state = SYU_ERASING;                    /* acknowledged once erased (syu_poll) */
    u->begin_qq = qq;
    u->blocks = (size + FW_BLOCK - 1) / FW_BLOCK;
    u->got = 0;
}

static void data(sy_update *u, uint8_t qq, const uint8_t *b, int len)
{
    int n = len >= 3 ? (int)lw_get14(&b[1]) : 0;
    if (!u->hal) {
        ack(u, qq, LW_FW_DATA, ST_UNKNOWN, n);
        return;
    }
    if (len != 3 + FW_PACKED + 1 || u->state == SYU_OFF || u->state == SYU_RESTARTING) {
        ack(u, qq, LW_FW_DATA, ST_RANGE, n);
        return;
    }
    if (u->state == SYU_ERASING) {
        ack(u, qq, LW_FW_DATA, ST_BUSY, n);
        return;
    }
    if (lw_fw_checksum(&b[3], FW_PACKED) != b[3 + FW_PACKED]) {
        ack(u, qq, LW_FW_DATA, ST_CHECKSUM, n);
        return;
    }
    uint8_t d[FW_BLOCK];
    lw_unpack7(&b[3], FW_BLOCK, d);
    int st = u->hal->block(u->hal->ctx, (uint32_t)n, d);
    if (st == ST_OK)
        u->got++;                              /* a resent block counts twice; it's only for the display */
    ack(u, qq, LW_FW_DATA, (uint8_t)st, n);
}

static void finish(sy_update *u, uint8_t qq)
{
    if (!u->hal) {
        ack(u, qq, LW_FW_FINISH, ST_UNKNOWN, -1);
        return;
    }
    if (u->state != SYU_RECEIVING) {
        ack(u, qq, LW_FW_FINISH, u->state == SYU_ERASING ? ST_BUSY : ST_RANGE, -1);
        return;
    }
    int st = u->hal->finish(u->hal->ctx);
    ack(u, qq, LW_FW_FINISH, (uint8_t)st, -1);
    if (st == ST_OK) {
        u->state = SYU_RESTARTING;
        u->restart_at = now(u) + SYU_RESTART_MS;
    } else {
        u->state = SYU_OFF;                    /* the old firmware carries on */
    }
}

bool syu_app_msg(sy_update *u, const uint8_t *m, int n)
{
    if (!lw_is_linework(m, n))
        return false;
    uint8_t cmd = LW_CMD(m), qq = LW_QQ(m);
    const uint8_t *b = LW_BODY(m);
    int len = n - (LW_HDR_LEN + 2) - 1;       /* bytes between qq and F7 */
    if (cmd == LW_FW_BEGIN) {
        if (len != LW_SERIAL_LEN + 8 || memcmp(b, u->serial, LW_SERIAL_LEN) != 0)
            return false;                      /* for a pedal: into the loop */
        u->last = now(u);
        begin(u, qq, &b[LW_SERIAL_LEN]);
        return true;
    }
    if ((cmd == LW_FW_DATA || cmd == LW_FW_FINISH) && len >= 1 && b[0] == LW_NO_CHANNEL) {
        u->last = now(u);
        if (cmd == LW_FW_DATA)
            data(u, qq, b, len);
        else
            finish(u, qq);
        return true;
    }
    return false;
}

void syu_poll(sy_update *u)
{
    switch (u->state) {
    case SYU_ERASING:
        /* one 4 KB sector per call: about 45 ms on the chip, so USB keeps being served between */
        if (u->hal->erase_step(u->hal->ctx)) {
            u->state = SYU_RECEIVING;
            u->last = now(u);
            ack(u, u->begin_qq, LW_FW_BEGIN, ST_OK, -1);
        }
        break;
    case SYU_RECEIVING:
        if (now(u) - u->last >= SYU_IDLE_MS)
            u->state = SYU_OFF;                /* the app went away: nothing was installed */
        break;
    case SYU_RESTARTING:
        if ((int32_t)(now(u) - u->restart_at) >= 0) {
            u->state = SYU_OFF;
            u->hal->restart(u->hal->ctx);      /* the loader installs it */
        }
        break;
    default:
        break;
    }
}

int syu_percent(const sy_update *u)
{
    if (u->state == SYU_ERASING || !u->blocks)
        return -1;
    uint32_t p = u->got * 100u / u->blocks;
    return p > 99 ? 99 : (int)p;
}
