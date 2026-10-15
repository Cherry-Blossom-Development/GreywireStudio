# Flashover breadboard, stage 1 (Option A): audio only

Oct 15, 2026

This is the sound test. It's the main board's audio path with the same values, but uses through-hole parts, three ordinary knob pots in place of the digital pots, and a toggle switch in place of the Soft/Hard relay. The parts and connections come from `Flashover/Flashover.kicad_sch`. Background: `FLASHOVER_BREADBOARD_OPTIONS.md`. Hole-by-hole build steps: `FLASHOVER_BREADBOARD_STAGE1_BUILD.md`. Order checklist (Tayda): `../Flashover Breadboard Tayda Order.docx`.

**Left out:** the controller, encoders, LED rings, MIDI, the bypass relay and footswitch, and the L7805 5 V regulator (only the logic and relays use 5 V). With no bypass, the input and output jacks connect straight to the circuit. To compare with the dry guitar, plug the guitar straight into the amp.

## Parts list

Order a few spares of the cheap parts (resistors, capacitors, diodes). Quantities below are what the circuit uses.

### Chips

| Qty | Part | Board refs | Notes |
| --- | --- | --- | --- |
| 1 | LM741 op-amp, 8-pin DIP | U1 | Gain stage |
| 1 | TL072 dual op-amp, 8-pin DIP (e.g. TL072CP) | U2 | Half A: tone/output amp. Half B: 4.5 V bias buffer |

### Resistors (1/4 W metal film, 1%)

| Qty | Value | Board refs |
| --- | --- | --- |
| 5 | 1M | R1, R3, R7, R8, R10 |
| 3 | 10k | R2, R6, R17 |
| 2 | 100k | R5, R11 |
| 2 | 6.8k | R14, R15 |
| 1 | 30k | R16 |
| 1 | 330 | R4 |
| 1 | 100 | R18 |

### Capacitors

| Qty | Value | Type | Board refs |
| --- | --- | --- | --- |
| 1 | 10 nF | Film | C1 |
| 2 | 1 nF | Film or C0G ceramic | C2, C5 |
| 1 | 22 nF | Film | C12 |
| 1 | 100 nF | Film | C8 (tone filter) |
| 1 | 470 nF | Film | C3 |
| 2 | 1 µF | Film | C4, C9 |
| 2 | 100 nF | Ceramic | C10, C11 (power filtering, one at each chip) |
| 2 | 10 µF, 25 V or more | Electrolytic | C6, C15 |
| 1 | 100 µF, 25 V or more | Electrolytic | C7 |

Film capacitors rated 63 V or 100 V are fine.

### Diodes

| Qty | Part | Board refs | Notes |
| --- | --- | --- | --- |
| 2 | 1N34A germanium | D1, D2 | Soft clipping |
| 2 | 1N4148 silicon | D4, D5 | Hard clipping (the board uses the surface-mount 1N4148W, the same diode) |
| 1 | 1N5817 Schottky | D3 | Reverse-polarity protection (the board uses the surface-mount SS14, which does the same job) |

### Controls, jacks and power

| Qty | Part | Replaces | Notes |
| --- | --- | --- | --- |
| 1 | B100k pot (linear) | U7 Gain digital pot | |
| 2 | B10k pot (linear) | U8 Tone and U9 Level digital pots | |
| 1 | SPDT on-on mini toggle | K1 Soft/Hard relay | |
| 2 | 1/4" mono jack | J1, J2 | |
| 1 | 2.1 mm DC jack | J3 | Standard pedal power, **center negative** |

The digital pots are linear, so linear (B) pots behave the same as the finished pedal. A log (A10k) Level pot would feel smoother, but the curve wouldn't match the real pedal.

Pots with PCB pins (9 mm size) plug straight into a breadboard. 16 mm or 24 mm pots with solder lugs need short wires soldered on. The same goes for the jacks and the toggle switch.

### Tools and supplies

