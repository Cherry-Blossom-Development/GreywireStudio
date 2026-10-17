# Switchyard breadboard, stage 1: control, without phone charging

Oct 17, 2026

This is the first Switchyard prototype. It has everything except phone charging: the two footswitches and their LEDs, the 4-digit song.part readout, the MIDI loop out and in, the MIDI out for other brands' pedals, 9V power, and running from the phone's USB power when no 9V is connected. The design comes from `Switchyard Product Document.docx` and the `Linework Control Specification.docx`. Order checklist (Tayda plus a few other shops): `Switchyard Breadboard Tayda Order.docx`.

**Built on a Raspberry Pi Pico.** The Pico is a small ready-made board with the same RP2040 chip the finished pedal will use, plus its flash memory, crystal, USB socket and 3.3 V regulator. So the prototype runs the real firmware, and nothing on it needs surface-mount soldering.

**Left out until stage 2:** keep-alive phone charging. It needs a USB-C power controller chip and a 9V-to-5V converter, which will be chosen later and come on small breakout boards.

**Also included (optional, recommended): a stand-in pedal.** This is a second Pico with its own MIDI in and out. Its firmware passes messages on and answers Switchyard like a Linework pedal would. That lets us test the whole loop (counting, identifying, assigning channels, presets) before Flashover has its MIDI parts. It can also become the starting point for Flashover's own firmware.

## Parts list

Order a few spares of the cheap parts. The quantities below are what the circuit uses; the order sheet includes spares.

### Boards and chips

| Qty | Part | Ref | Notes |
| --- | --- | --- | --- |
| 1 | Raspberry Pi Pico H (with headers already soldered on) | U1 | The controller. Not sold by Tayda |
| 1 | Adafruit 0.56" 4-digit 7-segment display with I2C backpack, red (product 878) | DS1 | The song.part readout. Not sold by Tayda. Comes as a kit: the display and header pins need soldering to the backpack |
| 1 | H11L1 optocoupler, 6-pin DIP | U2 | MIDI in. The same part as Flashover |
| 1 | L7805 5 V regulator, TO-220 | U3 | Turns 9V into 5V for the Pico |

The Pico makes its own 3.3 V from the 5 V, so the stage 1 board doesn't need the AP2112K regulator that the finished pedal uses.

### Resistors (1/4 W metal film, 1%)

| Qty | Value | Refs | Used for |
| --- | --- | --- | --- |
| 2 | 33 | R1, R3 | MIDI outs, ring side |
| 2 | 10 | R2, R4 | MIDI outs, tip side |
| 1 | 220 | R5 | MIDI in, sets the optocoupler's LED current |
| 1 | 1k | R6 | MIDI in, pull-up on the optocoupler output |
| 2 | 470 | R7, R8 | Footswitch LEDs |
| 2 | 4.7k | R9, R10 | Display data lines (pull-ups) |

The 33 and 10 ohm values are the MIDI Association's standard values for a 3.3 V MIDI output. They limit the current while staying compatible with every MIDI input.

### Capacitors

| Qty | Value | Type | Refs |
| --- | --- | --- | --- |
| 2 | 10 µF, 25 V or more | Electrolytic | C1 (L7805 input), C2 (L7805 output) |
| 2 | 100 nF | Ceramic | C3 (at the H11L1), C4 (at the L7805 output) |

### Diodes

| Qty | Part | Refs | Notes |
| --- | --- | --- | --- |
| 2 | 1N5817 Schottky | D1, D2 | D1: reverse-polarity protection. D2: lets 9V power and phone power share the Pico's power input |
| 1 | 1N4148 silicon | D3 | Protects the optocoupler if a MIDI cable is wired backwards |

### Switches, LEDs and jacks

| Qty | Part | Refs | Notes |
| --- | --- | --- | --- |
| 2 | SPST momentary soft-touch footswitch | SW1 (Down), SW2 (Up) | Solder lugs, so wires need soldering on |
| 2 | 3 mm red LED | LED1, LED2 | Above Down and Up |
| 3 | 3.5 mm stereo (TRS) jack | J1 loop out, J2 loop in, J3 out for other brands | |
| 1 | 2.1 mm DC jack | J4 | Standard pedal power, **center negative** |

### Stand-in pedal (optional)

| Qty | Part | Notes |
| --- | --- | --- |
| 1 | Raspberry Pi Pico H | |
| 1 | H11L1 optocoupler | MIDI in |
| 1 each | 220, 1k, 33 and 10 ohm resistors, 100 nF ceramic capacitor, 1N4148 diode | The same MIDI in and out circuits as Switchyard |
| 2 | 3.5 mm stereo (TRS) jack | Its MIDI in and out |

It runs from a USB cable to the computer, so it needs no power parts.

### Cables

- A **USB-C to micro-USB data cable**, from the phone to the Pico. This type of cable tells the phone that it's in charge of the connection and should power the Pico. Charge-only cables won't work; it must say "data" or "sync."
- A **USB to micro-USB cable** for the computer, used to load the firmware. You may already have one.
- Three short **3.5 mm stereo (TRS) male-to-male cables**. Ordinary stereo audio cables work as MIDI cables with our standard Type A wiring.

Older iPhones with a Lightning port need Apple's Lightning to USB 3 Camera Adapter plus a USB-A to micro-USB cable.

### Tools and supplies

- Two 830-point breadboards: one for Switchyard, one for the stand-in pedal. Keep the Flashover breadboard separate.
- A 22 AWG solid jumper wire kit.
- The multimeter, soldering iron and 9V supply from the Flashover build.

## Connection list

Each line is one connection point, called a net. Everything listed on a line gets wired together.

