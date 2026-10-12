# Flashover circuit boards: where we are (updated 2026-10-12)

## In short

The schematic is finished and the parts are placed on two circuit boards. Wiring the boards together
("routing") is partly done: the control board is nearly finished, but the main board still needs
substantial work. Nothing is ready to order yet.

## What exists

Two KiCad projects, one per board:

| Folder | Board | What's on it |
|---|---|---|
| `Flashover/` | Main board | Audio circuit, microcontroller, digital pots, MIDI, bypass relay, and all five jacks: Output, DC and Input on the top edge, MIDI Out and MIDI In on the left and right edges |
| `Flashover_Controls/` | Control board | The three knobs with their LED rings, the LED driver, the Soft/Hard button and the four panel LEDs |

- **Enclosure:** Hammond 1590BB. Output, DC and Input are on the top wall; MIDI Out is on the left wall and
  MIDI In on the right.
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
| Control board | **0 — fully routed** (last 7 by hand, 2026-10-10) | 0 |
| Main board | 60 (hand-routing in progress, see below) | 0 |

- **Control board:** finished. The last 7 connections, all around the LED driver chip (U10), were
  routed by hand. Only cosmetic silkscreen (label) warnings remain, to be tidied before ordering.
  - L7 takes a long way round, past the right-hand knob and back. It works; tidy it later if wanted.
- **Main board:** the automatic router (Freerouting) gets most of it, but leaves gaps, mostly around
  the microcontroller's very fine pins.
- **Why route the main board by hand:** even a fully auto-routed main board would route the audio path
  poorly. My recommendation is to route it by hand in KiCad, starting with the audio path:
  input jack → gain → clipping → tone → output.

## Main board hand-routing: progress (2026-10-12)

We're routing the main board by hand in KiCad, one connection at a time, starting with the audio path.
Coordinates below are KiCad's own (the X/Y readout at the bottom of the KiCad window). After each save, Claude
checks the board with `scripts/drc.sh`.

**Done (62 → 58 open):**
1. **Net-(C1-Pad2):** R2 pad 1 → via (170.8, 86.0) → B.Cu → the router's via at (177.99, 84.19).
2. **Net-(C3-Pad1):** U7 pin 11 → via (166.5, 83.6) → B.Cu around the end of the R4 track → via (175.6, 80.1) →
   C3 pad 1. KiCad's router picked this path itself; it's clean.

3. **FX_OUT:** via (110.52, 88.21) → B.Cu along Y 87.6 → via (121.335, 87.6) → R11 pad 1. Clean.

4. **Net-(C9-Pad1):** U9 pin 12 → F.Cu through the gap → via (121.9, 83.75) → B.Cu along Y 87.2 → via
   (106.4, 86.3) → C9 pad 1. Clean.

**Next: connection 5, /Clipping/CLIP (tested on a scratch copy: 0 errors).** All on F.Cu, no new vias:
- Press X, click the via at (134.12, 101.15). Click (135.67, 102.7), then (167.45, 102.7).
- Click C5 pad 1 (171.62, 98.53). Esc twice, then save.
- It runs just below the clipping diodes, about 9 mm from U4, the flash and the crystal.

**Still open after these:** about 40 connections
around the microcontroller (U4), mostly +3V3, the flash memory, the crystal and the board connector J7.
Remaining dangling vias are autorouter leftovers to delete or connect along the way.

## Fit check against the datasheets (2026-10-11)

I checked the design against the manufacturers' drawings: the Hammond 1590BB, Bourns PEC11R, Neutrik NRJ4HF,
CUI SJ1-3523N and CUI PJ-102AH.

**Fixed:**
- **Encoder footprint:** the PEC11R's mounting legs are 13.2 mm apart, not the 11.2 mm of the Alps
  footprint we had borrowed. I made a correct footprint (`libraries/GreywireStudio.pretty`) and swapped
  it onto the control board. Three tracks were bent around the wider legs; the control board is still
  fully routed and clean.
- **Encoder part:** changed to **PEC11R-4220F-S0024** (20 mm shaft). Its 7 mm threaded collar leaves
  room for the washer and nut on the 2.25 mm-thick panel.
- **Control board height:** must sit **6.5 mm** below the inside of the panel (the encoder bodies
  press against the panel, nuts outside), not 8 mm.
- **Jacks:** the 1590BB's walls are thick, about 3 mm, and drafted (thicker towards the panel).
  To make the fit roomy:
  - The **MIDI jacks moved to the side walls**, so the top wall has just three jacks, about
    13 mm apart.
  - Every jack front now sits 0.2 mm from the inside of the wall.
  - The small jacks get **wider holes on the outside** (shallow pockets), so plugs can seat fully.
    See the drilling guide below.

