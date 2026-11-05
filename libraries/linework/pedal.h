/* A Linework pedal's loop behaviour, independent of the hardware. Shared by every Linework
 * pedal (Flashover, Tap Changer), the stand-in pedal on the Switchyard breadboard, and the PC
 * simulations.
 *
 * It merges (passes on everything it hears and adds its own messages), counts its position,
 * answers identity requests, takes a channel, keeps 128 presets of its settings, reports
 * hand changes, and reports a quiet MIDI in. With flash access it keeps the channel, the
 * current sound and the presets through a power cycle (pd_store.h). */
#ifndef PEDAL_H
#define PEDAL_H

#include "lw_proto.h"
#include "pd_store.h"

#define PD_MAX_CONTROLS PS_VALUES
#define PD_REPORT_MS 20        /* hand changes: at most one control change per control this often */
#define PD_SAVE_MS 2000        /* the current sound is saved this long after the last change */
#define PD_QUIET_MS 20         /* flash erases wait for this long a gap on MIDI in */

/* How a control's value is read (Linework Control Specification, Message map) */
enum {
    PD_KNOB = 0,      /* 0-127 */
    PD_SWITCH,        /* 0 or 1; any value of 1 or more counts as 1 (so 0/127 controllers work) */
    PD_ONOFF,         /* 0-63 off, 64-127 on; stored and sent as 0 or 127 */
};

typedef struct {
    void *ctx;
    uint32_t (*now_ms)(void *ctx);
    void (*send)(void *ctx, const uint8_t *m, int n);       /* MIDI out */
    /* optional: called whenever a control's value changes (by MIDI, a preset, or
     * pd_set_control), so the hardware can follow */
    void (*apply)(void *ctx, int i, uint8_t v);
    const lw_flash *flash;     /* optional: NULL keeps everything in memory only */
    uint8_t serial[LW_SERIAL_LEN];
    uint8_t model[2];
    uint8_t fw[3];
    /* the pedal's controls, in the order the settings pairs are sent: control change numbers
     * (spec Message map), kinds and values for a new pedal */
    int ncontrols;
    uint8_t cc[PD_MAX_CONTROLS];
    uint8_t kind[PD_MAX_CONTROLS];
    uint8_t value[PD_MAX_CONTROLS];
} pd_hal;

typedef struct {
    pd_hal hal;
    lw_parser rx;
    uint8_t position;      /* 0 until counted */
    uint8_t channel;       /* 7F until assigned */
    uint8_t value[PD_MAX_CONTROLS];
    uint8_t preset[128][PD_MAX_CONTROLS];
    int last_preset;       /* -1 if none recalled yet */
    uint32_t heard, reported, last_byte;
    /* hand changes waiting to be reported */
    uint32_t sent_at[PD_MAX_CONTROLS];
    bool pending[PD_MAX_CONTROLS];
    /* flash */
    ps_log log;
    bool dirty;            /* the current sound changed since it was saved */
    uint32_t changed_at;
    int flash_writes, flash_erases;   /* counters, for the simulation */
} lw_pedal;

void pd_init(lw_pedal *p, const pd_hal *hal);
void pd_byte(lw_pedal *p, uint8_t b);          /* byte from MIDI in */
void pd_poll(lw_pedal *p);
void pd_set_control(lw_pedal *p, int i, uint8_t v);   /* changed on the pedal itself */
void pd_set_channel(lw_pedal *p, uint8_t ch);  /* manual setup on the pedal: 0-15 */
bool pd_linked(const lw_pedal *p);             /* counted by Switchyard, has a channel, loop alive */
uint8_t pd_normalize(const lw_pedal *p, int i, uint8_t v);

#endif
