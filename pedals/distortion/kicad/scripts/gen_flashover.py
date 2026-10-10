"""Generate the Flashover KiCad project (hierarchical schematic)."""
import os, json, shutil
from kigen import Sheet, U, UK, mm, px, py, f, font

POT = "GreywireStudio:MCP45HV51"
PA, PW, PB = '13', '12', '11'   # pot unit pins: end A, wiper, end B
from paths import KI
OUT = KI + "Flashover"
os.makedirs(OUT, exist_ok=True)

# =====================================================================
# Sheet 2: Input and gain stage
# =====================================================================
s2 = Sheet("Flashover - Input and Gain", 3, "Input buffer network and the GS1 gain stage, scaled for a 100k digital pot")
s = s2
s.box(8, 40, 58, 112, "A: INPUT")
s.box(60, 40, 112, 112, "B: GAIN STAGE")
s.label('global', 'FX_IN', 22, 63, 180, 'input')
s.wire((22, 63), (32, 63), (35, 63))
s.text("FX_IN comes from the input jack\nthrough the bypass relay\n(Bypass sheet).", 10, 80, size=1.0)
r1 = s.add("R1", "Device:R", "1M", 32, 70)
s.wire((32, 63), r1.p('1'))
s.pwr('GND', 32, 73)
c1 = s.add("C1", "Device:C", "10n", 38, 63, rot=90)
r2 = s.add("R2", "Device:R", "10k", 47, 63, rot=90)
s.wire(c1.p('2'), r2.p('1'))
u1 = s.add("U1", "Amplifier_Operational:LM741", "LM741", 72, 65)
s.wire(r2.p('2'), (55, 63), u1.p('3'))
r3 = s.add("R3", "Device:R", "1M", 55, 56)
s.pwr('VB', 55, 53)
s.wire(r3.p('2'), (55, 63))
c2 = s.add("C2", "Device:C", "1n", 55, 70)
s.wire((55, 63), c2.p('1'))
s.pwr('GND', 55, 73)
s.pwr('+9V', 70, 56)
s.wire((70, 56), u1.p('7'))
s.pwr('GND', 70, 74)
s.wire(u1.p('4'), (70, 74))
s.nc(u1.p('1'))
s.nc(u1.p('5'))
# feedback network below the op-amp
r5 = s.add("R5", "Device:R", "100k", 72, 80, rot=90, fields={'ref': (0, 2, None), 'val': (0, 4, None)})
s.wire(u1.p('2'), (63, 67), (63, 80), r5.p('1'))
s.wire(r5.p('2'), (82, 80), (82, 65))
s.wire(u1.p('6'), (82, 65), (92, 65))
s.label('hier', 'GAIN_OUT', 92, 65, 0, 'output')
r4 = s.add("R4", "Device:R", "330", 63, 86)
s.wire((63, 80), r4.p('1'))
rv1 = s.add("U7", POT, "MCP45HV51-104", 63, 95, unit=1, fields={'ref': (5, -1, 'left'), 'val': (5, 1, 'left')})
s.wire(r4.p('2'), rv1.p(PA))
s.wire(rv1.p(PW), (66, 98), rv1.p(PB))
c3 = s.add("C3", "Device:C", "470n", 63, 104)
s.wire(rv1.p(PB), c3.p('1'))
s.pwr('GND', 63, 107)
# decoupling
c10 = s.add("C10", "Device:C", "100n", 100, 92)
s.pwr('+9V', 100, 89)
s.pwr('GND', 100, 95)
s.text("U1 supply decoupling", 92, 83)
s.text("Gain = 1 + R5 / (R4 + U7A):  about 2x (min) to 216x (max)\n"
       "U7A is the Gain knob: the pot side of an MCP45HV51 digital pot (100k).\n"
       "Its control and power pins (U7B) are on the Digital Pots sheet.", 8, 118)

# =====================================================================
# Sheet 3: Clipping (Soft / Hard)
# =====================================================================
s3 = Sheet("Flashover - Clipping", 4, "Soft (germanium) or Hard (silicon) clipping, selected by relay K1")
s = s3
s.box(14, 36, 54, 66, "C: COUPLING")
s.box(54, 36, 140, 104, "D: SOFT / HARD CLIPPING")
s.label('hier', 'GAIN_OUT', 12, 48, 180, 'input')
c4 = s.add("C4", "Device:C", "1u", 20, 48, rot=90)
s.wire((12, 48), c4.p('1'))
r6 = s.add("R6", "Device:R", "10k", 29, 48, rot=90)
s.wire(c4.p('2'), r6.p('1'))
k1 = s.add("K1", "Relay:G6K-2", "G6K-2F-Y 5VDC", 80, 60, rot=180,
           fields={'ref': (4, -10, 'left'), 'val': (4, -8.5, 'left')})
s.wire(r6.p('2'), (36, 48), (40, 48), (46, 48), (72, 48), (80, 48), k1.p('3'))
s.wire((72, 48), k1.p('6'))
r10 = s.add("R10", "Device:R", "1M", 40, 55)
s.wire((40, 48), r10.p('1'))
s.pwr('VB', 40, 58, rot=180)
c5 = s.add("C5", "Device:C", "1n", 46, 55)
s.wire((46, 48), c5.p('1'))
s.pwr('VB', 46, 58, rot=180)
s.wire((36, 48), (36, 42))
s.label('hier', 'CLIP', 36, 42, 0, 'output')
# unused contacts
s.nc(k1.p('4'))
s.nc(k1.p('7'))
# Soft: germanium pair on the normally-closed contact (relay off = Soft)
L = {'ref': (-2, -1, 'right'), 'val': (-2, 1, 'right')}
Rr = {'ref': (2, -1, 'left'), 'val': (2, 1, 'left')}
s.wire(k1.p('2'), (82, 70), (74, 70))
d1 = s.add("D1", "Device:D", "1N34A", 82, 75, rot=90, fields=Rr)
d2 = s.add("D2", "Device:D", "1N34A", 74, 75, rot=270, fields=Rr)
s.wire((82, 70), d1.p('2'))
s.wire((74, 70), d2.p('1'))
s.wire(d1.p('1'), (82, 80), (78, 80), (74, 80), d2.p('2'))
s.wire((78, 80), (78, 82))
s.pwr('VB', 78, 82, rot=180)
s.text("SOFT\n(germanium)", 74, 89)
# Hard: silicon pair on the normally-open contact of the second pole
s.wire(k1.p('5'), (70, 70), (60, 70))
d4 = s.add("D4", "Device:D", "1N4148W", 70, 75, rot=90, fields=L)
d5 = s.add("D5", "Device:D", "1N4148W", 60, 75, rot=270, fields=L)
s.wire((70, 70), d4.p('2'))
s.wire((60, 70), d5.p('1'))
s.wire(d4.p('1'), (70, 80), (65, 80), (60, 80), d5.p('2'))
s.wire((65, 80), (65, 82))
s.pwr('VB', 65, 82, rot=180)
s.text("HARD\n(silicon)", 61, 89)
# coil driver
d6 = s.add("D6", "Device:D", "1N4148W", 92, 60, rot=90, fields=Rr)
q1 = s.add("Q1", "Transistor_BJT:MMBT3904", "MMBT3904", 108, 74, mirror='y', fields={'ref': (-6, -1, 'right'), 'val': (-6, 1, 'right')})
s.wire(k1.p('8'), (92, 54), (106, 54), q1.p('3'))
s.wire((92, 54), d6.p('2'))
s.wire(d6.p('1'), (92, 66), k1.p('1'))
s.wire(k1.p('1'), (88, 69))
s.pwr('+5V', 88, 69, rot=180)
s.pwr('GND', 106, 78)
r12 = s.add("R12", "Device:R", "4.7k", 118, 74, rot=90)
s.wire(q1.p('1'), (113, 74), r12.p('1'))
r13 = s.add("R13", "Device:R", "100k", 113, 79)
s.wire((113, 74), r13.p('1'))
s.pwr('GND', 113, 82)
s.wire(r12.p('2'), (124, 74))
s.label('global', 'HARD_SEL', 124, 74, 0, 'input')
s.text("Relay off = Soft, relay on = Hard.\n"
       "HARD_SEL comes from the microcontroller (controller sheet).\n"
       "R13 keeps the relay off (Soft) until the microcontroller starts.\n"
       "+5V comes from the regulator on the Power sheet.", 8, 112)

