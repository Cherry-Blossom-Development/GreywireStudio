#include "switchyard.h"
#include <string.h>

#define SL_MAGIC 0x4C53574Cu       /* "LWSL" */
#define HDR_OFF 0x0000u
#define TABLE_OFF 0x0100u
#define PARTS_OFF 0x1000u
#define PART_REC 72u
#define PART_OFF(i) (PARTS_OFF + (uint32_t)(i) * PART_REC)

/* timings (ms) */
#define T_SENSE 250                /* Active Sensing into the loop */
#define T_BROKEN 1000              /* no Active Sensing back for this long = loop broken */
#define T_BREAK_MEMORY 2500        /* break reports count as current for this long */
#define T_SCAN_POWERUP 500         /* let the pedals start before the first scan */
#define T_COUNT 300
#define T_IDENT 300
#define T_ASSIGN 200
#define T_SAVE 2000                /* save the current part this long after the last step */

static uint32_t now(switchyard *sy) { return sy->hal.now_ms(sy->hal.ctx); }

static void to_loop(switchyard *sy, const uint8_t *m, int n) { sy->hal.loop_send(sy->hal.ctx, m, n); }

static void to_app(switchyard *sy, const uint8_t *m, int n)
{
    if (sy->hal.app_connected(sy->hal.ctx))
        sy->hal.app_send(sy->hal.ctx, m, n);
}

static uint8_t next_seq(switchyard *sy)
{
    sy->seq = (sy->seq >= 0x3F) ? 0x01 : sy->seq + 1;
    return sy->seq;
}

static void ack(switchyard *sy, uint8_t qq, uint8_t cmd, uint8_t st)
{
    lw_msg m;
    lw_begin(&m, LW_ACK, qq);
    lw_put(&m, LW_NO_CHANNEL);     /* 7F: Switchyard itself answering */
    lw_put(&m, cmd);
    lw_put(&m, st);
    lw_end(&m);
    to_app(sy, m.b, m.n);
}

/* ================================================================ display */

static void show(switchyard *sy)
{
    char t[4] = {'-', '-', '-', '-'};
    uint8_t dp = 0x02;
    sy_part p;
    if (sy->have_setlist && sy_read_part(sy, sy->cur, &p)) {
        t[0] = '0' + (p.song / 10) % 10;
        t[1] = '0' + p.song % 10;
        if (p.part < 10) {
            t[2] = '0' + p.part;
            t[3] = ' ';
        } else {
            t[2] = '0' + (p.part / 10) % 10;
            t[3] = '0' + p.part % 10;
        }
    }
    sy->hal.display(sy->hal.ctx, t, dp);
}

/* ================================================================ setlist storage */

bool sy_read_part(switchyard *sy, int idx, sy_part *out)
{
    if (!sy->have_setlist || idx < 0 || idx >= sy->hdr.nparts)
        return false;
    uint8_t r[PART_REC];
    sy->hal.store_read(sy->hal.ctx, SY_SLOT_OFF(sy->slot) + PART_OFF(idx), r, PART_REC);
    if (r[0] != 0x00 || r[3] > SY_MAX_PAIRS)
        return false;
    out->song = r[1];
    out->part = r[2];
    out->npairs = r[3];
    memcpy(out->pair, &r[4], 2u * r[3]);
    return true;
}

static bool load_slot(switchyard *sy, int s, sy_slhdr *h)
{
    sy->hal.store_read(sy->hal.ctx, SY_SLOT_OFF(s) + HDR_OFF, h, sizeof *h);
    return h->magic == SL_MAGIC && h->nparts >= 1 && h->nparts <= SY_MAX_PARTS && h->np <= SY_MAX_TABLE;
}

static void activate(switchyard *sy, int s, const sy_slhdr *h)
{
    sy->slot = s;
    sy->hdr = *h;
    sy->have_setlist = true;
    sy->hal.store_read(sy->hal.ctx, SY_SLOT_OFF(s) + TABLE_OFF, sy->table, sizeof sy->table);
}

