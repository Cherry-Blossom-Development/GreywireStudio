/* Switchyard's logic, independent of the hardware.
 *
 * Everything the pedal does as a Linework controller lives here: scanning the loop, the
 * setlist store, stepping through parts, finding a break in the loop, and routing messages
 * between the phone app (USB) and the loop. The hardware layer (pico/) or the PC simulation
 * (test/) supplies the hal functions and calls sy_poll() often. */
#ifndef SWITCHYARD_H
#define SWITCHYARD_H

#include "lw_proto.h"

#define SY_FW_MAJOR 0
#define SY_FW_MINOR 1
#define SY_FW_PATCH 0
#define SY_MODEL_0 0x02            /* Switchyard's model number: 02 00 */
#define SY_MODEL_1 0x00

#define SY_MAX_PEDALS 16           /* on the loop, one per channel */
#define SY_MAX_TABLE 32            /* pedal table entries in a setlist */
#define SY_MAX_PARTS 4000
#define SY_MAX_PAIRS 32            /* pedal-and-preset pairs in one part */

/* Storage map, as offsets into the region the hal gives Switchyard. Writes only clear bits
 * (NOR flash), erases are in 64 KB blocks (4 KB for the state sector). */
#define SY_SLOT_SIZE 0x50000u      /* 320 KB per setlist copy */
#define SY_SLOT_OFF(s) ((uint32_t)(s) * SY_SLOT_SIZE)
#define SY_STATE_OFF (2u * SY_SLOT_SIZE)
#define SY_STATE_SIZE 0x1000u
#define SY_STORE_SIZE (SY_STATE_OFF + SY_STATE_SIZE)
#define SY_ERASE_BLOCK 0x10000u

typedef struct {
    void *ctx;
    uint32_t (*now_ms)(void *ctx);
    void (*loop_send)(void *ctx, const uint8_t *m, int n);   /* MIDI loop out (J1) */
    void (*other_send)(void *ctx, const uint8_t *m, int n);  /* MIDI out for other brands (J3) */
    void (*app_send)(void *ctx, const uint8_t *m, int n);    /* USB MIDI to the phone */
    bool (*app_connected)(void *ctx);
    bool (*power_9v)(void *ctx);                              /* SENSE_9V */
    void (*store_read)(void *ctx, uint32_t off, void *buf, uint32_t n);
    void (*store_write)(void *ctx, uint32_t off, const void *buf, uint32_t n);
    void (*store_erase)(void *ctx, uint32_t off, uint32_t n);
    /* 4 characters ('0'-'9', '-', ' ') and a decimal point mask (bit 0 = digit 1) */
    void (*display)(void *ctx, const char text[4], uint8_t dp);
    uint8_t serial[LW_SERIAL_LEN];                            /* from the RP2040's flash ID */
} sy_hal;

typedef struct {
    uint8_t pos, ch, model[2], fw[3], spec;
    uint8_t serial[LW_SERIAL_LEN];
} sy_pedal;

typedef struct {
    uint8_t type;                  /* 0 = Linework (by serial), 1 = other brand (by channel) */
    uint8_t ch;
    uint8_t serial[LW_SERIAL_LEN];
} sy_entry;

typedef struct {                   /* setlist header, as stored */
    uint32_t magic, gen, id, crc;
    uint16_t nparts;
    uint8_t np, flags;
} sy_slhdr;

typedef struct {
    uint8_t song, part, npairs;
    uint8_t pair[SY_MAX_PAIRS][2]; /* table index, preset slot */
} sy_part;

typedef enum { SCAN_IDLE, SCAN_WAIT, SCAN_COUNT, SCAN_IDENT, SCAN_ASSIGN } sy_scan_state;

typedef struct {
    sy_hal hal;
    lw_parser loop_rx, app_rx;
    uint8_t seq;                   /* Switchyard's own sequence numbers, 01-3F */

    /* last scan */
    sy_pedal pedal[SY_MAX_PEDALS];
    int npedals;
    sy_scan_state scan;
    uint32_t scan_t;
    uint8_t scan_qq, scan_count, scan_assign_i;
    bool scan_reply_app;           /* answer the app's Scan loop when done */
    uint8_t scan_app_qq;
    bool scan_then_apply;          /* power-up: return to the saved part when done */
    bool have_ident;
    uint8_t ident[8];              /* pending identity reply: dd m0 m1 aa bb cc ss */
    uint8_t want_ch[SY_MAX_PEDALS];

    /* loop health */
    uint32_t fe_sent, fe_heard;
    bool broken;
    uint8_t last_bb;
    uint32_t break_seen[SY_MAX_PEDALS + 1];

    /* active setlist */
    bool have_setlist;
    int slot;
    sy_slhdr hdr;
    sy_entry table[SY_MAX_TABLE];
    int cur;                       /* current part */
    bool save_pending;
    uint32_t save_t;
    uint32_t state_off;            /* next free entry in the state sector */

    /* setlist upload */
    bool up_active, up_erasing, up_failed;
    int up_slot, up_erase_i;
    uint8_t up_qq;
    sy_slhdr up_hdr;
    uint32_t up_crc;
    uint32_t up_pedals_got;
    uint8_t up_parts_got[(SY_MAX_PARTS + 7) / 8];
    int up_nparts_got;
} switchyard;

void sy_init(switchyard *sy, const sy_hal *hal);
void sy_poll(switchyard *sy);                       /* call often (every ms or faster) */
void sy_loop_byte(switchyard *sy, uint8_t b);       /* byte from MIDI loop in (J2) */
void sy_app_byte(switchyard *sy, uint8_t b);        /* byte from USB MIDI */
void sy_step(switchyard *sy, int dir);              /* footswitch: +1 Up, -1 Down */

/* for tests */
bool sy_read_part(switchyard *sy, int idx, sy_part *out);

#endif
