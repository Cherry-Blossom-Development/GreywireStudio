# Tap Changer breadboard, stage 1: step-by-step build

Nov 1, 2026

This builds the Tap Changer audio circuit on one 830-point breadboard, with an ordinary knob pot in place of the digital pot and a stomp switch in place of the bypass relay. The values come from `sim/TapChanger_circuit.inc`, and the parts are in `Tap Changer Breadboard Tayda Order.docx`. The wiring below was checked by script against that circuit, but nobody has built it yet. To see where everything goes, open `TAP_CHANGER_BREADBOARD_STAGE1_LAYOUT.html` or the picture `TAP_CHANGER_BREADBOARD_STAGE1_LAYOUT.png`. The same steps are also a tick-off checklist in the Claude Doc at https://claude.ai/artifact/KsizN1bWHvuWPb2jqyFpgn.

The only chip is a TL072 for now. It clips a little sooner than the OPA1678 planned for the finished pedal.

## Before you start

Lay the board flat with **row a at the top** and **column −2 on the left**. If your board prints row A at the bottom, turn it round so A is at the top.

**Line numbers aren't locations.** "Line 12" at the start of each item is only a tick-off number. The holes to use are the bold names.

**Hole names:** a name like **g23** means row g, column 23, using the numbers printed on your board. Column −1 is written in brackets, so **a(−1)** means row a, column −1.

**Rail holes have no printed name.** Each rail is one long connected row, so "top + rail, nearest column 3" just means any hole in that rail close to column 3. "Leftmost hole" and "rightmost hole" mean the end of the rail.

**What's connected inside the board:**

- In each column, holes a–e are joined to each other, and holes f–j are joined to each other. The groove in the middle separates the two halves.
- Each rail is joined along its whole length. On some boards a rail has a gap halfway along, shown by a break in the printed line. If yours does, put a short jumper across the gap on the top + rail, the top − rail and the bottom − rail before you start.

**What each rail carries:**

| Rail | Carries |
| --- | --- |
| Top + (red) | +9 V |
| Top − (blue) | GND (ground) |
| Bottom + (red) | Nothing; this build doesn't use it |
| Bottom − (blue) | GND |

**Part orientation:**

- **Chip:** the notch (or dot) marks pin 1. The chip straddles the middle groove with the notch pointing left.
- **Diode:** the striped end is the cathode. The line says which end goes where.
- **Electrolytic capacitors:** the long leg is +; the stripe on the side marks −.
- **Resistors, film and ceramic capacitors:** either way round.

**Pot lugs:** looking at the pot from the front, with the shaft toward you and the lugs pointing down, the lugs are 1, 2, 3 from left to right.

**Off-board parts** (pot, footswitch, LED, jacks) connect with a wire from the lug to the named hole. Solder the wire to the lug if you can. Alligator clips work if they can't touch each other, but the footswitch needs soldered wires.

**Parts in tight spots:** where a part has to reach further than its legs, bend the legs out. Make sure no bare legs touch each other.

Unplug the 9 V supply whenever you place parts. Plug it in only for the checks.

**If you're short of a part:**

| Line says | Use instead, in the same holes |
| --- | --- |
| R4, 1.1k | A 1k here, and a 100 ohm in place of the c22 to c18 jumper. Same result |
| R4, 1.1k, with no 1k either | A 330 ohm here, and a 330 ohm in place of the c22 to c18 jumper. Boost then goes up to about +24 dB |
| R2, 1k | A 100 ohm |
| C2, 100 pF | A 1 nF |
| R1 and R3, 2.2M | 1M each. This loads the guitar a little more, so use the real parts before judging tone |

## Step 1: Rails and power

