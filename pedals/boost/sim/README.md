# Boost - LTspice Simulations

The Tap Changer audio path: one non-inverting op-amp stage with a 10k digital pot in its feedback. See `Tap Changer Product Document.docx` one folder up.

| File | Purpose |
|------|---------|
| `TapChanger_circuit.inc` | The shared circuit. Not run directly; the files below include it |
| `TapChanger_response.cir` | 10 Hz-100 kHz response at five Boost settings |
| `TapChanger_headroom.cir` | Distortion and output level at full Boost for rising input levels: TL072 at 9V, OPA1678 at 9V, OPA1678 at about 17.5V |
| `TapChanger_noise.cir` | Hiss at minimum and full Boost, TL072 against OPA1678 |

**To run:** open a `.cir` file in LTspice (File > Open, set the file type to "Netlists"), then press Run. Results are in View > SPICE Output Log.

**Settings** (`.param` lines at the top of each run file): `boost` (knob as a fraction of the pot's travel, 0 to 1), `Rp` (digital pot value), `Rw` (wiper resistance), `Vsup` (supply), `Rail` (how close the op-amp output gets to its rails), `En`, `Gbw`, `Slw` (op-amp noise, bandwidth, slew rate), `Rsrc` (source resistance standing in for a pickup), `amp`, `freq`.

**Models:** both op-amps are LTspice's "universal" op amp set to published figures, so clipping levels and noise are meaningful but low-level distortion isn't. The digital pot is a plain resistor, so any distortion from it doesn't show up. The charge pump isn't modelled; `Vsup` just sets the voltage.

Generated output (`.raw`, `.log`, `.net`, `.db`) is ignored by git.

**Results (October 2026):**

- Boost range at 1 kHz: +1.0 dB to +20.1 dB.
- Within 0.1 dB of the 1 kHz level from 100 Hz to 20 kHz at every setting. At 20 Hz it's 0.4 dB down at minimum Boost and 1.8 dB down at full Boost.
- Half of the pot's travel already gives +15 dB, so the firmware should map the knob in dB.
- Largest clean input at full Boost (peak volts, 220 Hz): about 0.29V with a TL072 at 9V, 0.36V with an OPA1678 at 9V, 0.78V with an OPA1678 at about 17.5V.
- Hiss at full Boost, 20 Hz-20 kHz, with a 10k source: 21 uV with the OPA1678, 33 uV with a TL072.
- The digital pot's terminals stay inside the supply in every run.
