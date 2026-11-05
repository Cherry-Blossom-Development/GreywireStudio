/* The Linework MIDI loop on UART0 (31,250 baud), with ring buffers, and the pedal's serial number
 * from the flash chip's unique ID. */
#ifndef LW_MIDI_H
#define LW_MIDI_H

#include <stdint.h>
#include "lw_proto.h"

void lw_midi_init(unsigned tx_pin, unsigned rx_pin);
int lw_midi_get(void);                          /* next received byte, or -1 */
void lw_midi_send(const uint8_t *m, int n);     /* whole message or nothing */
void lw_midi_flush(void);                       /* feed the UART; call often */
void lw_serial(uint8_t out[LW_SERIAL_LEN]);

#endif
