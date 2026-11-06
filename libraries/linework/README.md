# Linework firmware library

The C code every Linework pedal shares. It's plain C with no Pico code (except in `pico/`), so the PC simulations run it unchanged. Moved here from `pedals/controller/firmware/core/` on Nov 5, 2026, when Flashover became its second user.

| File | What it does |
|---|---|
| `lw_proto` | The Linework Control Specification 0.2 basics: the message header (one constant, to change when a paid MIDI Association ID arrives), commands, 7-bit number packing, CRC-32 and a MIDI parser |
| `pedal` | A pedal's loop behaviour: merging, position count, identity, channel, settings report, 128 presets, hand-change reports (at most one per control every 20 ms), loop-break reports, firmware updates (commands 30–32), and saving all of it to flash |
| `pd_store` | The settings log in flash: 12-byte records in two 4 KB sectors, compacted when full, safe against power cuts |
| `lw_ui` | Panel helpers: the endless knobs' speed-up, the 11-LED rings (bar or dot), LED brightness, and pot ramps |
| `lw_fw` | Firmware updates and the boot decision: the flash layout, receiving an update into the download area, installing it at power-up, the update record, and the 7-bit packing of firmware data |
| `pico/ring.h` | A byte ring buffer for UART data |
| `pico/pedal_flash` | `pd_store`'s flash access on the RP2040, in the last 64 KB of flash |
| `pico/lw_midi` | The MIDI loop on UART0 with buffers, and the pedal's serial number from the flash chip's ID |
| `pico/lw_input` | Endless knobs (encoders, on GPIO interrupts) and debounced buttons |
| `pico/lw_chips` | The MCP45HV51 digital pots and the IS31FL3236A LED driver on I2C |
| `pico/lw_update` | `lw_fw` on the RP2040: the whole flash as one device, the update hooks, and starting the firmware |
| `pico/loader.c` | The loader program (below) |
| `pico/linework.cmake` | Build helpers: link a pedal's firmware after the loader, build its loader, merge the two into one `.uf2` |
| `tools/uf2_merge.py` | Merges `.uf2` files |
| `test/fw_sim.c` | The firmware update simulation (below) |

**Used by:**
- Switchyard's `standin` pedal (`pedals/controller/firmware/`), and Switchyard's own code uses `lw_proto`.
- Flashover (`pedals/distortion/firmware/`).
- Tap Changer (`pedals/boost/firmware/`).

A pedal describes its controls in `pd_hal`: the control change numbers, in the order its settings are reported, and each control's kind:
- **knob:** 0–127
- **switch:** 0 or 1, where any value of 1 or more counts as 1
- **on/off:** 0–63 off, 64–127 on

It also gives an `apply` callback, which the hardware follows on every change, and optionally the update hooks.

## Firmware updates and the loader

Added Nov 5, 2026. It is built and simulated, but not yet run on hardware.

**Flash layout (2 MB):**

| From | Size | What |
|---|---|---|
| 0x000000 | 32 KB | The loader. It runs first at every power-up and is never erased by an update |
| 0x008000 | 960 KB | The pedal's firmware, linked to run here |
| 0x0F8000 | 960 KB | Download area: a new firmware arrives here first |
| 0x1E8000 | 4 KB | Update record: "pending" and "installed" notes |
| 0x1F0000 | 64 KB | The pedal's settings (`pd_store`) |

**An update:**
1. **Firmware: begin** carries the pedal's serial, the size and the CRC-32. The pedal goes to bypass and erases only as much of the download area as it needs, a sector at a time in gaps on MIDI in. The acknowledgement comes once that is done; a data block sent sooner gets "busy".
2. **Firmware: data** blocks of 64 bytes go into the download area. Each block is acknowledged with its number. A damaged block gets "checksum failed", and Switchyard (or the app) sends it again. The pedal passes every block on round the loop, like any other message (this settles the open decision in `FLASHOVER_MIDI_REFERENCE.md`).
3. **Firmware: finish** checks the CRC-32 of the whole download:
   - If it doesn't match, the answer is "firmware check failed", and the old firmware carries on.
   - If it matches, the pedal writes a "pending" note, acknowledges, and restarts.
4. **The loader**, at power-up:
   - It checks the download again, copies it over the firmware, and checks the copy.
   - It writes an "installed" note and starts the new firmware.
   - The new firmware sends a fresh identity reply once, as the spec asks.

**What can go wrong:**
- **Power cut during the download:** the old firmware starts, because there's no pending note yet.
- **Power cut during the install:** the download is untouched, so the next power-up simply copies again.
- **Damaged firmware area:** every power-up checks the firmware against its installed note. If it has been damaged, the loader repairs it from the download area if that still holds the same version.
- **Nothing it can start:** the loader stays in **recovery mode**.

**Recovery mode** is a minimal Linework pedal running in the loader:
- It keeps the MIDI loop going and answers Switchyard as its model, with firmware 0.0.0.
- It keeps its channel, and it takes an update.
- The Link LED flashes quickly.
- You can force it by holding two buttons at power-up:
  - Flashover: footswitch + Soft/Hard.
  - Tap Changer: footswitch + knob push.

  Holding the footswitch alone still means setup mode, which the firmware handles.

**Data block format:** 64 bytes travel as 74 MIDI data bytes. Each group of 7 bytes is sent as one byte holding their top bits (bit *i* is byte *i*'s top bit), followed by their low 7 bits. The 64th byte is a group of one. The checksum `xx` is the sum of the 74 bytes, low 7 bits. Block numbers are two 7-bit bytes, low first.

**Speed:** if each block waits for its acknowledgement, 40 KB takes about a minute through a two-pedal loop, three times slower than the spec's 1.5–2 minutes for 200 KB. So the spec (Nov 6, 2026) lets the sender keep up to 4 blocks waiting for an answer. The pedal writes each block as it arrives, and its 2 KB output buffer (4 KB in Switchyard) has plenty of room for 4.

**Building:** each pedal's `CMakeLists.txt` uses `pico/linework.cmake`, which makes:
- `<pedal>_loader.uf2`: the loader alone (about 18 KB).
- `<pedal>.uf2`: the firmware alone, linked at 0x10008000. Copy it over USB once the loader is on.
- `<pedal>_full.uf2`: both together, for a blank chip or a new Pico.

## Tests

The tests live with their users:

| Suite | Checks |
|---|---|
| `pedals/controller/firmware/test` | 62 |
| `pedals/distortion/firmware/test` | 57 |
| `pedals/boost/firmware/test` | 31 |
| `test/fw_sim.c` here, run with `make -C test test` | 39 |

Run them all after changing anything here.

`fw_sim` sends a 40 KB firmware from the fake phone app through Switchyard to Flashover on a simulated 2 MB flash chip. It checks:
- the refusals: wrong serial, too big, data before Begin;
- "busy" during the erase, and that only 10 sectors are erased;
- a damaged block and its resend, and the block-number acks;
- the install and the announcement, with settings kept;
- a wrong CRC;
- power cuts during the download and during the install;
- repair from the download area, and recovery mode when both copies are damaged;
- an update from recovery mode, the recovery buttons, a blank chip, and the 7-bit packing.
