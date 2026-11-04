/* Linework Control Specification 0.2: constants and byte helpers shared by Switchyard and the pedals.
 * Portable C (no Pico SDK), so it also builds on a PC for the loop simulation in test/. */
#ifndef LW_PROTO_H
#define LW_PROTO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* The header every Linework system message starts with. The spec asks for this to be one constant:
 * when our own MIDI Association ID (3 bytes) replaces 7D, only this changes. Byte positions are
 * counted from the end of the header, never hard-coded. */
static const uint8_t LW_HEADER[] = {0xF0, 0x7D, 0x4C, 0x57};
#define LW_HDR_LEN ((int)sizeof(LW_HEADER))
#define LW_CMD(m) ((m)[LW_HDR_LEN])          /* command byte */
#define LW_QQ(m) ((m)[LW_HDR_LEN + 1])       /* sequence number */
#define LW_BODY(m) (&(m)[LW_HDR_LEN + 2])    /* first byte after qq */

#define LW_SPEC_VERSION 0x02                  /* "ss": this firmware follows spec 0.2 */

/* Commands, pedal side (00-3F) */
enum {
    LW_POSITION = 0x01, LW_PEDAL_INFO = 0x02, LW_ASSIGN = 0x03, LW_LOOP_BREAK = 0x08,
    LW_REPORT_REQ = 0x10, LW_REPORT = 0x11, LW_STORE = 0x20, LW_READ_REQ = 0x21, LW_READ = 0x22,
    LW_FW_BEGIN = 0x30, LW_FW_DATA = 0x31, LW_FW_FINISH = 0x32,
    LW_ACK = 0x7E,
};
/* Commands, Switchyard (40-5F) */
enum {
    SY_INFO_REQ = 0x40, SY_INFO = 0x41, SY_SCAN = 0x42, SY_LIST_REQ = 0x44, SY_LIST_COUNT = 0x45,
    SY_LIST_ENTRY = 0x46, SY_STATUS_REQ = 0x48, SY_STATUS = 0x49, SY_OTHER_SEND = 0x4A,
    SY_SL_BEGIN = 0x50, SY_SL_PEDAL = 0x51, SY_SL_PART = 0x52, SY_SL_FINISH = 0x53,
    SY_SL_INFO_REQ = 0x54, SY_SL_INFO = 0x55, SY_POS_REQ = 0x58, SY_POS = 0x59, SY_JUMP = 0x5A,
};
/* Acknowledgement results (st) */
enum {
    ST_OK = 0, ST_UNKNOWN = 1, ST_RANGE = 2, ST_CHECKSUM = 3, ST_BUSY = 4, ST_FW_FAIL = 5,
    ST_NO_SETLIST = 6, ST_SETLIST_FAIL = 7,
};
/* Why a Position report was sent */
enum { WHY_UP = 0, WHY_DOWN = 1, WHY_UP_END = 2, WHY_DOWN_START = 3, WHY_JUMP = 4, WHY_POWERUP = 5, WHY_GET = 6 };

#define LW_NO_CHANNEL 0x7F
#define LW_SERIAL_LEN 10                      /* 64-bit ID in ten 7-bit bytes */
#define MIDI_ACTIVE_SENSING 0xFE

/* A message under construction. Every builder starts with the header. */
typedef struct {
    uint8_t b[160];
    int n;
} lw_msg;

void lw_begin(lw_msg *m, uint8_t cmd, uint8_t qq);
void lw_put(lw_msg *m, uint8_t v);            /* one data byte, masked to 7 bits */
void lw_put_n(lw_msg *m, const uint8_t *v, int n);
void lw_end(lw_msg *m);                       /* adds F7 */

bool lw_is_linework(const uint8_t *m, int n); /* F0 <header> cmd qq ... F7 */

/* Numbers split into 7-bit bytes */
void lw_put14(lw_msg *m, unsigned v);                       /* n0 n1 */
unsigned lw_get14(const uint8_t *p);
void lw_put32(lw_msg *m, uint32_t v);                       /* five bytes, low 7 bits first */
uint32_t lw_get32(const uint8_t *p);
void lw_serial_from_u64(uint64_t id, uint8_t out[LW_SERIAL_LEN]);

/* Standard CRC-32 (as ZIP), incremental: start with crc = 0 */
uint32_t lw_crc32(uint32_t crc, const uint8_t *data, size_t n);

/* ---- MIDI byte-stream parser ----
 * Turns a raw MIDI byte stream (UART or USB) into whole messages: channel messages (with
 * running status), system exclusive (up to LW_PARSE_MAX bytes, longer ones are dropped),
 * other system common messages, and real-time bytes, which may arrive inside a sysex and
 * are delivered on their own without breaking it. */
#define LW_PARSE_MAX 256
typedef void (*lw_msg_fn)(void *ctx, const uint8_t *msg, int n);
typedef struct {
    uint8_t buf[LW_PARSE_MAX];
    int n, need;           /* bytes so far, bytes this message needs (-1 = sysex) */
    uint8_t running;       /* running status, 0 if none */
    bool overflow;
    lw_msg_fn fn;
    void *ctx;
} lw_parser;

void lw_parser_init(lw_parser *p, lw_msg_fn fn, void *ctx);
void lw_parse(lw_parser *p, uint8_t byte);

#endif