# =====================================================================
# Sheet 4: Tone, output amplifier and Level
# =====================================================================
s4 = Sheet("Flashover - Tone and Output", 5, "DS-1 style tone control, x4 output amplifier and Level")
s = s4
s.box(14, 36, 46, 92, "E: TONE")
s.box(46, 36, 84, 92, "F: OUTPUT AMP")
s.box(84, 36, 138, 92, "G: LEVEL AND OUTPUT")
s.label('hier', 'CLIP', 12, 60, 180, 'input')
r14 = s.add("R14", "Device:R", "6.8k", 24, 54, rot=90)
c12 = s.add("C12", "Device:C", "22n", 24, 66, rot=90, fields={'ref': (0, 2, None), 'val': (0, 4, None)})
s.wire((12, 60), (18, 60))
s.wire((18, 60), (18, 54), r14.p('1'))
s.wire((18, 60), (18, 66), c12.p('1'))
c8 = s.add("C8", "Device:C", "100n", 32, 47)
s.pwr('VB', 32, 44)
s.wire(r14.p('2'), (32, 54), (40, 54))
s.wire(c8.p('2'), (32, 54))
r15 = s.add("R15", "Device:R", "6.8k", 32, 73)
s.pwr('VB', 32, 76, rot=180)
s.wire(c12.p('2'), (32, 66), (40, 66))
s.wire((32, 66), r15.p('1'))
rv2 = s.add("U8", POT, "MCP45HV51-103", 40, 60, unit=1, fields={'ref': (-2, -1, 'right'), 'val': (-2, 1, 'right')})
s.wire((40, 54), rv2.p(PA))
s.wire((40, 66), rv2.p(PB))
s.text("low-pass (dark)", 20, 57, size=1.0)
s.text("high-pass (bright)", 20, 63.5, size=1.0)
u2a = s.add("U2", "Amplifier_Operational:TL072", "TL072", 56, 62, unit=1)
s.wire(rv2.p(PW), u2a.p('3'))
r16 = s.add("R16", "Device:R", "30k", 55, 72, rot=90, fields={'ref': (0, 2, None), 'val': (0, 4, None)})
s.wire(u2a.p('2'), (48, 64), (48, 72), r16.p('1'))
s.wire(r16.p('2'), (66, 72), (66, 62))
r17 = s.add("R17", "Device:R", "10k", 48, 78)
s.wire((48, 72), r17.p('1'))
s.pwr('VB', 48, 81, rot=180)
rv3 = s.add("U9", POT, "MCP45HV51-103", 100, 68, unit=1, fields={'ref': (-2, -1, 'right'), 'val': (-2, 1, 'right')})
s.wire(u2a.p('1'), (66, 62), (100, 62), rv3.p(PA))
s.pwr('VB', 100, 71, rot=180)
c9 = s.add("C9", "Device:C", "1u", 107, 68, rot=90)
s.wire(rv3.p(PW), c9.p('1'))
r11 = s.add("R11", "Device:R", "100k", 114, 75)
s.wire(c9.p('2'), (114, 68), (122, 68))
s.label('global', 'FX_OUT', 122, 68, 0, 'output')
s.text("FX_OUT goes to the output jack\nthrough the bypass relay\n(Bypass sheet).", 117, 84, size=1.0)
s.wire((114, 68), r11.p('1'))
s.pwr('GND', 114, 78)
s.text("Gain = 1 + R16 / R17 = 4 (+12 dB), makes up the tone network's loss", 47, 88, size=1.0)
s.text("U8A (Tone) and U9A (Level) are MCP45HV51 digital pots (10k);\n"
       "their control and power pins are on the Digital Pots sheet.\n"
       "Everything after the gain stage is centered on VB (4.5V)\n"
       "so the digital pots only see 0V to 9V.\n"
       "Level is reduced to about 38% in Hard mode to match Soft's volume.", 8, 100)

# =====================================================================
# Sheet 1: Power and bias
# =====================================================================
s1 = Sheet("Flashover - Power and Bias", 2, "9V input, 4.5V bias (VB) and 5V supply")
s = s1
s.box(8, 36, 56, 76, "H: 9V INPUT")
s.box(56, 36, 112, 76, "I: BIAS (VB = 4.5V)")
s.box(8, 80, 112, 114, "J: 5V SUPPLY AND OP-AMP POWER")
# J3 mirrored so pin 2 (+) is on top. As in GS1: pin 2 -> D3 anode, pin 1 -> GND.
j3 = s.add("J3", "Connector:Barrel_Jack", "9V DC", 14, 50, mirror='x',
           fields={'ref': (-2, -6, None), 'val': (-2, 5, None)})
d3 = s.add("D3", "Device:D_Schottky", "SS14", 28, 48, rot=180,
           fields={'ref': (0, -4.5, None), 'val': (0, -3, None)})
s.wire(j3.p('2'), d3.p('2'))
s.wire(j3.p('1'), (22, 52), (22, 54), (22, 58))
s.pwr('GND', 22, 58)
s.wire((22, 54), (28, 54))
s.pwr('FLAG', 28, 54)
s.wire(d3.p('1'), (34, 48), (40, 48), (46, 48))
s.pwr('+9V', 34, 48)
c7 = s.add("C7", "Device:C_Polarized", "100u", 40, 51)
s.pwr('GND', 40, 54)
s.pwr('FLAG', 46, 48)
s.text("D3 protects against a reversed adapter", 10, 66, size=1.0)
# bias divider and buffer
s.pwr('+9V', 62, 44)
r7 = s.add("R7", "Device:R", "1M", 62, 47)
r8 = s.add("R8", "Device:R", "1M", 62, 56)
s.pwr('GND', 62, 59)
s.wire(r7.p('2'), (62, 51), r8.p('1'))
c6 = s.add("C6", "Device:C_Polarized", "10u", 69, 55)
s.pwr('GND', 69, 58)
u2b = s.add("U2", "Amplifier_Operational:TL072", "TL072", 85, 53, unit=2)
s.wire((62, 51), (69, 51), u2b.p('5'))
s.wire((69, 51), c6.p('1'))
s.wire(u2b.p('6'), (77, 55), (77, 61), (93, 61), (93, 53))
r18 = s.add("R18", "Device:R", "100", 97, 53, rot=90)
s.wire(u2b.p('7'), (93, 53), r18.p('1'))
s.wire(r18.p('2'), (103, 53), (106, 53), (109, 53))
c15 = s.add("C15", "Device:C_Polarized", "10u", 103, 57)
s.wire((103, 53), c15.p('1'))
s.pwr('GND', 103, 60)
s.pwr('FLAG', 106, 53)
s.pwr('VB', 109, 53)
s.text("U2B buffers VB so the clipping diodes, tone network\nand Level pot can all return to it.", 58, 68, size=1.0)
# 5V regulator
s.pwr('+9V', 24, 90)
u3 = s.add("U3", "Regulator_Linear:L7805", "L7805", 40, 92)
s.wire((24, 90), (24, 92), (28, 92), u3.p('1'))
c13 = s.add("C13", "Device:C", "330n", 28, 96)
s.wire((28, 92), c13.p('1'))
s.pwr('GND', 28, 99)
s.pwr('GND', 40, 98)
c14 = s.add("C14", "Device:C", "100n", 52, 96)
s.wire(u3.p('3'), (52, 92), (58, 92))
s.wire((52, 92), c14.p('1'))
s.pwr('GND', 52, 99)
s.pwr('+5V', 58, 92)
s.text("+5V: relays, LED rings, 3.3V regulator.\nUp to about 200 mA: about 0.8 W in U3,\nso give it a small heatsink if it runs hot.", 10, 105, size=1.0)
# TL072 power and decoupling
u2c = s.add("U2", "Amplifier_Operational:TL072", "TL072", 80, 96, unit=3, fields={'ref': (2, -1, 'left'), 'val': (2, 1, 'left')})
s.pwr('+9V', 78, 90)
s.pwr('GND', 78, 102)
c11 = s.add("C11", "Device:C", "100n", 90, 96)
s.pwr('+9V', 90, 93)
s.pwr('GND', 90, 99)
s.text("U2 = TL072 dual op-amp:\nU2A output amp (Tone sheet), U2B VB buffer (above)", 64, 109, size=1.0)