- [ ] Line 1: Jumper wire: top − rail, leftmost hole, to bottom − rail, leftmost hole
- [ ] Line 2: Jumper wire: top − rail, rightmost hole, to bottom − rail, rightmost hole
- [ ] Line 3: Wire: DC jack sleeve lug (+, the outer barrel) to **a(−1)**
- [ ] Line 4: Wire: DC jack center-pin lug (−) to top − rail, nearest column 1
- [ ] Line 5: D3, 1N5817 diode: plain end in **b(−1)**, striped end in top + rail, nearest column 1
- [ ] Line 6: C7, 100 µF electrolytic: long (+) leg in top + rail, nearest column 3; striped (−) leg in top − rail, nearest column 3

Pedal supplies are center negative: the center pin is ground and the outer sleeve is +9 V. If you're unsure which jack lug is which, plug the supply's plug in (unpowered) and find out with the meter on continuity.

**Check:** plug in the supply. Meter on DC volts, black probe on a − rail.

- Red probe on the top + rail: about **8.6–8.8 V**. Write this number down.
- Red probe on the bottom − rail: **0 V**, which confirms both ground rails are joined.

Unplug before the next step.

## Step 2: The bias voltage

This makes VB, the voltage halfway up the supply that the audio is centered on.

- [ ] Line 7: Chip U1, TL072, notch pointing left: pin 1 in **f21**, pin 4 in **f24**, pin 5 in **e24**, pin 8 in **e21**
- [ ] Line 8: C10, 100 nF ceramic: top + rail, nearest column 20, to top − rail, nearest column 20
- [ ] Line 9: Jumper wire: **a21** to top + rail, nearest column 21
- [ ] Line 10: Jumper wire: **j24** to bottom − rail, nearest column 24
- [ ] Line 11: R7, 1M resistor: **b24** to top + rail, nearest column 26
- [ ] Line 12: R8, 1M resistor: **a24** to top − rail, nearest column 28
- [ ] Line 13: C6, 10 µF electrolytic: long (+) leg in **c24**; striped (−) leg in top − rail, nearest column 25
- [ ] Line 14: Jumper wire: **d22** to **d23**

**Check:** plug in. Black probe on a − rail. Give it about 20 seconds to settle, because C6 charges slowly.

- **b21:** the same as your rail reading, about 8.7 V.
- **a22:** about **4.3 V**, half your rail reading.
- **d24:** about 4.3 V as well. It can read a little low here, because the meter itself loads this point.

Unplug before the next step.

## Step 3: The boost stage

These parts sit on both sides of the chip. Nothing goes in **i21**: R4 passes over it.

- [ ] Line 15: Jumper wire: **c22** to **c18**. This carries VB to column 18
- [ ] Line 16: R4, 1.1k resistor: **i22** to **i18**
- [ ] Line 17: C3, 10 µF electrolytic: long (+) leg in **d18**, striped (−) leg in **g18**, across the groove
- [ ] Line 18: Wire: Boost pot lug 1 to **g22**
- [ ] Line 19: Wire: Boost pot lug 2 to **g21**
- [ ] Line 20: Short wire on the pot: lug 2 to lug 3

C3 has about 0 V across it, so it works either way round.

Clockwise is more boost.

There's no check on this step: this half of the chip won't settle until the input parts in the next step are in.

## Step 4: The input

The guitar comes in on the right and works left toward the chip.

- [ ] Line 21: R1, 2.2M resistor: **j30** to bottom − rail, nearest column 30
- [ ] Line 22: C1, 100 nF film: **h30** to **h27**
- [ ] Line 23: R2, 1k resistor: **g27** to **g23**
- [ ] Line 24: C2, 100 pF ceramic: **j23** to bottom − rail, nearest column 23
- [ ] Line 25: R3, 2.2M resistor: **h23** to **c23**, arching over the chip

R3 is the one part that crosses the chip. Bend its legs so the body sits above the chip, and make sure its bare legs touch nothing on the way.

**Check:** plug in, wait about 20 seconds, and put the red probe on **h21** (the same strip as chip pin 1). Expect about **4.3 V** at every Boost setting. A reading near 0 V or near 8 V means a wiring mistake around the chip. Don't measure chip pin 3 itself: the meter loads R3 and reads low even when it's right.

Unplug before the next step.

## Step 5: The output

