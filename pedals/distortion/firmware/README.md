# Flashover firmware

Started Nov 5, 2026. This is the firmware for the RP2040 on Flashover's main board. It follows `../FLASHOVER_MIDI_REFERENCE.md` and the Linework Control Specification 0.2. The pin numbers come from the Controller sheet of the schematic (`../kicad/Flashover`). A Pico on a breadboard has all the same pins, so the same file runs on a stage 2 breadboard.

**It has not run on hardware yet.** It builds without warnings, and its behaviour is checked in a PC simulation (below).

| Folder | What's in it |
|---|---|
| `core/` | `flashover.c`: Flashover's own behaviour, in plain C with no Pico code |
| `pico/` | The hardware layer: the MIDI UART, the encoders and buttons, the I2C chips and the relays |
| `test/` | `fo_sim.c`: a PC simulation of Flashover on a loop with Switchyard and a stand-in pedal |
| `../../../libraries/linework/` | The Linework code shared with Switchyard's stand-in pedal and later pedals: the protocol, the pedal's loop behaviour, and the settings log in flash |

## What it does

**Linework (shared code).** All of this comes from the shared pedal code:
- Passes on everything it hears on MIDI in, and adds its own messages.
- Counts its position, answers identity requests (model 01 00, firmware 0.1.0) and takes a channel.
- Answers Report settings, Store preset and Read preset. Settings are sent in the order `14 gg 15 tt 16 ll 51 mm 50 oo`.
- Recalls a preset on a program change and reports the five settings back.
- Sends a Loop break report when MIDI in goes quiet.
- Values are tidied as they arrive:
  - Gain, Tone and Level: 0–127.
  - Soft/Hard (CC 81): 0 is Soft; any value of 1 or more is Hard.
  - On/off (CC 80): 0–63 is off and 64–127 is on. It's stored and sent as 0 or 127.

**The knobs.** Each click of an endless knob moves its setting by 1. Turned quickly, a click moves it by 2 (clicks under 80 ms apart) or by 4 (under 30 ms apart). Slowly, the full range of 128 steps takes about 5 turns; spun fast, a little over 1 turn. A hand change is sent to Switchyard straight away, then at most once every 20 ms per knob, always ending on the final value.

**The digital pots.** Each knob value 0–127 is spread across the pot's 256 positions. The mapping is linear, like the B-taper pots on the stage 1 breadboard.
- **Gain** turned up gives less resistance, so more gain.
- **Tone's** code runs backwards, because terminal A is on the dark side.
- **Level** is cut to 38% in Hard mode, but CC 22 still carries your own Level setting.

Every change ramps over a few milliseconds (up to 32 codes per ms), so the pots don't click or "zip".

**The relays, without clicks.** For on/off or Soft/Hard:
1. Level ramps down to 0, which takes up to 8 ms.
2. The relay switches.
3. After 10 ms for the relay to settle, Level ramps back up.

If another change arrives in the meantime, it's switched before Level comes back. A footswitch press takes about 25 ms in all.

**LEDs.**
- Gain and Level show a bar on their rings, and Tone shows a dot. The ends of the bar and the dot fade across the gap between two LEDs, so the rings move smoothly.
- The Soft, Hard and On LEDs show the setting.
- The driver's PWM runs at 22 kHz, so it can't be heard in the audio.
- The brightness is a placeholder (half), until the LED colour is chosen.

**Link LED.** Lit while three things are true: Switchyard has counted the pedal since power-up, it has a channel, and MIDI in has been heard within the last second.

**Power off and on.** The pedal starts up exactly as it was left: the channel, the sound and all 128 presets. At power-up:
1. Level starts silent, while the relays and the other pots go straight to their saved settings.
2. Level ramps up once the relays have settled.

**Setup mode.** Hold the footswitch while plugging in power.
- The Soft and Hard LEDs blink in turn.
- Turn Gain to pick a channel. Channels 1–11 light that LED on the Gain ring. Channels 12–16 light Gain LED 11 plus Tone LED 1–5 ("11 plus").
- Press the footswitch to save the channel and start normally.

## Settings in flash

The pedal keeps an 8 KB log in the last 64 KB of the 2 MB flash (`libraries/linework/pd_store.h`).
- **Why a log:** flash can only be erased 4 KB at a time, and an erase stops the chip for about 45 ms. So instead of rewriting the settings, each change is added to the end of the log as one 12-byte record.
- **When it saves:** the current sound is saved 2 seconds after the last change, so turning a knob writes once rather than hundreds of times. Presets and channels are saved straight away.
- **When the log fills:** after 200–340 changes (fewer when many presets are stored), the latest value of everything is copied into the other 4 KB sector. Its header goes in last, so a power cut halfway through leaves the old copy in use.
- **Erases at quiet moments:** erases are done in a gap of at least 20 ms on MIDI in. The UART's 32-byte buffer holds anything that arrives during the erase, such as an Active Sensing byte.