# =====================================================================
# Sheet 5: Controller (microcontroller, 3.3V, flash, crystal, SWD)
# =====================================================================
s5 = Sheet("Flashover - Controller", 6, "RP2040 microcontroller with its 3.3V supply, flash memory, crystal and programming header")
s = s5
s.box(8, 18, 76, 60, "K: 3.3V SUPPLY")
s.box(76, 18, 166, 124, "L: MICROCONTROLLER")
s.box(8, 64, 40, 98, "O: PROGRAMMING")
s.box(40, 64, 76, 98, "N: FLASH MEMORY")
s.box(8, 100, 76, 124, "M: CRYSTAL")
NV = {'ref': (-2, -1, 'right'), 'val': (-2, 1, 'right')}   # fields to the left of a vertical part

# --- K: 3.3V regulator from +5V
u6 = s.add("U6", "Regulator_Linear:AP2112K-3.3", "AP2112K-3.3", 40, 36)
s.pwr('+5V', 20, 34)
s.wire((20, 34), (26, 34), (30, 34), u6.p('1'))
s.wire(u6.p('3'), (30, 36), (30, 34))
c31 = s.add("C31", "Device:C", "1u", 26, 37, fields=NV)
s.pwr('GND', 26, 40)
s.pwr('GND', 40, 42)
s.nc(u6.p('4'))
s.wire(u6.p('5'), (52, 34), (58, 34))
c32 = s.add("C32", "Device:C", "1u", 52, 37)
s.pwr('GND', 52, 40)
s.pwr('+3V3', 58, 34)
s.text("+5V comes from U3 on the Power sheet.\n+3V3 feeds the microcontroller, flash\nand the digital pots' logic side.", 10, 50, size=1.0)

# --- L: RP2040
mcu = s.add("U4", "MCU_RaspberryPi:RP2040", "RP2040", 110, 80,
            fields={'ref': (-18, -38, 'left'), 'val': (-18, 38, 'left')})
pin_of = {}
for k, v in mcu.pins.items():
    pin_of.setdefault(v[3], k)
# supply pins along the top
s.pwr('+3V3', 96, 40)
s.wire((96, 40), (100, 40), (102, 40), (108, 40), (112, 40))
for nm in ('USB_VDD', 'ADC_AVDD', 'VREG_VIN'):
    s.wire(mcu.p(pin_of[nm]), (mcu.p(pin_of[nm])[0], 40))
s.wire(mcu.p('1'), (108, 40))                              # IOVDD (all six pins)
s.wire(mcu.p(pin_of['VREG_VOUT']), (116, 40), (120, 40), mcu.p('23'))   # internal 1.1V regulator -> DVDD
s.label('local', '+1V1', 116, 40)
s.pwr('GND', *mcu.p(pin_of['GND']))
# decoupling rows
xs33 = [80 + 7 * i for i in range(9)]
vals33 = ['1u'] + ['100n'] * 8
s.pwr('+3V3', 80, 24)
s.wire(*[(x, 24) for x in xs33])
s.wire(*[(x, 30) for x in xs33 + [145, 152, 159]])
for i, (x, v) in enumerate(zip(xs33, vals33)):
    s.add(f"C{16 + i}", "Device:C", v, x, 27)
xs11 = [145, 152, 159]
s.wire(*[(x, 24) for x in xs11])
s.label('local', '+1V1', 145, 24, 180)
for i, (x, v) in enumerate(zip(xs11, ['1u', '100n', '100n'])):
    s.add(f"C{25 + i}", "Device:C", v, x, 27)
s.pwr('GND', 159, 30)
s.text("3.3V: C16 on VREG_VIN, one 100n at each of the six IOVDD pins, USB_VDD, ADC_AVDD.   1.1V: C25 on VREG_VOUT, 100n at each DVDD pin.",
       80, 34, size=1.0)
# left-side pins
s.wire(mcu.p(pin_of['TESTEN']), (88, 56), (88, 58))
s.pwr('GND', 88, 58)
r20 = s.add("R20", "Device:R", "10k", 82, 59, fields=NV)
s.pwr('+3V3', 82, 56)
s.wire(mcu.p(pin_of['RUN']), r20.p('2'))
s.nc(mcu.p(pin_of['USB_DM']))
s.nc(mcu.p(pin_of['USB_DP']))
for nm in ('XIN', 'XOUT', 'SWCLK', 'SWDIO'):
    x, y = mcu.p(pin_of[nm])
    s.wire((x, y), (x - 4, y))
    s.label('local', nm, x - 4, y, 180)
# right-side pins: the ones used so far, the rest wait for later sheets
used = {'GPIO0': ('MIDI_TX', 'output'), 'GPIO1': ('MIDI_RX', 'input'), 'GPIO2': ('HARD_SEL', 'output'), 'GPIO3': ('FX_ON', 'output'), 'GPIO15': ('FOOTSW', 'input'), 'GPIO20': ('LINK_LED', 'output'), 'GPIO4': ('SDA', 'bidirectional'), 'GPIO5': ('SCL', 'output'),
        'GPIO6': ('GAIN_A', 'input'), 'GPIO7': ('GAIN_B', 'input'), 'GPIO8': ('GAIN_SW', 'input'),
        'GPIO9': ('TONE_A', 'input'), 'GPIO10': ('TONE_B', 'input'), 'GPIO11': ('TONE_SW', 'input'),
        'GPIO12': ('LEVEL_A', 'input'), 'GPIO13': ('LEVEL_B', 'input'), 'GPIO14': ('LEVEL_SW', 'input'),
        'GPIO16': ('MODE_BTN', 'input'), 'GPIO17': ('LED_EN', 'output'),
        'GPIO18': ('LED_SDA', 'bidirectional'), 'GPIO19': ('LED_SCL', 'output')}
for nm, num in pin_of.items():
    if not nm.startswith('GPIO'):
        continue
    x, y = mcu.p(num)
    if nm in used:
        s.wire((x, y), (x + 4, y))
        s.label('global', used[nm][0], x + 4, y, 0, used[nm][1])
    else:
        s.nc((x, y))
s.text("GPIO plan (no-connect = spare)\nGPIO0       MIDI out (UART0 TX)\nGPIO1       MIDI in (UART0 RX)\nGPIO2       HARD_SEL: Soft/Hard relay\nGPIO3       FX_ON: bypass relay\nGPIO4-5    SDA, SCL: digital pots (I2C0)\nGPIO6-8    Gain encoder A, B, push\nGPIO9-11  Tone encoder A, B, push\nGPIO12-14 Level encoder A, B, push\nGPIO15     FOOTSW: footswitch\nGPIO16     Soft/Hard button\nGPIO17     LED_EN: LED driver on/off\nGPIO18-19 LED_SDA, LED_SCL (I2C1)\nGPIO20     LINK_LED\nGPIO21-29 Spare", 168, 22, size=1.0)
s.text("USB is not used (Flashover has no USB port).\nFirmware updates arrive over MIDI.", 122, 119, size=1.0)

# --- N: flash
u5 = s.add("U5", "Memory_Flash:W25Q16JVSS", "W25Q16JVSS", 66, 82, mirror='y',
           fields={'ref': (-6, -12, None), 'val': (-6, -10, None)})
for a, b in (('~{QSPI_SS}', '1'), ('QSPI_SCLK', '6'), ('QSPI_SD0', '5'), ('QSPI_SD1', '2'), ('QSPI_SD2', '3'), ('QSPI_SD3', '7')):
    s.wire(u5.p(b), mcu.p(pin_of[a]))