static void load_setlist(switchyard *sy)
{
    sy_slhdr a, b;
    bool va = load_slot(sy, 0, &a), vb = load_slot(sy, 1, &b);
    sy->have_setlist = false;
    if (va && (!vb || a.gen >= b.gen))
        activate(sy, 0, &a);
    else if (vb)
        activate(sy, 1, &b);
}

/* The state sector is a log of 4-byte entries: A5, part low, part high, A5 ^ low ^ high.
 * The last good entry is the current part. Erased entries read FF FF FF FF. */
static int load_position(switchyard *sy)
{
    int part = 0;
    uint8_t e[4];
    sy->state_off = 0;
    for (uint32_t off = 0; off < SY_STATE_SIZE; off += 4) {
        sy->hal.store_read(sy->hal.ctx, SY_STATE_OFF + off, e, 4);
        if (e[0] == 0xFF && e[1] == 0xFF && e[2] == 0xFF && e[3] == 0xFF)
            break;
        if (e[0] == 0xA5 && e[3] == (uint8_t)(0xA5 ^ e[1] ^ e[2]))
            part = e[1] | (e[2] << 8);
        sy->state_off = off + 4;
    }
    return part;
}

static void save_position(switchyard *sy)
{
    if (sy->state_off + 4 > SY_STATE_SIZE) {
        sy->hal.store_erase(sy->hal.ctx, SY_STATE_OFF, SY_STATE_SIZE);
        sy->state_off = 0;
    }
    uint8_t e[4] = {0xA5, (uint8_t)(sy->cur & 0xFF), (uint8_t)(sy->cur >> 8), 0};
    e[3] = 0xA5 ^ e[1] ^ e[2];
    sy->hal.store_write(sy->hal.ctx, SY_STATE_OFF + sy->state_off, e, 4);
    sy->state_off += 4;
    sy->save_pending = false;
}

/* ================================================================ stepping */

static sy_pedal *find_serial(switchyard *sy, const uint8_t *serial)
{
    for (int i = 0; i < sy->npedals; i++)
        if (memcmp(sy->pedal[i].serial, serial, LW_SERIAL_LEN) == 0)
            return &sy->pedal[i];
    return NULL;
}

static void send_part(switchyard *sy, int idx)
{
    sy_part p;
    if (!sy_read_part(sy, idx, &p))
        return;
    for (int i = 0; i < p.npairs; i++) {
        uint8_t ix = p.pair[i][0], pp = p.pair[i][1] & 0x7F;
        if (ix >= sy->hdr.np)
            continue;
        const sy_entry *e = &sy->table[ix];
        if (e->type == 0) {
            sy_pedal *pd = find_serial(sy, e->serial);
            if (!pd || pd->ch > 0x0F)
                continue;                     /* not found in the last scan: skipped */
            uint8_t m[2] = {(uint8_t)(0xC0 | pd->ch), pp};
            to_loop(sy, m, 2);
        } else {
            uint8_t m[2] = {(uint8_t)(0xC0 | (e->ch & 0x0F)), pp};
            sy->hal.other_send(sy->hal.ctx, m, 2);
        }
    }
}

static void position_report(switchyard *sy, uint8_t qq, uint8_t why)
{
    sy_part p;
    if (!sy_read_part(sy, sy->cur, &p))
        return;
    lw_msg m;
    lw_begin(&m, SY_POS, qq);
    lw_put14(&m, (unsigned)sy->cur);
    lw_put(&m, p.song);
    lw_put(&m, p.part);
    lw_put(&m, why);
    lw_end(&m);
    to_app(sy, m.b, m.n);
}

static void go_to(switchyard *sy, int idx, uint8_t qq, uint8_t why)
{
    sy->cur = idx;
    send_part(sy, idx);
    position_report(sy, qq, why);
    show(sy);
    sy->save_pending = true;
    sy->save_t = now(sy);
}

void sy_step(switchyard *sy, int dir)
{
    if (!sy->have_setlist)
        return;
    int n = sy->hdr.nparts;
    bool wrap = sy->hdr.flags & 1;
    int next = sy->cur + (dir > 0 ? 1 : -1);
    if (next >= n || next < 0) {
        if (!wrap) {
            position_report(sy, 0, dir > 0 ? WHY_UP_END : WHY_DOWN_START);
            return;
        }
        next = (next < 0) ? n - 1 : 0;
    }
    go_to(sy, next, 0, dir > 0 ? WHY_UP : WHY_DOWN);
}

