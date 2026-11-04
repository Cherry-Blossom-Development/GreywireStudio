# Switchyard firmware

Started Nov 4, 2026. This is firmware for the stage 1 breadboard (`../SWITCHYARD_BREADBOARD_STAGE1.md`) and for the stand-in pedal. It follows the Linework Control Specification 0.2. The pin numbers are the same on the breadboard and the board schematic (`../kicad/`), so this code carries over to the board.

| Folder | What's in it |
|---|---|
| `core/` | The logic, in plain C with no Pico code, so it also runs on a PC |
| `pico/` | The Pico hardware layer: MIDI UARTs, USB MIDI, flash storage, footswitches, LEDs and display |
| `test/` | A PC simulation of a whole loop, which checks the core against the spec (`sim.c`), and a bridge that lets the test tool in `../tools/linework-test/` drive the same loop (`bridge.c`) |

`core/` holds three files:
- `lw_proto` holds the shared protocol pieces: the message header, the 7-bit number packing, CRC-32 (see Glossary) and a MIDI parser.
- `switchyard` is the controller itself.
- `pedal` is a Linework pedal's loop behaviour. The stand-in uses it, and it's meant to become the shared code for Flashover and Tap Changer.

## Two programs

| Program | Runs on | Does |
|---|---|---|
| `switchyard` | The Switchyard Pico | The controller: USB MIDI to the phone, the MIDI loop, the setlist, footswitches, LEDs and the song.part display |
| `standin` | The second Pico | Acts like a Linework pedal on the loop. Its USB serial port prints its position and channel, and typing 1-4 turns a "knob" so you can test hand-change reports |

## What works (checked in the simulation)

`test/sim.c` connects Switchyard and three stand-in pedals with simulated MIDI cables running at the real speed (31,250 baud), and adds a fake phone app. All 62 checks pass:

- **Power-up scan:** count, identify and assign channels; 3 pedals get positions 1-3 and channels 1-3.
- **Switchyard's own messages:** the identity reply (model 02 00), Switchyard info, the pedal list, and an unknown command getting result 01.
- **App to pedal through the loop:** Store preset, Read preset, and program changes all reach the right pedal, and the answers come back to the app. A knob turned by hand is reported to the app. Active Sensing never reaches the app.
- **Setlist upload:** the pedal table (Linework pedals by serial, other brands by channel), parts, and the CRC check at the end. A bad CRC is refused and the old setlist stays.
- **Stepping:** Up, Down and Jump send each pedal its program change. Other brands get theirs on their own MIDI out. Stopping at the end, or wrapping around, follows the setlist's setting. The display shows song.part.
- **Loop breaks:** a pulled cable between pedals 2 and 3 is reported as "look at pedal 2"; a pulled cable from Switchyard's MIDI out is reported as "look at that cable". When the cable goes back in, Switchyard reports the repair and scans again.
- **Two pedals on one channel:** a scan moves one of them to a free channel.
- **Power cycle:** Switchyard comes back on the saved part and resends its program changes.

## Not done yet

- **Running on a Pico.** Both programs build with no warnings (Nov 4, 2026), but neither has run on real hardware yet. Switchyard's firmware is about 37 KB.
- **Firmware updates** (commands 30-32). For now Switchyard is updated by holding BOOTSEL and copying the `.uf2` file. The stand-in answers firmware commands with "unknown command".
- **Phone charging** (stage 2): the USB-C power controller, the power role swap, and VBUS_EN. That needs a USB Power Delivery stack.
- **The stand-in keeps its channel and presets in memory only,** so it forgets them at power-off. Real pedals must keep them (spec: "Settings survive power off").
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

This gives `build/switchyard.uf2` and `build/standin.uf2`. To load one, hold the Pico's BOOTSEL button while plugging it into the computer, then copy the file onto the drive that appears. For the board, add `-DSWITCHYARD_BOARD=pcb` to the first command; that turns on the 9V sense input.

## Storage

The setlists and the current part live in flash from 1 MB upward. There are two setlist copies of 320 KB each, plus a 4 KB sector for the current part. A new setlist goes into the spare copy and only becomes active once its check at Setlist finish passes. The current part is saved 2 seconds after the last step, so a burst of steps causes only one flash write.

The firmware checks at startup that it doesn't reach into this area.

## Glossary

- **CRC-32:** a check number calculated from data. If any byte changes, the number changes. It's the same one ZIP files use.
- **UART:** the chip's serial port. MIDI is a serial signal at 31,250 bits per second.
- **BOOTSEL:** the button on the Pico. Hold it while plugging in, and the Pico appears as a USB drive for loading firmware.
- **`.uf2`:** the firmware file format the Pico's USB drive accepts.
- **Power role swap:** a USB Power Delivery message that swaps which side supplies power, while the phone stays in charge of the data.
