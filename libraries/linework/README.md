# Linework firmware library

The C code every Linework pedal shares. It's plain C with no Pico code (except in `pico/`), so the PC simulations run it unchanged. Moved here from `pedals/controller/firmware/core/` on Nov 5, 2026, when Flashover became its second user.

| File | What it does |
|---|---|
| `lw_proto` | The Linework Control Specification 0.2 basics: the message header (one constant, to change when a paid MIDI Association ID arrives), commands, 7-bit number packing, CRC-32 and a MIDI parser |
| `pedal` | A pedal's loop behaviour: merging, position count, identity, channel, settings report, 128 presets, hand-change reports (at most one per control every 20 ms), loop-break reports, and saving all of it to flash |
| `pd_store` | The settings log in flash: 12-byte records in two 4 KB sectors, compacted when full, safe against power cuts |
| `pico/ring.h` | A byte ring buffer for UART data |
| `pico/pedal_flash` | `pd_store`'s flash access on the RP2040, in the last 64 KB of flash |

**Used by:**
- Switchyard's `standin` pedal (`pedals/controller/firmware/`), and Switchyard's own code uses `lw_proto`.
- Flashover (`pedals/distortion/firmware/`).
- Tap Changer, later.

A pedal describes its controls in `pd_hal`: the control change numbers, in the order its settings are reported, and each control's kind:
- **knob:** 0–127
- **switch:** 0 or 1, where any value of 1 or more counts as 1
- **on/off:** 0–63 off, 64–127 on

It also gives an `apply` callback, which the hardware follows on every change.

The tests live with their users: `pedals/controller/firmware/test` (62 checks) and `pedals/distortion/firmware/test` (57 checks). Run both after changing anything here.