/* ================================================================ scanning */

static void scan_start(switchyard *sy, bool for_app, uint8_t app_qq)
{
    if (for_app) {
        sy->scan_reply_app = true;
        sy->scan_app_qq = app_qq;
    }
    if (sy->scan != SCAN_IDLE && sy->scan != SCAN_WAIT)
        return;                               /* already running: it answers when done */
    sy->scan = SCAN_COUNT;
    sy->scan_qq = next_seq(sy);
    sy->scan_t = now(sy);
    lw_msg m;
    lw_begin(&m, LW_POSITION, sy->scan_qq);
    lw_put(&m, 0);
    lw_end(&m);
    to_loop(sy, m.b, m.n);
}

static void send_list(switchyard *sy, uint8_t qq)
{
    lw_msg m;
    lw_begin(&m, SY_LIST_COUNT, qq);
    lw_put(&m, (uint8_t)sy->npedals);
    lw_end(&m);
    to_app(sy, m.b, m.n);
    for (int i = 0; i < sy->npedals; i++) {
        const sy_pedal *p = &sy->pedal[i];
        lw_begin(&m, SY_LIST_ENTRY, qq);
        lw_put(&m, p->pos);
        lw_put(&m, p->ch);
        lw_put_n(&m, p->model, 2);
        lw_put_n(&m, p->fw, 3);
        lw_put(&m, p->spec);
        lw_put_n(&m, p->serial, LW_SERIAL_LEN);
        lw_end(&m);
        to_app(sy, m.b, m.n);
    }
}

static void plan_channels(switchyard *sy)
{
    bool used[16] = {false};
    for (int i = 0; i < sy->npedals; i++) {   /* keep existing channels, first come first kept */
        uint8_t c = sy->pedal[i].ch;
        if (c <= 0x0F && !used[c]) {
            used[c] = true;
            sy->want_ch[i] = c;
        } else {
            sy->want_ch[i] = 0xFF;
        }
    }
    for (int i = 0; i < sy->npedals; i++) {   /* new or clashing pedals: lowest free channel */
        if (sy->want_ch[i] != 0xFF)
            continue;
        for (int c = 0; c < 16; c++)
            if (!used[c]) {
                used[c] = true;
                sy->want_ch[i] = (uint8_t)c;
                break;
            }
    }
}

static void assign_next(switchyard *sy)
{
    while (sy->scan_assign_i < sy->npedals) {
        int i = sy->scan_assign_i;
        if (sy->want_ch[i] != 0xFF && sy->want_ch[i] != sy->pedal[i].ch) {
            sy->scan_qq = next_seq(sy);
            sy->scan_t = now(sy);
            lw_msg m;
            lw_begin(&m, LW_ASSIGN, sy->scan_qq);
            lw_put_n(&m, sy->pedal[i].serial, LW_SERIAL_LEN);
            lw_put(&m, sy->want_ch[i]);
            lw_end(&m);
            to_loop(sy, m.b, m.n);
            return;                           /* wait for its acknowledgement */
        }
        sy->scan_assign_i++;
    }
    /* scan finished */
    sy->scan = SCAN_IDLE;
    if (sy->scan_reply_app) {
        sy->scan_reply_app = false;
        send_list(sy, sy->scan_app_qq);
    }
    if (sy->scan_then_apply) {
        sy->scan_then_apply = false;
        if (sy->have_setlist)
            go_to(sy, sy->cur, 0, WHY_POWERUP);
    }
}

static int cmp_pos(const sy_pedal *a, const sy_pedal *b) { return (int)a->pos - (int)b->pos; }

static void ident_done(switchyard *sy)
{
    /* sort by position (insertion sort, at most 16) */
    for (int i = 1; i < sy->npedals; i++)
        for (int j = i; j > 0 && cmp_pos(&sy->pedal[j - 1], &sy->pedal[j]) > 0; j--) {
            sy_pedal t = sy->pedal[j];
            sy->pedal[j] = sy->pedal[j - 1];
            sy->pedal[j - 1] = t;
        }
    plan_channels(sy);
    sy->scan = SCAN_ASSIGN;
    sy->scan_assign_i = 0;
    assign_next(sy);
}

