# Flashover breadboard, stage 1: step-by-step build

Oct 15, 2026

This guide places every part of the stage 1 (Option A) circuit on one 830-point breadboard. The parts list and the connections it builds are in `FLASHOVER_BREADBOARD_STAGE1.md`. Each step ends with a check, so problems show up while they're easy to find.

For the bench, use `FLASHOVER_BREADBOARD_STAGE1_HOLES.md` instead: it lists every placement as one line with exact holes, numbered to match the columns printed on the breadboard (−2, −1, 1 … 61). This guide counts columns 1–63 from the left edge.

## How the breadboard works

- **Orientation:** lay the board flat with the column numbers along the top and **row a at the top**. Rows a–e form the **top half**, rows f–j form the **bottom half**, and the groove in the middle separates them.
- **Columns:** each numbered column is two separate strips. Holes a–e in one column are connected to each other (the top strip), and holes f–j are connected to each other (the bottom strip). This guide writes a top strip as **T** plus the column number and a bottom strip as **B** plus the number. For example, **T13** means any hole from a13 to e13, and **B20** means any hole from f20 to j20. Each strip has 5 holes.
- **Rails:** the long rows along the top and bottom edges are the rails. Each edge has a red (+) rail and a blue (−) rail, connected along their length. This build uses them as follows:

| Rail | Used for |
| --- | --- |
| Top red | **+9V** |
| Top blue | **GND** (ground) |
| Bottom red | **VB**, the 4.5 V bias that half the circuit works around |
| Bottom blue | **GND** |

  On some boards each rail is split in the middle; look for a gap in the printed red or blue line. If yours has a gap, bridge it with a short jumper on all four rails.

**Part orientation:**
- **Chips:** pin 1 is next to the notch or dot. Every chip goes across the middle groove with the **notch pointing left**. Pins 1–4 then run left to right along row f, and pins 5–8 run right to left along row e.
- **Diodes:** the striped end is the cathode.
- **Electrolytic capacitors:** the longer leg is +, and the stripe on the side marks −.
- **Everything else** (resistors, film and ceramic capacitors) can go either way round.

**Long parts:** where a part has to reach further than its legs, bend the legs out. Two resistors cross the middle groove and arch over the gain chip; that's fine as long as no bare legs touch.

**Off-board parts:** the three pots, the toggle switch, the footswitch, the LED and the three jacks sit off the board on wires. The footswitch needs soldered wires. The most reliable way is to solder a short length of solid jumper wire to each lug. Alligator clip leads also work, but check they can't touch each other.

**Pot lugs:** looking at the pot from the front, with the shaft toward you and the lugs pointing down, the lugs are 1, 2 and 3 from left to right. Lug 2 is the wiper.

## Step 1: rails and power

| Part | From | To |
| --- | --- | --- |
| Jumper | Top blue rail (left end) | Bottom blue rail (left end) |
| Jumper | Top blue rail (right end) | Bottom blue rail (right end) |
| DC jack + wire (the sleeve lug, the outer barrel) | T2 | |
| D3, 1N5817 | T2 (plain end, anode) | Top red rail (striped end, cathode) |
| DC jack − wire (the center pin lug) | Top blue rail | |
| C7, 100 µF | Top red rail (+ leg) | Top blue rail (− leg), near column 3 |
| C10, 100 nF ceramic | Top red rail | Top blue rail, near column 12 |
| C11, 100 nF ceramic | Top red rail | Top blue rail, near column 41 |

Pedal supplies are center negative. That means the barrel's center pin is ground and the outer sleeve is +9 V. If you're unsure which lug is which on your jack, find out with a multimeter on continuity mode and a plug inserted.

**Check:**
1. Plug in the 9 V supply.
2. Set the multimeter to DC volts. Put the black probe on any blue rail and the red probe on the top red rail. Expect about **8.6–8.8 V** (the diode drops a little).
3. Put the red probe on the bottom blue rail. Expect **0 V**, which confirms both ground rails are joined.

Unplug the supply before going on, and before every later step.