- An 830-point solderless breadboard.
- A 22 AWG solid jumper wire kit.
- A few alligator clip leads.
- A 9 V center-negative pedal power supply.

## Connection list

Each line is one connection point, called a net. Every part listed on a line gets wired together. "Lug 1/2/3" are the pot's pins as seen from the front with the shaft toward you and the pins pointing down, left to right, so lug 2 is the wiper. Turning the pot clockwise moves the wiper toward lug 3.

**Chip pins:**
- **LM741 (U1):** 2 = − input, 3 = + input, 4 = ground (V−), 6 = output, 7 = +9 V. Pins 1, 5 and 8 are left unconnected.
- **TL072 (U2):** 1 = output A, 2 = − input A, 3 = + input A, 4 = ground (V−), 5 = + input B, 6 = − input B, 7 = output B, 8 = +9 V.

Pin 1 is next to the notch or dot on the chip.

### Power and the 4.5 V bias (VB)

| Net | Connects |
| --- | --- |
| DC jack + (sleeve) | 1N5817 anode (the end without the stripe) |
| +9V | 1N5817 cathode (striped end), C7 +, C10, C11, R7, LM741 pin 7, TL072 pin 8 |
| GND | DC jack center pin, C7 −, C10, C11, R8, C6 −, C15 −, R1, C2, C3, R11, LM741 pin 4, TL072 pin 4, both jack sleeves |
| Divider | R7, R8, C6 +, TL072 pin 5 |
| Buffer | TL072 pin 6, TL072 pin 7, R18 |
| VB (4.5 V) | R18, C15 +, R3, R10, C5, C8, R15, R17, Level pot lug 1, and the diode pairs (below) |

Put C10 next to the LM741 and C11 next to the TL072.

### Input and gain

| Net | Connects |
| --- | --- |
| Input | Input jack tip, R1, C1 |
| | C1, R2 |
| LM741 + | R2, C2, R3, LM741 pin 3 |
| LM741 − | LM741 pin 2, R4, R5 |
| | R4, Gain pot lug 3 |
| | Gain pot lug 2 (wiper), Gain pot lug 1, C3 |
| GAIN_OUT | LM741 pin 6, R5, C4 |

Lug 1 is tied to the wiper, so the pot works as a variable resistor, the same as on the board. Clockwise gives more gain.

### Clipping

| Net | Connects |
| --- | --- |
| | C4, R6 |
| CLIP | R6, R10, C5, toggle center pin, R14, C12 |
| Soft pair | Toggle end pin 1, D1 anode, D2 cathode (striped end) |
| | D1 cathode (striped end) to VB, D2 anode to VB |
| Hard pair | Toggle end pin 2, D4 anode, D5 cathode (striped end) |
| | D4 cathode (striped end) to VB, D5 anode to VB |

Each pair is two diodes facing opposite ways between the toggle and VB.

### Tone and output

| Net | Connects |
| --- | --- |
| Low-pass | R14, C8, Tone pot lug 1 |
| High-pass | C12, R15, Tone pot lug 3 |
| | Tone pot lug 2 (wiper), TL072 pin 3 |
| TL072 − A | TL072 pin 2, R16, R17 |
| Amp out | TL072 pin 1, R16, Level pot lug 3 |
| | Level pot lug 2 (wiper), C9 |
| Output | C9, R11, Output jack tip |

Tone clockwise is brighter. Level clockwise is louder.

## Checks before plugging in a guitar

1. Power it with no chips in. With a multimeter, check for 9 V (roughly 8.7 V after the diode) on the +9V rail.
2. Insert the chips the right way round, with the notch at pin 1.
3. Check for about 4.5 V at the TL072 pin 7 (VB) and on LM741 pin 6. The output may sit slightly off 4.5 V.
4. Turn Level down, plug in the guitar and amp, then bring Level up slowly. At full Gain the output can be up to about 14 dB louder than the dry guitar (Hard mode).

Expect some hum and hiss at high Gain. Breadboards pick up noise, and the finished board will be quieter.
