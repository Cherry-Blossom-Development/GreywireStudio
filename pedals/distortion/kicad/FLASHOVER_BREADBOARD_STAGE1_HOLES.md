# Flashover breadboard: hole-by-hole build

Oct 23, 2026

Every line below is one action: one part or wire placed in exact holes. Tick each one off as you go, and do the check at the end of each step before starting the next.

This is a copy of the Claude Doc at https://claude.ai/artifact/NF5wAYg6kSDFNLnQHvrkSc. It builds the same circuit as `FLASHOVER_BREADBOARD_STAGE1_BUILD.md` and `FLASHOVER_BREADBOARD_STAGE1_LAYOUT.html`, but uses the column numbers printed on the actual breadboard (−2, −1, 1 … 61). Column N in those two files is column N − 2 here; their columns 1 and 2 are −2 and −1 here.

## Before you start

Lay the board flat with **row a at the top** and **column −2 on the left**. If your board prints row A at the bottom, turn it round so A is at the top.

**Hole names:** a name like **b41** means row b, column 41. Rows are the letters a–j (down). Columns use the numbers printed on your board: −2 and −1 for the first two columns, then 1 to 61. A negative column is written in brackets, so a(−1) means row a, column −1.

**What's connected inside the board:**

- In each column, holes a–e are joined to each other, and holes f–j are joined to each other. The groove in the middle separates the two halves.
- Each rail (the long + and − rows along the edges) is joined along its whole length. Rail holes don't line up exactly with the columns, so "at column 41" just means the nearest rail hole to column 41. Any hole in the right rail works electrically.
- On some boards each rail has a gap halfway along (a break in the printed red or blue line). If yours does, put a short jumper across the gap on all four rails before step 1.

**What each rail carries:**

| Rail | Carries |
| --- | --- |
| Top + (red) | +9 V |
| Top − (blue) | GND (ground) |
| Bottom + (red) | **VB, 4.5 V.** This is not 9 V; never connect it to the top + rail. |
| Bottom − (blue) | GND |

**Part orientation:**

- **Chips:** the notch (or dot) marks pin 1. Both chips straddle the middle groove with the notch pointing left.
- **Diodes:** the striped end is the cathode. Each diode line says which end goes where.
- **Electrolytic capacitors:** the long leg is +; the stripe on the side marks −.
- **Resistors, film and ceramic capacitors:** either way round.

**Pot lugs:** looking at the pot from the front, with the shaft toward you and the lugs pointing down, the lugs are 1, 2, 3 from left to right.

**Off-board parts** (pots, toggle, footswitch, LED, jacks) connect with a wire from the lug to the named hole. Solder the wire to the lug if you can; alligator clips work if they can't touch each other.

Unplug the 9 V supply whenever you place parts. Plug it in only for the checks.

## Step 1: rails and power

- [ ] **1.1** Jumper wire: top − rail at column −2 to bottom − rail at column −2
- [ ] **1.2** Jumper wire: top − rail at column 60 to bottom − rail at column 60
- [ ] **1.3** Wire: DC jack sleeve lug (+, the outer barrel) to **a(−1)**
- [ ] **1.4** Wire: DC jack center-pin lug (−) to top − rail at column 1
- [ ] **1.5** D3, 1N5817 diode: plain end in **b(−1)**, striped end in top + rail at column 1
- [ ] **1.6** C7, 100 µF electrolytic: long (+) leg in top + rail at column 3, striped (−) leg in top − rail at column 3
- [ ] **1.7** C10, 100 nF ceramic: top + rail at column 7 to top − rail at column 7
- [ ] **1.8** C11, 100 nF ceramic: top + rail at column 36 to top − rail at column 36

Pedal supplies are center negative: the center pin is ground and the outer sleeve is +9 V. If you're unsure which jack lug is which, plug the supply's plug in (unpowered) and find out with the meter on continuity.

**Check:** plug in the supply. Meter on DC volts, black probe on a − rail.

- Red probe on the top + rail: about **8.6–8.8 V**.
- Red probe on the bottom − rail: **0 V** (both ground rails are joined).

Unplug before step 2.

## Step 2: the 4.5 V bias (VB)