static void scan_poll(switchyard *sy)
{
    uint32_t t = now(sy) - sy->scan_t;
    switch (sy->scan) {
    case SCAN_WAIT:
        if (t >= T_SCAN_POWERUP)
            scan_start(sy, false, 0);
        break;
    case SCAN_COUNT:
        if (t >= T_COUNT) {                   /* nothing came back: the loop is broken. Keep the last table */
            sy->scan = SCAN_IDLE;
            sy->scan_assign_i = (uint8_t)sy->npedals;
            assign_next(sy);
        }
        break;
    case SCAN_IDENT:
        if (t >= T_IDENT + 20u * sy->scan_count)
            ident_done(sy);
        break;
    case SCAN_ASSIGN:
        if (t >= T_ASSIGN) {                  /* no acknowledgement: move on */
            sy->scan_assign_i++;
            assign_next(sy);
        }
        break;
    default:
        break;
    }
}

/* Linework or identity messages coming back around the loop that belong to a scan */
static void scan_rx(switchyard *sy, const uint8_t *m, int n)
{
    if (sy->scan == SCAN_COUNT && lw_is_linework(m, n) && LW_CMD(m) == LW_POSITION && LW_QQ(m) == sy->scan_qq
        && n == LW_HDR_LEN + 4) {
        sy->scan_count = LW_BODY(m)[0];
        if (sy->scan_count > SY_MAX_PEDALS)
            sy->scan_count = SY_MAX_PEDALS;
        sy->npedals = 0;
        sy->have_ident = false;
        sy->scan = SCAN_IDENT;
        sy->scan_t = now(sy);
        static const uint8_t req[] = {0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7};
        to_loop(sy, req, sizeof req);
        if (sy->scan_count == 0)
            ident_done(sy);
        return;
    }
    if (sy->scan == SCAN_IDENT) {
        /* identity reply: F0 7E dd 06 02 <7D 4C 57> m0 m1 aa bb cc ss F7 */
        if (n == 5 + (LW_HDR_LEN - 1) + 7 && m[0] == 0xF0 && m[1] == 0x7E && m[3] == 0x06 && m[4] == 0x02
            && memcmp(&m[5], &LW_HEADER[1], LW_HDR_LEN - 1) == 0) {
            const uint8_t *p = &m[5 + LW_HDR_LEN - 1];
            sy->ident[0] = m[2];
            memcpy(&sy->ident[1], p, 6);
            sy->have_ident = true;
            return;
        }
        /* pedal info: <header> 02 qq pp dd s0..s9 F7 */
        if (lw_is_linework(m, n) && LW_CMD(m) == LW_PEDAL_INFO && n == LW_HDR_LEN + 2 + 2 + LW_SERIAL_LEN + 1
            && sy->npedals < SY_MAX_PEDALS) {
            const uint8_t *b = LW_BODY(m);
            sy_pedal *p = &sy->pedal[sy->npedals++];
            memset(p, 0, sizeof *p);
            p->pos = b[0];
            p->ch = b[1];
            memcpy(p->serial, &b[2], LW_SERIAL_LEN);
            if (sy->have_ident && sy->ident[0] == p->ch) {
                memcpy(p->model, &sy->ident[1], 2);
                memcpy(p->fw, &sy->ident[3], 3);
                p->spec = sy->ident[6];
            }
            sy->have_ident = false;
            if (sy->npedals >= sy->scan_count)
                ident_done(sy);
        }
        return;
    }
    if (sy->scan == SCAN_ASSIGN && lw_is_linework(m, n) && LW_CMD(m) == LW_ACK && LW_QQ(m) == sy->scan_qq
        && n >= LW_HDR_LEN + 6 && LW_BODY(m)[1] == LW_ASSIGN) {
        int i = sy->scan_assign_i;
        if (LW_BODY(m)[2] == ST_OK && i < sy->npedals)
            sy->pedal[i].ch = sy->want_ch[i];
        sy->scan_assign_i++;
        assign_next(sy);
    }
}

