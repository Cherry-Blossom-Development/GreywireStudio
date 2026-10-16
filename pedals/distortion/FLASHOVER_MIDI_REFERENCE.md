# Flashover MIDI Reference

Oct 16, 2026

Exported from the Claude Doc at https://claude.ai/code/artifact/8f50dea1-ba3c-4b74-af05-f3ebc4ffcad4. Builds on `Linework Control Specification.docx` and `Flashover Product Document.docx`.

## How MIDI differs from an API

MIDI has no function calls and no return values. It is a stream of short one-way messages, each 1 to 3 bytes, sent at 31,250 bits per second (about 1 ms per 3-byte message). The sender never waits for an answer.

What this doc calls an "API call" is a message Switchyard (the controller pedal) sends. Most messages get nothing back. Where Switchyard needs an answer, it asks with a question message and the pedal sends a separate answer message later. The Linework loop carries that answer back to Switchyard.

- **Status byte:** the first byte of each message, which says what kind of message it is. It runs 80–FF in hex (128–255). See "Status byte and data byte" in the Glossary.
- **Data bytes:** the bytes after it, which carry the values. Every value runs 0–127; the Glossary explains why.
- **Channel:** most messages carry a channel number, 1–16, in the low half of the status byte. Each Linework pedal listens on its own channel, so up to 16 pedals share one cable. In the bytes, channel 1 is written as 0 and channel 16 as F.
- **System exclusive (SysEx):** a message of any length for a maker's own commands. It starts with F0 and ends with F7. Linework uses SysEx for setup, presets, settings dumps and firmware updates.

In this doc, bytes are written in hex (see "Hex" in the Glossary), and `n` stands for the channel minus 1. For example, `B2` is a control change on channel 3.

## Flashover's MIDI hardware

The main board has one MIDI in and one MIDI out, both on 3.5 mm TRS jacks, wired as on the `midi.kicad_sch` sheet. There is no MIDI thru jack: the firmware does that job (merging, below).

| Item | As designed |
| --- | --- |
| Jacks | MIDI in and MIDI out, 3.5 mm TRS, Type A wiring: tip = MIDI pin 5, ring = MIDI pin 4. One jack on each side wall |
| MIDI in | Through an H11L1 optocoupler (a part that passes the signal as light, so pedals stay electrically separate). Sleeve left unconnected to avoid ground loops. Diode D43 protects it if a reversed (Type B) cable is plugged in |
| MIDI out | 3.3 V version from the MIDI Association: 33 ohm from +3.3 V to the ring, 10 ohm from the chip to the tip. Sleeve grounded |
| Chip connection | UART0 on the RP2040: GPIO1 receives (MIDI_RX), GPIO0 sends (MIDI_TX). Standard MIDI speed: 31,250 baud, 8 data bits, no parity, 1 stop bit |
| Link LED | GPIO20, lit when the pedal is linked to Switchyard |

**The loop.** Switchyard's MIDI out goes to the first pedal's MIDI in, each pedal's out goes to the next pedal's in, and the last pedal's out goes back to Switchyard. Flashover passes on every message it receives and adds its own: a knob turned by hand, or an answer to a question. That is how every reply in this doc reaches Switchyard.

If Flashover loses power the loop stops at that pedal. The bypass relay still passes the guitar through, and Switchyard notices that its messages stop coming back.

## Sound messages: control change and program change

These six messages are fixed by the Linework Control Specification and the Flashover Product Document. Each is sent on Flashover's channel; Flashover ignores them on any other channel and passes them on unchanged.

| Message | Bytes Switchyard sends | What Flashover does | What comes back |
| --- | --- | --- | --- |
| Set Gain | `Bn 14 vv` (CC 20) | Sets the Gain digital pot; the Gain ring shows the new value | Nothing |
| Set Tone | `Bn 15 vv` (CC 21) | Sets the Tone pot and ring | Nothing |
| Set Level | `Bn 16 vv` (CC 22) | Sets the Level pot and ring | Nothing |
| Effect on/off | `Bn 50 vv` (CC 80) | 0–63 switches the bypass relay to bypass; 64–127 switches the effect on. Level dips for a few ms during the switch to stop a click. The On LED follows | Nothing |
| Clipping mode | `Bn 51 vv` (CC 81) | 0 = Soft, 1 = Hard. Switches the diode relay and the Soft/Hard LEDs | Nothing |
| Recall preset | `Cn pp` (program change) | Loads preset pp + 1 (0 = preset 1, 127 = preset 128): Gain, Tone, Level, mode and on/off | Five control changes with the new settings: CC 20, 21, 22, 81 and 80 |

