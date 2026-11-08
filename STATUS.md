# Where we are (Nov 8, 2026)

A pick-up note for the next session. The details are in each folder's README.

## Done and pushed

| Pedal / area | State |
|---|---|
| **Flashover** (`pedals/distortion/`) | Stage 1 breadboard: steps 1–4 done and checked (bias 4.3–4.4 V; gain stage 4.5 V on c11; D1 and D2 are 1N60P germanium, as the 1N34A was out of stock). **Step 5 (tone and output) is next.** Firmware drafted (`firmware/`, 57 simulated checks pass). Stage 2 planned (`kicad/FLASHOVER_BREADBOARD_STAGE2.md`), breakout board designed and ready to order (`kicad/Flashover_Breakout/fab/`), order sheet `Flashover Breadboard Stage 2 Order.docx`. **Nothing for stage 2 is ordered.** |
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
4. ~~Recovery buttons in the Product Documents~~ **Done Nov 6.** Tap Changer's document now says footswitch + knob push, and Switchyard's says both footswitches; Flashover's already said footswitch + Soft/Hard. The status lines and Tap Changer's model number are brought up to date.
5. ~~Stand-in Pico takes updates~~ **Done Nov 6.** It runs after the shared loader. `standin_next.uf2` is the same program reporting 0.1.1, so the Switchyard breadboard can try a real pedal update. The breadboard guide now says to load the `_full.uf2` files the first time.

6. ~~Tap Changer breadboard stage 2 guide~~ **Done Nov 6** (commit 434ca1e): `pedals/boost/TAP_CHANGER_BREADBOARD_STAGE2.md`. It borrows Flashover's stage 2 boards (Pico, breakout, Gain encoder, footswitch button), so it needs no new parts. A new firmware build, `tapchanger_breakout_debug`, uses the breakout's 10k Tone pot. Test 3 is the digital pot's distortion, Tap Changer's main risk. Flashover's stage 2 setup-mode step is also corrected: hold the button while plugging in the USB cable.

## Waiting on the breadboard (don't do yet)

- Flashover main board:
  - add the 6 missing decoupling capacitors on the digital pots (U7–U9);
  - choose the LED driver package (JLCPCB stocks only the QFN-44, and the board has the TQFP-48).
- Knob direction, knob feel (taper), relay clicks, digital pot distortion, LED colour and brightness.

## Waiting on you

- Build stage 1 step 5, tone and output (Flashover breadboard). That completes the audio path.
- Order stage 2 (JLCPCB breakout board + the order sheet), when you're ready.
- Review the open decisions in `pedals/distortion/FLASHOVER_MIDI_REFERENCE.md` and the firmware READMEs.