/* ================================================================ loop health */

static uint8_t where_to_look(switchyard *sy)
{
    uint32_t t = now(sy);
    for (int p = 1; p <= SY_MAX_PEDALS; p++)
        if (sy->break_seen[p] && t - sy->break_seen[p] < T_BREAK_MEMORY)
            return (uint8_t)(p == 1 ? 0x00 : p - 1);
    return 0x7F;
}

static void loop_status(switchyard *sy, uint8_t qq)
{
    lw_msg m;
    lw_begin(&m, SY_STATUS, qq);
    lw_put(&m, sy->broken ? 1 : 0);
    lw_put(&m, (uint8_t)sy->npedals);
    lw_put(&m, sy->broken ? sy->last_bb : 0x7F);
    lw_end(&m);
    to_app(sy, m.b, m.n);
}

static void health_poll(switchyard *sy)
{
    uint32_t t = now(sy);
    if (t - sy->fe_sent >= T_SENSE) {
        static const uint8_t fe = MIDI_ACTIVE_SENSING;
        to_loop(sy, &fe, 1);
        sy->fe_sent = t;
    }
    if (!sy->broken && t - sy->fe_heard > T_BROKEN) {
        sy->broken = true;
        sy->last_bb = where_to_look(sy);
        loop_status(sy, 0);
    } else if (sy->broken) {
        uint8_t bb = where_to_look(sy);
        if (bb != sy->last_bb) {
            sy->last_bb = bb;
            loop_status(sy, 0);
        }
    }
}

static void heard_sensing(switchyard *sy)
{
    sy->fe_heard = now(sy);
    if (sy->broken) {                         /* repaired */
        sy->broken = false;
        memset(sy->break_seen, 0, sizeof sy->break_seen);
        loop_status(sy, 0);
        scan_start(sy, false, 0);
    }
}

/* ================================================================ setlist upload */

static void upload_fail(switchyard *sy, uint8_t qq, uint8_t cmd, uint8_t st)
{
    sy->up_failed = true;
    ack(sy, qq, cmd, st);
}

static void sl_begin(switchyard *sy, uint8_t qq, const uint8_t *b, int len)
{
    if (len != 5 + 1 + 1 + 2) {
        ack(sy, qq, SY_SL_BEGIN, ST_RANGE);
        return;
    }
    unsigned np = b[6], nparts = lw_get14(&b[7]);
    if (np < 1 || np > SY_MAX_TABLE || nparts < 1 || nparts > SY_MAX_PARTS) {
        ack(sy, qq, SY_SL_BEGIN, ST_RANGE);
        return;
    }
    memset(&sy->up_hdr, 0, sizeof sy->up_hdr);
    sy->up_hdr.id = lw_get32(b);
    sy->up_hdr.flags = b[5];
    sy->up_hdr.np = (uint8_t)np;
    sy->up_hdr.nparts = (uint16_t)nparts;
    sy->up_hdr.gen = (sy->have_setlist ? sy->hdr.gen : 0) + 1;
    sy->up_slot = sy->have_setlist ? 1 - sy->slot : 0;
    sy->up_crc = 0;
    sy->up_pedals_got = 0;
    memset(sy->up_parts_got, 0, sizeof sy->up_parts_got);
    sy->up_nparts_got = 0;
    sy->up_failed = false;
    sy->up_active = true;
    sy->up_erasing = true;
    sy->up_erase_i = 0;
    sy->up_qq = qq;                           /* acknowledged once the spare copy is erased */
}

static void upload_poll(switchyard *sy)
{
    if (!sy->up_erasing)
        return;
    /* one 64 KB block per poll, so USB keeps running between them */
    sy->hal.store_erase(sy->hal.ctx, SY_SLOT_OFF(sy->up_slot) + (uint32_t)sy->up_erase_i * SY_ERASE_BLOCK, SY_ERASE_BLOCK);
    if (++sy->up_erase_i * SY_ERASE_BLOCK >= SY_SLOT_SIZE) {
        sy->up_erasing = false;
        ack(sy, sy->up_qq, SY_SL_BEGIN, ST_OK);
    }
}

