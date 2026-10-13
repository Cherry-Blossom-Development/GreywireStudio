# Flashover circuit boards: where we are (updated 2026-10-13)

## In short

The schematic is finished, and **both circuit boards are fully routed**: every connection is made, there are no
wiring errors, and both boards match their schematics. Before ordering, the silkscreen (printed labels) needs
tidying, and the boards need a final look-over.

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

Both boards are 4-layer. The control board's inner layers are ground and +5V planes. On the main board, In1 is
a solid ground plane and In2 is a ground pour that also carries some signals.

| Board | Connections still missing | Wiring errors (shorts or too-close tracks) |
|---|---|---|
| Control board | **0 — fully routed** (finished by hand, 2026-10-11) | 0 |
| Main board | **0 — fully routed** (2026-10-13) | 0 |

Only cosmetic silkscreen warnings remain on both boards (labels overlapping each other, copper or the edge).

- **Control board:** L7 takes a long way round, past the right-hand knob and back. It works; tidy it later if
  wanted.

## How the main board was finished (2026-10-12 to 10-13)

- **By hand, in KiCad:** the audio path (FX_OUT, C9, CLIP, plus C1 and C3), the supplies around U4's top pins,
  and U4's bottom-edge pins (RUN, LED_EN, SWDIO, SWCLK, MODE_BTN, +1V1).
- **Net class change:** "Logic power" (+3V3, +1V1) now uses 0.2 mm tracks and 0.45/0.2 mm vias. KiCad
  couldn't fit 0.3 mm tracks between the RP2040's 0.4 mm-pitch pins. `scripts/rules.py` matches.
- **Parts moved** (commit 0b4bf70):
  - The crystal group (Y1, R19, C29, C30) now sits right under U4's XIN/XOUT pins, cutting the crystal tracks
    from about 25 mm to under 6 mm. The old spread-out placement risked an unreliable clock.
  - The 3.3 V regulator U6 and its caps C31/C32 moved beside J7.
  - C22, C23 and C28 moved into the gaps this left.
- **The rest by script:** each track was found by a two-layer (plus In2) search on a scratch copy and checked
  with DRC before going in. The autorouter's tracks for GAIN/TONE on U4's crowded left side were pulled up
  and rerouted.
  - A short SCL segment by U9 was moved, so U9's pin 1 could reach +3V3 through a via.
  - A ground stitching via at (130.9, 120.0) rejoins a cut-off patch of ground fill.
- The planning helpers are in `scripts/build/` (`hr2.py` search, `plan2.py` ordering, `place.py` part moves).
  That folder is git-ignored; they're scratch tools, not part of the board.

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

1. **Look the main board over in KiCad**, especially around U4 and the new crystal and regulator spots.
2. **Tidy the silkscreen** on both boards: move overlapping labels clear of each other and of pads.
3. **Final checks before ordering:** DRC on both boards, then the 3D view for the stack-up and jack fit.
4. **Fabrication outputs:** Gerbers, drill files, BOM and placement files.