- [ ] **2.1** Chip U2, TL072, notch pointing left: pin 1 in **f38**, pin 4 in **f41**, pin 5 in **e41**, pin 8 in **e38**
- [ ] **2.2** Jumper wire: **a38** to top + rail at column 38
- [ ] **2.3** Jumper wire: **j41** to bottom − rail at column 41
- [ ] **2.4** R7, 1M resistor: **b41** to top + rail at column 41
- [ ] **2.5** R8, 1M resistor: **a41** to top − rail at column 45
- [ ] **2.6** C6, 10 µF electrolytic: long (+) leg in **c41**, striped (−) leg in top − rail at column 43
- [ ] **2.7** Jumper wire: **c39** to **c40**
- [ ] **2.8** R18, 100 Ω resistor: **d40** to **d44**
- [ ] **2.9** C15, 10 µF electrolytic: long (+) leg in **c44**, striped (−) leg in top − rail at column 47
- [ ] **2.10** Jumper wire: **e44** to bottom + rail at column 44 (this makes the bottom + rail the 4.5 V VB rail)

**Check:** power up, black probe on ground.

- About **4.5 V** on c41, c39 and the bottom + rail. Allow a few seconds to settle.
- About **8.7 V** on a38.

## Step 3: input and gain stage

- [ ] **3.1** Chip U1, LM741, notch pointing left: pin 1 in **f9**, pin 4 in **f12**, pin 5 in **e12**, pin 8 in **e9**
- [ ] **3.2** Jumper wire: **a10** to top + rail at column 10
- [ ] **3.3** Jumper wire: **j12** to bottom − rail at column 12
- [ ] **3.4** R1, 1M resistor: **g1** to bottom − rail at column −1
- [ ] **3.5** C1, 10 nF film capacitor: **f1** to **f4**
- [ ] **3.6** R2, 10k resistor: **j4** to **j11**
- [ ] **3.7** C2, 1 nF film capacitor: **i11** to bottom − rail at column 13
- [ ] **3.8** R3, 1M resistor: **h11** to bottom + rail (VB) at column 14
- [ ] **3.9** R5, 100k resistor: **h10** to **c11**, arching over the chip
- [ ] **3.10** R4, 330 Ω resistor: **i10** to **c6**, across the groove
- [ ] **3.11** C3, 470 nF film capacitor: **c4** to top − rail at column 5
- [ ] **3.12** Wire: Gain pot lug 3 to **a6**
- [ ] **3.13** Wire: Gain pot lug 2 to **a4**
- [ ] **3.14** Short wire soldered on the pot: Gain lug 1 to Gain lug 2

Leave a9–d9, a12–d12 and g9–j9 empty; chip pins 8, 5 and 1 aren't used. Gain clockwise is more gain.

**Check:** power up. Expect about **4.5 V** on c11 (chip pin 6) at any Gain setting. Readings near 0 V or near 8 V mean a wiring mistake around the chip.

## Step 4: clipping and the Soft/Hard switch

- [ ] **4.1** C4, 1 µF film capacitor: **b11** to **b15**
- [ ] **4.2** R6, 10k resistor: **c15** to **f18**, across the groove
- [ ] **4.3** Jumper wire: **g18** to **g19**
- [ ] **4.4** R10, 1M resistor: **h18** to bottom + rail (VB) at column 17
- [ ] **4.5** C5, 1 nF film capacitor: **i18** to bottom + rail (VB) at column 19
- [ ] **4.6** Wire: toggle center lug to **j18**
- [ ] **4.7** Wire: toggle end lug A (Soft) to **j27**
- [ ] **4.8** Wire: toggle end lug B (Hard) to **j30**
- [ ] **4.9** D1, 1N34A germanium diode: plain end in **g27**, striped end in bottom + rail (VB) at column 26
- [ ] **4.10** D2, 1N34A germanium diode: plain end in bottom + rail (VB) at column 28, striped end in **h27**
- [ ] **4.11** D4, 1N4148 silicon diode: plain end in **g30**, striped end in bottom + rail (VB) at column 29
- [ ] **4.12** D5, 1N4148 silicon diode: plain end in bottom + rail (VB) at column 31, striped end in **h30**