s.pwr('+3V3', *u5.p('8'))
s.pwr('GND', *u5.p('4'))
c28 = s.add("C28", "Device:C", "100n", 46, 82, fields=NV)
s.pwr("+3V3", 46, 79)
s.pwr("GND", 46, 85)
s.text("2 MB: two firmware copies,\nsettings and 128 presets", 42, 94, size=1.0)

# --- M: 12 MHz crystal
s.label('local', 'XIN', 14, 106, 180)
y1 = s.add("Y1", "Device:Crystal_GND24", "12MHz", 33, 106, fields={'ref': (0, -6.5, None), 'val': (0, -4.5, None)})
s.wire((14, 106), (22, 106), y1.p('1'))
c29 = s.add("C29", "Device:C", "15p", 22, 109, fields=NV)
s.pwr('GND', 22, 112)
s.pwr('GND', *y1.p('2'))
r19 = s.add("R19", "Device:R", "1k", 47, 106, rot=90)
s.wire(y1.p('3'), (39, 106), r19.p('1'))
c30 = s.add("C30", "Device:C", "15p", 39, 109)
s.pwr('GND', 39, 112)
s.wire(r19.p('2'), (56, 106))
s.label('local', 'XOUT', 56, 106, 0)
s.text("ABM8-272-T3, as on the Raspberry Pi Pico", 10, 120, size=1.0)

# --- O: SWD header
j4 = s.add("J4", "Connector_Generic:Conn_01x03", "SWD", 16, 80, mirror='y',
           fields={'ref': (-1, -5, None), 'val': (-1, 5, None)})
s.label('local', 'SWCLK', *j4.p('1'))
s.label('local', 'SWDIO', *j4.p('3'))
s.wire(j4.p('2'), (30, 80), (30, 84))
s.pwr('GND', 30, 84)
s.text("Loads the first firmware\n(Raspberry Pi Debug Probe).\nLater updates come over MIDI.", 10, 90, size=1.0)

# =====================================================================
# Sheet 6: Digital pots (control side of U7, U8, U9)
# =====================================================================
s6 = Sheet("Flashover - Digital Pots", 7, "Control and power side of the three MCP45HV51 digital pots (I2C)")
s = s6
POTS = [("U7", "MCP45HV51-104", "GAIN POT (100k)", "0x3C", 0, 0),
        ("U8", "MCP45HV51-103", "TONE POT (10k)", "0x3D", 0, 1),
        ("U9", "MCP45HV51-103", "LEVEL POT (10k)", "0x3E", 1, 0)]
for i, (ref, val, title, addr, a1, a0) in enumerate(POTS):
    x0 = 8 + 66 * i
    s.box(x0, 38, x0 + 66, 96, f"{'PQR'[i]}: {title}")
    cx, cy = x0 + 42, 64
    u = s.add(ref, POT, val, cx, cy, unit=2)
    s.label('global', 'SCL', *u.p('2'), 180, 'input')
    s.label('global', 'SDA', *u.p('4'), 180, 'bidirectional')
    for num, hi, xoff in (('5', a0, 17), ('3', a1, 21), ('6', 0, 17), ('8', 1, 21)):
        x, y = u.p(num)
        s.wire((x, y), (cx - xoff, y))
        if hi:
            s.pwr('+3V3', cx - xoff, y, rot=90)
        else:
            s.pwr('GND', cx - xoff, y, rot=270, hide_val=True)
    s.wire(u.p('1'), (cx - 2, cy - 16))
    s.pwr('+3V3', cx - 2, cy - 16)
    s.wire(u.p('14'), (cx + 2, cy - 13))
    s.pwr('+9V', cx + 2, cy - 13)
    s.wire(u.p('9'), (cx - 2, cy + 14), (cx, cy + 14), (cx + 2, cy + 14), u.p('10'))
    s.pwr('GND', cx, cy + 14)
    s.nc(u.p('7'))
    s.text(f"I2C address {addr} (A1={a1}, A0={a0})", x0 + 2, 90, size=1.0)
s.box(8, 102, 78, 130, "S: I2C PULL-UPS")
for x, ref, net, lx, ang in ((20, "R21", "SDA", 14, 180), (30, "R22", "SCL", 36, 0)):
    s.add(ref, "Device:R", "4.7k", x, 112)
    s.pwr('+3V3', x, 109)
    s.wire((x, 115), (x, 120), (lx, 120))
    s.label('global', net, lx, 120, ang, 'bidirectional' if net == 'SDA' else 'input')
s.text("The pot side of each chip (U7A, U8A, U9A) is drawn on the audio sheets where it acts.\n"
       "V+ = 9V and V- = GND, so the pot ends can swing anywhere from 0V to 9V.\n"
       "VL = 3.3V matches the microcontroller's logic.  WLAT tied low: a new setting takes effect at once.\n"
       "SHDN tied high: normal operation.  At power-up each wiper starts at mid-scale until the\n"
       "microcontroller restores the saved setting.\n\n"
       "Note: the datasheet specifies the analog side from V+ = 10V.  At about 8.6V it still works\n"
       "(it resets below 6V) but the wiper resistance is higher: about 150 ohms instead of 75,\n"
       "which R4 on the Input and Gain sheet allows for.  Confirm on the breadboard.", 82, 108, size=1.0)

# =====================================================================
# Sheet 7: Controls (three encoders with push switches, Soft/Hard button)
# =====================================================================
s7 = Sheet("Flashover - Controls", 8, "Gain, Tone and Level endless knobs (encoders with push switch) and the Soft/Hard button")
s = s7
ENC = "PEC11R-4215F-S0024"
rn, cn = 23, 33
for i, (nm, title) in enumerate((("GAIN", "T: GAIN KNOB"), ("TONE", "U: TONE KNOB"), ("LEVEL", "V: LEVEL KNOB"))):
    x0 = 8 + 66 * i
    s.box(x0, 38, x0 + 66, 78, title)
    cx, cy = x0 + 36, 60
    enc = s.add(f"SW{i + 1}", "Device:RotaryEncoder_Switch", ENC, cx, cy,
                fields={'ref': (0, -7, None), 'val': (-2, 11, None)})
    # common to ground
    s.wire(enc.p('C'), (cx - 9, cy))
    s.pwr('GND', cx - 9, cy, rot=270, hide_val=True)
    # A: pull-up and filter cap above the line
    ra = s.add(f"R{rn}", "Device:R", "10k", cx - 14, cy - 5)
    ca = s.add(f"C{cn}", "Device:C", "10n", cx - 19, cy - 5, fields=NV)
    s.pwr('+3V3', cx - 14, cy - 8)
    s.pwr('GND', cx - 19, cy - 8, rot=180, hide_val=True)
    s.wire(enc.p('A'), (cx - 14, cy - 2), (cx - 19, cy - 2), (cx - 24, cy - 2))
    s.label('global', f"{nm}_A", cx - 24, cy - 2, 180, 'output')
    # B: pull-up and filter cap below the line
    rb = s.add(f"R{rn + 1}", "Device:R", "10k", cx - 14, cy + 5)
    cb = s.add(f"C{cn + 1}", "Device:C", "10n", cx - 19, cy + 5, fields=NV)
    s.pwr('+3V3', cx - 14, cy + 8, rot=180)
    s.pwr('GND', cx - 19, cy + 8)
    s.wire(enc.p('B'), (cx - 14, cy + 2), (cx - 19, cy + 2), (cx - 24, cy + 2))
    s.label('global', f"{nm}_B", cx - 24, cy + 2, 180, 'output')
    # push switch: pull-up, other side to ground
    rs = s.add(f"R{rn + 2}", "Device:R", "10k", cx + 12, cy - 5)
    s.pwr('+3V3', cx + 12, cy - 8)
    s.wire(enc.p('S1'), (cx + 12, cy - 2), (cx + 18, cy - 2))
    s.label('global', f"{nm}_SW", cx + 18, cy - 2, 0, 'output')
    s.wire(enc.p('S2'), (cx + 8, cy + 2), (cx + 8, cy + 5))
    s.pwr('GND', cx + 8, cy + 5)
    rn += 3
    cn += 2
