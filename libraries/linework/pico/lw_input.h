/* Endless knobs (quadrature encoders, read on GPIO interrupts) and debounced push buttons, for
 * the Linework pedals on the RP2040. */
#ifndef LW_INPUT_H
#define LW_INPUT_H

#include <stdbool.h>
#include "pico/stdlib.h"

#define LW_MAX_ENC 4

/* n encoders with A and B on the given pins (10k pull-ups on the board; the internal ones are
 * turned on too, for a bare breadboard) */
void lw_enc_init(int n, const uint *a, const uint *b);
int lw_enc_take(int k);                 /* clicks since the last call, + clockwise */

typedef struct {
    uint pin;
    int id;
    bool down;
    uint8_t count;
} lw_button;

/* Pressed = low. Init waits 5 ms for the switch filters to settle and records which buttons are
 * already held (a held button's release is not a press). */
void lw_buttons_init(lw_button *b, int n);
/* call once a millisecond: a change must hold for 10 ms; calls press(id) on each press */
void lw_buttons_poll(lw_button *b, int n, void (*press)(int id));

#endif
