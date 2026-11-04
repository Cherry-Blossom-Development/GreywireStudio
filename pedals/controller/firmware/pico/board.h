/* Pin plan: the same GPIO numbers on the stage 1 breadboard (Pico) and on the board
 * (pedals/controller/kicad/, Controller sheet). */
#ifndef BOARD_H
#define BOARD_H

#define PIN_LOOP_TX   0    /* UART0 TX: MIDI loop out, J1 */
#define PIN_LOOP_RX   1    /* UART0 RX: MIDI loop in, J2 (through the H11L1) */
#define PIN_SW_DOWN   2
#define PIN_SW_UP     3
#define PIN_OTHER_TX  4    /* UART1 TX: MIDI out for other brands, J3 */
#define PIN_SDA       6    /* I2C1: display (and USB-C controller on the board) */
#define PIN_SCL       7
#define PIN_PD_INT    8    /* board only: FUSB302B interrupt */
#define PIN_VBUS_EN   9    /* board only: keep-alive charge switch */
#define PIN_VBUS_FLT  10   /* board only */
#define PIN_LED_DOWN  14
#define PIN_LED_UP    15
#define PIN_SENSE_9V  26   /* ADC0, board only */

#define MIDI_BAUD 31250

/* Flash: the firmware lives below 1 MB; Switchyard's setlists and state above it */
#define STORE_BASE (1024u * 1024u)

#endif
