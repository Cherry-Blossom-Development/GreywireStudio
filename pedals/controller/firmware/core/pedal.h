/* A Linework pedal's loop behaviour, independent of the hardware.
 *
 * Used by the stand-in pedal (a second Pico on the breadboard) and by the PC simulation.
 * It merges (passes on everything it hears and adds its own messages), counts its
 * position, answers identity requests, takes a channel, keeps 128 presets of its settings,
 * and reports a quiet MIDI in. It is meant to grow into the shared code for real pedals. */
#ifndef PEDAL_H
#define PEDAL_H

#include "lw_proto.h"

#define PD_MAX_CONTROLS 8

typedef struct {
    void *ctx;
    uint32_t (*now_ms)(void *ctx);
    void (*send)(void *ctx, const uint8_t *m, int n);       /* MIDI out */
    uint8_t serial[LW_SERIAL_LEN];
    uint8_t model[2];
    uint8_t fw[3];
    /* the pedal's controls: control change numbers (spec Message map) and starting values */
    int ncontrols;
    uint8_t cc[PD_MAX_CONTROLS];
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
    uint32_t heard, reported;
} lw_pedal;

void pd_init(lw_pedal *p, const pd_hal *hal);
void pd_byte(lw_pedal *p, uint8_t b);          /* byte from MIDI in */
void pd_poll(lw_pedal *p);
void pd_set_control(lw_pedal *p, int i, uint8_t v);   /* a knob turned by hand */

#endif