The signal leaves the chip and keeps working left.

- [ ] Line 26: R12, 100 ohm resistor: **j21** to **j17**
- [ ] Line 27: C9, 1 µF film: **i17** to **i13**
- [ ] Line 28: R11, 100k resistor: **j13** to bottom − rail, nearest column 13

**Check:** plug in and put the red probe on **h13**. Expect **0 V**, because C9 blocks the 4.3 V from reaching the amp.

Unplug before the next step.

## Step 6: Footswitch, LED and jacks

The footswitch switches the guitar between the circuit and a straight wire from input to output (true bypass). Its third switch lights the LED when the boost is on.

**Footswitch lugs:** turn the footswitch over and look at the lugs. They form a 3 × 3 grid, numbered like a phone keypad: 1 2 3 across the top row, 4 5 6 across the middle and 7 8 9 across the bottom. Hold it with any edge as "top", as long as you keep the same view throughout. Each middle lug connects to the lug above it or below it, depending on the switch position.

- [ ] Line 29: Solder a short link on the switch: lug 7 to lug 9
- [ ] Line 30: Solder a wire: input jack tip lug to footswitch lug 4
- [ ] Line 31: Solder a wire: output jack tip lug to footswitch lug 6
- [ ] Line 32: Solder a wire: LED short leg (flat side) to footswitch lug 2
- [ ] Line 33: Wire: footswitch lug 1 to **g30**
- [ ] Line 34: Wire: footswitch lug 3 to **g13**
- [ ] Line 35: Wire: footswitch lug 5 to bottom − rail, nearest column 40
- [ ] Line 36: LED resistor, 4.7k: top + rail, nearest column 34, to **a35**
- [ ] Line 37: Wire: **b35** to LED long leg
- [ ] Line 38: Wire: input jack sleeve lug to bottom − rail, nearest column 42
- [ ] Line 39: Wire: output jack sleeve lug to bottom − rail, nearest column 44

Lug 8 isn't used.

**Check:** with the power off, set the meter to continuity (the beep setting). Probe the two jack tip lugs, then press the footswitch.

- In one position it beeps. That's bypass: the guitar goes straight through.
- In the other it doesn't. That's boost on.

Then plug in and stomp: the LED should light with the boost on. If it lights in the bypass position instead, the LED wire is on lug 8; move it to lug 2.

## Step 7: first sound

Full boost is about ten times louder than the plain guitar, so start low.

1. Turn Boost fully counter-clockwise, and start with the boost off (LED dark).
2. Turn the amp down. Plug the guitar into the input jack and the output jack into the amp, then plug in the supply. You should hear the plain guitar.
3. Stomp the boost on. With Boost fully down, the volume should barely change.
4. Turn Boost up slowly while playing. It climbs quickly: about +15 dB by halfway, and about +20 dB at the top.
5. Stomp on and off at a few settings, and listen for any change in tone besides the volume.

**What to listen for:**

- **Tone:** does anything besides the volume change when you stomp? This is what the pedal is meant to get right.
- **Clipping:** at full Boost, a TL072 is predicted to start clipping at about 0.29 V from the guitar, so hard picking or hot pickups may roughen the sound. Try it at about +12 dB, around a third of the way up, to hear the difference.
- **Hiss:** is it noticeable at full Boost with the guitar's volume down? Breadboards pick up more noise than a finished board.

**If something's wrong:**

- **No sound with the boost off:** check the lug 7 to lug 9 link and the jack wiring.
- **No sound with the boost on only:** recheck **h21** for about 4.3 V, then the wires on lugs 1 and 3.
- **Boost knob does nothing:** check the two pot wires and the short wire from lug 2 to lug 3. Also check C3, R4 and the c22 to c18 jumper.
- **Knob works backward:** the lug 1 wire and the lug 2 and 3 pair are swapped.
- **Loud hum:** usually a loose ground or a long input wire. Keep the input wire short, and away from the power wires and the output wire.
- **Squealing at high Boost:** move the input and output wires apart.