static void sl_pedal(switchyard *sy, uint8_t qq, const uint8_t *b, int len)
{
    if (!sy->up_active || sy->up_erasing) {
        ack(sy, qq, SY_SL_PEDAL, sy->up_erasing ? ST_BUSY : ST_NO_SETLIST);
        return;
    }
    sy->up_crc = lw_crc32(sy->up_crc, b, (size_t)len);
    sy_entry e;
    memset(&e, 0, sizeof e);
    uint8_t ix = b[0];
    if (len >= 2 && b[1] == 0x00 && len == 2 + LW_SERIAL_LEN) {
        e.type = 0;
        e.ch = LW_NO_CHANNEL;
        memcpy(e.serial, &b[2], LW_SERIAL_LEN);
    } else if (len == 3 && b[1] == 0x01 && b[2] <= 0x0F) {
        e.type = 1;
        e.ch = b[2];
    } else {
        upload_fail(sy, qq, SY_SL_PEDAL, ST_RANGE);
        return;
    }
    if (ix >= sy->up_hdr.np || (sy->up_pedals_got & (1u << ix))) {
        upload_fail(sy, qq, SY_SL_PEDAL, ST_RANGE);
        return;
    }
    sy->up_pedals_got |= 1u << ix;
    sy->hal.store_write(sy->hal.ctx, SY_SLOT_OFF(sy->up_slot) + TABLE_OFF + ix * sizeof(sy_entry), &e, sizeof e);
}

static void sl_part(switchyard *sy, uint8_t qq, const uint8_t *b, int len)
{
    if (!sy->up_active || sy->up_erasing) {
        ack(sy, qq, SY_SL_PART, sy->up_erasing ? ST_BUSY : ST_NO_SETLIST);
        return;
    }
    sy->up_crc = lw_crc32(sy->up_crc, b, (size_t)len);
    if (len < 4 || (len - 4) % 2 != 0 || (len - 4) / 2 > SY_MAX_PAIRS) {
        upload_fail(sy, qq, SY_SL_PART, ST_RANGE);
        return;
    }
    unsigned idx = lw_get14(b);
    uint8_t song = b[2], part = b[3];
    int npairs = (len - 4) / 2;
    if (idx >= sy->up_hdr.nparts || (sy->up_parts_got[idx / 8] & (1u << (idx % 8)))
        || song < 1 || song > 99 || part < 1 || part > 99) {
        upload_fail(sy, qq, SY_SL_PART, ST_RANGE);
        return;
    }
    uint8_t r[PART_REC];
    memset(r, 0xFF, sizeof r);
    r[0] = 0x00;
    r[1] = song;
    r[2] = part;
    r[3] = (uint8_t)npairs;
    for (int i = 0; i < npairs; i++) {
        if (b[4 + 2 * i] >= sy->up_hdr.np) {
            upload_fail(sy, qq, SY_SL_PART, ST_RANGE);
            return;
        }
        r[4 + 2 * i] = b[4 + 2 * i];
        r[5 + 2 * i] = b[5 + 2 * i];
    }
    sy->up_parts_got[idx / 8] |= (uint8_t)(1u << (idx % 8));
    sy->up_nparts_got++;
    sy->hal.store_write(sy->hal.ctx, SY_SLOT_OFF(sy->up_slot) + PART_OFF(idx), r, PART_REC);
}

static void sl_finish(switchyard *sy, uint8_t qq, const uint8_t *b, int len)
{
    if (!sy->up_active) {
        ack(sy, qq, SY_SL_FINISH, ST_NO_SETLIST);
        return;
    }
    sy->up_active = false;
    uint32_t want_pedals = (sy->up_hdr.np >= 32) ? 0xFFFFFFFFu : ((1u << sy->up_hdr.np) - 1u);
    if (len != 5 || sy->up_erasing || sy->up_failed || lw_get32(b) != sy->up_crc
        || sy->up_pedals_got != want_pedals || sy->up_nparts_got != sy->up_hdr.nparts) {
        ack(sy, qq, SY_SL_FINISH, ST_SETLIST_FAIL);   /* the old setlist stays */
        return;
    }
    sy->up_hdr.crc = sy->up_crc;
    sy->up_hdr.magic = SL_MAGIC;
    sy->hal.store_write(sy->hal.ctx, SY_SLOT_OFF(sy->up_slot) + HDR_OFF, &sy->up_hdr, sizeof sy->up_hdr);
    activate(sy, sy->up_slot, &sy->up_hdr);
    if (sy->cur >= sy->hdr.nparts)             /* same part number if it still exists, else the first */
        sy->cur = 0;
    sy->save_pending = true;
    sy->save_t = now(sy);
    show(sy);                                  /* sends nothing: the app sends Jump to apply */
    ack(sy, qq, SY_SL_FINISH, ST_OK);
}

