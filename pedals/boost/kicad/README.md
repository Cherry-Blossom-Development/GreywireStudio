# Tap Changer - KiCad projects

Schematics only, started Nov 3, 2026. There is no board layout yet: that waits for the breadboard to show whether the digital pot is clean enough, and for the enclosure choice.

| Folder | Board | What's on it |
|---|---|---|
| `TapChanger/` | Main board | Audio circuit, power and bias, microcontroller, digital pot, MIDI, bypass relay and the five jacks |
| `TapChanger_Controls/` | Control board | The Boost knob with its 11-LED ring, the LED driver, and the On and Link LEDs |

The two boards connect through a 2x6 header (J8 on the control board into J7 on the main board), the same arrangement as Flashover.

## How it's made

`scripts/gen_tapchanger.py` writes both projects. Run it with any Python 3. It reuses Flashover's schematic writer in `pedals/distortion/kicad/scripts/`, and it overwrites the schematics each time it runs. Once the schematics are edited by hand in KiCad, stop running it.

- **Audio:** values from `pedals/boost/sim/TapChanger_circuit.inc`. The part references (R1-R4, R7, R8, R11, R12, C1-C3, C6, C7, C9, C10, U1, D3) match the breadboard guide.
- **Copied from Flashover:** power, controller, MIDI, bypass, knob and LED driver sheets, cut down to one knob. The microcontroller pins and LED driver outputs are the same as Flashover's (Boost uses Flashover's Gain pins), so the firmware carries over.
- **Differences from Flashover:**
  - The op-amp is an OPA1678 (U1). The breadboard uses a TL072, which has the same pins.
  - The VB buffer has no 100 ohm resistor or extra capacitor on its output. The Boost stage returns its audio to VB through R4 and C3, so a resistor there would change the gain. The project's electrical rules check is set to allow an op-amp output on the VB supply net for this reason.
  - Decoupling capacitors C35 and C36 sit on the digital pot's two supply pins.

## Checks

On Nov 3, 2026, both projects passed KiCad's electrical rules check with no errors or warnings. The audio connections in the exported netlist match the LTspice circuit, and the 12 connector pins carry the same signals on both boards.
