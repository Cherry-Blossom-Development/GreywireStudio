#include "pd_store.h"
#include <string.h>

static const uint8_t MAGIC[4] = {'L', 'W', 'P', 'S'};
#define VERSION 1

static uint8_t rec_check(const uint8_t *b)
{
    uint8_t c = 0x5A;
    for (int i = 0; i < 10; i++)
        c = (uint8_t)((c << 1 | c >> 7) ^ b[i]);   /* rotate and mix, so swapped bytes are caught */
    return c;
}

static uint32_t rec_off(int bank, uint32_t i) { return (uint32_t)bank * PS_SECTOR + PS_HEADER + i * PS_REC; }

static bool read_header(const ps_log *s, int bank, uint32_t *gen)
{
    uint8_t h[PS_HEADER];
    s->f->read(s->f->ctx, (uint32_t)bank * PS_SECTOR, h, sizeof h);
    if (memcmp(h, MAGIC, 4) != 0 || h[8] != VERSION)
        return false;
    uint32_t g = (uint32_t)h[4] | (uint32_t)h[5] << 8 | (uint32_t)h[6] << 16 | (uint32_t)h[7] << 24;
    if (h[9] != (uint8_t)~(h[4] ^ h[5] ^ h[6] ^ h[7] ^ h[8]) || g == 0xFFFFFFFFu)
        return false;
    *gen = g;
    return true;
}

static void write_header(ps_log *s, int bank, uint32_t gen)
{
    uint8_t h[PS_HEADER];
    memset(h, 0xFF, sizeof h);
    memcpy(h, MAGIC, 4);
    h[4] = (uint8_t)gen;
    h[5] = (uint8_t)(gen >> 8);
    h[6] = (uint8_t)(gen >> 16);
    h[7] = (uint8_t)(gen >> 24);
    h[8] = VERSION;
    h[9] = (uint8_t)~(h[4] ^ h[5] ^ h[6] ^ h[7] ^ h[8]);
    s->f->write(s->f->ctx, (uint32_t)bank * PS_SECTOR, h, sizeof h);
}

static void encode(const ps_rec *r, uint8_t b[PS_REC])
{
    b[0] = r->type;
    b[1] = r->slot;
    memcpy(&b[2], r->v, PS_VALUES);
    b[10] = rec_check(b);
    b[11] = 0x00;
}

static bool blank(const uint8_t *b, int n)
{
    for (int i = 0; i < n; i++)
        if (b[i] != 0xFF)
            return false;
    return true;
}

bool ps_open(ps_log *s, const lw_flash *f, ps_fn fn, void *ctx)
{
    memset(s, 0, sizeof *s);
    s->f = f;
    uint32_t g[2];
    bool ok[2] = {read_header(s, 0, &g[0]), read_header(s, 1, &g[1])};
    if (!ok[0] && !ok[1]) {                       /* blank (or never formatted): start fresh */
        f->erase(f->ctx, 0, PS_SECTOR);
        write_header(s, 0, 1);
        s->bank = 0;
        s->gen = 1;
        s->spare_dirty = true;                    /* sector 1 may hold anything */
        return false;
    }
    s->bank = (ok[0] && (!ok[1] || g[0] > g[1])) ? 0 : 1;
    s->gen = g[s->bank];
    int other = 1 - s->bank;
    uint8_t b[PS_REC];
    if (ok[other]) {
        s->spare_dirty = true;                    /* an older copy: erase before reuse */
    } else {
        s->spare_dirty = false;                   /* blank already? check its first bytes */
        uint8_t h[PS_HEADER];
        f->read(f->ctx, (uint32_t)other * PS_SECTOR, h, sizeof h);
        f->read(f->ctx, rec_off(other, 0), b, sizeof b);
        s->spare_dirty = !blank(h, sizeof h) || !blank(b, sizeof b);
    }
    for (uint32_t i = 0; i < PS_RECORDS; i++) {
        f->read(f->ctx, rec_off(s->bank, i), b, sizeof b);
        if (blank(b, sizeof b))
            break;
        s->next = i + 1;                          /* even a damaged record uses up its place */
        if (b[11] != 0x00 || b[10] != rec_check(b) || (b[0] != PS_STATE && b[0] != PS_PRESET))
            continue;
        ps_rec r = {.type = b[0], .slot = b[1]};
        memcpy(r.v, &b[2], PS_VALUES);
        fn(ctx, &r);
    }
    return true;
}

bool ps_append(ps_log *s, const ps_rec *r)
{
    if (s->next >= PS_RECORDS)
        return false;
    uint8_t b[PS_REC];
    encode(r, b);
    s->f->write(s->f->ctx, rec_off(s->bank, s->next), b, sizeof b);
    s->next++;
    return true;
}

void ps_erase_spare(ps_log *s)
{
    if (!s->spare_dirty)
        return;
    s->f->erase(s->f->ctx, (uint32_t)(1 - s->bank) * PS_SECTOR, PS_SECTOR);
    s->spare_dirty = false;
}

void ps_compact(ps_log *s, const ps_rec *recs, int n)
{
    ps_erase_spare(s);                            /* usually done already, at a quiet moment */
    int to = 1 - s->bank;
    if (n > (int)PS_RECORDS)
        n = (int)PS_RECORDS;
    uint8_t buf[PS_REC * 16];                     /* write in runs of 16 records */
    for (int i = 0; i < n; i += 16) {
        int k = n - i < 16 ? n - i : 16;
        for (int j = 0; j < k; j++)
            encode(&recs[i + j], &buf[j * PS_REC]);
        s->f->write(s->f->ctx, rec_off(to, (uint32_t)i), buf, (uint32_t)k * PS_REC);
    }
    write_header(s, to, s->gen + 1);              /* last: only now does the new copy count */
    s->bank = to;
    s->gen++;
    s->next = (uint32_t)n;
    s->spare_dirty = true;                        /* the old copy */
}