## Step 2: the 4.5 V bias (VB)

Insert the **TL072** (U2) at columns 40–43, notch left: pin 1 in f40, pin 4 in f43, pin 5 in e43, pin 8 in e40.

| Part | From | To |
| --- | --- | --- |
| Jumper | T40 (pin 8) | Top red rail |
| Jumper | B43 (pin 4) | Bottom blue rail |
| R7, 1M | T43 (pin 5) | Top red rail |
| R8, 1M | T43 | Top blue rail |
| C6, 10 µF | T43 (+ leg) | Top blue rail (− leg) |
| Jumper | T41 (pin 7) | T42 (pin 6) |
| R18, 100 | T42 | T46 |
| C15, 10 µF | T46 (+ leg) | Top blue rail (− leg) |
| Long jumper | T46 | Bottom red rail (this makes it the VB rail) |

**Check:** power up. With the black probe on ground, expect:
- **About 4.5 V** on T43, T41 and the bottom red rail. Give it a few seconds to settle, because C6 charges slowly through 1M.
- **About 8.7 V** on T40.

## Step 3: input and gain stage

Insert the **LM741** (U1) at columns 11–14, notch left: pin 1 in f11, pin 4 in f14, pin 5 in e14, pin 8 in e11. Pins 1, 5 and 8 aren't used, so nothing else goes in T11, T14 or B11.

| Part | From | To |
| --- | --- | --- |
| Jumper | T12 (pin 7) | Top red rail |
| Jumper | B14 (pin 4) | Bottom blue rail |
| R1, 1M | B3 | Bottom blue rail |
| C1, 10 nF | B3 | B6 |
| R2, 10k | B6 | B13 (pin 3) |
| C2, 1 nF | B13 | Bottom blue rail |
| R3, 1M | B13 | Bottom red rail (VB) |
| R5, 100k | B12 (pin 2) | T13 (pin 6), arching over the chip |
| R4, 330 | B12 | T8, across the groove |
| C3, 470 nF | T6 | Top blue rail |
| Gain pot lug 3 wire | T8 | |
| Gain pot lug 2 wire | T6 | |
| Short wire on the pot | Gain lug 1 | Gain lug 2 |

Clockwise is more gain.

**Check:** power up and expect **about 4.5 V** on T13 (pin 6) at any Gain setting. Readings well away from 4.5 V, such as near 0 V or near 8 V, mean a wiring mistake around the chip.

## Step 4: clipping and the Soft/Hard switch

| Part | From | To |
| --- | --- | --- |
| C4, 1 µF | T13 | T17 |
| R6, 10k | T17 | B20, across the groove |
| Jumper | B20 | B21 |
| R10, 1M | B20 | Bottom red rail (VB) |
| C5, 1 nF | B20 | Bottom red rail (VB) |
| Toggle center lug wire | B20 | |
| Toggle end lug A wire | B29 | |
| Toggle end lug B wire | B32 | |
| D1, 1N34A | B29 (plain end) | Bottom red rail (striped end) |
| D2, 1N34A | B29 (striped end) | Bottom red rail (plain end) |
| D4, 1N4148 | B32 (plain end) | Bottom red rail (striped end) |
| D5, 1N4148 | B32 (striped end) | Bottom red rail (plain end) |

B20 is now full (5 holes); B21 continues the same connection.

- Lug A, B29, is **Soft** (germanium).
- Lug B, B32, is **Hard** (silicon).

On a toggle the lever points away from the lug it connects, so label it once you can hear which is which. The 1N34A diodes are glass and fragile: bend their legs gently, away from the body.

**Check:** power up and expect **about 4.5 V** on B20 in both switch positions.

## Step 5: tone, output amplifier and Level

