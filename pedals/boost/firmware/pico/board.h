/* Tap Changer's pin plan, from the Controller sheet of its main board schematic
 * (pedals/boost/kicad/TapChanger). The same numbers as Flashover: Boost uses Flashover's Gain
 * pins. A Pico on a breadboard can use the same pins. */
#ifndef BOARD_H
#define BOARD_H

#define PIN_MIDI_TX    0    /* UART0 TX: MIDI out */
#define PIN_MIDI_RX    1    /* UART0 RX: MIDI in (through the H11L1) */
#define PIN_FX_ON      3    /* bypass relay: high = effect on */
#define PIN_POT_SDA    4    /* I2C0: the Boost digital pot */
#define PIN_POT_SCL    5
#define PIN_BOOST_A    6    /* the Boost encoder: A, B and push, each with a 10k pull-up */
#define PIN_BOOST_B    7
#define PIN_BOOST_SW   8
#define PIN_FOOTSW     15   /* low = pressed */
#define PIN_LED_EN     17   /* LED driver SDB: high = on */
#define PIN_LED_SDA    18   /* I2C1: the LED driver */
#define PIN_LED_SCL    19
#define PIN_LINK_LED   20

#define ADDR_POT       0x3C /* U2: A1 = 0, A0 = 0 */
#define ADDR_LEDS      0x3C /* U7 on the control board, AD at ground, on its own bus */

/* If the knob turns the wrong way on the first board, set this to true */
#define ENC_REVERSE false

#endif
