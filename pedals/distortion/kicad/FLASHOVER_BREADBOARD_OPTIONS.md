# Flashover breadboard prototype options

Oct 14, 2026 · Dallas Caley

Exported from the online version: https://claude.ai/code/artifact/00c4e9d0-b0e3-4eba-a132-372aaa97fd25

## Why breadboard first

Building the Flashover on a breadboard lets you hear it and use it before paying for circuit boards and factory assembly. "Will I like it?" is really two questions:

- **Will I like the sound?** That depends only on the audio circuit, which is the easiest part to breadboard.
- **Will I like how it works?** That covers the endless knobs, the LED rings, Soft/Hard switching, MIDI and bypass. It needs the controller and its program.

The circuit boards are finished, saved and pushed to GitHub, so they can wait while you test. If testing changes anything, such as a resistor value or a different diode, the schematic, simulation and boards get updated before ordering.

## Option A: audio circuit only, with ordinary knobs

This is the quickest sound test: roughly $30–50 in parts and a weekend of building. You plug in the guitar, play through your amp and turn real knobs.

| On the circuit board | On the breadboard |
| --- | --- |
| LM741 and TL072 op-amps (small amplifier chips) | The same chips in the 8-pin plug-in (DIP) package |
| Resistors and capacitors (tiny surface-mount parts) | Ordinary parts with wire legs, same values |
| 1N34A germanium diodes (Soft clipping) | Already have wire legs |
| 1N4148 silicon diodes (Hard clipping) | Wire-leg 1N4148 |
| L7805 5 V regulator | Already a wire-leg part |
| 3 digital pots (chip-controlled "knobs") | 3 ordinary knob pots: 100k Gain, 10k Tone, plus Level |
| Soft/Hard relay | A small toggle switch |

Left out: the microcontroller, encoders, LED rings, MIDI and the bypass relay.

Real pots sound the same as the digital pots, within what matters. In the simulations, the only differences were the pot's internal resistance (about 150 ohms) and its 0–9 V signal limit, and the design already accounts for both.

## Option B: the full circuit on a breadboard

The whole pedal can be breadboarded with the same chips, values and connections, including the controller, MIDI, digital pots and LED rings. Two chips need special handling (next section).

| Section | On the breadboard | Exact same? |
| --- | --- | --- |
| Audio path (op-amps, diodes, resistors, capacitors) | Plug-in versions of the same parts | Yes |
| Controller (RP2040 chip, its memory chip, 12 MHz crystal, 3.3 V regulator) | A Raspberry Pi Pico ($5). The board's controller section is essentially a Pico: same chip, same memory chip, same crystal. The pins have the same names, so the program carries straight over | Yes, in practice |
| Encoders and Soft/Hard button | The same Bourns PEC11R encoders and the same button, connected with jumper wires | Yes |
| MIDI in and out | The same H11L1 isolator chip in its plug-in package, plus two 3.5 mm jacks on small breakout boards | Yes |
| Bypass and Soft/Hard relays | The through-hole version of the same Omron G6K relay, connected with short wires | Yes (same relay type) |
| Digital pots (MCP45HV51 ×3) | No plug-in version exists; see the next section | Possible |
| LED driver (IS31FL3236A) | No plug-in version exists; see the next section | Possible |
| LED rings (33 LEDs plus 4 panel LEDs) | Ordinary 3 mm LEDs arranged in three arcs. A lot of wiring, but simple | Same driver, same layout |

## The two tiny chips

The recommended route is a small factory-assembled breakout board. The digital pots (MCP45HV51) and the LED driver (IS31FL3236A) only come as tiny surface-mount chips, with legs 0.65 mm and 0.5 mm apart.

| Way | Cost and time | Exact chips? | Notes |
| --- | --- | --- | --- |
| **Factory-assembled breakout board (recommended).** A small board with just those four chips and pins for the breadboard, designed in KiCad; the factory (e.g. JLCPCB) solders the chips on | About $20–40, 1–2 weeks | Yes | No fine soldering. A cheap board order, not the real pedal board |
| **Solder them onto adapter boards yourself.** Adapter boards turn a chip into plug-in pins | About $1 per adapter | Yes | The digital pots are doable with flux and a fine soldering tip. The LED driver is hard for a first attempt |
| **Substitute similar plug-in chips** | Cheapest | No | Easier, but no longer the exact circuit, so it defeats the purpose of Option B |

## Staged build plan

Option B is built in five stages, each one playable or testable before the next. Stage 1 is Option A, so it's the first stage either way. Each stage adds to the last, so nothing gets thrown away.

1. **Audio path with ordinary knob pots.** The sound test. After this stage you know whether you like the sound, before spending money on the rest.
2. **Pico, encoders and digital pots.** The knobs now control the sound through the chips, as in the finished pedal.
3. **LED rings and the Soft/Hard button.** The rings show each knob's setting.
4. **MIDI in and out.**
5. **Relays.** Bypass and Soft/Hard switching, as in the finished pedal.

The Pico board (stage 2) and breakout board are only needed from stage 2 on, so that order can wait until stage 1 is done.

## Free first step: simulate a real guitar recording

A recorded-guitar simulation costs nothing and gives a rough sense of the pedal's character before buying anything.

- The LTspice simulation already writes WAV files (`sim/Flashover_soft.wav`, `sim/Flashover_hard.wav`), but they come from a test tone, not real playing.
- LTspice can take a real recording as its input. Record a few seconds of dry guitar (straight into an audio interface, no effects), and Claude runs it through the simulated circuit at a few knob settings, in Soft and Hard.
- Limits: it won't show the feel or how the pedal responds to picking harder, and the germanium diode model is only approximate. It doesn't replace playing through an amp.

## Things to know before deciding

- **There's no firmware yet.** The pedal needs a program regardless: reading the knobs, setting the digital pots, lighting the rings, handling MIDI. A breadboard is the ideal place to write and debug it, and the same program then runs on the real board.
- **Option B is a big breadboard project:** about 150 parts on 2–3 large breadboards, which is why it's split into stages.
- **Breadboards are noisier.** At full Gain (about 170×), long wires pick up hum and hiss. That affects how quiet it is, not how it sounds or works, and a proper board with a ground plane is quieter.
- **Nothing is lost on the board side.** Both circuit boards are fully routed and checked, and the latest change (the side-entry footswitch connector) is pushed to GitHub.

## Decisions and next steps

Start with the stage 1 parts list and wiring guide: it's needed whichever way the other decisions go.

- [ ] Decide: Option A (sound test only) or Option B (the full circuit, in stages)
- [ ] Decide, for Option B: factory-assembled breakout board or self-soldered adapters for the two tiny chips
- [ ] Optional: record a few seconds of dry guitar for the LTspice listening test
- [ ] Claude prepares the stage 1 parts list (orderable from Tayda or Mouser) and a step-by-step breadboard wiring guide
- [ ] Pick a firmware language before stage 2, for example MicroPython (easier) or C (Pico SDK)