**Board stack (measured from the top of the panel):**

| Layer | Depth |
|---|---|
| Panel (top of the box) | 0 – 2.25 mm |
| Control board | top at 8.75 mm (6.5 mm below the inside of the panel), 1.6 mm thick |
| Gap between boards | 11 mm: an 8.5 mm-tall 2x10 socket (J7) plus a standard pin header (J8). Use 11 mm M3 spacers |
| Main board | top at 21.35 mm, 1.6 mm thick; about 9 mm clear above the lid |

**Still to check with real parts in hand:**
- **Ring LEDs:** they sit below the panel, so each knob needs 11 small panel holes, or light pipes.
- **Soft/Hard button:** the 13 mm switch will stick up about 4.25 mm above the panel. Choose a cap to suit.
- **Wall thickness:** measure a real box. If the walls are thinner than the drawing suggests, the pockets
  below can be shallower.
- **Audio capacitors:** C1–C5, C8, C9 and C12 must be C0G or film, not X7R.

## Enclosure drilling guide

**How positions are measured:**
- Looking down at the top of the pedal, with the jack wall at the top.
- "Across" is measured from the box's left outside edge; "down" from the top wall's outside face.
- "Depth" is measured from the top (panel) face of the box, down the wall.

| Hole | Wall | Position | Depth (hole centre) | Hole | Outside pocket |
|---|---|---|---|---|---|
| Output (1/4", J2) | Top | 21.4 mm across | 14.9 mm | 11.5 mm | none (threaded nut) |
| 9V DC (J3) | Top | 47.8 mm across | 14.9 mm | 8 mm | 13 mm wide, 2.3 mm deep |
| Input (1/4", J1) | Top | 75.5 mm across | 14.9 mm | 11.5 mm | none (threaded nut) |
| MIDI Out (3.5 mm, J6) | Left | 72.5 mm down | 18.9 mm | 6.5 mm | 11 mm wide, 1.9 mm deep |
| MIDI In (3.5 mm, J5) | Right | 72.5 mm down | 18.9 mm | 6.5 mm | 11 mm wide, 1.9 mm deep |
| Knobs (encoders) | Panel | Gain 22, 46 · Tone 47, 68 · Level 72, 46 (across, down) | — | 7.5 mm | — |

- **Outside pockets:** a pocket is a shallow, flat-bottomed widening of the hole on the outside of the
  wall, made with a larger drill or a counterbore bit to a set depth. It lets the plug's moulded body
  sit inside the wall, so the plug reaches all the way into the jack.
- **Hole sizes:** they are generous on purpose. The 1/4" jacks are clamped by their nuts, and the small
  jacks are held by the board, so a little play makes assembly easier.
- **Printable template:** `Flashover_drill_template.pdf` shows the top panel and the three jack walls at
  full size. Print it at 100% ("Actual size") and check that its 100 mm line measures 100 mm.
  `scripts/drill_template.py` redraws it.
- **Before drilling:** confirm every position against the real box and boards.

## Done on 2026-10-10

- **Generator scripts saved** in `scripts/`, with a README explaining how to run them. Run from
  there, they reproduce the committed schematics exactly.
  - Once routing by hand starts, the KiCad files become the master copy. Don't re-run
    `make_boards.sh` after that: it overwrites the boards.
- **Product document updated** for the two-board design: a new "Circuit boards" section, and the
  sheet table now shows which board each sheet is on.
- **Tried to help the autorouter:** I turned the main board's In2 layer into a 3.3V plane, because
  half of the missing connections were 3.3V.
  - It made things worse, 70 missing connections instead of 50, including parts of the audio path.
    Losing In2 as a routing layer cost more than the plane saved.
  - I reverted it. The boards are as committed on 2026-10-09.
- **Conclusion:** the automatic router has gone as far as it usefully can on the main board.

## Next

**Hand-route the main board** in KiCad, together, the same way as the control board: one connection
at a time, with a check after each save. Start with the audio path:
input jack → gain → clipping → tone → output.

How to route by hand in KiCad (what worked on the control board):
- Use the **Design Rules Checker → Unconnected Items** list to find each gap.
- Set the grid to **0.1 mm** so the cursor can reach the fine-pitch pads.
- **Page Up** selects the top layer (F.Cu, red); **Page Down** the bottom (B.Cu, blue).
- Drawing a track:
  - **X**, then click the start, to begin.
  - Click to fix a corner.
  - **V**, then click, drops a via and changes layer.
  - **Esc** twice to stop.
- Surface-mount pads are only on the top layer, so a track arriving on the bottom needs a via
  just before the pad.
- If clicking opens the footprint chooser, a tool is on: press **Esc** until the arrow is selected.