**Pico pins** are numbered 1 to 40 around the board: with the USB socket at the top, pin 1 is top left, pins 1 to 20 run down the left side, and pins 21 to 40 run up the right side, so pin 40 is top right. The pins used here:

| Pin | Name | Used for |
| --- | --- | --- |
| 1 | GP0 | MIDI loop out (UART0 transmit) |
| 2 | GP1 | MIDI loop in (UART0 receive) |
| 3 | GND | Ground |
| 4 | GP2 | Down footswitch |
| 5 | GP3 | Up footswitch |
| 6 | GP4 | MIDI out for other brands (UART1 transmit) |
| 9 | GP6 | Display data (SDA) |
| 10 | GP7 | Display clock (SCL) |
| 19 | GP14 | Down LED |
| 20 | GP15 | Up LED |
| 36 | 3V3 | 3.3 V output, for the MIDI, display and pull-up parts |
| 38 | GND | Ground |
| 39 | VSYS | Power input |

Leave pin 40 (VBUS) unconnected. Power from the phone or computer arrives through the USB socket, and the Pico's own diode passes it to VSYS. That's how it runs from the phone when no 9V is plugged in.

**Other pinouts:**
- **L7805 (U3):** with the printed side facing you and the legs down: 1 = input, 2 = ground, 3 = output.
- **H11L1 (U2):** 1 = LED anode, 2 = LED cathode, 3 = not connected, 4 = output, 5 = ground, 6 = supply. Pin 1 is next to the dot.
- **3.5 mm jacks:** each has a tip, ring and sleeve pin. Find which is which by plugging in a cable and using the multimeter's continuity beep between each pin and the cable plug's tip, ring and sleeve. Type A MIDI wiring: **tip = MIDI pin 5, ring = MIDI pin 4, sleeve = MIDI pin 2 (shield).**

### Power

| Net | Connects |
| --- | --- |
| DC jack + (sleeve) | D1 anode (the end without the stripe) |
| +9V | D1 cathode (striped end), L7805 pin 1, C1 + |
| +5V | L7805 pin 3, C2 +, C4, D2 anode |
| VSYS | D2 cathode (striped end), Pico pin 39 |
| 3V3 | Pico pin 36, H11L1 pin 6, C3, R1, R3, R6, R9, R10, display + (and display IO, if its backpack has that pin) |
| GND | DC jack center pin, L7805 pin 2, C1 −, C2 −, C4, C3, Pico pins 3 and 38, H11L1 pin 5, SW1, SW2, LED1 and LED2 cathodes, J1 sleeve, J3 sleeve, display − |

Put C3 right next to the H11L1 and C2 and C4 right next to the L7805.

### MIDI loop out (J1) and MIDI out for other brands (J3)

| Net | Connects |
| --- | --- |
| | Pico pin 1 (GP0), R2 |
| J1 tip | R2 |
| J1 ring | R1 (other end at 3V3) |
| J1 sleeve | GND |
| | Pico pin 6 (GP4), R4 |
| J3 tip | R4 |
| J3 ring | R3 (other end at 3V3) |
| J3 sleeve | GND |

### MIDI loop in (J2)

| Net | Connects |
| --- | --- |
| J2 ring | R5 |
| | R5, H11L1 pin 1, D3 cathode (striped end) |
| J2 tip | H11L1 pin 2, D3 anode |
| J2 sleeve | **Not connected** |
| MIDI_IN | H11L1 pin 4, R6 (other end at 3V3), Pico pin 2 (GP1) |

The sleeve stays unconnected on purpose. MIDI inputs are isolated: the optocoupler passes the signal as light, so the two devices' grounds never touch, and that prevents hum loops between pedals.

### Footswitches and LEDs

| Net | Connects |
| --- | --- |
| Down | SW1 one lug, Pico pin 4 (GP2) (the other lug to GND) |
| Up | SW2 one lug, Pico pin 5 (GP3) (the other lug to GND) |
| | Pico pin 19 (GP14), R7, then R7 to LED1 anode (long leg) |
| | Pico pin 20 (GP15), R8, then R8 to LED2 anode (long leg) |

The footswitches need no resistors. The firmware turns on the Pico's built-in pull-up resistors and filters out switch bounce.

### Display

| Display pin | Connects |
| --- | --- |
| + | 3V3 |
| − | GND |
| D (data) | Pico pin 9 (GP6), R9 |
| C (clock) | Pico pin 10 (GP7), R10 |

The display runs at 3.3 V so its data lines match the Pico's. It's a little dimmer than at 5 V but plenty for a test, and the firmware can set the brightness.

### Stand-in pedal

Its own Pico gets copies of two circuits:
- **MIDI in:** the J2 circuit (R5, R6, D3, H11L1, C3) on its pin 2 (GP1), powered from its own pin 36.
- **MIDI out:** the J1 circuit (R1, R2) on its pin 1 (GP0).

Cable the loop: Switchyard J1 (loop out) → stand-in MIDI in → stand-in MIDI out → Switchyard J2 (loop in). With no stand-in, a single cable from J1 to J2 makes a "loop with zero pedals," which is enough for the first firmware tests.

## Checks before connecting a phone

1. With the Pico out of the breadboard, plug in the 9V supply. Check for about 8.7 V at L7805 pin 1, 5 V at pin 3, and about 4.7 V at D2's striped end.
2. Unplug the 9V and fit the Pico. Plug in the 9V again and check for 3.3 V at Pico pin 36.
3. Load the firmware from the computer, over the Pico's USB socket.
4. Then connect the phone. A Pico without firmware shows up as a USB drive, so load the firmware first.
