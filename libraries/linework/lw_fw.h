/* Firmware updates over MIDI, and the boot decision: plain C, shared by every Linework pedal's
 * firmware (which receives an update) and its loader (which installs it), and run unchanged in
 * the PC simulation (test/fw_sim.c).
 *
 * Flash layout (2 MB), offsets from the start of flash:
 *   0x000000  loader, 32 KB: starts first; installs a waiting update; recovery mode
 *   0x008000  the pedal's firmware, 960 KB (linked to run here)
 *   0x0F8000  download area, 960 KB: a new firmware arrives here first
 *   0x1E8000  update record, 4 KB: a log of "pending" and "installed" notes
 *   0x1F0000  the pedal's settings, 64 KB (pd_store.h uses the first 8 KB)
 *
 * An update, start to finish:
 *  1. The running firmware gets Firmware: begin (size, CRC-32), erases as much of the download
 *     area as the new firmware needs, then takes Firmware: data blocks of 64 bytes into it.
 *  2. Firmware: finish checks the CRC-32 of the whole download. Only if it matches is a
 *     "pending" note written; then the pedal restarts.
 *  3. At power-up the loader sees the note, checks the download again, copies it over the
 *     firmware, checks the copy, writes an "installed" note and starts the new firmware.
 * A power cut at any point leaves either the old firmware running (before the note) or the
 * download still intact for the loader to copy again (after it). The firmware area is checked
 * against its installed note at every power-up; if it's damaged and can't be repaired, the
 * loader stays in recovery mode and waits for an update itself. */
#ifndef LW_FW_H
#define LW_FW_H

#include <stdint.h>
#include <stdbool.h>
#include "pd_store.h"            /* lw_flash */

#define FW_LOADER_SIZE 0x8000u
#define FW_APP_OFF     0x8000u
#if LW_LAYOUT_SWITCHYARD
/* Switchyard keeps its setlists (644 KB) at 1 MB, so its firmware areas are smaller:
 *   0x000000 loader, 32 KB    0x008000 firmware, 480 KB    0x080000 download area, 480 KB
 *   0x0F8000 update record, 4 KB    0x100000 setlist store (pedals/controller/firmware) */
#define FW_APP_SIZE    0x78000u
#define FW_DL_OFF      0x80000u
#define FW_REC_OFF     0xF8000u
#else
#define FW_APP_SIZE    0xF0000u
#define FW_DL_OFF      0xF8000u
#define FW_REC_OFF     0x1E8000u
#endif
#define FW_SECTOR      4096u
#define FW_VECTORS     0x100u    /* the firmware's vector table, after its unused 256-byte boot2 */
#define FW_BLOCK       64u       /* bytes per Firmware: data block */

_Static_assert(FW_APP_OFF + FW_APP_SIZE <= FW_DL_OFF && FW_DL_OFF + FW_APP_SIZE <= FW_REC_OFF,
               "the firmware, download and record areas overlap");

/* ---- receiving an update (in the running firmware, or the loader in recovery mode) ----
 * The flash device here spans the whole chip (offsets from 0). */
typedef struct {
    const lw_flash *f;
    uint32_t size, crc;
    uint32_t erase_at, erase_end;   /* sectors still to erase */
    bool active, erased;
    uint32_t received;              /* blocks written, for information */
} lw_fw_rx;

/* returns an ack status (lw_proto.h ST_*): ST_OK, or ST_RANGE if the size won't fit */
int lw_fw_begin(lw_fw_rx *u, const lw_flash *f, uint32_t size, uint32_t crc);
/* one sector per call (about 45 ms on the chip); true once everything needed is erased */
bool lw_fw_erase_step(lw_fw_rx *u);
/* block n (64 bytes) into the download area: ST_OK, ST_BUSY (still erasing), ST_RANGE */
int lw_fw_block(lw_fw_rx *u, uint32_t n, const uint8_t data[FW_BLOCK]);
/* checks the whole download's CRC-32 and writes the pending note: ST_OK or ST_FW_FAIL */
int lw_fw_finish(lw_fw_rx *u);

/* ---- at power-up (the loader) ---- */
enum { FW_BOOT_APP, FW_BOOT_RECOVERY };
enum { FW_DID_NOTHING, FW_DID_INSTALL, FW_DID_REPAIR };
/* Decides what to start, installing a waiting update first. recovery_held: the recovery
 * buttons were held at power-up. *did (optional) says whether it installed or repaired. */
int lw_fw_boot(const lw_flash *f, bool recovery_held, int *did);

/* For the running firmware, once after an install: true if this is the first start of a newly
 * installed version (it then sends a fresh identity reply, as the spec asks) */
bool lw_fw_take_announce(const lw_flash *f);

/* the 7-bit packing of Firmware: data (64 bytes travel as 74 MIDI bytes: each group of 7 bytes
 * is sent as one byte of their top bits, then their low 7 bits) and its checksum */
#define FW_PACKED 74
void lw_pack7(const uint8_t *in, int n, uint8_t *out);         /* out: n + (n + 6) / 7 bytes */
void lw_unpack7(const uint8_t *in, int n_out, uint8_t *out);
uint8_t lw_fw_checksum(const uint8_t *packed, int n);          /* sum of the bytes, low 7 bits */

#endif
