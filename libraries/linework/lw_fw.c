#include "lw_fw.h"
#include "lw_proto.h"
#include <string.h>

#define XIP 0x10000000u          /* where flash appears in the RP2040's address space */
#define RAM_LO 0x20000000u
#define RAM_HI 0x20042000u

/* ---------------------------------------------------------------- the update record
 * 16-byte notes, appended: "LWFU", kind ('P' pending / 'I' installed), announced (FF until the
 * new firmware has announced itself, then 00), check, 00, size, CRC-32. The check covers every
 * byte except "announced", which is cleared later. */
#define REC 16u
#define RECS (FW_SECTOR / REC)

typedef struct { uint8_t kind, announced; uint32_t size, crc; } fw_note;

static uint32_t get32le(const uint8_t *p) { return p[0] | p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static void put32le(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }

static uint8_t note_check(const uint8_t *b)
{
    uint8_t c = 0xA5;
    for (int i = 0; i < 16; i++)
        if (i != 5 && i != 6)
            c = (uint8_t)((c << 1 | c >> 7) ^ b[i]);
    return c;
}

static bool blank(const uint8_t *b, int n)
{
    for (int i = 0; i < n; i++)
        if (b[i] != 0xFF)
            return false;
    return true;
}

/* the latest valid note and the index of the first free slot */
static bool last_note(const lw_flash *f, fw_note *out, uint32_t *at, uint32_t *free_slot)
{
    bool found = false;
    uint32_t i;
    uint8_t b[REC];
    for (i = 0; i < RECS; i++) {
        f->read(f->ctx, FW_REC_OFF + i * REC, b, REC);
        if (blank(b, REC))
            break;
        if (memcmp(b, "LWFU", 4) != 0 || b[7] != 0 || b[6] != note_check(b) || (b[4] != 'P' && b[4] != 'I'))
            continue;                            /* cut short by a power loss: skip */
        out->kind = b[4];
        out->announced = b[5];
        out->size = get32le(&b[8]);
        out->crc = get32le(&b[12]);
        if (at)
            *at = i;
        found = true;
    }
    if (free_slot)
        *free_slot = i;
    return found;
}

static void add_note(const lw_flash *f, uint8_t kind, uint32_t size, uint32_t crc, uint8_t announced)
{
    uint32_t slot;
    fw_note n;
    last_note(f, &n, NULL, &slot);
    if (slot >= RECS) {                          /* full (after many updates): start the log again */
        f->erase(f->ctx, FW_REC_OFF, FW_SECTOR);
        slot = 0;
    }
    uint8_t b[REC] = {'L', 'W', 'F', 'U', kind, announced, 0, 0};
    put32le(&b[8], size);
    put32le(&b[12], crc);
    b[6] = note_check(b);
    f->write(f->ctx, FW_REC_OFF + slot * REC, b, REC);
}

static uint32_t crc_of(const lw_flash *f, uint32_t off, uint32_t n)
{
    uint8_t buf[256];
    uint32_t crc = 0;
    while (n) {
        uint32_t k = n < sizeof buf ? n : sizeof buf;
        f->read(f->ctx, off, buf, k);
        crc = lw_crc32(crc, buf, k);
        off += k;
        n -= k;
    }
    return crc;
}

static uint32_t round_up(uint32_t v, uint32_t to) { return (v + to - 1) / to * to; }

/* ---------------------------------------------------------------- receiving */

int lw_fw_begin(lw_fw_rx *u, const lw_flash *f, uint32_t size, uint32_t crc)
{
    memset(u, 0, sizeof *u);
    if (size < FW_VECTORS + 8 || size > FW_APP_SIZE)
        return ST_RANGE;
    u->f = f;
    u->size = size;
    u->crc = crc;
    u->erase_at = FW_DL_OFF;
    u->erase_end = FW_DL_OFF + round_up(size, FW_SECTOR);
    u->active = true;
    return ST_OK;
}

bool lw_fw_erase_step(lw_fw_rx *u)
{
    if (!u->active)
        return false;
    if (u->erase_at < u->erase_end) {
        u->f->erase(u->f->ctx, u->erase_at, FW_SECTOR);
        u->erase_at += FW_SECTOR;
    }
    u->erased = u->erase_at >= u->erase_end;
    return u->erased;
}

int lw_fw_block(lw_fw_rx *u, uint32_t n, const uint8_t data[FW_BLOCK])
{
    if (!u->active)
        return ST_RANGE;
    if (!u->erased)
        return ST_BUSY;
    if (n * FW_BLOCK >= u->size)
        return ST_RANGE;
    /* a resent block writes the same bytes again, which flash allows (it only clears bits) */
    u->f->write(u->f->ctx, FW_DL_OFF + n * FW_BLOCK, data, FW_BLOCK);
    u->received++;
    return ST_OK;
}

int lw_fw_finish(lw_fw_rx *u)
{
    if (!u->active || !u->erased)
        return ST_FW_FAIL;
    u->active = false;
    if (crc_of(u->f, FW_DL_OFF, u->size) != u->crc)
        return ST_FW_FAIL;                       /* nothing is installed; the old firmware stays */
    add_note(u->f, 'P', u->size, u->crc, 0xFF);
    return ST_OK;
}

/* ---------------------------------------------------------------- at power-up */

static bool app_looks_runnable(const lw_flash *f)
{
    uint8_t v[8];
    f->read(f->ctx, FW_APP_OFF + FW_VECTORS, v, 8);
    uint32_t sp = get32le(v), reset = get32le(&v[4]);
    return sp > RAM_LO && sp <= RAM_HI && (reset & 1u)
        && reset > XIP + FW_APP_OFF && reset < XIP + FW_APP_OFF + FW_APP_SIZE;
}

/* copies the download over the firmware and checks the copy */
static bool install(const lw_flash *f, uint32_t size, uint32_t crc)
{
    uint32_t span = round_up(size, FW_SECTOR);
    for (uint32_t o = 0; o < span; o += FW_SECTOR)
        f->erase(f->ctx, FW_APP_OFF + o, FW_SECTOR);
    uint8_t buf[256];
    for (uint32_t o = 0; o < size; o += sizeof buf) {
        uint32_t k = size - o < sizeof buf ? size - o : sizeof buf;
        f->read(f->ctx, FW_DL_OFF + o, buf, k);
        f->write(f->ctx, FW_APP_OFF + o, buf, k);
    }
    return crc_of(f, FW_APP_OFF, size) == crc;
}

int lw_fw_boot(const lw_flash *f, bool recovery_held, int *did)
{
    if (did)
        *did = FW_DID_NOTHING;
    if (recovery_held)
        return FW_BOOT_RECOVERY;
    fw_note n;
    if (!last_note(f, &n, NULL, NULL))           /* never updated (loaded over USB or SWD) */
        return app_looks_runnable(f) ? FW_BOOT_APP : FW_BOOT_RECOVERY;
    if (n.kind == 'I' && crc_of(f, FW_APP_OFF, n.size) == n.crc)
        return FW_BOOT_APP;                      /* the usual case */
    /* a pending update, or a damaged firmware area: copy from the download if it's good */
    if (crc_of(f, FW_DL_OFF, n.size) == n.crc) {
        if (!install(f, n.size, n.crc))
            return FW_BOOT_RECOVERY;             /* the copy didn't check out: try again next time */
        add_note(f, 'I', n.size, n.crc, n.kind == 'P' ? 0xFF : 0x00);
        if (did)
            *did = n.kind == 'P' ? FW_DID_INSTALL : FW_DID_REPAIR;
        return FW_BOOT_APP;
    }
    /* the download is no good. A pending update hasn't touched the firmware area yet, so the
     * old firmware can still start; a damaged installed one can't be repaired from here. */
    if (n.kind == 'P')
        return app_looks_runnable(f) ? FW_BOOT_APP : FW_BOOT_RECOVERY;
    return FW_BOOT_RECOVERY;
}

bool lw_fw_take_announce(const lw_flash *f)
{
    fw_note n;
    uint32_t at;
    if (!last_note(f, &n, &at, NULL) || n.kind != 'I' || n.announced != 0xFF)
        return false;
    uint8_t zero = 0x00;
    f->write(f->ctx, FW_REC_OFF + at * REC + 5, &zero, 1);
    return true;
}

/* ---------------------------------------------------------------- 7-bit packing */

void lw_pack7(const uint8_t *in, int n, uint8_t *out)
{
    for (int g = 0; g < n; g += 7) {
        int k = n - g < 7 ? n - g : 7;
        uint8_t top = 0;
        for (int i = 0; i < k; i++)
            top |= (uint8_t)((in[g + i] >> 7) << i);
        *out++ = top;
        for (int i = 0; i < k; i++)
            *out++ = in[g + i] & 0x7F;
    }
}

void lw_unpack7(const uint8_t *in, int n_out, uint8_t *out)
{
    for (int g = 0; g < n_out; g += 7) {
        int k = n_out - g < 7 ? n_out - g : 7;
        uint8_t top = *in++;
        for (int i = 0; i < k; i++)
            out[g + i] = (uint8_t)((*in++ & 0x7F) | ((top >> i) & 1) << 7);
    }
}

uint8_t lw_fw_checksum(const uint8_t *packed, int n)
{
    unsigned s = 0;
    for (int i = 0; i < n; i++)
        s += packed[i];
    return (uint8_t)(s & 0x7F);
}
