# Switchyard Audio: planning notes

Oct 18, 2026 · Status: early planning, nothing decided yet

Switchyard Audio is a second version of the Switchyard control pedal that also carries the guitar signal and adds digital effects to it. It sits at the end of the pedalboard, after the analog Linework pedals (Flashover and later ones), so the analog pedals shape the tone and Switchyard Audio adds effects such as delay and reverb on top before the signal goes to the amp.

It keeps everything Switchyard does: the phone connection, the two footswitches, the song.part readout, the MIDI loop to the Linework pedals and the MIDI out for other brands. The original control-only Switchyard (`pedals/controller/`) stays as it is. Whether we build one or both versions will be decided later, mostly on cost.

Related documents: `General Concept.docx` (the "Fully Integrated Mobile Device" section), `pedals/controller/Switchyard Product Document.docx`, `Linework Control Specification.docx`.

## Signal flow

```
Guitar → analog Linework pedals → Switchyard Audio → amp
                                   │
                                   ├─ input buffer
                                   ├─ dry path ─────────────────────┐
                                   └─ converter → effects → converter ┴→ mixer → output buffer → amp
                                   (bypass relay: input wired straight to output when off or unpowered)
```

The clean ("dry") signal stays analog the whole way through. Only the effect sound (the "wet" signal) is digital, and it's mixed back in at the output. See "Dry path" and "Wet signal" in the Glossary.

## Decision 1: where the effects are computed

**Decided Oct 18, 2026: option A, in the pedal.** This replaces the General Concept's plan of running reverb, delay and similar effects in the phone app. The comparison is kept below for the record.

| | A. In the pedal | B. In the phone (General Concept hybrid) | C. Both |
|---|---|---|---|
| How it works | A processor inside Switchyard Audio makes the effect sound | The pedal sends the guitar to the phone over USB audio; the app makes the effect sound and sends it back | Core effects in the pedal; extra effects and recording in the phone |
| Phone unplugged on stage | Effects keep working | Effects drop out; dry signal continues | Core effects keep working |
| Delay of the effect sound | Fixed and tiny, about 1–2 ms | Depends on the phone: fine on iPhone, varies on Android | Fixed for core effects |
| Extra parts cost | About $15–35 (processor, converter, memory) | About $5–10 (USB audio converter) | Highest |
| Software work | Effects written once for one known chip | Effects written for iPhone and Android, and tuned for slow phones | Both |
| New effects after sale | Firmware update from the app | App update | Either |

**Recommendation: A, with USB audio added later if we want recording.** Switchyard's rule so far is that the pedal keeps working on stage when the phone is unplugged, and option A keeps that rule for the effects too. It also gives every player the same sound whatever phone they own. Effects can still be sold or unlocked through the app; the app just installs them as firmware instead of running them.

## Hardware outline

| Part | Notes | Est. cost |
|---|---|---|
| Control | The same RP2040 block as Switchyard: footswitches, readout, MIDI loop, USB to the phone. Keeps the firmware shared between both versions | $2–4 |
| Effects processor | An STM32H7 chip (480 MHz, with hardware for fast decimal maths), talking to the RP2040 over a serial link. See "DSP" in the Glossary | $8–15 |
| Delay memory | SDRAM chip for long delays and reverbs (a 1-second stereo delay alone needs about 400 KB) | $3–6 |
| Audio converter | 24-bit, 48 kHz codec: one chip that converts in both directions (see "Codec") | $2–6 |
| Analog input and output | Input buffer (1 MΩ, a normal pedal input), level matching into the codec, dry/wet mixer, output buffer | $2–5 |
| Bypass relay | Connects input straight to output when the pedal is off or loses power | $2–4 |
| Power | Separate clean supply for the audio parts; more current than Switchyard (see Power) | $3–6 |
| Everything from Switchyard | Footswitches, readout, jacks, USB-C, phone charging, as in the Switchyard Product Document | $25–50 |
| Enclosure | Larger than the 1590BB to fit the extra jacks; size to be decided | $10–20 |
| **Total parts** | | **about $57–116** |

For comparison, Switchyard's estimate is about $29–58. Retail is usually 3–4× parts.