| Part | From | To |
| --- | --- | --- |
| C12, 22 nF | B21 | B23 |
| R15, 6.8k | B23 | Bottom red rail (VB) |
| R14, 6.8k | B21 | B26 |
| C8, 100 nF film | B26 | Bottom red rail (VB) |
| Tone pot lug 1 wire | B26 | |
| Tone pot lug 3 wire | B23 | |
| Tone pot lug 2 wire | B42 (TL072 pin 3) | |
| Jumper | B40 (pin 1) | B37 |
| R16, 30k | B37 | B41 (pin 2) |
| R17, 10k | B41 | Bottom red rail (VB) |
| Level pot lug 3 wire | B37 | |
| Level pot lug 1 wire | Bottom red rail (VB) | |
| Level pot lug 2 wire | B50 | |
| C9, 1 µF | B50 | B53 |
| R11, 100k | B53 | Bottom blue rail |

Tone clockwise is brighter, and Level clockwise is louder.

**Check:** power up and expect:
- **About 4.5 V** on B40 (TL072 pin 1) and B42.
- **0 V** on B53, the output. C9 blocks the 4.5 V from reaching the amp.

## Step 6: footswitch, LED and jacks

The footswitch switches the guitar between the circuit and a straight wire from input to output (true bypass). Its third switch lights the LED when the effect is on.

**Footswitch lugs:** turn the footswitch over and look at the lugs. They form a 3 × 3 grid, numbered like a phone keypad: 1 2 3 across the top row, 4 5 6 across the middle and 7 8 9 across the bottom. Holding it with any edge as "top" works, as long as you keep the same view throughout. The middle row are the commons. Each one connects to the lug above it or the lug below it, depending on the switch position.

First, solder these on the switch itself:

| Wire | From | To |
| --- | --- | --- |
| Short link | Lug 7 | Lug 9 |
| Input jack tip | Input jack tip lug | Footswitch lug 4 |
| Output jack tip | Output jack tip lug | Footswitch lug 6 |
| LED cathode wire | LED short leg (flat side) | Footswitch lug 2 |

Then wire the switch and LED to the breadboard:

| Part | From | To |
| --- | --- | --- |
| Wire | Footswitch lug 1 | B3 (circuit input) |
| Wire | Footswitch lug 3 | B53 (circuit output) |
| Wire | Footswitch lug 5 | Bottom blue rail (GND) |
| LED resistor, 4.7k | Top red rail | T30 |
| Wire | T30 | LED long leg (anode) |
| Input jack sleeve wire | Input jack sleeve lug | Bottom blue rail |
| Output jack sleeve wire | Output jack sleeve lug | Bottom blue rail |

Lug 8 isn't used.

**Check:** with the power off, set the multimeter to continuity (the beep setting). Probe the two jack tip lugs, then press the footswitch.
- In one position it beeps. That's bypass: the guitar goes straight through.
- In the other it doesn't. That's effect on.

Then power up and stomp: the LED should light in the effect-on position. If it lights in the bypass position instead, the LED wire is on lug 8; move it to lug 2.

## Step 7: first sound

1. Set Level fully down, Gain halfway and Tone halfway. Start with the effect off (LED dark).
2. Turn the amp down. Plug the guitar into the input jack and the output jack into the amp, then power up. You should hear the clean guitar.
3. Stomp the footswitch on and bring Level up slowly while playing. Then try Gain and Tone across their range, and flip Soft/Hard. Stomp off and on to compare with the dry guitar; set Level so the two are about equally loud for a fair comparison.
4. Hard is about 8.5 dB louder than Soft at the same settings, so lower Level before switching to Hard.

**If something's wrong:**
- **No sound with the effect off:** check the lug 7–9 link and the jack wiring from step 6.
- **No sound with the effect on only:** recheck steps 3–5 with the meter, looking for 4.5 V at the chip pins. Also check the lug 1 and lug 3 wires.
- **Loud hum:** usually a loose ground or a long input wire. Keep the input jack wire short, and away from the power wires and the output wire.
- **Squealing at high Gain:** move the input and output wires apart. Breadboards pick up hiss and hum at full Gain anyway, and the finished board is quieter.

## Growing into Option B later

Leave columns 55–63 and any free strips alone. In stage 2 the pots get replaced by the digital pots, and the toggle by the relay, so the wires for those parts are what change. The rest of the layout stays.