static void sl_info(switchyard *sy, uint8_t qq)
{
    if (!sy->have_setlist) {
        ack(sy, qq, SY_SL_INFO_REQ, ST_NO_SETLIST);
        return;
    }
    lw_msg m;
    lw_begin(&m, SY_SL_INFO, qq);
    lw_put32(&m, sy->hdr.id);
    lw_put32(&m, sy->hdr.crc);
    lw_put(&m, sy->hdr.flags);
    lw_put(&m, sy->hdr.np);
    lw_put14(&m, sy->hdr.nparts);
    lw_end(&m);
    to_app(sy, m.b, m.n);
}

/* ================================================================ app messages */

static void info(switchyard *sy, uint8_t qq)
{
    lw_msg m;
    lw_begin(&m, SY_INFO, qq);
    lw_put(&m, SY_FW_MAJOR);
    lw_put(&m, SY_FW_MINOR);
    lw_put(&m, SY_FW_PATCH);
    lw_put(&m, LW_SPEC_VERSION);
    lw_put_n(&m, sy->hal.serial, LW_SERIAL_LEN);
    lw_put(&m, sy->hal.power_9v(sy->hal.ctx) ? 0x01 : 0x00);   /* bit 1 (charging) comes with stage 2 */
    lw_end(&m);
    to_app(sy, m.b, m.n);
}

static void other_send(switchyard *sy, uint8_t qq, const uint8_t *b, int len)
{
    if (len != 4 || b[1] > 0x0F || b[0] > 1) {
        ack(sy, qq, SY_OTHER_SEND, ST_RANGE);
        return;
    }
    uint8_t m[3];
    if (b[0] == 0) {
        m[0] = 0xC0 | b[1];
        m[1] = b[2];
        sy->hal.other_send(sy->hal.ctx, m, 2);
    } else {
        m[0] = 0xB0 | b[1];
        m[1] = b[2];
        m[2] = b[3];
        sy->hal.other_send(sy->hal.ctx, m, 3);
    }
    ack(sy, qq, SY_OTHER_SEND, ST_OK);
}

static void jump(switchyard *sy, uint8_t qq, const uint8_t *b, int len)
{
    if (!sy->have_setlist) {
        ack(sy, qq, SY_JUMP, ST_NO_SETLIST);
        return;
    }
    unsigned idx = (len == 2) ? lw_get14(b) : 0xFFFF;
    if (idx >= sy->hdr.nparts) {
        ack(sy, qq, SY_JUMP, ST_RANGE);
        return;
    }
    ack(sy, qq, SY_JUMP, ST_OK);
    go_to(sy, (int)idx, qq, WHY_JUMP);
}

static void switchyard_cmd(switchyard *sy, const uint8_t *m, int n)
{
    uint8_t cmd = LW_CMD(m), qq = LW_QQ(m);
    const uint8_t *b = LW_BODY(m);
    int len = n - (LW_HDR_LEN + 2) - 1;       /* bytes between qq and F7 */
    switch (cmd) {
    case SY_INFO_REQ: info(sy, qq); break;
    case SY_SCAN: scan_start(sy, true, qq); break;
    case SY_LIST_REQ: send_list(sy, qq); break;
    case SY_STATUS_REQ:
        if (sy->broken)
            sy->last_bb = where_to_look(sy);
        loop_status(sy, qq);
        break;
    case SY_OTHER_SEND: other_send(sy, qq, b, len); break;
    case SY_SL_BEGIN: sl_begin(sy, qq, b, len); break;
    case SY_SL_PEDAL: sl_pedal(sy, qq, b, len); break;
    case SY_SL_PART: sl_part(sy, qq, b, len); break;
    case SY_SL_FINISH: sl_finish(sy, qq, b, len); break;
    case SY_SL_INFO_REQ: sl_info(sy, qq); break;
    case SY_POS_REQ:
        if (!sy->have_setlist)
            ack(sy, qq, SY_POS_REQ, ST_NO_SETLIST);
        else
            position_report(sy, qq, WHY_GET);
        break;
    case SY_JUMP: jump(sy, qq, b, len); break;
    default: ack(sy, qq, cmd, ST_UNKNOWN); break;
    }
}

