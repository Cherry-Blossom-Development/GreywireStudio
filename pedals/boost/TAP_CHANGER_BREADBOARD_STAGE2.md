# Tap Changer breadboard, stage 2: the digital pot, the Pico and the endless knob

Nov 6, 2026

Stage 2 swaps the ordinary Boost pot on the stage 1 breadboard for the real thing:
- **The digital pot (MCP45HV51).** It sets the boost. This answers Tap Changer's main open question: whether the digital pot adds distortion to a clean signal (Product Document, "Risks to test").
- **An endless knob (encoder).** It is read by a Raspberry Pi Pico running the Tap Changer firmware (`firmware/`).
- **A push button** standing in for the soft-touch footswitch.

**No new parts are needed.** Tap Changer uses the same digital pot, LED driver and Pico pins as Flashover. Its Boost knob is wired like Flashover's Gain knob, and its footswitch like Flashover's footswitch. So this stage borrows Flashover's stage 2 boards (the Pico, the breakout board, the regulator and the Gain encoder) and plugs them into the Tap Changer audio board instead. The only change is the firmware.

Build Tap Changer stage 1 first (`TAP_CHANGER_BREADBOARD_STAGE1_BUILD.md`) and check it works with its ordinary pot. Flashover's stage 2 (`../distortion/kicad/FLASHOVER_BREADBOARD_STAGE2.md`) must also be built, because this stage borrows it.

| Stage | Adds | How |
| --- | --- | --- |
| 2 (this one) | Digital pot, Pico, encoder, footswitch button | Borrows Flashover's stage 2 boards |
| 3 | The Boost LED ring and the On LED | Borrows Flashover's stage 3 LEDs: Tap Changer's ring is Flashover's Gain ring (LED driver outputs 1–11), and On is output 36 |
| 4 | MIDI in and out | As Flashover's stage 4 |
| 5 | Bypass relay | As Flashover's stage 5 |

## Which digital pot

The breakout board has three digital pots:
- U7 is a 100k pot, at I2C address 3C.
- U8 and U9 are 10k pots, at addresses 3D and 3E.

Tap Changer needs a 10k pot. Its own main board puts that pot at address 3C, so its usual firmware talks to 3C.

For this breadboard there is a separate firmware build, **`tapchanger_breakout_debug`**, which talks to address 3D instead: the breakout's **Tone pot (U8)**. Everything else about it is the same as the real firmware, including the knob set in dB, which assumes a 10k pot.

## What moves

Unplug the 9 V supply and the USB cable first.

**1. The power link.** Flashover's stage 2 boards get +9 V and ground from the Flashover audio board. Move those two wires to the Tap Changer audio board instead:
- **Wire:** from where it plugged into the Flashover board's top + rail, to the Tap Changer board's top + rail, nearest column 3.
- **Wire:** from where it plugged into the Flashover board's top − rail, to the Tap Changer board's top − rail, nearest column 3.

The 9 V supply now plugs into the Tap Changer board's DC jack. The Flashover audio board is no longer connected to anything.

**2. The Flashover pot wires.** Unplug all nine digital pot wires from the Flashover audio board (breakout row 1, pins 3–11). Three of them come across to Tap Changer:

| Breakout pin | Goes to (Tap Changer board) | Was (stage 1 Boost pot) |
| --- | --- | --- |
| Row 1 pin 6, **TA** | **g22** | Lug 1 |
| Row 1 pin 7, **TW** | **g21** | Lug 2 |
| Row 1 pin 8, **TB** | **g21** | Lug 3 (joined to lug 2 on the pot) |

Leave the other six (the Gain pins 3–5 and the Level pins 9–11) unplugged at their free ends. Make sure their bare ends can't touch anything.

**3. The Boost pot.** Unplug its two wires from **g22** and **g21**, and keep it nearby for the comparison test below.

TA goes to the op-amp's − input (g22), and the wiper TW with end TB goes to its output (g21). That's the way round the firmware expects: turning the knob up moves the wiper towards B, which is more boost.

**4. The knob and the button.** Nothing to move:
- Flashover's **Gain encoder** (on GP6, GP7 and GP8) is Tap Changer's Boost knob.
- The **footswitch button** (GP15) is Tap Changer's footswitch.
- The Tone and Level encoders are simply unused.

The 3PDT footswitch on the Tap Changer board stays as the true bypass until stage 5, as on Flashover.

## Loading the firmware

Build output is in `firmware/build/`:
- **`tapchanger_breakout_debug_full.uf2`:** the loader plus the breadboard firmware, with a report on the Pico's USB serial port. Use this one.
- `tapchanger_breakout_debug.uf2`: the same firmware alone, for later updates once the loader is on.

1. Hold the Pico's BOOTSEL button while plugging its USB cable into the computer. A drive called RPI-RP2 appears. Copy `tapchanger_breakout_debug_full.uf2` onto it. The Pico restarts by itself.
2. Plug in the 9 V supply, then open the Pico's serial port as for Flashover (any serial terminal, for example PuTTY or the Arduino IDE's Serial Monitor). Within 2 seconds it prints:
   ```
   Tap Changer 0.1.0 debug
     Boost pot (address 3D) answered
     LED driver   answered
   ```
   **NOT ANSWERING** means the pot didn't reply. The pot needs the 9 V and 3.3 V supplies, and the SDA and SCL wires, as on Flashover.

