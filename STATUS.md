# Where we are (Nov 6, 2026)

A pick-up note for the next session. The details are in each folder's README.

## Done and pushed (latest commit 1851e83)

| Pedal / area | State |
|---|---|
| **Flashover** (`pedals/distortion/`) | Stage 1 breadboard: steps 1–2 done and checked (bias 4.3–4.4 V); **step 3 in progress** (your marks in `FLASHOVER_BREADBOARD_STAGE1_HOLES.md` are uncommitted, on purpose). Firmware drafted (`firmware/`, 57 simulated checks pass). Stage 2 planned (`kicad/FLASHOVER_BREADBOARD_STAGE2.md`), breakout board designed and ready to order (`kicad/Flashover_Breakout/fab/`), order sheet `Flashover Breadboard Stage 2 Order.docx`. **Nothing for stage 2 is ordered.** |
| **Tap Changer** (`pedals/boost/`) | Schematics drafted. Firmware drafted (`firmware/`, Boost knob in dB, 31 checks pass). Breadboard parts not ordered. |
| **Switchyard** (`pedals/controller/`) | Schematic drafted, firmware drafted (62 checks), Web MIDI test tool (27 checks). Breadboard parts not ordered. |
| **Shared firmware** (`libraries/linework/`) | Protocol, pedal behaviour, settings in flash, knob/LED helpers, RP2040 drivers, and the **loader with firmware updates over MIDI** (39 checks, including power cuts and recovery mode). |

None of the firmware has run on real hardware yet.

## Next steps (terminal only), in the order suggested

1. ~~Update the Linework Control Specification~~ **Done Nov 6** (uncommitted). It now has a **Decisions to approve** section listing 7 items for your yes/no:
   - Tap Changer's model number 01 01
   - the firmware data block format and checksum
   - firmware blocks passed on round the loop
   - two-option switches: any value of 1 or more means the second option (CC 81 Hard)
   - hand changes sent at most every 20 ms per control
   - recovery mode reports firmware 0.0.0
   - up to 4 firmware blocks in flight at once
2. **Web MIDI test tool:** add "send firmware", with up to 4 blocks in flight, as the spec now says.
3. **Switchyard's own firmware updates over USB**, reusing the loader.

## Waiting on the breadboard (don't do yet)

- Flashover main board:
  - add the 6 missing decoupling capacitors on the digital pots (U7–U9);
  - choose the LED driver package (JLCPCB stocks only the QFN-44, and the board has the TQFP-48).
- Knob direction, knob feel (taper), relay clicks, digital pot distortion, LED colour and brightness.

## Waiting on you

- Finish stage 1 step 3 onward (Flashover breadboard).
- Order stage 2 (JLCPCB breakout board + the order sheet), when you're ready.
- Review the open decisions in `pedals/distortion/FLASHOVER_MIDI_REFERENCE.md` and the firmware READMEs.

Tidy-up: `grep.exe.stackdump` in the repo root is a stray file and can be deleted.