`vv` is the value, 0–127. `14`, `15`, `16`, `50` and `51` are 20, 21, 22, 80 and 81 in hex.

**Hand changes go out the same way.** When a player turns a knob, presses Soft/Hard or stomps the footswitch, Flashover sends the matching control change on its own channel. A fast knob turn would send many messages, so the firmware should send at most one per knob every 10–20 ms.

**Level in Hard mode.** Flashover cuts Level to about 38% internally in Hard mode, to match loudness. CC 22 always carries the player's Level setting, never the cut value, so the knob and the app agree.

**Resolution.** CC values have 128 steps, but the MCP45HV51 pots have 257. The spec reserves CC 52–54 as fine-adjust partners for CC 20–22. Flashover can ignore them at first and map 0–127 across the full pot range.

**Messages Flashover ignores but passes on:** notes, pitch bend, aftertouch, MIDI clock, start/stop and all other CC numbers. Other brands' MIDI controllers can still use Flashover with the six messages above, since they're all standard MIDI.

## Setup and system messages (proposed bytes)

These are the messages that get answers. The spec lists them but leaves the bytes "to be written once the first prototype is working", so the layouts below are a first proposal. Every one is SysEx, framed the same way:

`F0 7D 4C 57 cc qq … F7`

- `7D` is the manufacturer ID MIDI sets aside for development and non-commercial use. A paid ID from the MIDI Association replaces it before sale.
- **The header changes length with a paid ID.** A paid ID is 3 bytes (`00 xx yy`), which moves every later byte 2 places. Firmware and Switchyard code should keep the header (`F0 7D 4C 57` for now) as one named constant and count byte positions from its end, never hard-code them.
- `4C 57` ("LW") marks the message as Linework, since other gear may also use `7D`.
- `cc` is the command; `ch` is a channel, 00–0F.
- `qq` is a sequence number, 00–7F. Switchyard counts it up with each request, going from 7F back to 00, and every answer repeats it. That way a lost or late answer is never matched to the wrong request. Pedal info answers the standard identity request, which has no `qq`, so it sends 00.
- **Serial** `s0…s9`: the 64-bit unique ID of the pedal's flash chip, split into ten 7-bit bytes.
- **Settings** `(pairs)`: a CC number then its value, one pair per control, using the same values as the matching control change. Flashover sends `14 gg 15 tt 16 ll 51 mm 50 oo`: Gain, Tone, Level (0–127), mode (0 Soft, 1 Hard) and on/off (0 off, 127 on). A pedal with other controls sends its own CC numbers, so Switchyard can read any Linework pedal without knowing its model. The `F7` marks where the pairs end.

| Message | Bytes Switchyard sends | What Flashover does | What comes back |
| --- | --- | --- | --- |
| Position count | `F0 7D 4C 57 01 qq kk F7`, starting at kk = 00 | Saves kk + 1 as its position, and passes the message on with kk + 1 in place of kk | The number that reaches Switchyard is the number of pedals |
| Identity request (standard MIDI) | `F0 7E 7F 06 01 F7` | Passes it on, then adds its own two answers | Standard identity reply `F0 7E dd 06 02 7D 4C 57 01 00 aa bb cc ss F7`, then pedal info `F0 7D 4C 57 02 qq pp dd s0…s9 F7` |
| Assign channel | `F0 7D 4C 57 03 qq s0…s9 ch F7` | If the serial is its own, saves the channel to flash and lights the Link LED | Acknowledgement |
| Report settings | `F0 7D 4C 57 10 qq ch F7` | Reads its current settings | `F0 7D 4C 57 11 qq ch (pairs) F7` |
| Store preset | `F0 7D 4C 57 20 qq ch pp ff (pairs) F7` | Writes preset slot pp. ff = 0 stores the pairs sent; ff = 1 stores the current sound, and the message carries no pairs | Acknowledgement |
| Read preset | `F0 7D 4C 57 21 qq ch pp F7` | Reads slot pp | `F0 7D 4C 57 22 qq ch pp (pairs) F7` |
| Firmware: begin | `F0 7D 4C 57 30 qq s0…s9 z0 z1 z2 c0…c4 F7` | Goes to bypass and erases the spare firmware copy. z = size in bytes, c = CRC-32 check value of the whole file | Acknowledgement once erased |
| Firmware: data | `F0 7D 4C 57 31 qq ch b0 b1 (74 bytes) xx F7` | Writes 64 bytes of firmware as block b0 b1. xx is a checksum of the block | Acknowledgement with the block number |
| Firmware: finish | `F0 7D 4C 57 32 qq ch F7` | Checks the CRC-32 of the whole copy. If it matches, it switches to the new copy and restarts | Acknowledgement, then a fresh identity reply after the restart |

