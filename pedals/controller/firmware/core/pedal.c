#include "pedal.h"
#include <string.h>

#define T_QUIET 1000           /* MIDI in quiet this long: send Loop break reports, once a second */

static uint32_t now(lw_pedal *p) { return p->hal.now_ms(p->hal.ctx); }
static void out(lw_pedal *p, const uint8_t *m, int n) { p->hal.send(p->hal.ctx, m, n); }

static void ack(lw_pedal *p, uint8_t qq, uint8_t cmd, uint8_t st)
{
    lw_msg m;
    lw_begin(&m, LW_ACK, qq);
    lw_put(&m, p->channel);
    lw_put(&m, cmd);
    lw_put(&m, st);
    lw_end(&m);
    out(p, m.b, m.n);
}

static void put_pairs(lw_pedal *p, lw_msg *m, const uint8_t *vals)
{
    for (int i = 0; i < p->hal.ncontrols; i++) {
        lw_put(m, p->hal.cc[i]);
        lw_put(m, vals[i]);
    }
}

static void send_cc(lw_pedal *p, int i)
{
    if (p->channel > 0x0F)
        return;
    uint8_t m[3] = {(uint8_t)(0xB0 | p->channel), p->hal.cc[i], p->value[i]};
    out(p, m, 3);
}

static int control_index(lw_pedal *p, uint8_t cc)
{
    for (int i = 0; i < p->hal.ncontrols; i++)
        if (p->hal.cc[i] == cc)
            return i;
    return -1;
}

/* pairs (cc value ...) into a settings array; false if any control is unknown */
static bool read_pairs(lw_pedal *p, const uint8_t *b, int len, uint8_t *vals)
{
    if (len % 2)
        return false;
    for (int k = 0; k < len; k += 2) {
        int i = control_index(p, b[k]);
        if (i < 0)
            return false;
        vals[i] = b[k + 1];
    }
    return true;
}

static void identity(lw_pedal *p)
{
    uint8_t r[24] = {0xF0, 0x7E, p->channel, 0x06, 0x02};
    int k = 5;
    memcpy(&r[k], &LW_HEADER[1], LW_HDR_LEN - 1);
    k += LW_HDR_LEN - 1;
    r[k++] = p->hal.model[0];
    r[k++] = p->hal.model[1];
    r[k++] = p->hal.fw[0];
    r[k++] = p->hal.fw[1];
    r[k++] = p->hal.fw[2];
    r[k++] = LW_SPEC_VERSION;
    r[k++] = 0xF7;
    out(p, r, k);
    lw_msg m;                                   /* then pedal info, straight after */
    lw_begin(&m, LW_PEDAL_INFO, 0);
    lw_put(&m, p->position);
    lw_put(&m, p->channel);
    lw_put_n(&m, p->hal.serial, LW_SERIAL_LEN);
    lw_end(&m);
    out(p, m.b, m.n);
}