s.box(8, 82, 74, 118, "W: SOFT / HARD BUTTON")
sw4 = s.add("SW4", "Switch:SW_Push", "Soft/Hard", 42, 98, fields={'ref': (0, -4, None), 'val': (0, 3, None)})
r32 = s.add("R32", "Device:R", "10k", 30, 95)
s.pwr('+3V3', 30, 92)
s.wire(sw4.p('1'), (30, 98), (20, 98))
s.label('global', 'MODE_BTN', 20, 98, 180, 'output')
s.wire(sw4.p('2'), (50, 98), (50, 101))
s.pwr('GND', 50, 101)
s.text("Each press toggles Soft / Hard.\nThe LEDs showing the mode are on the LED Rings sheet.", 10, 110, size=1.0)
s.text("Encoders: Bourns PEC11R type, 24 steps per turn with a push switch\n"
       "(shaft length to suit the enclosure).  No end stops: the LED ring shows the setting.\n"
       "A and B have 10k pull-ups and 10n caps (0.1 ms) to smooth contact bounce;\n"
       "the push switches are debounced in firmware.\n"
       "All signals go to the microcontroller on the main board through J8.", 80, 88, size=1.0)

# =====================================================================
# Sheet 8: LED rings (IS31FL3236A, 36 outputs)
# =====================================================================
s8 = Sheet("Flashover - LED Rings", 9, "One 36-channel LED driver: three 11-LED rings plus the Soft, Hard and On LEDs")
s = s8
s.box(4, 34, 64, 132, "X: LED DRIVER")
drv = s.add("U10", "Driver_LED:IS31FL3236A-TQ", "IS31FL3236A", 40, 88,
            fields={'ref': (8, -42, 'left'), 'val': (8, -40, 'left')})
dp = {v[3]: k for k, v in drv.pins.items()}
s.wire(drv.p(dp['VCC']), (40, 40), (34, 40), (28, 40))
s.pwr('+5V', 40, 40)
s.add("C39", "Device:C", "1u", 28, 43, fields=NV)
s.add("C40", "Device:C", "100n", 34, 43)
s.pwr('GND', 28, 46)
s.pwr('GND', 34, 46)
s.label('global', 'LED_SDA', *drv.p(dp['SDA']), 180, 'bidirectional')
s.label('global', 'LED_SCL', *drv.p(dp['SCL']), 180, 'input')
x, y = drv.p(dp['AD'])
s.wire((x, y), (x - 4, y))
s.pwr('GND', x - 4, y, rot=270, hide_val=True)
x, y = drv.p(dp['~{SDB}'])
s.wire((x, y), (x - 6, y), (x - 12, y))
s.label('global', 'LED_EN', x - 12, y, 180, 'input')
s.add("R34", "Device:R", "100k", x - 6, y + 3, fields=NV)
s.pwr('GND', x - 6, y + 6)
x, y = drv.p(dp['R_EXT'])
s.wire((x, y), (x - 6, y))
s.add("R33", "Device:R", "15k", x - 6, y + 3, fields=NV)
s.pwr('GND', x - 6, y + 6)
s.pwr('GND', *drv.p(dp['GND']))
s.text("LED_EN low = LEDs off\n(R34 holds them off\nuntil the firmware starts)", 6, 76, size=1.0)
s.text("R33 sets the maximum LED\ncurrent: 58.5 x 1.3V / R33\n= about 5 mA.  Firmware can\nscale it down and dim with PWM.", 6, 102, size=1.0)
# outputs to local labels
names = [f"G{i}" for i in range(1, 12)] + [f"T{i}" for i in range(1, 12)] + [f"L{i}" for i in range(1, 12)] + ['SOFT', 'HARD', 'ON']
for i, n in enumerate(names):
    x, y = drv.p(dp[f"OUT{i + 1}"])
    s.wire((x, y), (x + 3, y))
    s.label('local', n, x + 3, y, 0)
# LED rows: anodes on a +5V rail, cathodes to the driver outputs
rows = [("Y: GAIN RING", [f"G{i}" for i in range(1, 12)], 7),
        ("Z: TONE RING", [f"T{i}" for i in range(1, 12)], 18),
        ("AA: LEVEL RING", [f"L{i}" for i in range(1, 12)], 29),
        ("AB: SOFT, HARD AND ON LEDS", ['SOFT', 'HARD', 'ON'], 40)]
for r, (title, nets, d0) in enumerate(rows):
    top = 34 + 24 * r
    s.box(66, top, 142, top + 24, title)
    yr = top + 9
    xs = [76 + 6 * k for k in range(len(nets))]
    s.pwr('+5V', 70, yr)
    s.wire(*[(70, yr)] + [(x, yr) for x in xs])
    for k, (x, n) in enumerate(zip(xs, nets)):
        s.add(f"D{d0 + k}", "Device:LED", "LED", x, yr + 3, rot=90, hide_val=True,
              fields={'ref': (2, 0, 'left'), 'val': (2, 0, 'left')})
        s.wire((x, yr + 6), (x, yr + 8))
        s.label('local', n, x, yr + 8, 270)
s.text("Each ring: 11 LEDs around the knob, from about 7 o'clock (1) to 5 o'clock (11),\n"
       "with LED 6 at 12 o'clock so Tone's middle position has its own LED.\n"
       "The firmware lights them as a bar (Gain, Level) or a dot (Tone) and dims them\n"
       "for dark stages using the driver's PWM.  LED color is still to be chosen.\n\n"
       "The driver runs from +5V so blue or white LEDs have enough headroom.\n"
       "Its I2C inputs accept 3.3V logic.  It sits on its own I2C bus\n"
       "(LED_SDA / LED_SCL) because its addresses overlap the digital pots'.\n\n"
       "The ON LED sits above the footswitch, which is wired to the\n"
       "main board along with the bypass relay.", 146, 38, size=1.0)
s.box(146, 96, 188, 124, "AC: LED BUS PULL-UPS")
for x, ref, net in ((156, "R35", "LED_SDA"), (168, "R36", "LED_SCL")):
    s.add(ref, "Device:R", "4.7k", x, 106)
    s.pwr('+3V3', x, 103)
    s.wire((x, 109), (x, 111))
    s.label('global', net, x, 111, 270, 'bidirectional' if net == 'LED_SDA' else 'input')

# =====================================================================
# Sheet 9: MIDI in / out (3.5 mm TRS, Type A)
# =====================================================================
s9 = Sheet("Flashover - MIDI", 10, "MIDI in and MIDI out on 3.5 mm TRS jacks (Type A) for the Linework loop")
s = s9
s.box(8, 36, 112, 82, "AD: MIDI IN")
s.box(8, 86, 112, 124, "AE: MIDI OUT")
# --- MIDI IN: optocoupler isolates the incoming current loop
j5 = s.add("J5", "Connector_Audio:AudioJack3", "MIDI In", 16, 60, fields={'ref': (0, -6, None), 'val': (0, -4.5, None)})
s.nc(j5.p('S'))
r37 = s.add("R37", "Device:R", "220", 30, 60, rot=90)
s.wire(j5.p('R'), r37.p('1'))
u11 = s.add("U11", "Isolator:H11L1", "H11L1", 60, 62, fields={'ref': (-4, -8, None), 'val': (-4, -6.5, None)})
s.wire(r37.p('2'), (42, 60), u11.p('1'))
d43 = s.add("D43", "Device:D", "1N4148W", 42, 63, rot=270, fields={'ref': (-2, -1, 'right'), 'val': (-2, 1, 'right')})
s.wire(j5.p('T'), (24, 62), (24, 66), (42, 66), (48, 66), (48, 64), u11.p('2'))
s.pwr('+3V3', 60, 52)
s.wire((60, 52), (70, 52))
s.wire((60, 52), u11.p('6'))
s.add("C41", "Device:C", "100n", 70, 55, fields={'ref': (-2, -1, 'right'), 'val': (-2, 1, 'right')})
s.pwr('GND', 70, 58)
s.pwr('GND', *u11.p('5'))
r38 = s.add("R38", "Device:R", "1k", 76, 59)
s.pwr('+3V3', 76, 56)
s.wire(u11.p('4'), (76, 62), (84, 62))
s.label('global', 'MIDI_RX', 84, 62, 0, 'output')
s.text("Type A wiring: Tip = MIDI pin 5, Ring = MIDI pin 4, Sleeve = shield.\n"
       "The sleeve is left unconnected on the input, as the MIDI spec asks, to avoid ground loops.\n"
       "D43 protects the optocoupler if a reversed (Type B) cable is plugged in.\n"
       "The H11L1 runs from 3.3V and its output idles high, as the UART expects.", 10, 74, size=1.0)