In the identity replies, `dd` is Flashover's channel (`7F` until one is assigned), `01 00` is the Flashover model number, `aa bb cc` is the firmware version, `ss` is the spec version, and `pp` is the position.

**Acknowledgement:** `F0 7D 4C 57 7E qq ch cc st F7`, where `qq` repeats the request's sequence number, `cc` repeats the command and `st` is the result. Firmware data adds the block number before `F7`.

| st | Meaning |
| --- | --- |
| 00 | OK |
| 01 | Unknown command |
| 02 | Value out of range (such as a slot above 127) |
| 03 | Checksum failed; Switchyard resends |
| 04 | Busy; Switchyard waits and resends |
| 05 | Firmware check failed; the old version keeps running |

**Why 74 bytes for 64.** MIDI data bytes only go up to 127, but firmware bytes go to 255. So every 7 firmware bytes travel as 8 MIDI bytes, with the extra byte carrying the top bits. A block takes about 27 ms, so a 200 KB update takes about 1.5 to 2 minutes, matching the spec.

**Setup, start to finish:**

1. Position count: Switchyard learns how many pedals there are.
2. Identity request: each pedal answers with its model, version, position and serial, so the app knows the board order.
3. Assign channel: one per pedal, by serial; each acknowledges.
4. Report settings: each pedal sends its current sound, and the app shows it.

## Open decisions

No firmware exists yet, so these can be settled when stage 2 of the breadboard brings in the RP2040.

- [ ] Accept the SysEx byte layouts above, or change them, then copy them into the Linework Control Specification.
- [ ] CC 81 from other brands' controllers: many send 0 and 127 for a switch. Proposal: any value of 1 or more means Hard.
- [ ] Fine adjustment (CC 52–54): support it now for the pots' full 257 steps, or wait until 128 steps proves too coarse.
- [ ] Firmware data blocks: should Flashover pass them on like everything else, or drop the ones addressed to itself to save loop time?
- [ ] How fast to send hand changes: 10 ms or 20 ms between messages per knob.
- [ ] Apply a new manufacturer ID from the MIDI Association before sale (small annual fee).

## Glossary

### Bit, byte and top bit

A byte is 8 bits. Written out, the most valuable bit is on the left:

```
bit value:  128  64  32  16   8   4   2   1
example:      1   0   1   1   0   0   0   0   = 128 + 32 + 16 = 176
```

The **top bit** is the leftmost one, worth 128. It's also called the most significant bit or the high bit. It names a position inside a single byte. It is unrelated to endianness, which is the order of the bytes in a number that takes up several bytes.

### Status byte and data byte

MIDI uses the top bit of every byte as a label:

- **Top bit 1:** a **status byte**, which starts a new message. With the 128 bit set, its value is always 128 or more.
- **Top bit 0:** a **data byte**, a value inside the message. Only the other 7 bits are left, so the value is always 0–127.

| | Decimal | Hex | Binary |
| --- | --- | --- | --- |
| Status bytes | 128–255 | 80–FF | 1xxxxxxx |
| Data bytes | 0–127 | 00–7F | 0xxxxxxx |

This is why every MIDI value (a knob position, a CC number, a preset number) stops at 127: one of the 8 bits is used for the label.

The label exists because MIDI has no gaps or markers between messages, only a steady stream of bytes. A pedal plugged in partway through a message reads until it sees a byte with the top bit set, and knows a new message starts there. It never mistakes a value for the start of a message.

### Hex

Hex (hexadecimal) counts in sixteens, using the digits 0–9 and then A–F (A = 10, F = 15). Each hex digit stands for exactly 4 bits, so any byte is exactly two hex digits.

MIDI documents write status bytes in hex because a status byte holds two things, one in each hex digit:

```
 B    2
1011 0010
 │    └─ channel: 0010 = 2, which means channel 3
 └────── message type: 1011 = B = control change
```

So `B2` reads at a glance as "control change on channel 3". The same byte in decimal is 178, which hides both parts. Data bytes are single values, such as Gain = 100, so they're often given in decimal instead. This doc writes bytes in hex and settings in decimal, and gives both where it helps.
