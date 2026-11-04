#include "lw_proto.h"
#include <string.h>

void lw_begin(lw_msg *m, uint8_t cmd, uint8_t qq)
{
    memcpy(m->b, LW_HEADER, LW_HDR_LEN);
    m->n = LW_HDR_LEN;
    m->b[m->n++] = cmd & 0x7F;
    m->b[m->n++] = qq & 0x7F;
}

void lw_put(lw_msg *m, uint8_t v)
{
    if (m->n < (int)sizeof(m->b) - 1)
        m->b[m->n++] = v & 0x7F;
}

void lw_put_n(lw_msg *m, const uint8_t *v, int n)
{
    for (int i = 0; i < n; i++)
        lw_put(m, v[i]);
}

void lw_end(lw_msg *m)
{
    m->b[m->n++] = 0xF7;
}

bool lw_is_linework(const uint8_t *m, int n)
{
    return n >= LW_HDR_LEN + 3 && memcmp(m, LW_HEADER, LW_HDR_LEN) == 0 && m[n - 1] == 0xF7;
}

void lw_put14(lw_msg *m, unsigned v)
{
    lw_put(m, v & 0x7F);
    lw_put(m, (v >> 7) & 0x7F);
}

unsigned lw_get14(const uint8_t *p)
{
    return (p[0] & 0x7F) | ((unsigned)(p[1] & 0x7F) << 7);
}

void lw_put32(lw_msg *m, uint32_t v)
{
    for (int i = 0; i < 5; i++)
        lw_put(m, (v >> (7 * i)) & 0x7F);
}

uint32_t lw_get32(const uint8_t *p)
{
    uint32_t v = 0;
    for (int i = 0; i < 5; i++)
        v |= (uint32_t)(p[i] & 0x7F) << (7 * i);
    return v;
}

void lw_serial_from_u64(uint64_t id, uint8_t out[LW_SERIAL_LEN])
{
    for (int i = 0; i < LW_SERIAL_LEN; i++)
        out[i] = (id >> (7 * i)) & 0x7F;
}

uint32_t lw_crc32(uint32_t crc, const uint8_t *data, size_t n)
{
    crc = ~crc;
    while (n--) {
        crc ^= *data++;
        for (int k = 0; k < 8; k++)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

/* ---- parser ---- */

static int data_len(uint8_t status)
{
    switch (status & 0xF0) {
    case 0xC0: case 0xD0: return 1;           /* program change, channel pressure */
    case 0x80: case 0x90: case 0xA0: case 0xB0: case 0xE0: return 2;
    }
    switch (status) {
    case 0xF1: case 0xF3: return 1;
    case 0xF2: return 2;
    }
    return 0;
}

void lw_parser_init(lw_parser *p, lw_msg_fn fn, void *ctx)
{
    memset(p, 0, sizeof *p);
    p->fn = fn;
    p->ctx = ctx;
}

void lw_parse(lw_parser *p, uint8_t b)
{
    if (b >= 0xF8) {                          /* real-time: never interrupts anything */
        p->fn(p->ctx, &b, 1);
        return;
    }
    if (b == 0xF0) {
        p->buf[0] = b;
        p->n = 1;
        p->need = -1;
        p->overflow = false;
        p->running = 0;
        return;
    }
    if (b == 0xF7) {
        if (p->need == -1 && p->n > 0) {
            if (!p->overflow && p->n < LW_PARSE_MAX) {
                p->buf[p->n++] = b;
                p->fn(p->ctx, p->buf, p->n);
            }
        }
        p->n = 0;
        p->need = 0;
        return;
    }
    if (b & 0x80) {                           /* other status byte: ends any sysex */
        p->n = 0;
        p->need = data_len(b);
        p->running = (b < 0xF0) ? b : 0;
        p->buf[p->n++] = b;
        if (p->need == 0) {                   /* F6 tune request and friends */
            p->fn(p->ctx, p->buf, 1);
            p->n = 0;
        }
        return;
    }
    /* data byte */
    if (p->need == -1) {
        if (p->n < LW_PARSE_MAX - 1)
            p->buf[p->n++] = b;
        else
            p->overflow = true;
        return;
    }
    if (p->n == 0) {                          /* running status */
        if (!p->running)
            return;                           /* stray data byte */
        p->buf[0] = p->running;
        p->n = 1;
        p->need = data_len(p->running);
    }
    p->buf[p->n++] = b;
    if (p->n == p->need + 1) {
        p->fn(p->ctx, p->buf, p->n);
        p->n = 0;
    }
}
