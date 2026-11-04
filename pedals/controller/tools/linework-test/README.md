# Linework test tool

Started Nov 4, 2026. It's a web page that stands in for the phone app while testing Switchyard. It talks to Switchyard over USB MIDI using the browser's Web MIDI support (Chrome or Edge), and speaks the Linework Control Specification 0.2.

It can also run against a simulated loop on this computer: Switchyard and three stand-in pedals, running the real firmware core. That lets you try it with no hardware at all.

## What it does

- **Switchyard:**
  - shows Switchyard's info and the loop status, with where to look if the loop is broken
  - shows the current position on a red readout
  - Previous, Next and Jump
- **Pedals:**
  - Scan loop and Get pedal list
  - for any channel: program change, store the current settings as a preset, read a preset, and report settings
- **Setlist:** type one in a short text format and send it, then check it with Setlist info (see below).
- **Other brands:** send a program change or control change out of the other-brands jack.
- **Raw message:** send any bytes, written in hex.
- **Messages:** a log of everything sent and received, each in plain words with the hex underneath.
- **Simulated loop only:** Up and Down footswitch buttons, and buttons to pull out or plug back in any of the four loop cables.

## Using it

**With the simulated loop:**

```
make -C pedals/controller/firmware/test bridge
node pedals/controller/tools/linework-test/sim-server.js
```

Then open http://localhost:8765 and press **Use the simulated loop**.

**With the breadboard:** plug the Switchyard Pico into the computer. Open the same page in Chrome or Edge, either through `sim-server.js` as above or by opening `index.html` directly. Allow MIDI access when the browser asks. The browser asks for the "system exclusive" permission, which the Linework messages need. Then pick Switchyard as the MIDI in and out, and press **Connect**.

## Setlist text format

```
wrap: no
pedal A = scan 1          # the pedal at position 1 in the last scan
pedal B = serial 0A1B2C3D4E5F60718293
pedal S = other 3         # another brand's pedal on MIDI channel 3

1.1  A:1 S:1              # song 1 part 1: preset 1 on A, preset 1 on S
1.2  A:2 B:1
```

Presets (1-128) and channels (1-16) are counted the way players see them.

A pedal that isn't listed in a part stays as it is. After sending a setlist, Switchyard stays on its part number and sends nothing, as the spec says; press **Jump** to apply the part.

## Files

| File | What it is |
|---|---|
| `lw.js` | The protocol: message building and decoding, CRC-32, the setlist text format, and a client with sequence numbers, a 1 second timeout and three tries (as the spec asks). It has no browser code, so Node can test it too |
| `app.js`, `index.html`, `style.css` | The page |
| `sim-server.js` | Serves the page and connects it to the simulated loop (`firmware/test/bridge.c`). Plain Node, no packages |
| `test/run.js` | Checks `lw.js` against the real firmware core through the bridge: `node test/run.js` |

## Checks

On Nov 4, 2026, `test/run.js` passed all 27 checks: info, identity, scan, pedal list, presets through the loop, setlist upload and info, jump, stepping, Get position, the other-brands jack, a loop break and its repair, and errors in the setlist text.

The page was also driven in headless Chrome against the simulated loop, with no script errors: connect, info, scan, setlist upload, jump, the footswitch, Next, reading a preset, and pulling and replacing a cable.

It hasn't been tried with real hardware yet, because the Switchyard Pico firmware isn't built.
