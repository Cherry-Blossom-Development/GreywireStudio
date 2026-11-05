/* Flashover's pin plan, from the Controller sheet of the main board schematic
 * (pedals/distortion/kicad/Flashover). A Pico on a breadboard can use the same pins. */
#ifndef BOARD_H
#define BOARD_H

#define PIN_MIDI_TX    0    /* UART0 TX: MIDI out (J6) */
#define PIN_MIDI_RX    1    /* UART0 RX: MIDI in (J5, through the H11L1) */
#define PIN_HARD_SEL   2    /* Soft/Hard relay K1: high = Hard */
#define PIN_FX_ON      3    /* bypass relay K2: high = effect on */
#define PIN_POT_SDA    4    /* I2C0: the three digital pots */
#define PIN_POT_SCL    5
#define PIN_GAIN_A     6    /* encoders: A, B and push, each with a 10k pull-up */
#define PIN_GAIN_B     7
#define PIN_GAIN_SW    8
#define PIN_TONE_A     9
#define PIN_TONE_B     10
#define PIN_TONE_SW    11
#define PIN_LEVEL_A    12
#define PIN_LEVEL_B    13
#define PIN_LEVEL_SW   14
#define PIN_FOOTSW     15   /* low = pressed */
#define PIN_MODE_BTN   16   /* Soft/Hard button, low = pressed */
#define PIN_LED_EN     17   /* LED driver SDB: high = on */
#define PIN_LED_SDA    18   /* I2C1: the LED driver (its address overlaps the pots') */
#define PIN_LED_SCL    19
#define PIN_LINK_LED   20



/* I2C addresses (7-bit): MCP45HV51 is 0111 1 A1 A0, IS31FL3236A with AD at ground is 0111 100 */
#define ADDR_POT_GAIN  0x3C /* U7: A1 = 0, A0 = 0 */
#define ADDR_POT_TONE  0x3D /* U8: A1 = 0, A0 = 1 */
#define ADDR_POT_LEVEL 0x3E /* U9: A1 = 1, A0 = 0 */
#define ADDR_LEDS      0x3C /* U10, on its own bus */

/* If a knob turns the wrong way on the first board, flip its entry here */
#define ENC_REVERSE {false, false, false}

#endif