## Simulation

`test/fo_sim.c` runs the real code of Switchyard, Flashover and a stand-in pedal, linked by MIDI cables at 31,250 baud, with a fake phone app. Flashover's pots, relays, LEDs and flash are simulated, and every change to them is logged. All 57 checks pass:

- **Power-up:** a new pedal comes up silent and bypassed, with its knobs at noon.
- **Switchyard finds it:** position 1, channel 1, model 01 00; the Link LED lights.
- **Knobs from the app:**
  - The pot codes are right for Gain, Tone and Level.
  - No pot moves more than 32 codes in a millisecond.
  - A control change for another channel is passed on and ignored.
- **Relays:**
  - Level reaches 0 before a relay switches.
  - Hard cuts Level to 38%.
  - CC 81 127 means Hard; CC 80 64 is on and 63 is off.
  - Two quick changes both land.
- **Report settings:** the pairs come back in the right order.
- **By hand:**
  - A slow turn moves 1 a click and is reported once.
  - A fast turn speeds up, and its reports are limited to one every 20 ms.
  - The button and the footswitch report CC 81 and CC 80.
  - Level stops at 0.
- **Presets:** store the current sound and store from pairs; a program change sets the pots and sends five control changes back.
- **Power cycle:**
  - The channel, the sound and the presets come back; a change less than 2 s old is lost, as designed.
  - It comes up silent, then ramps.
  - Switchyard sees the loop break and repair.
- **Flash wear:**
  - 1,500 saved changes take 5 erases.
  - Everything survives the compactions.
- **Power cut mid-write:**
  - A record cut off halfway is skipped, and the log carries on.
  - A compaction cut off before its header is ignored.
- **Setup mode:**
  - The channel display works and stops at 16.
  - The LEDs blink.
  - The footswitch saves the channel, which survives a power cycle, and Switchyard keeps it at its next scan.
- **Loop break:** the Link LED goes off when MIDI in goes quiet, and comes back on after the repair.

## Building

The simulation needs only a C compiler (WinLibs GCC is installed through winget):

```
make -C test test
```

For the RP2040 you need the same tools as Switchyard: CMake, Ninja, the Arm GNU Toolchain, and the Pico SDK. The SDK is the submodule in `pedals/controller/firmware/lib/pico-sdk`, and both projects share it. To save building picotool again, point at the copy Switchyard's build made (leave `-Dpicotool_DIR` out and the first build makes its own):

```
cmake -S . -B build -G Ninja -Dpicotool_DIR=C:/Users/dalla/Repo/GreywireStudio/pedals/controller/firmware/build/_deps/picotool
cmake --build build
```

This gives `build/flashover.uf2`, about 30 KB of program. The board has no USB, so the first firmware goes in through the programming header (SWD, see Glossary). On a Pico breadboard, hold BOOTSEL while plugging it in and copy the file across.

## Not done yet

- **Firmware updates over MIDI** (commands 30–32) and **recovery mode**. Both need a small loader program that keeps two copies of the firmware. For now the pedal answers "unknown command".
- **Knob pushes** do nothing. The Product Document leaves open whether pressing a knob resets it.
- **LED colour and brightness**, and dimming for dark stages: there's no message for dimming yet.
- **Knob feel (taper).** Gain and Level are linear, to match the breadboard. A Distortion+ normally uses a log Gain pot. If linear feels bunched up at one end, a curve is a small change in `fo_pot_code`.
- **Things to check on the first board:**
  - That each knob turns the right way. If one doesn't, flip it in `ENC_REVERSE` in `pico/board.h`.
  - That the relays switch silently with the timings above.
  - That the pots work at 8.6 V, as the schematic notes say.

## Glossary

- **Digital pot (MCP45HV51):** a chip that acts as a pot, with a wiper set by the microcontroller to one of 256 positions. Code 00 puts the wiper at terminal B and FF at terminal A.
- **Encoder:** an endless knob that gives two on/off signals as it turns. The order in which they change gives the direction.
- **Ramp:** moving a value in small steps over a few milliseconds instead of one jump, which would click.
- **PWM:** dimming an LED by switching it on and off very fast; the share of time it's on sets the brightness.
- **SWD:** the RP2040's two-wire programming and debugging connection, on the board's programming header.
- **Log (in flash):** a list that is only ever added to; the newest record for each setting wins.