# --- MIDI OUT: 3.3V current-loop driver
j6 = s.add("J6", "Connector_Audio:AudioJack3", "MIDI Out", 90, 104, rot=180,
           fields={'ref': (2, -8, None), 'val': (2, -6.5, None)})
r40 = s.add("R40", "Device:R", "10", 74, 102, rot=90)
s.wire(j6.p('T'), r40.p('2'))
s.wire(r40.p('1'), (60, 102))
s.label('global', 'MIDI_TX', 60, 102, 180, 'input')
r39 = s.add("R39", "Device:R", "33 0.5W", 80, 107, fields={'ref': (-2, -1, 'right'), 'val': (-2, 1, 'right')})
s.wire(j6.p('R'), (80, 104))
s.pwr('+3V3', 80, 110, rot=180)
s.wire(j6.p('S'), (86, 110))
s.pwr('GND', 86, 110)
s.text("3.3V MIDI output, using the values in the MIDI Association's 3.3V update:\n"
       "33 ohm from +3.3V to pin 4 (Ring) and 10 ohm from the UART to pin 5 (Tip).\n"
       "The sleeve is grounded on the output.", 10, 116, size=1.0)
s.text("No hardware MIDI thru.  The firmware passes on every message it receives\n"
       "and adds its own (merging, per the Linework Control Specification).\n"
       "If the pedal loses power the loop stops here; Switchyard detects the break.\n\n"
       "MIDI_RX and MIDI_TX go to UART0 on the microcontroller (GPIO1 and GPIO0).", 116, 40, size=1.0)

# =====================================================================
# Sheet 10: Bypass relay, jacks, footswitch and Link LED
# =====================================================================
s10 = Sheet("Flashover - Bypass", 11, "Fail-safe relay bypass with the input and output jacks and the footswitch")
s = s10
s.box(8, 36, 134, 92, "AF: JACKS AND BYPASS RELAY")
s.box(8, 94, 66, 124, "AG: FOOTSWITCH")
# jacks
j1 = s.add("J1", "Connector_Audio:AudioJack2", "Input", 14, 52, mirror='x',
           fields={'ref': (-2, -6, None), 'val': (-2, -4, None)})
s.wire(j1.p('S'), (22, 54), (22, 58))
s.pwr('GND', 22, 58)
j2 = s.add("J2", "Connector_Audio:AudioJack2", "Output", 124, 46, rot=180,
           fields={'ref': (2, -6, None), 'val': (2, -4, None)})
s.wire(j2.p('S'), (116, 48), (116, 52))
s.pwr('GND', 116, 52)
# relay K2, rotated so the commons are on top: pole B (pin 6) = input, pole A (pin 3) = output
k2 = s.add("K2", "Relay:G6K-2", "G6K-2F-Y 5VDC", 60, 64, rot=180,
           fields={'ref': (-20, -3, None), 'val': (-20, -1, None)})
s.wire(j1.p('T'), (52, 52), k2.p('6'))
s.wire(k2.p('3'), (60, 46), j2.p('T'))
for pin, kind, nm in (('5', 'global', 'FX_IN'), ('7', 'local', 'BYPASS'), ('4', 'global', 'FX_OUT'), ('2', 'local', 'BYPASS')):
    x, y = k2.p(pin)
    s.wire((x, y), (x, y + 2))
    if kind == 'global':
        s.label('global', nm, x, y + 2, 270, 'output' if nm == 'FX_IN' else 'input')
    else:
        s.label('local', nm, x, y + 2, 270)
# coil, flyback diode and driver
q2 = s.add("Q2", "Transistor_BJT:MMBT3904", "MMBT3904", 88, 80, mirror='y', fields={'ref': (-6, -1, 'right'), 'val': (-6, 1, 'right')})
s.wire(k2.p('8'), (68, 54), (74, 54), (86, 54), q2.p('3'))
s.wire((74, 54), (74, 61))
d44 = s.add("D44", "Device:D", "1N4148W", 74, 64, rot=90, fields={'ref': (2, -1, 'left'), 'val': (2, 1, 'left')})
s.wire(k2.p('1'), (68, 72), (74, 72), (74, 67))
s.wire((68, 72), (68, 75))
s.pwr('+5V', 68, 75, rot=180)
s.pwr('GND', *q2.p('2'))
r43 = s.add("R43", "Device:R", "4.7k", 97, 80, rot=90)
s.wire(q2.p('1'), (94, 80))
r44 = s.add("R44", "Device:R", "100k", 94, 83, fields={'ref': (-2, -1, 'right'), 'val': (-2, 1, 'right')})
s.pwr('GND', 94, 86)
s.wire(r43.p('2'), (106, 80))
s.label('global', 'FX_ON', 106, 80, 0, 'input')
s.text("Relay off = BYPASS: the input jack\nconnects straight to the output jack\nthrough both NC contacts (BYPASS).\nWith no power the relay is off, so a\ndead pedal still passes the guitar.\nRelay on = effect: input -> FX_IN,\nFX_OUT -> output.  R44 keeps it in\nbypass until the firmware starts.", 10, 72, size=1.0)
s.text("To avoid clicks, the firmware\nturns Level down for a few ms\nwhile the relay switches.", 100, 58, size=1.0)
# footswitch
sw5 = s.add("SW5", "Switch:SW_Push", "Footswitch", 40, 104, fields={'ref': (0, -4, None), 'val': (0, 3, None)})
r45 = s.add("R45", "Device:R", "10k", 30, 101, fields={'ref': (-2, -1, 'right'), 'val': (-2, 1, 'right')})
s.pwr('+3V3', 30, 98)
c42 = s.add("C42", "Device:C", "100n", 26, 107, fields={'ref': (-2, -1, 'right'), 'val': (-2, 1, 'right')})
s.pwr('GND', 26, 110)
s.wire(sw5.p('1'), (30, 104), (26, 104), (22, 104))
s.label('global', 'FOOTSW', 22, 104, 180, 'output')
s.wire(sw5.p('2'), (48, 104), (48, 107))
s.pwr('GND', 48, 107)
s.text("Soft-touch momentary switch on the panel, wired to a 2-pin\nJST PH connector.  Read by the firmware; C42 filters\nnoise picked up on the switch wires.", 10, 117, size=1.0)

# =====================================================================
# Board connectors (main board J7 <-> control board J8)
# =====================================================================
LINK_PINS = {1: '+5V', 2: '+5V', 3: 'GND', 4: 'GND', 5: '+3V3', 6: 'GND',
             7: 'GAIN_A', 8: 'GAIN_B', 9: 'GAIN_SW', 10: 'TONE_A', 11: 'TONE_B', 12: 'TONE_SW',
             13: 'LEVEL_A', 14: 'LEVEL_B', 15: 'LEVEL_SW', 16: 'MODE_BTN',
             17: 'LED_SDA', 18: 'LED_SCL', 19: 'LED_EN', 20: 'LINK_LED'}


def board_connector(s, ref, value, cx, cy):
    j = s.add(ref, "Connector_Generic:Conn_02x10_Odd_Even", value, cx, cy,
              fields={'ref': (1, -10, None), 'val': (1, 12, None)})
    for n, net in LINK_PINS.items():
        x, y = j.p(str(n))
        left = x < cx
        ex = x - 4 if left else x + 4
        s.wire((x, y), (ex, y))
        if net == 'GND':
            s.pwr('GND', ex, y, rot=270 if left else 90, hide_val=True)
        elif net in ('+5V', '+3V3'):
            s.pwr(net, ex, y, rot=90 if left else 270)
        else:
            s.label('global', net, ex, y, 180 if left else 0, 'passive')
    return j