D1/D2 and D4/D5 point opposite ways on purpose. The 1N34A diodes are glass and fragile: bend their legs gently, away from the body. Label the toggle once you can hear which position is Soft.

**Check:** power up. Expect about **4.5 V** on h18 in both toggle positions.

## Step 5: tone, output amplifier and Level

- [ ] **5.1** C12, 22 nF film capacitor: **i19** to **i21**
- [ ] **5.2** R15, 6.8k resistor: **h21** to bottom + rail (VB) at column 23
- [ ] **5.3** R14, 6.8k resistor: **f19** to **f24**
- [ ] **5.4** C8, 100 nF film capacitor: **h24** to bottom + rail (VB) at column 25
- [ ] **5.5** Wire: Tone pot lug 1 to **j24**
- [ ] **5.6** Wire: Tone pot lug 3 to **j21**
- [ ] **5.7** Wire: Tone pot lug 2 to **j40**
- [ ] **5.8** Jumper wire: **g38** to **g35**
- [ ] **5.9** R16, 30k resistor: **h35** to **h39**
- [ ] **5.10** R17, 10k resistor: **i39** to bottom + rail (VB) at column 37
- [ ] **5.11** Wire: Level pot lug 3 to **j35**
- [ ] **5.12** Wire: Level pot lug 1 to bottom + rail (VB) at column 45
- [ ] **5.13** Wire: Level pot lug 2 to **j48**
- [ ] **5.14** C9, 1 µF film capacitor: **g48** to **g51**
- [ ] **5.15** R11, 100k resistor: **i51** to bottom − rail at column 52

Tone clockwise is brighter; Level clockwise is louder.

**Check:** power up and expect:

- About **4.5 V** on g38 (TL072 pin 1) and j40.
- **0 V** on g51, the output.

## Step 6: footswitch, LED and jacks

**Footswitch lugs:** turn the footswitch over. The 9 lugs form a 3 × 3 grid, numbered like a phone keypad: 1 2 3 across the top row, 4 5 6 across the middle, 7 8 9 across the bottom. Keep the same view throughout.

Solder these first, off the board:

- [ ] **6.1** Short wire: footswitch lug 7 to footswitch lug 9
- [ ] **6.2** Wire: input jack tip lug to footswitch lug 4
- [ ] **6.3** Wire: output jack tip lug to footswitch lug 6
- [ ] **6.4** Wire: LED short leg (cathode, flat side) to footswitch lug 2

Then connect to the board:

- [ ] **6.5** Wire: footswitch lug 1 to **j1** (circuit input)
- [ ] **6.6** Wire: footswitch lug 3 to **j51** (circuit output)
- [ ] **6.7** Wire: footswitch lug 5 to bottom − rail at column 6
- [ ] **6.8** LED resistor, 4.7k: top + rail at column 27 to **b28**
- [ ] **6.9** Wire: LED long leg (anode) to **e28**
- [ ] **6.10** Wire: input jack sleeve lug to bottom − rail at column 1
- [ ] **6.11** Wire: output jack sleeve lug to bottom − rail at column 58

Lug 8 isn't used.

**Check:** power off, meter on continuity (the beep setting). Probe the two jack tip lugs and press the footswitch:

- One position beeps: that's bypass.
- The other doesn't: that's effect on.

Then power up and stomp. The LED should light with the effect on. If it lights in bypass instead, move the LED wire from lug 2 to lug 8.

## Step 7: first sound

1. Set Level fully down, Gain and Tone halfway. Start with the effect off (LED dark).
2. Turn the amp down. Plug the guitar into the input jack and the output jack into the amp, then power up. You should hear clean guitar.
3. Stomp the effect on and bring Level up slowly while playing. Try Gain and Tone across their range, and flip Soft/Hard.
4. Hard is about 8.5 dB louder than Soft, so lower Level before switching to Hard.

**If something's wrong:**

- **No sound with the effect off:** check the lug 7–9 link and the jack wiring (6.1–6.3).
- **No sound with the effect on only:** redo the step 2–5 meter checks, and check 6.5 and 6.6.
- **Loud hum:** usually a loose ground or a long input wire. Keep the input wire short and away from the power and output wires.
- **Squealing at high Gain:** move the input and output wires apart.
