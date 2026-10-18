# Switchyard Audio breadboard, stage 1: hear a digital delay

Oct 18, 2026

This first prototype answers one question: do you like a digital delay added on top of the Flashover breadboard's sound? It's the audio half of Switchyard Audio on its own: a Daisy Seed running a delay, the analog circuits around it and the bypass relay. It has no phone, MIDI or setlist yet. Three knobs set the delay while we test; in the finished pedal those settings come from presets and the app. The plan is in `SWITCHYARD_AUDIO_PLAN.md`. Order checklist: `Switchyard Audio Breadboard Tayda Order.docx`.

**Built on a Daisy Seed.** The Daisy Seed is a small board with the same kind of processor, converter and memory the finished pedal will use (see "Daisy Seed" in the Glossary). It plugs into a breadboard like a large chip, and nothing needs surface-mount soldering. Figures below come from Electrosmith's Daisy Seed datasheet v1.0.5.

**How it connects:** guitar → Flashover breadboard → this board → amp.

## How the circuit works

1. The **bypass relay** decides whether the signal goes through the circuit or straight from input to output. With no power it always connects input to output, so a dead board never silences the guitar. See "Relay" in the Glossary.
2. The **input buffer** (half of a TL072 op-amp) gives the Flashover a normal 1 MΩ pedal input and drives the rest of the circuit.
3. The buffered signal splits in two. The **dry** copy goes straight to the mixer. The other copy goes into the Daisy's audio input.
4. The Daisy makes only the **echoes** (the wet signal, see the Glossary) and sends them out of its audio output.
5. The **mixer** (the other half of the TL072) adds dry and wet together and sends the result to the amp.

The guitar's own notes stay analog the whole way through; only the echoes are digital.

## Parts list

Order a few spares of the cheap parts.

### Boards and chips

| Qty | Part | Ref | Notes |
| --- | --- | --- | --- |
| 1 | Electrosmith Daisy Seed, 65 MB version | U1 | About $30, from Electrosmith or a music-electronics shop. Not sold by Tayda. If its two 20-pin headers come loose, they need soldering on |
| 1 | TL072 dual op-amp, 8-pin DIP | U2 | Input buffer and mixer |
| 1 | DPDT signal relay, 5 V coil, 8-pin (Fujitsu NA-5W-K) | K1 | The bypass. A signal relay is made for small currents like a guitar signal; big power relays can make poor contact at these levels |
| 1 | 78L05 5 V regulator, TO-92 | U3 | Powers the relay coil from the 9V supply |
| 1 | 2N3904 NPN transistor | Q1 | Lets the Daisy switch the relay coil |

### Resistors (1/4 W metal film, 1%)

| Qty | Value | Refs | Used for |
| --- | --- | --- | --- |
| 2 | 1M | R1, R2 | R1: input jack to ground (stops pops when the relay switches). R2: holds the buffer input at the bias voltage |
| 2 | 10k | R3, R4 | Bias voltage divider (makes 4.5 V from 9 V) |
| 3 | 100k | R5, R6, R7 | Mixer: dry in, wet in, feedback |
| 1 | 100k | R8 | Output, to ground (stops pops) |
| 1 | 1k | R9 | Output, in series with the jack |
| 1 | 1k | R10 | Relay transistor base |
| 1 | 470 | R11 | Status LED |

### Capacitors