s11 = Sheet("Flashover - Board Connector", 10, "J7: connector to the control board (Flashover_Controls)")
s = s11
s.box(40, 40, 140, 96, "AI: CONTROL BOARD CONNECTOR")
board_connector(s, "J7", "To control board", 88, 66)
s.text("2x10 socket, 2.54 mm.  The control board's J8 header plugs in from above.\n"
       "It carries +5V (LED driver), +3V3 (pull-ups), the encoder and button\n"
       "signals, the LED driver bus and the Link LED.", 42, 84, size=1.0)
s.box(144, 40, 188, 70, "AJ: MOUNTING")
_h = s.add("H1", "Mechanical:MountingHole", "M3", 156, 54, fields={'ref': (3, -1, 'left'), 'val': (3, 1, 'left')})
_h.no_bom = True
_h = s.add("H2", "Mechanical:MountingHole", "M3", 172, 54, fields={'ref': (3, -1, 'left'), 'val': (3, 1, 'left')})
_h.no_bom = True
s.text("M3 holes for the standoffs\nthat hold the control board.", 146, 63, size=1.0)

s12 = Sheet("Flashover Controls - Board Connector", 4, "J8: connector to the main board, and the Link LED")
s = s12
s.box(8, 36, 104, 100, "AI: MAIN BOARD CONNECTOR")
board_connector(s, "J8", "To main board", 56, 62)
for i, net in enumerate(('+5V', '+3V3', 'GND')):
    x = 16 + 14 * i
    s.pwr(net, x, 88)
    s.wire((x, 88), (x + 4, 88))
    s.pwr('FLAG', x + 4, 88)
s.text("2x10 header, 2.54 mm, on the underside;\nplugs into J7 on the main board.\n"
       "Power comes from the main board (flags).", 10, 94, size=1.0)
s.box(108, 36, 176, 66, "AH: LINK LED")
s.label('global', 'LINK_LED', 114, 46, 180, 'input')
r46 = s.add("R46", "Device:R", "330", 122, 46, rot=90)
s.wire((114, 46), r46.p('1'))
d45 = s.add("D45", "Device:LED", "Link", 132, 46, rot=180, fields={'ref': (0, -3, None), 'val': (0, 3, None)})
s.wire(r46.p('2'), d45.p('2'))
s.wire(d45.p('1'), (138, 46), (138, 49))
s.pwr('GND', 138, 49)
s.text("Lit while the pedal is linked to Switchyard.\nDriven straight from GPIO20 (about 4 mA for a red LED).", 110, 58, size=1.0)
s.box(108, 70, 176, 100, "AJ: MOUNTING")
_h = s.add("H3", "Mechanical:MountingHole", "M3", 120, 84, fields={'ref': (3, -1, 'left'), 'val': (3, 1, 'left')})
_h.no_bom = True
_h = s.add("H4", "Mechanical:MountingHole", "M3", 136, 84, fields={'ref': (3, -1, 'left'), 'val': (3, 1, 'left')})
_h.no_bom = True
s.text("M3 holes, lined up with H1 and H2 on the main board.", 110, 93, size=1.0)

# control project page numbers
s7.page, s8.page = 2, 3
s9.page, s10.page = 8, 9

# =====================================================================
# Root sheets
# =====================================================================
W, H = 40, 16


def make_root(root, project, subs):
    syms = []
    for sh, fn, nm, x, y, pl in subs:
        su = UK(project + '/' + fn)
        sh.sym_uuid = su
        sh.file = fn
        sh.root = root
        sh.project = project
        pinstr = ""
        for pn, d, side, dy in pl:
            pxx = x + W if side == 'R' else x
            ang = 0 if side == 'R' else 180
            j = 'right' if side == 'R' else 'left'
            pinstr += (f"\t\t(pin \"{pn}\" {d}\n\t\t\t(at {f(px(pxx))} {f(py(y + dy))} {ang})\n"
                       f"\t\t\t(effects {font()} (justify {j}))\n\t\t\t(uuid \"{U()}\")\n\t\t)\n")
            root.labelpts.append((pxx, y + dy))
        syms.append(
            f"\t(sheet\n\t\t(at {f(px(x))} {f(py(y))})\n\t\t(size {f(mm(W))} {f(mm(H))})\n\t\t(exclude_from_sim no)\n"
            f"\t\t(in_bom yes)\n\t\t(on_board yes)\n\t\t(dnp no)\n\t\t(fields_autoplaced yes)\n"
            f"\t\t(stroke\n\t\t\t(width 0.1524)\n\t\t\t(type solid)\n\t\t)\n\t\t(fill\n\t\t\t(color 0 0 0 0.0000)\n\t\t)\n"
            f"\t\t(uuid \"{su}\")\n"
            f"\t\t(property \"Sheetname\" \"{nm}\"\n\t\t\t(at {f(px(x))} {f(py(y) - 0.7116)} 0)\n\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n"
            f"\t\t\t(effects {font()} (justify left bottom))\n\t\t)\n"
            f"\t\t(property \"Sheetfile\" \"{fn}\"\n\t\t\t(at {f(px(x))} {f(py(y + H) + 0.5846)} 0)\n\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n"
            f"\t\t\t(effects {font()} (justify left top))\n\t\t)\n"
            + pinstr +
            f"\t\t(instances\n\t\t\t(project \"{project}\"\n\t\t\t\t(path \"/{root.uuid}\"\n\t\t\t\t\t(page \"{sh.page}\")\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)")
    return syms


root = Sheet("Flashover", 1, "Flashover distortion / overdrive - Linework series - main board")
subs = [  # (sheet, file, name, x, y, pins[(name, dir, side, dy)])
    (s10, "bypass.kicad_sch", "Bypass", 16, 40, []),
    (s2, "input_gain.kicad_sch", "Input and Gain", 66, 40, [("GAIN_OUT", "output", "R", 8)]),
    (s3, "clipping.kicad_sch", "Clipping", 116, 40, [("GAIN_OUT", "input", "L", 8), ("CLIP", "output", "R", 8)]),
    (s4, "tone_output.kicad_sch", "Tone and Output", 166, 40, [("CLIP", "input", "L", 8)]),
    (s1, "power.kicad_sch", "Power and Bias", 16, 72, []),
    (s5, "controller.kicad_sch", "Controller", 66, 72, []),
    (s6, "digital_pots.kicad_sch", "Digital Pots", 116, 72, []),
    (s9, "midi.kicad_sch", "MIDI", 166, 72, []),
    (s11, "board_connector.kicad_sch", "Board Connector", 16, 102, []),
]
sheet_syms = make_root(root, "Flashover", subs)
root.wire((106, 48), (116, 48))
root.wire((156, 48), (166, 48))
root.text("FLASHOVER", 24, 24, size=3.0, bold=True)
root.text("Distortion / overdrive pedal, Linework series.  Main board, version 0.1.\n"
          "Based on GS1 (MXR Distortion+) with Soft/Hard clipping, a DS-1 style Tone control,\n"
          "a x4 output amplifier and digital pots for Gain, Tone and Level.\n"
          "Values match the LTspice model in pedals/distortion/sim/Flashover_circuit.inc.", 24, 30)
root.text("Signal flow:  input jack (Bypass)  ->  Input and Gain  ->  Clipping  ->  Tone and Output  ->  output jack (Bypass)", 16, 62)
root.text("Two boards in a 1590BB enclosure.  This main board holds the audio circuit, controller,\n"
          "digital pots, MIDI, bypass and all five jacks (along the top edge).  The knobs, LED rings,\n"
          "Soft/Hard button and panel LEDs are on the control board (project Flashover_Controls),\n"
          "which plugs into J7 on the Board Connector sheet.", 66, 102)