static bool is_identity_request(const uint8_t *m, int n)
{
    return n == 6 && m[0] == 0xF0 && m[1] == 0x7E && m[3] == 0x06 && m[4] == 0x01;
}

static void on_app_msg(void *ctx, const uint8_t *m, int n)
{
    switchyard *sy = ctx;
    if (m[0] >= 0xF8)
        return;                               /* real-time bytes from the app aren't passed on */
    if (lw_is_linework(m, n) && LW_CMD(m) >= 0x40 && LW_CMD(m) <= 0x5F) {
        switchyard_cmd(sy, m, n);
        return;
    }
    if (is_identity_request(m, n)) {          /* answered by Switchyard itself */
        uint8_t r[16] = {0xF0, 0x7E, 0x7F, 0x06, 0x02};
        int k = 5;
        memcpy(&r[k], &LW_HEADER[1], LW_HDR_LEN - 1);
        k += LW_HDR_LEN - 1;
        r[k++] = SY_MODEL_0;
        r[k++] = SY_MODEL_1;
        r[k++] = SY_FW_MAJOR;
        r[k++] = SY_FW_MINOR;
        r[k++] = SY_FW_PATCH;
        r[k++] = LW_SPEC_VERSION;
        r[k++] = 0xF7;
        to_app(sy, r, k);
        return;
    }
    to_loop(sy, m, n);                        /* pedal commands, program and control changes */
}

static void on_loop_msg(void *ctx, const uint8_t *m, int n)
{
    switchyard *sy = ctx;
    if (n == 1 && m[0] == MIDI_ACTIVE_SENSING) {
        heard_sensing(sy);                    /* the one message the app doesn't see */
        return;
    }
    if (lw_is_linework(m, n) && LW_CMD(m) == LW_LOOP_BREAK && n == LW_HDR_LEN + 4) {
        uint8_t p = LW_BODY(m)[0];
        if (p >= 1 && p <= SY_MAX_PEDALS)
            sy->break_seen[p] = now(sy) | 1u;
    }
    scan_rx(sy, m, n);
    to_app(sy, m, n);                         /* everything else from the loop goes to the app */
}

/* ================================================================ main entry points */

void sy_init(switchyard *sy, const sy_hal *hal)
{
    memset(sy, 0, sizeof *sy);
    sy->hal = *hal;
    lw_parser_init(&sy->loop_rx, on_loop_msg, sy);
    lw_parser_init(&sy->app_rx, on_app_msg, sy);
    uint32_t t = now(sy);
    sy->fe_heard = t;
    sy->fe_sent = t - T_SENSE;
    load_setlist(sy);
    sy->cur = load_position(sy);
    if (!sy->have_setlist || sy->cur >= sy->hdr.nparts)
        sy->cur = 0;
    show(sy);
    /* power-up: scan, then return to the saved part and send its program changes */
    sy->scan = SCAN_WAIT;
    sy->scan_t = t;
    sy->scan_then_apply = true;
}

void sy_loop_byte(switchyard *sy, uint8_t b) { lw_parse(&sy->loop_rx, b); }

void sy_app_byte(switchyard *sy, uint8_t b) { lw_parse(&sy->app_rx, b); }

void sy_poll(switchyard *sy)
{
    scan_poll(sy);
    health_poll(sy);
    upload_poll(sy);
    if (sy->save_pending && now(sy) - sy->save_t >= T_SAVE)
        save_position(sy);
}