| Qty | Value | Type | Refs |
| --- | --- | --- | --- |
| 1 | 100 nF | Film (box) | C1, input coupling |
| 3 | 10 µF, 25 V or more | Electrolytic | C2 (to the Daisy input), C3 (from the Daisy output), C4 (to the output jack) |
| 1 | 47 µF, 25 V or more | Electrolytic | C5, smooths the bias voltage |
| 1 | 100 µF, 25 V or more | Electrolytic | C6, across the 9V supply |
| 2 | 100 nF | Ceramic | C7 (at the TL072's power pins), C8 (at the 78L05 output) |

### Diodes

| Qty | Part | Refs | Notes |
| --- | --- | --- | --- |
| 1 | 1N5817 Schottky | D1 | Reverse-polarity protection |
| 1 | 1N4148 silicon | D2 | Across the relay coil, absorbs the voltage spike when it switches off |

### Controls, LED and jacks

| Qty | Part | Refs | Notes |
| --- | --- | --- | --- |
| 3 | 10k linear (B10k) pot, 9 mm, breadboard pins | RV1 Time, RV2 Repeats, RV3 Mix | Test controls only |
| 1 | SPST momentary soft-touch footswitch | SW1 | Effect on/off. Solder lugs, so wires need soldering on |
| 1 | 3 mm red LED | LED1 | Lights when the effect is on |
| 2 | 1/4" mono jack | J1 in, J2 out | |
| 1 | 2.1 mm DC jack | J3 | Standard pedal power, **center negative** |

### Also needed

- A full-size breadboard and jumper wires. The Daisy Seed is as wide as a 40-pin chip and sits across the breadboard's center gap.
- A **micro-USB data cable** to load firmware into the Daisy from the computer.
- A 9V pedal power supply (you already have one for the Flashover breadboard; a two-outlet supply or a daisy-chain cable runs both).
- Two 1/4" guitar cables for the extra connections.

## Connections

Daisy pins are numbered 1–40, counting down the left side from the USB end (1–20) and back up the right side (21–40). Check against the pinout drawing in the datasheet before wiring.

### Power

| From | To |
| --- | --- |
| J3 center pin (negative) | Ground rail |
| J3 sleeve (positive) | D1 anode |
| D1 cathode | +9V rail |
| C6 (100 µF) | + leg to +9V rail, − leg to ground |
| Daisy pin 39 (VIN) | +9V rail (VIN accepts 4–17 V) |
| Daisy pin 40 (DGND) | Ground rail |
| Daisy pin 20 (AGND) | Ground rail. The datasheet says AGND must always connect to DGND |
| TL072 pin 8 | +9V rail |
| TL072 pin 4 | Ground rail |
| C7 (100 nF) | Between TL072 pins 8 and 4, as close to the chip as possible |
| U3 (78L05) input pin | +9V rail. With the flat face toward you and the legs down, the pins are output, ground, input from left to right |
| U3 ground pin | Ground rail |
| U3 output pin | +5V (used only by the relay coil) |
| C8 (100 nF) | Between U3's output and ground |

Powering the Daisy from 9V and USB at the same time is safe, according to the datasheet, so it can stay plugged into the computer while you test.

### Bias voltage (4.5 V, called Vref)

| From | To |
| --- | --- |
| R3 (10k) | +9V rail to Vref |
| R4 (10k) | Vref to ground |
| C5 (47 µF) | + leg to Vref, − leg to ground |

### Bypass relay

The relay has two separate switches inside (poles A and B). Each has a common pin, an NC pin (connected when the relay is off) and an NO pin (connected when it's on). Find which pins are which on the NA-5W-K datasheet (linked from Tayda's product page) before wiring. Check its pin spacing on the datasheet too: if the pins don't line up with the breadboard holes, solder short wires to them.

| From | To |
| --- | --- |
| J1 tip (input) | Pole A common |
| R1 (1M) | J1 tip to ground |
| J1 sleeve, J2 sleeve | Ground |
| Pole A NC | Pole B NC (the straight-through bypass wire) |
| Pole A NO | C1 (to the input buffer) |
| Pole B common | J2 tip (output) |
| Pole B NO | Output network (see Mixer and output) |
| Coil, one end | +5V (U3 output) |
| Coil, other end | Q1 collector |
| D2 (1N4148) | Across the coil: cathode (striped end) to +5V, anode to Q1 collector |
| Q1 emitter | Ground |
| Q1 base | R10 (1k) to Daisy pin 9 (D8) |

### Input buffer (TL072 half A)

| From | To |
| --- | --- |
| C1 (100 nF), other side | TL072 pin 3 |
| R2 (1M) | TL072 pin 3 to Vref |
| TL072 pin 1 | TL072 pin 2 (makes it a unity-gain buffer) |

### To and from the Daisy

| From | To |
| --- | --- |
| TL072 pin 1 | C2 + leg |
| C2 − leg | Daisy pin 16 (Audio In 1) |
| Daisy pin 18 (Audio Out 1) | C3 − leg |
| C3 + leg | R6 (100k), wet into the mixer |

The Daisy's audio input takes up to 3.6 V peak to peak (about 1 V rms). The Flashover's output should stay under that at sensible level settings. If the echoes sound harsh or crackly on loud notes, lower the Flashover's Level knob, or add a two-resistor pad before C2 (ask and we'll size it).

### Mixer and output (TL072 half B)

| From | To |
| --- | --- |
| TL072 pin 1 | R5 (100k), dry into the mixer |
| R5 other end, R6 other end | TL072 pin 6 |
| R7 (100k) | TL072 pin 6 to pin 7 (feedback) |
| TL072 pin 5 | Vref |
| TL072 pin 7 | C4 + leg |
| C4 − leg | R9 (1k), and R8 (100k) to ground |
| R9 other end | Relay pole B NO |

The mixer turns the signal upside down (it's an inverting mixer). Both dry and wet are flipped the same way, so they stay in step with each other.

### Controls

| From | To |
| --- | --- |
| RV1, RV2, RV3 outer legs | Daisy pin 21 (+3V3 analog) and Daisy pin 20 (AGND) |
| RV1 (Time) middle leg | Daisy pin 22 (A0) |
| RV2 (Repeats) middle leg | Daisy pin 23 (A1) |
| RV3 (Mix) middle leg | Daisy pin 24 (A2) |
| SW1 | Daisy pin 8 (D7) and ground. The Daisy's internal pull-up is used, so no resistor is needed |
| Daisy pin 10 (D9) | R11 (470), then LED1 anode; LED1 cathode to ground |

Pins 14 and 15 (D13 and D14, the Daisy's serial port) are kept free for stage 2, when the board connects to the Switchyard Pico.

## Firmware

The test firmware is a simple delay with three settings: Time (about 50 ms to 1 s), Repeats (how many echoes, from one to almost endless) and Mix (how loud the echoes are). The footswitch turns the effect on and off and switches the relay. It's written in C++ with Electrosmith's free libraries (libDaisy and DaisySP) and loaded over USB. I'll write it once the parts are ordered.

## Glossary

**Daisy Seed:** a small circuit board from Electrosmith made for audio projects. It has an STM32H750 processor running at 480 MHz, a 24-bit stereo audio converter (codec), 64 MB of memory for long delays, and a USB socket for loading programs. It's used here to prototype the effects before we design our own board with the same chips.

**Dry and wet:** the dry signal is the original guitar sound; the wet signal is the sound an effect produces (here, the echoes). Mixing them keeps the original notes untouched while adding the effect.

**Relay:** a switch flipped by an electromagnet (the coil). Here the Daisy turns the coil on through a transistor to route the guitar through the circuit. With the coil off, a spring holds the switch in its "NC" (normally closed) position, which connects input straight to output.

**Vref (bias voltage):** the op-amp runs from a single 9 V supply, so the audio signal is centered on 4.5 V instead of 0 V, giving it room to swing up and down. Capacitors at the input and output block that 4.5 V so it never reaches the guitar, the Daisy or the amp.