root.text("Footprints: surface-mount for assembly, through-hole for jacks, encoders, panel LEDs,\n"
          "the Soft/Hard button, the regulator (laid flat) and the germanium diodes.\n"
          "Audio-path capacitors (C1-C5, C8, C9, C12) must be C0G or film, not X7R.", 16, 126)

root2 = Sheet("Flashover Controls", 1, "Flashover control board - knobs, LED rings, button and panel LEDs")
subs2 = [
    (s7, "controls.kicad_sch", "Controls", 16, 40, []),
    (s8, "led_rings.kicad_sch", "LED Rings", 66, 40, []),
    (s12, "board_connector.kicad_sch", "Board Connector", 116, 40, []),
]
sheet_syms2 = make_root(root2, "Flashover_Controls", subs2)
root2.text("FLASHOVER CONTROLS", 24, 24, size=3.0, bold=True)
root2.text("Control board for Flashover, version 0.1.  It sits behind the top panel of the 1590BB:\n"
           "three endless knobs with 11-LED rings, the Soft/Hard button and the Soft, Hard, On and Link LEDs.\n"
           "It plugs into J7 on the main board (project Flashover) through J8 on its underside.", 24, 30)


def path_of(sh):
    return f"/{sh.root.uuid}/{sh.sym_uuid}"


# =====================================================================
# Footprints
# =====================================================================
FP_BY_REF = {
    # audio-path capacitors: C0G or film, larger bodies
    'C1': 'Capacitor_SMD:C_1206_3216Metric', 'C12': 'Capacitor_SMD:C_1206_3216Metric',
    'C8': 'Capacitor_SMD:C_1206_3216Metric',
    'C3': 'Capacitor_SMD:C_1210_3225Metric', 'C4': 'Capacitor_SMD:C_1210_3225Metric',
    'C9': 'Capacitor_SMD:C_1210_3225Metric',
    'C7': 'Capacitor_SMD:CP_Elec_6.3x5.4',
    'C6': 'Capacitor_SMD:CP_Elec_5x5.4', 'C15': 'Capacitor_SMD:CP_Elec_5x5.4',
    'R39': 'Resistor_SMD:R_2010_5025Metric',
    # germanium clipping diodes only come as through-hole glass
    'D1': 'Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal', 'D2': 'Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal',
    'D3': 'Diode_SMD:D_SMA',
    # panel LEDs poke through the enclosure
    'D40': 'LED_THT:LED_D3.0mm', 'D41': 'LED_THT:LED_D3.0mm', 'D42': 'LED_THT:LED_D3.0mm',
    'D45': 'LED_THT:LED_D3.0mm',
    'U1': 'Package_SO:SOIC-8_3.9x4.9mm_P1.27mm', 'U2': 'Package_SO:SOIC-8_3.9x4.9mm_P1.27mm',
    'U3': 'Package_TO_SOT_THT:TO-220-3_Horizontal_TabDown',   # laid flat under the control board
    'U4': 'Package_DFN_QFN:QFN-56-1EP_7x7mm_P0.4mm_EP3.2x3.2mm',
    'U5': 'Package_SO:SOIC-8_5.3x5.3mm_P1.27mm',
    'U6': 'Package_TO_SOT_SMD:SOT-23-5',
    'U7': 'Package_SO:TSSOP-14_4.4x5mm_P0.65mm', 'U8': 'Package_SO:TSSOP-14_4.4x5mm_P0.65mm',
    'U9': 'Package_SO:TSSOP-14_4.4x5mm_P0.65mm',
    'U10': 'Package_QFP:TQFP-48-1EP_7x7mm_P0.5mm_EP4.11x4.11mm',
    'U11': 'Package_DIP:SMDIP-6_W7.62mm',
    'K1': 'Relay_SMD:Relay_DPDT_Omron_G6K-2F-Y', 'K2': 'Relay_SMD:Relay_DPDT_Omron_G6K-2F-Y',
    'Y1': 'Crystal:Crystal_SMD_3225-4Pin_3.2x2.5mm',
    'Q1': 'Package_TO_SOT_SMD:SOT-23', 'Q2': 'Package_TO_SOT_SMD:SOT-23',
    'J1': 'Connector_Audio:Jack_6.35mm_Neutrik_NRJ4HF_Horizontal',
    'J2': 'Connector_Audio:Jack_6.35mm_Neutrik_NRJ4HF_Horizontal',
    'J3': 'Connector_BarrelJack:BarrelJack_CUI_PJ-102AH_Horizontal',
    'J4': 'Connector_JST:JST_SH_SM03B-SRSS-TB_1x03-1MP_P1.00mm_Horizontal',
    'J5': 'Connector_Audio:Jack_3.5mm_CUI_SJ1-3523N_Horizontal',
    'J6': 'Connector_Audio:Jack_3.5mm_CUI_SJ1-3523N_Horizontal',
    'SW1': 'Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm',
    'SW2': 'Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm',
    'SW3': 'Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm',
    'SW4': 'Button_Switch_THT:SW_PUSH_6mm_H13mm',
    'SW5': 'Connector_JST:JST_PH_B2B-PH-K_1x02_P2.00mm_Vertical',   # footswitch is wired off-board
    'J7': 'Connector_PinSocket_2.54mm:PinSocket_2x10_P2.54mm_Vertical',
    'J8': 'Connector_PinHeader_2.54mm:PinHeader_2x10_P2.54mm_Vertical',   # on the underside
    'H1': 'MountingHole:MountingHole_3.2mm_M3', 'H2': 'MountingHole:MountingHole_3.2mm_M3',
    'H3': 'MountingHole:MountingHole_3.2mm_M3', 'H4': 'MountingHole:MountingHole_3.2mm_M3',
}
FP_BY_LIB = {
    'Device:R': 'Resistor_SMD:R_0805_2012Metric',
    'Device:C': 'Capacitor_SMD:C_0805_2012Metric',
    'Device:D': 'Diode_SMD:D_SOD-123',
    'Device:LED': 'LED_SMD:LED_0805_2012Metric',
}
ALL = (s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12)
missing = []
for sh in ALL:
    for c in sh.comps.values():
        if c.power:
            continue
        c.footprint = FP_BY_REF.get(c.ref) or FP_BY_LIB.get(c.lib_id, '')
        if not c.footprint:
            missing.append(c.ref)
if missing:
    print("NO FOOTPRINT:", sorted(set(missing)))

import kigen
GS1 = KI + "GS1"
OUT2 = KI + "Flashover_Controls"


def write_project(out, project, root, subs, syms):
    os.makedirs(out, exist_ok=True)
    kigen.PROJECT = project
    for sh, fn, *_ in subs:
        bad = sh.check_dangling()
        if bad:
            print("DANGLING on", sh.title, bad)
        with open(os.path.join(out, fn), 'w', encoding='utf-8', newline='\n') as fh:
            fh.write(sh.render(path_of(sh)))
    with open(os.path.join(out, project + ".kicad_sch"), 'w', encoding='utf-8', newline='\n') as fh:
        fh.write(root.render(f"/{root.uuid}", root_extra='\n'.join(syms), is_root=True))
    # project file, based on GS1's settings
    pro = json.load(open(os.path.join(GS1, "GS1.kicad_pro"), encoding='utf-8'))
    pro["meta"]["filename"] = project + ".kicad_pro"
    pro["schematic"]["top_level_sheets"] = [{"filename": project + ".kicad_sch", "name": "Root", "uuid": root.uuid}]
    pro["schematic"]["used_designators"] = ""
    pro["sheets"] = [[root.uuid, "Root"]] + [[sh.sym_uuid, n] for sh, _, n, *_ in subs]
    import rules
    rules.apply(pro)   # design rules, shared with the board scripts
    json.dump(pro, open(os.path.join(out, project + ".kicad_pro"), 'w', encoding='utf-8', newline='\n'), indent=2)
    shutil.copy(os.path.join(GS1, "sym-lib-table"), os.path.join(out, "sym-lib-table"))
    print("written", sorted(os.listdir(out)))


write_project(OUT, "Flashover", root, subs, sheet_syms)
write_project(OUT2, "Flashover_Controls", root2, subs2, sheet_syms2)
