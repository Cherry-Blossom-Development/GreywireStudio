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
