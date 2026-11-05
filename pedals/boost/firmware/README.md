# Tap Changer firmware

Started Nov 5, 2026. This is the firmware for the RP2040 on Tap Changer's main board. The pin numbers come from the Controller sheet of its schematic (`../kicad/TapChanger`), and they are the same as Flashover's, so a Pico on a breadboard uses the same pins.

**It has not run on hardware yet.** Both versions build without warnings, and the behaviour is checked in a PC simulation (below).

| Folder | What's in it |
|---|---|
| `core/` | `tapchanger.c`: Tap Changer's own behaviour, in plain C with no Pico code |
| `pico/` | The pin plan and the main program; the MIDI port, knob, buttons and chips use the shared code |
| `test/` | `tc_sim.c`: a PC simulation of Tap Changer on a loop with Switchyard and a stand-in pedal |
| `../../../libraries/linework/` | The code shared with Flashover and Switchyard's stand-in: the Linework protocol, the pedal's loop behaviour and settings in flash, the knob and LED ring helpers, and the RP2040 drivers |

Most of Tap Changer is shared with Flashover. What's its own:

- **The Boost knob is set in dB.** The knob's 128 steps run evenly from +1.1 dB to +20.2 dB, and the firmware works out the digital pot setting for each one. A linear pot would put half its travel above +15 dB, as the Product Document points out.

  | Knob | 0 | 32 | 64 (noon) | 96 | 127 |
  | --- | --- | --- | --- | --- | --- |
  | Boost | +1.1 dB | +6.0 dB | +10.8 dB | +15.5 dB | +20.2 dB |

  The largest step between two knob positions is under 0.5 dB. The figures assume the pot's typical 10k and 150-ohm wiper resistance, so a real pedal can be off by up to about 1.5 dB at full Boost (Product Document, "Gain differences").
- **Switching on and off:**
  1. Boost ramps to its minimum.
  2. The relay switches and settles for 10 ms.
  3. Boost ramps back.

  Flashover mutes with its Level pot, but Tap Changer has no Level pot. So this softens the jump in volume rather than silencing it. Whether the relay clicks is one of the things to listen for on the breadboard.
- **The ring** is a bar showing Boost. The On LED shows on or off, as on Flashover. The driver outputs are Flashover's: 1–11 for the ring, 36 for On.
- **Settings report:** `14 bb 50 oo`. The model number is 01 01, as proposed in the Product Document. It isn't in the Linework Control Specification yet.
- **Setup mode:** hold the footswitch while plugging in. The On LED blinks. Turn the knob to pick a channel, then press the footswitch to save it.
  - Channels 1–11 light that LED on the ring.
  - Channels 12–16 light LED 11 together with LED 1–5 ("11 plus").

## Simulation

`test/tc_sim.c` runs the real Switchyard, Tap Changer and stand-in code on a simulated loop. All 31 checks pass:

- **The knob in dB:** the ends and noon are right, more knob never gives less boost, and no step is larger than 0.5 dB.
- **Power-up:** a new pedal comes up bypassed at minimum Boost, then ramps to noon.
- **Switchyard finds it:** model 01 01, and the Link LED lights.
- **From the app:** Boost ramps with no jumps over 32 codes. On/off switches the relay only once Boost is at its minimum.
- **Messages:** the settings report gives `14 bb 50 oo`. Store and recall a preset; a recall sends two control changes back.
- **By hand:** the knob turn and the footswitch are reported. The knob's push does nothing yet.
- **Power cycle:** the channel, the sound and the presets come back.
- **Setup mode:** the channel display, the blinking On LED, and saving the channel.

## Building

```
make -C test test
cmake -S . -B build -G Ninja -Dpicotool_DIR=C:/Users/dalla/Repo/GreywireStudio/pedals/controller/firmware/build/_deps/picotool
cmake --build build
```

This gives:
- `build/tapchanger.uf2`, about 34 KB of program.
- `build/tapchanger_debug.uf2`, which also reports over the Pico's USB serial port: whether the pot and the LED driver answered at power-up, then a line for each change. For example:

  ```
  boost  64 (+10.8 dB)  off  | pot 52 | ch 1  pos 1  linked
  ```

The tools are the same as Flashover's; see `pedals/distortion/firmware/README.md`.

## Not done yet

- **Firmware updates over MIDI, and recovery mode.** As for Flashover, these wait for the shared loader. The Product Document suggests footswitch plus knob push for Tap Changer's recovery mode.
- **The knob's push** does nothing. The Product Document leaves open whether it should.
- **Things to check on a breadboard:**
  - The digital pot's distortion, which is the main open question for this pedal.
  - Whether the relay clicks.
  - Which way the knob turns. Flip `ENC_REVERSE` in `pico/board.h` if it's backwards.

## Glossary

- **dB (decibel):** the usual measure of loudness change. +6 dB is about twice the signal voltage, and +20 dB is ten times.
- **Ramp:** moving a value in small steps over a few milliseconds instead of one jump, which would click.
