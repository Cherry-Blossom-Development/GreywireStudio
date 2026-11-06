# Switchyard firmware

Started Nov 4, 2026. This is firmware for the stage 1 breadboard (`../SWITCHYARD_BREADBOARD_STAGE1.md`) and for the stand-in pedal. It follows the Linework Control Specification 0.2. The pin numbers are the same on the breadboard and the board schematic (`../kicad/`), so this code carries over to the board.

| Folder | What's in it |
|---|---|
| `core/` | The logic, in plain C with no Pico code, so it also runs on a PC |
| `pico/` | The Pico hardware layer: MIDI UARTs, USB MIDI, flash storage, footswitches, LEDs and display |
| `test/` | A PC simulation of a whole loop, which checks the core against the spec (`sim.c`), and a bridge that lets the test tool in `../tools/linework-test/` drive the same loop (`bridge.c`) |

`core/` holds `switchyard`, the controller itself. The code shared with the pedals is in `libraries/linework/` at the top of the repo (moved there on Nov 5, 2026, for Flashover):
- `lw_proto` holds the protocol pieces: the message header, the 7-bit number packing, CRC-32 (see Glossary) and a MIDI parser.
- `pedal` is a Linework pedal's loop behaviour, which the stand-in and Flashover both use.
- `pd_store` keeps a pedal's settings in flash.

## Two programs

| Program | Runs on | Does |
|---|---|---|
| `switchyard` | The Switchyard Pico | The controller: USB MIDI to the phone, the MIDI loop, the setlist, footswitches, LEDs and the song.part display |
| `standin` | The second Pico | Acts like a Linework pedal on the loop, and keeps its channel, presets and settings in flash like a real pedal. Its USB serial port prints its position and channel; typing 1-3 turns a "knob" and 4 switches it on or off, to test hand-change reports |

## What works (checked in the simulation)

`test/sim.c` connects Switchyard and three stand-in pedals with simulated MIDI cables running at the real speed (31,250 baud), and adds a fake phone app. All 81 checks pass:

- **Power-up scan:** count, identify and assign channels; 3 pedals get positions 1-3 and channels 1-3.
- **Switchyard's own messages:** the identity reply (model 02 00), Switchyard info, the pedal list, and an unknown command getting result 01.
- **App to pedal through the loop:** Store preset, Read preset, and program changes all reach the right pedal, and the answers come back to the app. A knob turned by hand is reported to the app. Active Sensing never reaches the app.
- **Setlist upload:** the pedal table (Linework pedals by serial, other brands by channel), parts, and the CRC check at the end. A bad CRC is refused and the old setlist stays.
- **Stepping:** Up, Down and Jump send each pedal its program change. Other brands get theirs on their own MIDI out. Stopping at the end, or wrapping around, follows the setlist's setting. The display shows song.part.
- **Loop breaks:** a pulled cable between pedals 2 and 3 is reported as "look at pedal 2"; a pulled cable from Switchyard's MIDI out is reported as "look at that cable". When the cable goes back in, Switchyard reports the repair and scans again.
- **Two pedals on one channel:** a scan moves one of them to a free channel.
- **Switchyard's own firmware update** (added Nov 6, 2026; 19 checks):
  - a Begin for another serial goes into the loop as before;
  - a data block during the erase is answered "busy";
  - a damaged block is answered "checksum failed";
  - every block is acknowledged with its number and ch 7F, and none of it goes into the loop;
  - the display shows the progress, the footswitches wait, and no false loop break is reported;
  - after the restart the loader has installed it, and the setlist, the part and the pedals are all still there;
  - a wrong CRC installs nothing;
  - an update abandoned for 10 seconds puts everything back to normal.

**After a pedal's firmware update** (added Nov 6, 2026; checked by `fw_sim` and the test tool's checks, not by `sim.c`): the pedal restarts and introduces itself with a fresh identity reply. Switchyard updates that pedal's firmware version in its pedal list. A restarted pedal has lost its place in the loop, so Switchyard also scans again; channels stay the same.
- **Power cycle:** Switchyard comes back on the saved part and resends its program changes.

## Not done yet

- **Running on a Pico.** Everything builds with no warnings (Nov 6, 2026), but none of it has run on real hardware yet. Switchyard's firmware is about 41 KB, and its loader about 25 KB.
- **The stand-in Pico doesn't take updates:** it answers firmware commands with "unknown command". The simulated stand-ins in `test/` do take them.
- **Phone charging** (stage 2): the USB-C power controller, the power role swap, and VBUS_EN. That needs a USB Power Delivery stack.
- **USB IDs:** the USB IDs are TinyUSB's development numbers. A free product ID from Raspberry Pi is needed before selling.

