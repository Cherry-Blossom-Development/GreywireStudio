# Distortion - LTspice Simulations

## GS1.cir

Simulation of the GS1 schematic's effect-on signal path (footswitch engaged), using the same part names and values as `kicad/GS1/GS1.kicad_sch`.

**To run:** open `GS1.cir` in LTspice (File > Open, set the file type to "Netlists"), then press Run. Click a node or use Plot Settings > Add Traces to view `V(in)` (guitar) and `V(out)` (to the amp). Peak-to-peak levels are in View > SPICE Error Log.

**Settings** are the `.param` lines at the top of the file:

| Param | Meaning |
|-------|---------|
| `dist` | Distortion knob, 0 (min) to 1 (max) |
| `vol` | Volume knob, 0 to 1 |
| `amp` | Guitar signal level, volts peak (pickups give roughly 0.1-0.3) |
| `freq` | Guitar note frequency, Hz |

By default it steps the Distortion knob through 0, 0.5 and 1 and overlays the three results.

**Models:** LTspice has no LM741 model, so U1 is LTspice's "universal" op amp set to 741 specs. The 1N34A germanium diode model is a common community model, not a manufacturer one.

Generated output (`.raw`, `.log`, `.net`, `.db`, `.wav`) is ignored by git.

## Flashover simulations

The Flashover audio path: the GS1 gain stage scaled for 100k digital pots, switchable Soft (germanium) / Hard (silicon) clipping, a DS-1-style tone control, an output amplifier and a digital-pot Level control. See `Flashover Product Document.docx` one folder up.

| File | Purpose |
|------|---------|
| `Flashover_circuit.inc` | The shared circuit. Not run directly; the files below include it |
| `Flashover_listen.cir` | Runs 2 seconds and writes `Flashover_out.wav` at the knob settings in the file |
| `Flashover_compare.cir` | Soft vs Hard at two Gain settings: output level, and checks that every digital pot terminal stays within 0-9V |
| `Flashover_tone.cir` | 20 Hz-20 kHz response at three Tone settings, 10k vs 20k Tone pot, and min/max Gain |

**Settings** (`.param` lines at the top of each run file): `dist`, `tone`, `level` (knobs, 0 to 1), `hard` (0 = Soft, 1 = Hard), `Rt` (Tone pot value), `Rw` (digital pot wiper resistance), `amp`, `freq`.

**Design changes from GS1, and why:**

- Digital pots only accept signals between 0V and 9V, so everything after the gain stage is centered on the 4.5V reference (VB) instead of ground. VB is buffered by an op-amp (U3) because the tone network and Level pot now return to it.
- Gain stage scaled by 10 for a 100k digital pot: R5 100k, R4 330 (instead of 470, to allow for the pot's wiper resistance: about 150 ohms at 9V according to the MCP45HV51 datasheet), C3 470n. Gain at 1 kHz: about 1.8x to 170x, the same as GS1.
- Output amplifier (U2) has a gain of 4 (+12 dB) to make up the tone network's loss, so the pedal can play louder than the bypassed guitar.

**Results (October 2026):**

- All digital pot terminals stay between about 3.4V and 5.5V, inside the 0-9V limit.
- Hard is about 8.5 dB louder than Soft at full Gain (7.7 dB at half Gain). Multiplying Level by 0.376 in Hard mode matches the loudness to within 0.1 dB, so the firmware can compensate automatically.
- A 10k Tone pot (available as a 9V digital pot) gives tone curves within about 3 dB of the DS-1's 20k, so 10k is used.
- At full Level and Gain, output is about +6 dB (Soft) and +14.5 dB (Hard) above the input guitar signal.
