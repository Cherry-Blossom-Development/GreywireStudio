# Flashover circuit boards: where we are (2026-10-09)

## In short

The schematic is finished and the parts are placed on two circuit boards. Wiring the boards together
("routing") is partly done: the control board is nearly finished, but the main board still needs
substantial work. Nothing is ready to order yet.

## What exists

Two KiCad projects, one per board:

| Folder | Board | What's on it |
|---|---|---|
| `Flashover/` | Main board | Audio circuit, microcontroller, digital pots, MIDI, bypass relay, and all five jacks along the top edge |
| `Flashover_Controls/` | Control board | The three knobs with their LED rings, the LED driver, the Soft/Hard button and the four panel LEDs |

- **Enclosure:** Hammond 1590BB, with the jacks mounted along the top edge.
- **Stacking:** the control board sits just under the top panel and plugs into the main board below it
  through a 20-pin connector (J8 on the control board into J7 on the main board).
- **Spacers:** two M3 screw holes on each board, in matching positions.
- **Panel layout (looking down at the pedal):**
  - Gain and Level knobs side by side at the top, with Tone below and centred.
  - Soft/Hard button at the lower left, with the Soft and Hard LEDs just above it.
  - On LED just above the footswitch; Link LED under the Level knob.
- **Schematic check:** passes with no errors on both projects.
- **Board check:** both boards match their schematics exactly.

## Where the routing stands

Both boards are 4-layer. The two inner layers are copper "planes" for ground (and +5V on the control
board), which keeps the audio quiet and cuts down the wiring.

| Board | Connections still missing | Wiring errors (shorts or too-close tracks) |
|---|---|---|
| Control board | 7 | 0 |
| Main board | 50 | 0 |

- **Control board:** the 7 missing connections are all around the LED driver chip (U10). Fixing them
  is about ten minutes' work by hand in KiCad.
- **Main board:** the automatic router (Freerouting) gets most of it, but leaves gaps, mostly around
  the microcontroller's very fine pins.
- **Why route the main board by hand:** even a fully auto-routed main board would route the audio path
  poorly. My recommendation is to route it by hand in KiCad, starting with the audio path:
  input jack → gain → clipping → tone → output.

## Things to check against real parts before ordering

- **Jack spacing:** the five jacks along the top edge are very tight, with about 1 mm between bodies.
  A smaller 1/4" jack such as the Lumberg KLBM 3 would free about 10 mm (it needs a custom footprint).
- **Enclosure measurements:** I assumed walls about 3 mm thick and corner screw posts about 6.5 mm
  across. Both need checking against Hammond's 1590BB drawing.
- **Board stacking height:** the boards are about 11 mm apart, with the control board about 8 mm below
  the panel. This depends on the actual encoder and button heights.
- **Ring LEDs:** they sit below the panel, so each knob needs 11 small panel holes, or light pipes.
- **Encoders:** they use the Alps EC11E footprint. Confirm it matches the Bourns PEC11R drawing.
- **Audio capacitors:** C1–C5, C8, C9 and C12 must be C0G or film, not X7R.

## Decisions to make next session

1. **How to finish the main board:**
   - (a) route it by hand in KiCad (recommended), or
   - (b) keep trying the automatic router.
2. **Whether to save the generator scripts in this repo.** They are the Python scripts that produced
   the schematics and board layouts. Right now they live only in Claude's temporary session folder.
   - If that folder is lost, the design can still be edited by hand in KiCad, but it can no longer be
     regenerated from the scripts.
   - The routing scripts would overwrite any hand routing. Once routing by hand starts, the KiCad
     files become the master copy.
3. **Update the Flashover product document** to describe the two-board design.