## Building

The PC simulation needs only a C compiler. WinLibs GCC is installed through winget. From this folder:

```
make -C test test
```

To build for the Pico you need the Arm GNU Toolchain, Ninja and CMake, which are all installed. CMake came through `pip install --user cmake`, so it lives in `%APPDATA%\Python\Python312\Scripts`. You also need the Pico SDK 2.3.1, which is a git submodule in `lib/pico-sdk`; after cloning the repo, run `git submodule update --init` there and in its `lib/tinyusb`. The first build also downloads and builds Raspberry Pi's `picotool`, which makes the `.uf2` files, using the WinLibs GCC compiler. Then:

```
cmake -S . -B build -G Ninja
cmake --build build
```

This gives:
- `build/switchyard_full.uf2`: Switchyard's loader and firmware together. Use it the first time, or after anything else has been on the Pico.
- `build/switchyard.uf2`: the firmware alone. This is the file the app (or the test tool) sends to update Switchyard over USB. Copying it on by BOOTSEL also works, once the loader is on.
- `build/standin.uf2`: the stand-in pedal.

To load a file, hold the Pico's BOOTSEL button while plugging it into the computer, then copy the file onto the drive that appears. For the board, add `-DSWITCHYARD_BOARD=pcb` to the first command; that turns on the 9V sense input.

## Firmware updates over USB

Added Nov 6, 2026. As the spec says, the app sends Switchyard the same firmware messages a pedal gets:
- **Begin** is addressed to Switchyard's own serial number. **Data** and **Finish** use ch 7F.
- Switchyard handles them itself (`core/sy_update.c`), and none of them go into the loop.

**During an update:**
- The display shows `----` while Switchyard erases, then `--` and the percentage sent, such as `--42`.
- The footswitches do nothing.
- Loop-break reports pause, because erasing flash can drop a few bytes from the loop.
- If the app goes quiet for 10 seconds, the update is abandoned and Switchyard carries on as before.

**Finish:** once the CRC checks out, Switchyard acknowledges and restarts. The loader then copies the new firmware into place and starts it. The USB connection drops for a moment, so the app reconnects. The setlist and the current part are kept.

**The loader** (`pico/sy_loader.c`) is Switchyard's own version of the pedals' loader, because Switchyard is updated over USB rather than the MIDI loop.
- **Recovery mode:** hold both footswitches while plugging in. The loader also stays in recovery mode by itself if there is no firmware it can start.
- **In recovery mode:**
  - It appears over USB as "Switchyard (recovery)".
  - It answers Switchyard info and the identity request with firmware 0.0.0.
  - It takes an update.
  - Both footswitch LEDs flash quickly.
  - The loop and the display are left alone.

**Flash layout:** the setlists stay at 1 MB, so Switchyard's firmware areas are smaller than a pedal's (`LW_LAYOUT_SWITCHYARD` in `libraries/linework/lw_fw.h`):

| From | Size | What |
|---|---|---|
| 0x000000 | 32 KB | The loader |
| 0x008000 | 480 KB | Switchyard's firmware |
| 0x080000 | 480 KB | Download area |
| 0x0F8000 | 4 KB | Update record |
| 0x100000 | 644 KB | Setlists and the current part (below) |

The firmware checks at startup that it fits its 480 KB.

## Storage

The setlists and the current part live in flash from 1 MB upward. There are two setlist copies of 320 KB each, plus a 4 KB sector for the current part. A new setlist goes into the spare copy and only becomes active once its check at Setlist finish passes. The current part is saved 2 seconds after the last step, so a burst of steps causes only one flash write.

The update record ends where this area starts, and the firmware checks that at startup.

## Glossary

- **CRC-32:** a check number calculated from data. If any byte changes, the number changes. It's the same one ZIP files use.
- **UART:** the chip's serial port. MIDI is a serial signal at 31,250 bits per second.
- **BOOTSEL:** the button on the Pico. Hold it while plugging in, and the Pico appears as a USB drive for loading firmware.
- **`.uf2`:** the firmware file format the Pico's USB drive accepts.
- **Power role swap:** a USB Power Delivery message that swaps which side supplies power, while the phone stays in charge of the data.