**Two processors or one?** The STM32H7 has USB and could run the controls too, which would save the RP2040. Keeping the RP2040 means the control firmware is identical in both Switchyard versions, which matters while both are still candidates. Revisit for the production board.

## Prototyping

The **Electrosmith Daisy Seed** (about $30) is a small board with an STM32H7, a 24-bit codec and 64 MB of SDRAM. It comes with a free effects library (DaisySP), and several boutique pedal makers ship products built on it. That makes it the obvious first prototype: everything in option A except the analog parts and the controls.

**Prototype only, not for production.** Electrosmith's datasheet (v1.0.5) guarantees the Daisy Seed will be made and supported only until January 2027. Its schematics are published under the MIT license, so the production board would use the same chips (STM32H750, SDRAM, codec) laid out on our own board, and the prototype firmware carries straight over.

Proposed stages:

1. **Hear it.** Daisy Seed on a breadboard with input and output buffers, the bypass relay and a dry/wet mixer, running a simple delay. Put it after the Flashover breadboard. This answers whether you like the combined sound before any other work. Parts and connections: `SWITCHYARD_AUDIO_BREADBOARD_STAGE1.md`.
2. **Control it.** Connect it to a Switchyard breadboard (the Pico circuit in `pedals/controller/SWITCHYARD_BREADBOARD_STAGE1.md`) over a serial link, so the footswitches and setlist change the effect.
3. **Phone.** Edit the effect settings from the app, using the Linework protocol.

Stage 1 doesn't need the Switchyard parts, so it can start whenever the Flashover breadboard is working.

## First effects

Start with a few done well, and add more through firmware updates:

- **Delay**, with its time set from the song's tempo in the setlist. Standalone delays need a tap-tempo switch for this; Switchyard Audio gets it for free.
- **Reverb**
- **Chorus**
- **Tremolo**

Pitch effects, a looper and amp simulation can come later. They need more processing and more design work.

## Fit with the Linework system

- Switchyard Audio lists itself as one of the pedals in each song part, so the setlist format and the "only distinct sounds" preset rule work unchanged.
- It needs its own model number in the Linework spec (Switchyard is 02 00; this could be 02 01).
- It follows the same rule as the analog pedals: settings come from presets, and hand changes are reported back.

## Power

The processor, memory and codec draw roughly 150–300 mA more than Switchyard. Together with the 2.5 W phone keep-alive charge (about 0.35 A from 9V), that's around 0.5–0.65 A. Many pedalboard supply outlets give 0.5 A, so we may need to make the phone charge optional or recommend a high-current outlet. To be worked out once the parts are chosen.

## Open questions

1. Production processor board: copy the Daisy Seed's open design, or buy a ready-made module if Electrosmith offers a longer-lived one.
2. Mono or stereo output? Stereo matters to many delay and reverb players, but adds a jack, a converter channel and cost.
3. Should the dry signal be switchable off ("kill dry") for players who run it in an amp's effects loop or in parallel?
4. A send/return loop, so some digital effects can go before the analog pedals (compressor, octave)? Probably a later version.
5. Recording to the phone over USB audio: in the first version or later?
6. Name: Switchyard Audio or Switchyard V2.
7. Enclosure size and jack layout.

## Glossary

**Codec:** a single chip that contains both an analog-to-digital converter (turning the guitar signal into numbers) and a digital-to-analog converter (turning numbers back into a signal).

**DSP (digital signal processing):** making or changing sound with maths on a processor. Effects like delay and reverb are DSP programs. A chip suited to it needs to be fast and handle decimal numbers in hardware, which the RP2040 can't.

**Dry path:** the route the original, unprocessed guitar signal takes through the pedal. Keeping it analog means the player's tone is never converted to digital, and the effect's small processing delay is never heard on the notes themselves.

**Kill dry:** a setting that removes the dry signal so only the effect sound comes out. Used when the pedal runs in parallel with another signal path that already carries the dry signal.

**SDRAM:** a type of memory chip that holds large amounts of data while powered. Delay and reverb effects store the recent sound in it.

**Send/return loop:** a pair of jacks that sends the signal out to other pedals and brings it back, so those pedals sit in the middle of this pedal's processing.

**Wet signal:** the sound produced by an effect (the echoes, the reverb tail), as opposed to the original dry signal.
