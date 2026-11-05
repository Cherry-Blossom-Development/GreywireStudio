/* A small log in flash for a pedal's settings: its channel, current sound and presets.
 *
 * Flash (NOR) can only be erased a 4 KB sector at a time, and erasing stalls the chip for
 * about 45 ms. So instead of rewriting the settings in place, each change is added to the end
 * of a log as one 12-byte record, and only when the log is full is it compacted: the latest
 * value of everything is written into the other sector (the "spare"), which then becomes the
 * active one. The spare is erased ahead of time, at a quiet moment on the MIDI loop.
 *
 * Two sectors (8 KB) in all:
 *   sector header (16 bytes, written last, so a half-written compaction is never used):
 *     "LWPS", generation (4 bytes), version, check, then FF padding
 *   records from byte 16 on, 12 bytes each:
 *     type, slot, 8 values, check, 00
 * At power-up the valid sector with the highest generation is replayed record by record. A
 * record cut short by a power loss fails its check and is skipped. */
#ifndef PD_STORE_H
#define PD_STORE_H

#include <stdint.h>
#include <stdbool.h>

#define PS_SECTOR 4096u
#define PS_SIZE (2u * PS_SECTOR)          /* the area the pedal needs in flash */
#define PS_HEADER 16u
#define PS_REC 12u
#define PS_RECORDS ((PS_SECTOR - PS_HEADER) / PS_REC)   /* 340 per sector */
#define PS_VALUES 8

/* Flash access, with offsets counted from the start of the pedal's area */
typedef struct {
    void *ctx;
    void (*read)(void *ctx, uint32_t off, void *buf, uint32_t n);
    void (*write)(void *ctx, uint32_t off, const void *buf, uint32_t n);   /* may only clear bits */
    void (*erase)(void *ctx, uint32_t off, uint32_t n);                    /* whole 4 KB sectors */
} lw_flash;

enum { PS_STATE = 'S', PS_PRESET = 'P' };   /* record types: S slot = channel, P slot = preset number */

typedef struct {
    uint8_t type, slot, v[PS_VALUES];
} ps_rec;

typedef struct {
    const lw_flash *f;
    int bank;            /* active sector, 0 or 1 */
    uint32_t gen;
    uint32_t next;       /* records used in the active sector */
    bool spare_dirty;    /* the spare sector still needs erasing */
} ps_log;

typedef void (*ps_fn)(void *ctx, const ps_rec *r);

/* Finds the active sector and replays its records in order. On a blank or unreadable store it
 * starts a fresh one and returns false. */
bool ps_open(ps_log *s, const lw_flash *f, ps_fn fn, void *ctx);
static inline uint32_t ps_free(const ps_log *s) { return PS_RECORDS - s->next; }
bool ps_append(ps_log *s, const ps_rec *r);                 /* false if the sector is full */
void ps_compact(ps_log *s, const ps_rec *recs, int n);     /* everything, into the spare sector */
void ps_erase_spare(ps_log *s);                             /* call at a quiet moment */

#endif