static void linework(lw_pedal *p, const uint8_t *m, int n)
{
    uint8_t cmd = LW_CMD(m), qq = LW_QQ(m);
    const uint8_t *b = LW_BODY(m);
    int len = n - (LW_HDR_LEN + 2) - 1;

    if (cmd == LW_POSITION && len == 1) {       /* count: save kk + 1 and pass on kk + 1 */
        p->position = (uint8_t)(b[0] + 1);
        lw_msg r;
        lw_begin(&r, LW_POSITION, qq);
        lw_put(&r, p->position);
        lw_end(&r);
        out(p, r.b, r.n);
        return;
    }
    out(p, m, n);                               /* everything else passes on unchanged */

    if (cmd == LW_ASSIGN && len == LW_SERIAL_LEN + 1) {
        if (memcmp(b, p->hal.serial, LW_SERIAL_LEN) == 0) {
            if (b[LW_SERIAL_LEN] <= 0x0F) {
                p->channel = b[LW_SERIAL_LEN];
                ack(p, qq, LW_ASSIGN, ST_OK);
            } else {
                ack(p, qq, LW_ASSIGN, ST_RANGE);
            }
        }
        return;
    }
    if (len < 1 || b[0] != p->channel || p->channel > 0x0F)
        return;                                 /* the rest are addressed by channel */
    switch (cmd) {
    case LW_REPORT_REQ: {
        lw_msg r;
        lw_begin(&r, LW_REPORT, qq);
        lw_put(&r, p->channel);
        put_pairs(p, &r, p->value);
        lw_end(&r);
        out(p, r.b, r.n);
        break;
    }
    case LW_STORE: {                            /* ch pp ff (pairs) */
        if (len < 3 || b[1] > 127) {
            ack(p, qq, LW_STORE, ST_RANGE);
            break;
        }
        uint8_t *slot = p->preset[b[1]];
        if (b[2] == 1) {
            memcpy(slot, p->value, sizeof p->value);
        } else {
            uint8_t vals[PD_MAX_CONTROLS];
            memcpy(vals, slot, sizeof vals);
            if (!read_pairs(p, &b[3], len - 3, vals)) {
                ack(p, qq, LW_STORE, ST_RANGE);
                break;
            }
            memcpy(slot, vals, sizeof vals);
        }
        ack(p, qq, LW_STORE, ST_OK);
        break;
    }
    case LW_READ_REQ: {
        if (len != 2 || b[1] > 127) {
            ack(p, qq, LW_READ_REQ, ST_RANGE);
            break;
        }
        lw_msg r;
        lw_begin(&r, LW_READ, qq);
        lw_put(&r, p->channel);
        lw_put(&r, b[1]);
        put_pairs(p, &r, p->preset[b[1]]);
        lw_end(&r);
        out(p, r.b, r.n);
        break;
    }
    case LW_FW_BEGIN: case LW_FW_DATA: case LW_FW_FINISH:
        ack(p, qq, cmd, ST_UNKNOWN);            /* the stand-in has no firmware update */
        break;
    default:
        ack(p, qq, cmd, ST_UNKNOWN);
        break;
    }
}

static void on_msg(void *ctx, const uint8_t *m, int n)
{
    lw_pedal *p = ctx;
    p->heard = now(p);
    if (lw_is_linework(m, n)) {
        linework(p, m, n);
        return;
    }
    out(p, m, n);                               /* merge: pass everything else on first */
    if (n == 6 && m[0] == 0xF0 && m[1] == 0x7E && m[3] == 0x06 && m[4] == 0x01
        && (m[2] == 0x7F || m[2] == p->channel)) {
        identity(p);
        return;
    }
    if (p->channel > 0x0F || (m[0] & 0x0F) != p->channel)
        return;
    if ((m[0] & 0xF0) == 0xC0 && n == 2) {      /* program change: recall, then report the settings */
        p->last_preset = m[1];
        memcpy(p->value, p->preset[m[1]], sizeof p->value);
        for (int i = 0; i < p->hal.ncontrols; i++)
            send_cc(p, i);
    } else if ((m[0] & 0xF0) == 0xB0 && n == 3) {
        int i = control_index(p, m[1]);
        if (i >= 0)
            p->value[i] = m[2];
    }
}

void pd_init(lw_pedal *p, const pd_hal *hal)
{
    memset(p, 0, sizeof *p);
    p->hal = *hal;
    p->channel = LW_NO_CHANNEL;
    p->last_preset = -1;
    memcpy(p->value, hal->value, sizeof p->value);
    for (int s = 0; s < 128; s++)
        memcpy(p->preset[s], hal->value, sizeof p->value);
    lw_parser_init(&p->rx, on_msg, p);
    p->heard = p->reported = now(p);
}

void pd_byte(lw_pedal *p, uint8_t b) { lw_parse(&p->rx, b); }

void pd_set_control(lw_pedal *p, int i, uint8_t v)
{
    if (i < 0 || i >= p->hal.ncontrols)
        return;
    p->value[i] = v & 0x7F;
    send_cc(p, i);                              /* hand changes are reported on the pedal's channel */
}

void pd_poll(lw_pedal *p)
{
    uint32_t t = now(p);
    if (p->position && t - p->heard >= T_QUIET && t - p->reported >= 1000) {
        lw_msg m;
        lw_begin(&m, LW_LOOP_BREAK, 0);
        lw_put(&m, p->position);
        lw_end(&m);
        out(p, m.b, m.n);
        p->reported = t;
    }
}