**Going back to Flashover:** copy `flashover_debug_full.uf2` (from `../distortion/firmware/build/`) onto the Pico the same way, and move the wires back.

**Settings carried over:** both firmwares keep their settings in the same place in the Pico's memory. So the first time, Tap Changer starts with Flashover's last Gain setting as its Boost, and it may start switched on. That's expected: turn the knob and press the button to set it. The same happens when you go back to Flashover.

## Tests

The report prints a line for each change, for example:
```
boost  64 (+10.8 dB)  on   | pot  52 | ch -  pos 0
```
- **boost** is the setting, 0–127, with the boost it gives in dB.
- **on / off** is the firmware's on/off setting.
- **pot** is the code sent to the digital pot, 0–255. The firmware works it out from the dB, so it doesn't rise evenly with the setting.
- **ch** and **pos** stay at "-" and 0 until MIDI arrives in stage 4.

**Test 1: the knob.**
- **Direction:** clockwise should raise the number. If it goes the wrong way, it's the same one-word change as on Flashover (`ENC_REVERSE`, this time in `firmware/pico/board.h`), and a rebuild.
- **Speed:** a slow turn moves 1 per click, and a quick spin moves faster.
- **Range:** the boost runs from +1.1 dB at 0 to +20.2 dB at 127, with +10.8 dB at 64. Each click should sound like the same small step all the way round. That even feel is the point of setting the knob in dB.

**Test 2: on and off.** Press the button. The report switches between on and off. You'll hear a brief dip in volume: the firmware turns the boost down, switches the bypass relay, then turns it back up. Until stage 5 there's no relay, so the 3PDT is still what really switches the effect.

**Test 3: the digital pot's sound.** This is the important one. The finished pedal stands or falls on it.
1. Unplug the USB cable and run from the 9 V supply alone, so there's no hum from the computer. The settings stay.
2. With the 3PDT on, play clean through a clean amp. Try three settings:
   - about noon (+10.8 dB);
   - full boost (127, +20.2 dB), picking hard;
   - near zero.

   Full boost is the hardest case, because the signal across the pot is largest there.
3. **Listen for:**
   - roughness or fuzz on loud notes or chords, especially as they die away;
   - any whine, buzz or ticking that wasn't there in stage 1;
   - clicks or a "zipper" sound while turning the knob. The firmware moves the pot in small steps to avoid this.
4. **Compare with the ordinary pot.**
   1. Unplug the 9 V supply.
   2. Take TA out of g22 and TW and TB out of g21, and plug the stage 1 Boost pot's wires back in (lug 1 in **g22**, lug 2 in **g21**).
   3. Plug in again and set the pot to the same loudness by ear.
   4. Play the same things. Swap back and forth a few times.
5. **Write down what you hear.**
   - If the digital pot sounds the same, the design stands.
   - If it adds roughness at high boost, the Product Document's fallbacks come next: a different digital pot, or a smaller boost range.

   Either way, it's worth telling me what you heard.

**Test 4: memory.** Set the knob, wait 3 seconds, then unplug everything and plug back in. The report should show the same setting.

**Test 5: setup mode.**
1. Unplug the 9 V supply and the USB cable.
2. Hold the footswitch button while plugging the USB cable back in, then plug in the 9 V.

   The Pico has to start up with the button held, and with USB connected so the report can be seen.
3. The report says "setup mode". Turn the knob to pick a channel, and press the button to save it.

**Recovery mode** (footswitch button and knob push held at power-up) has nothing to show until stage 4: it waits for an update over MIDI, and it doesn't use the USB report.

## If Flashover's stage 2 isn't available

To build this stage on its own, without borrowing, you need the same parts as Flashover's stage 2 (`../distortion/kicad/FLASHOVER_BREADBOARD_STAGE2.md`, "Parts list"), with two differences:
- **One encoder** instead of three. Leave out its Tone and Level filter parts.
- **The spare breakout board** from the Flashover order, which makes two assembled boards. You'll have to solder its headers.

The wiring is then Flashover's stage 2 wiring for the Gain encoder, the footswitch button, the power and the breakout, plus the three pot wires above.

## Glossary

- **Digital pot:** a chip that works like a pot, with two ends (A and B) and a wiper (W), set by the Pico over I2C instead of by hand. This one has 256 positions.
- **Rheostat:** a pot used as a variable resistor, with its wiper joined to one end. The Boost pot is used this way, as the stage 1 pot was.
- **dB (decibel):** the usual measure of loudness change. +6 dB is about twice the signal voltage, and +20 dB is ten times.
- **I2C address:** each chip on an I2C bus answers to its own number. The three pots on the breakout are set to 3C, 3D and 3E by how their address pins are wired.
- **Zipper noise:** small clicks heard when a digital control jumps between steps while a signal passes through it.
