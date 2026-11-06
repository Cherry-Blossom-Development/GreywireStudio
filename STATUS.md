# Where we are (Nov 6, 2026)

A pick-up note for the next session. The details are in each folder's README.

## Done and pushed

| Pedal / area | State |
|---|---|
| **Flashover** (`pedals/distortion/`) | Stage 1 breadboard: steps 1–2 done and checked (bias 4.3–4.4 V); **step 3 in progress** (your marks in `FLASHOVER_BREADBOARD_STAGE1_HOLES.md` are uncommitted, on purpose). Firmware drafted (`firmware/`, 57 simulated checks pass). Stage 2 planned (`kicad/FLASHOVER_BREADBOARD_STAGE2.md`), breakout board designed and ready to order (`kicad/Flashover_Breakout/fab/`), order sheet `Flashover Breadboard Stage 2 Order.docx`. **Nothing for stage 2 is ordered.** |
| **Tap Changer** (`pedals/boost/`) | Schematics drafted. Firmware drafted (`firmware/`, Boost knob in dB, 31 checks pass). Breadboard parts not ordered. |
| **Switchyard** (`pedals/controller/`) | Schematic drafted. Firmware drafted (81 checks), with its own loader and updates over USB. Web MIDI test tool (41 checks), which can send firmware to any pedal or to Switchyard. Breadboard parts not ordered. |
| **Shared firmware** (`libraries/linework/`) | Protocol, pedal behaviour, settings in flash, knob/LED helpers, RP2040 drivers, and the **loader with firmware updates over MIDI** (39 checks, including power cuts and recovery mode). |

None of the firmware has run on real hardware yet.

## Next steps (terminal only), in the order suggested

1. ~~Update the Linework Control Specification~~ **Done Nov 6** (commit 851c750). Its **Decisions to approve** section lists 7 items for your yes/no:
   - Tap Changer's model number 01 01
   - the firmware data block format and checksum
   - firmware blocks passed on round the loop
   - two-option switches: any value of 1 or more means the second option (CC 81 Hard)
   - hand changes sent at most every 20 ms per control
   - recovery mode reports firmware 0.0.0
   - up to 4 firmware blocks in flight at once
2. ~~Web MIDI test tool: "send firmware"~~ **Done Nov 6** (commit 9dd729e). Tried in headless Chrome on the simulated loop: the real `flashover.uf2` (28 KB) took 12 s. Switchyard now refreshes a pedal's version when it introduces itself after an update, and scans again, because the restarted pedal has lost its place in the loop.
3. ~~Switchyard's own firmware updates over USB~~ **Done Nov 6.** Switchyard has its own loader with a USB recovery mode, and a flash layout that keeps the setlists at 1 MB. Approved: recovery mode is both footswitches held while plugging in, and the display shows `----` while erasing, then `--42` and so on.
4. **Possible next terminal steps:**
   - Put each pedal's recovery buttons into its Product Document, as the spec asks:
     - Flashover: footswitch + Soft/Hard;
     - Tap Changer: footswitch + knob push;
     - Switchyard: both footswitches.
   - Let the stand-in Pico take updates, so the Switchyard breadboard can try a pedal update for real.

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
