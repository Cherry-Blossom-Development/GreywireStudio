"""Generate the Tap Changer KiCad projects (main board and control board schematics).

Reuses Flashover's schematic writer (pedals/distortion/kicad/scripts/kigen.py) and copies
Flashover's power, controller, MIDI, bypass, knob and LED ring sheets, cut down to one knob.
The audio values match pedals/boost/sim/TapChanger_circuit.inc, and the audio part
references match the breadboard guide (TAP_CHANGER_BREADBOARD_STAGE1_BUILD.md).

Run with any Python 3:  python gen_tapchanger.py
"""
import os, sys, json, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
FLASH = os.path.normpath(os.path.join(HERE, "../../../distortion/kicad/scripts"))
sys.path.insert(0, FLASH)
import kigen
from kigen import Sheet, U, UK, mm, px, py, f, font

KI = os.path.normpath(os.path.join(HERE, "..")).replace("\\", "/") + "/"     # pedals/boost/kicad/
GS1 = os.path.normpath(os.path.join(FLASH, "../GS1"))
DATE = "2026-11-03"

POT = "GreywireStudio:MCP45HV51"
PA, PW, PB = '13', '12', '11'   # pot unit pins: end A, wiper, end B
OPAMP = "Amplifier_Operational:OPA1678"
NV = {'ref': (-2, -1, 'right'), 'val': (-2, 1, 'right')}   # fields to the left of a vertical part
Rr = {'ref': (2, -1, 'left'), 'val': (2, 1, 'left')}

# =====================================================================
# Audio: input, Boost stage and output
# =====================================================================
sa = Sheet("Tap Changer - Audio", 2, "Input network, the Boost stage (digital pot in the feedback) and the output network")
s = sa
s.box(8, 40, 62, 100, "A: INPUT")
s.box(62, 40, 116, 100, "B: BOOST STAGE")
s.box(116, 40, 160, 100, "C: OUTPUT")
s.label('global', 'FX_IN', 14, 60, 180, 'input')
s.wire((14, 60), (24, 60), (30, 60))
r1 = s.add("R1", "Device:R", "2.2M", 24, 66)
s.wire((24, 60), r1.p('1'))
s.pwr('GND', 24, 69)
c1 = s.add("C1", "Device:C", "100n", 33, 60, rot=90)
r2 = s.add("R2", "Device:R", "1k", 42, 60, rot=90)
s.wire(c1.p('2'), r2.p('1'))
c2 = s.add("C2", "Device:C", "100p", 50, 66)
s.pwr('GND', 50, 69)
r3 = s.add("R3", "Device:R", "2.2M", 56, 54, fields=NV)
s.pwr('VB', 56, 51)
u1a = s.add("U1", OPAMP, "OPA1678", 74, 62, unit=1)
s.wire(r2.p('2'), (50, 60), (56, 60), u1a.p('3'))
s.wire((50, 60), c2.p('1'))
s.wire(r3.p('2'), (56, 60))
s.text("FX_IN comes from the input jack\nthrough the bypass relay\n(Bypass sheet).", 10, 76, size=1.0)
s.text("R1 stops a pop at the input.\nR1 and R3 load the guitar with\nabout 1.1M.  R2 and C2 keep radio\nsignals out of the op-amp.", 10, 86, size=1.0)
# feedback: Boost pot from the output to the inverting input, then R4 and C3 to VB
s.wire(u1a.p('2'), (64, 64), (64, 74), (64, 77))
rv = s.add("U2", POT, "MCP45HV51-103", 75, 74, rot=90, unit=1, fields={'ref': (9, 3, None), 'val': (9, 5, None)})
s.wire((64, 74), rv.p(PA))
s.wire(rv.p(PW), (78, 71), rv.p(PB))
s.wire(u1a.p('1'), (86, 62), (98, 62))
s.wire(rv.p(PB), (86, 74), (86, 62))
r4 = s.add("R4", "Device:R", "1.1k", 64, 80)
c3 = s.add("C3", "Device:C_Polarized", "10u", 64, 90, rot=180, fields={'ref': (2, -1, 'right'), 'val': (2, 1, 'right')})
s.wire(r4.p('2'), c3.p('2'))
s.pwr('VB', 64, 93, rot=180)
# output network
r12 = s.add("R12", "Device:R", "100", 101, 62, rot=90)
c9 = s.add("C9", "Device:C", "1u", 110, 62, rot=90)
s.wire(r12.p('2'), c9.p('1'))
s.wire(c9.p('2'), (124, 62), (134, 62))
r11 = s.add("R11", "Device:R", "100k", 124, 68)
s.wire((124, 62), r11.p('1'))
s.pwr('GND', 124, 71)
s.label('global', 'FX_OUT', 134, 62, 0, 'output')
s.text("FX_OUT goes to the output jack\nthrough the bypass relay\n(Bypass sheet).", 118, 78, size=1.0)
s.text("R12 keeps a long cable from\nupsetting the op-amp.  R11 drains\nC9 so switching doesn't pop.", 118, 88, size=1.0)
s.text("Gain = 1 + (U2A + wiper) / R4:  about 1.14x (+1 dB) to 10.2x (+20 dB)\n"
       "U2A is the Boost knob: the pot side of an MCP45HV51 digital pot (10k), wired as a\n"
       "variable resistor.  Its control and power pins (U2B) are on the Digital Pot sheet.\n"
       "C3 with R4 sets the bass roll-off (about 14 Hz) and keeps the DC gain at 1.", 8, 104, size=1.0)
s.text("Everything is centered on VB (4.5V) so the digital pot only sees 0V to 9V.\n"
       "U1 = OPA1678 dual op-amp: U1A here, U1B buffers VB (Power sheet).\n"
       "The breadboard uses a TL072 in its place: same pins.\n"
       "Audio-path capacitors C1, C2 and C9 must be C0G or film, not X7R.", 8, 116, size=1.0)

# =====================================================================
# Power and bias
# =====================================================================
sp = Sheet("Tap Changer - Power and Bias", 3, "9V input, 4.5V bias (VB) and 5V supply")
s = sp
s.box(8, 36, 56, 76, "D: 9V INPUT")
s.box(56, 36, 112, 76, "E: BIAS (VB = 4.5V)")
s.box(8, 80, 112, 114, "F: 5V SUPPLY AND OP-AMP POWER")
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
s.text("D3 protects against a reversed adapter.\nThe breadboard uses a 1N5817 (same part, leaded).", 10, 66, size=1.0)
# bias divider and buffer
s.pwr('+9V', 62, 44)
r7 = s.add("R7", "Device:R", "1M", 62, 47)
r8 = s.add("R8", "Device:R", "1M", 62, 56)
s.pwr('GND', 62, 59)
s.wire(r7.p('2'), (62, 51), r8.p('1'))
c6 = s.add("C6", "Device:C_Polarized", "10u", 69, 55)
s.pwr('GND', 69, 58)
u1b = s.add("U1", OPAMP, "OPA1678", 85, 53, unit=2)
s.wire((62, 51), (69, 51), u1b.p('5'))
s.wire((69, 51), c6.p('1'))
s.wire(u1b.p('6'), (77, 55), (77, 61), (93, 61), (93, 53))
s.wire(u1b.p('7'), (93, 53), (103, 53), (109, 53))
s.pwr('FLAG', 103, 53)
s.pwr('VB', 109, 53)
s.text("U1B buffers VB.  It drives VB directly, with no series resistor,\nbecause the Boost stage's R4 and C3 return\nthe audio to VB: a resistor here would change the gain.", 58, 66, size=1.0)
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
s.text("+5V: bypass relay, LED driver, 3.3V regulator.\nAbout 100 mA at most: about 0.4 W in U3.", 10, 105, size=1.0)
# op-amp power and decoupling
u1c = s.add("U1", OPAMP, "OPA1678", 80, 96, unit=3, fields={'ref': (2, -1, 'left'), 'val': (2, 1, 'left')})
s.pwr('+9V', 78, 90)
s.pwr('GND', 78, 102)
c10 = s.add("C10", "Device:C", "100n", 90, 96)
s.pwr('+9V', 90, 93)
s.pwr('GND', 90, 99)
s.text("U1 = OPA1678 dual op-amp:\nU1A Boost stage (Audio sheet), U1B VB buffer (above)", 64, 109, size=1.0)

# =====================================================================
# Controller (copied from Flashover, same GPIO numbers so the firmware carries over)
# =====================================================================
sc = Sheet("Tap Changer - Controller", 4, "RP2040 microcontroller with its 3.3V supply, flash memory, crystal and programming header")
s = sc
s.box(8, 18, 76, 60, "G: 3.3V SUPPLY")
s.box(76, 18, 166, 124, "H: MICROCONTROLLER")
s.box(8, 64, 40, 98, "K: PROGRAMMING")
s.box(40, 64, 76, 98, "J: FLASH MEMORY")
s.box(8, 100, 76, 124, "I: CRYSTAL")
u6 = s.add("U6", "Regulator_Linear:AP2112K-3.3", "AP2112K-3.3", 40, 36)
s.pwr('+5V', 20, 34)
s.wire((20, 34), (26, 34), (30, 34), u6.p('1'))
s.wire(u6.p('3'), (30, 36), (30, 34))
s.add("C31", "Device:C", "1u", 26, 37, fields=NV)
s.pwr('GND', 26, 40)
s.pwr('GND', 40, 42)
s.nc(u6.p('4'))
s.wire(u6.p('5'), (52, 34), (58, 34))
s.add("C32", "Device:C", "1u", 52, 37)
s.pwr('GND', 52, 40)
s.pwr('+3V3', 58, 34)
s.text("+5V comes from U3 on the Power sheet.\n+3V3 feeds the microcontroller, flash\nand the digital pot's logic side.", 10, 50, size=1.0)
mcu = s.add("U4", "MCU_RaspberryPi:RP2040", "RP2040", 110, 80,
            fields={'ref': (-18, -38, 'left'), 'val': (-18, 38, 'left')})
pin_of = {}
for k, v in mcu.pins.items():
    pin_of.setdefault(v[3], k)
s.pwr('+3V3', 96, 40)
s.wire((96, 40), (100, 40), (102, 40), (108, 40), (112, 40))
for nm in ('USB_VDD', 'ADC_AVDD', 'VREG_VIN'):
    s.wire(mcu.p(pin_of[nm]), (mcu.p(pin_of[nm])[0], 40))
s.wire(mcu.p('1'), (108, 40))                              # IOVDD (all six pins)
s.wire(mcu.p(pin_of['VREG_VOUT']), (116, 40), (120, 40), mcu.p('23'))   # internal 1.1V regulator -> DVDD
s.label('local', '+1V1', 116, 40)
s.pwr('GND', *mcu.p(pin_of['GND']))
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
s.wire(mcu.p(pin_of['TESTEN']), (88, 56), (88, 58))
s.pwr('GND', 88, 58)
s.add("R20", "Device:R", "10k", 82, 59, fields=NV)
s.pwr('+3V3', 82, 56)
s.wire(mcu.p(pin_of['RUN']), (82, 62))
s.nc(mcu.p(pin_of['USB_DM']))
s.nc(mcu.p(pin_of['USB_DP']))
for nm in ('XIN', 'XOUT', 'SWCLK', 'SWDIO'):
    x, y = mcu.p(pin_of[nm])
    s.wire((x, y), (x - 4, y))
    s.label('local', nm, x - 4, y, 180)
used = {'GPIO0': ('MIDI_TX', 'output'), 'GPIO1': ('MIDI_RX', 'input'), 'GPIO3': ('FX_ON', 'output'),
        'GPIO4': ('SDA', 'bidirectional'), 'GPIO5': ('SCL', 'output'),
        'GPIO6': ('BOOST_A', 'input'), 'GPIO7': ('BOOST_B', 'input'), 'GPIO8': ('BOOST_SW', 'input'),
        'GPIO15': ('FOOTSW', 'input'), 'GPIO17': ('LED_EN', 'output'),
        'GPIO18': ('LED_SDA', 'bidirectional'), 'GPIO19': ('LED_SCL', 'output'), 'GPIO20': ('LINK_LED', 'output')}
for nm, num in pin_of.items():
    if not nm.startswith('GPIO'):
        continue
    x, y = mcu.p(num)
    if nm in used:
        s.wire((x, y), (x + 4, y))
        s.label('global', used[nm][0], x + 4, y, 0, used[nm][1])
    else:
        s.nc((x, y))
s.text("GPIO plan (no-connect = spare)\nSame numbers as Flashover,\nso the firmware carries over.\n\n"
       "GPIO0       MIDI out (UART0 TX)\nGPIO1       MIDI in (UART0 RX)\nGPIO2       Spare (Flashover's Soft/Hard)\n"
       "GPIO3       FX_ON: bypass relay\nGPIO4-5    SDA, SCL: digital pot (I2C0)\nGPIO6-8    Boost encoder A, B, push\n"
       "GPIO9-14  Spare\nGPIO15     FOOTSW: footswitch\nGPIO16     Spare\nGPIO17     LED_EN: LED driver on/off\n"
       "GPIO18-19 LED_SDA, LED_SCL (I2C1)\nGPIO20     LINK_LED\nGPIO21-29 Spare", 168, 22, size=1.0)
s.text("USB is not used (Tap Changer has no USB port).\nFirmware updates arrive over MIDI.", 122, 119, size=1.0)
u5 = s.add("U5", "Memory_Flash:W25Q16JVSS", "W25Q16JVSS", 66, 82, mirror='y',
           fields={'ref': (-6, -12, None), 'val': (-6, -10, None)})
for a, b in (('~{QSPI_SS}', '1'), ('QSPI_SCLK', '6'), ('QSPI_SD0', '5'), ('QSPI_SD1', '2'), ('QSPI_SD2', '3'), ('QSPI_SD3', '7')):
    s.wire(u5.p(b), mcu.p(pin_of[a]))
s.pwr('+3V3', *u5.p('8'))
s.pwr('GND', *u5.p('4'))
s.add("C28", "Device:C", "100n", 46, 82, fields=NV)
s.pwr("+3V3", 46, 79)
s.pwr("GND", 46, 85)
s.text("2 MB: two firmware copies,\nsettings and 128 presets", 42, 94, size=1.0)
s.label('local', 'XIN', 14, 106, 180)
y1 = s.add("Y1", "Device:Crystal_GND24", "12MHz", 33, 106, fields={'ref': (0, -6.5, None), 'val': (0, -4.5, None)})
s.wire((14, 106), (22, 106), y1.p('1'))
s.add("C29", "Device:C", "15p", 22, 109, fields=NV)
s.pwr('GND', 22, 112)
s.pwr('GND', *y1.p('2'))
r19 = s.add("R19", "Device:R", "1k", 47, 106, rot=90)
s.wire(y1.p('3'), (39, 106), r19.p('1'))
s.add("C30", "Device:C", "15p", 39, 109)
s.pwr('GND', 39, 112)
s.wire(r19.p('2'), (56, 106))
s.label('local', 'XOUT', 56, 106, 0)
s.text("ABM8-272-T3, as on the Raspberry Pi Pico", 10, 120, size=1.0)
j4 = s.add("J4", "Connector_Generic:Conn_01x03", "SWD", 16, 80, mirror='y',
           fields={'ref': (-1, -5, None), 'val': (-1, 5, None)})
s.label('local', 'SWCLK', *j4.p('1'))
s.label('local', 'SWDIO', *j4.p('3'))
s.wire(j4.p('2'), (30, 80), (30, 84))
s.pwr('GND', 30, 84)
s.text("Loads the first firmware\n(Raspberry Pi Debug Probe).\nLater updates come over MIDI.", 10, 90, size=1.0)

# =====================================================================
# Digital pot (control side of U2)
# =====================================================================
sd = Sheet("Tap Changer - Digital Pot", 5, "Control and power side of the MCP45HV51 Boost pot (I2C)")
s = sd
s.box(8, 38, 74, 96, "L: BOOST POT (10k)")
cx, cy = 50, 64
u = s.add("U2", POT, "MCP45HV51-103", cx, cy, unit=2)
s.label('global', 'SCL', *u.p('2'), 180, 'input')
s.label('global', 'SDA', *u.p('4'), 180, 'bidirectional')
for num, hi, xoff in (('5', 0, 17), ('3', 0, 21), ('6', 0, 17), ('8', 1, 21)):
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
s.text("I2C address 0x3C (A1=0, A0=0), as Flashover's Gain pot", 10, 90, size=1.0)
s.box(78, 38, 120, 66, "M: DECOUPLING")
for x, ref, net in ((88, "C35", '+3V3'), (104, "C36", '+9V')):
    s.add(ref, "Device:C", "100n", x, 52)
    s.pwr(net, x, 49)
    s.pwr('GND', x, 55)
s.text("C35 on VL, C36 on V+, next to U2.", 80, 62, size=1.0)
s.box(8, 102, 78, 130, "N: I2C PULL-UPS")
for x, ref, net, lx, ang in ((20, "R21", "SDA", 14, 180), (30, "R22", "SCL", 36, 0)):
    s.add(ref, "Device:R", "4.7k", x, 112)
    s.pwr('+3V3', x, 109)
    s.wire((x, 115), (x, 120), (lx, 120))
    s.label('global', net, lx, 120, ang, 'bidirectional' if net == 'SDA' else 'input')
s.text("The pot side of the chip (U2A) is drawn on the Audio sheet.\n"
       "V+ = 9V and V- = GND, so the pot ends can swing anywhere from 0V to 9V.\n"
       "VL = 3.3V matches the microcontroller's logic.  WLAT tied low: a new setting takes effect at once.\n"
       "SHDN tied high: normal operation.  At power-up the wiper starts at mid-scale (about +15 dB)\n"
       "until the microcontroller restores the saved setting; the bypass relay is still off then.\n\n"
       "Note: the datasheet specifies the analog side from V+ = 10V.  At about 8.6V it still works\n"
       "(it resets below 6V) but the wiper resistance is higher: about 150 ohms instead of 75.\n"
       "That sets the minimum Boost (+1 dB).  Confirm on the breadboard, along with the pot's\n"
       "distortion in a clean signal, the main open question for this pedal.", 82, 74, size=1.0)

# =====================================================================
# MIDI in / out (copied from Flashover)
# =====================================================================
sm = Sheet("Tap Changer - MIDI", 6, "MIDI in and MIDI out on 3.5 mm TRS jacks (Type A) for the Linework loop")
s = sm
s.box(8, 36, 112, 82, "O: MIDI IN")
s.box(8, 86, 112, 124, "P: MIDI OUT")
j5 = s.add("J5", "Connector_Audio:AudioJack3", "MIDI In", 16, 60, fields={'ref': (0, -6, None), 'val': (0, -4.5, None)})
s.nc(j5.p('S'))
ra = s.add("R23", "Device:R", "220", 30, 60, rot=90)
s.wire(j5.p('R'), ra.p('1'))
u8 = s.add("U8", "Isolator:H11L1", "H11L1", 60, 62, fields={'ref': (-4, -8, None), 'val': (-4, -6.5, None)})
s.wire(ra.p('2'), (42, 60), u8.p('1'))
s.add("D4", "Device:D", "1N4148W", 42, 63, rot=270, fields=NV)
s.wire(j5.p('T'), (24, 62), (24, 66), (42, 66), (48, 66), (48, 64), u8.p('2'))
s.pwr('+3V3', 60, 52)
s.wire((60, 52), (70, 52))
s.wire((60, 52), u8.p('6'))
s.add("C33", "Device:C", "100n", 70, 55, fields=NV)
s.pwr('GND', 70, 58)
s.pwr('GND', *u8.p('5'))
s.add("R24", "Device:R", "1k", 76, 59)
s.pwr('+3V3', 76, 56)
s.wire(u8.p('4'), (76, 62), (84, 62))
s.label('global', 'MIDI_RX', 84, 62, 0, 'output')
s.text("Type A wiring: Tip = MIDI pin 5, Ring = MIDI pin 4, Sleeve = shield.\n"
       "The sleeve is left unconnected on the input, as the MIDI spec asks, to avoid ground loops.\n"
       "D4 protects the optocoupler if a reversed (Type B) cable is plugged in.\n"
       "The H11L1 runs from 3.3V and its output idles high, as the UART expects.", 10, 74, size=1.0)
j6 = s.add("J6", "Connector_Audio:AudioJack3", "MIDI Out", 90, 104, rot=180,
           fields={'ref': (2, -8, None), 'val': (2, -6.5, None)})
rb = s.add("R26", "Device:R", "10", 74, 102, rot=90)
s.wire(j6.p('T'), rb.p('2'))
s.wire(rb.p('1'), (60, 102))
s.label('global', 'MIDI_TX', 60, 102, 180, 'input')
s.add("R25", "Device:R", "33 0.5W", 80, 107, fields=NV)
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
# Bypass relay, jacks and footswitch (copied from Flashover)
# =====================================================================
sb = Sheet("Tap Changer - Bypass", 7, "Fail-safe relay bypass with the input and output jacks and the footswitch")
s = sb
s.box(8, 36, 134, 92, "Q: JACKS AND BYPASS RELAY")
s.box(8, 94, 66, 124, "R: FOOTSWITCH")
j1 = s.add("J1", "Connector_Audio:AudioJack2", "Input", 14, 52, mirror='x',
           fields={'ref': (-2, -6, None), 'val': (-2, -4, None)})
s.wire(j1.p('S'), (22, 54), (22, 58))
s.pwr('GND', 22, 58)
j2 = s.add("J2", "Connector_Audio:AudioJack2", "Output", 124, 46, rot=180,
           fields={'ref': (2, -6, None), 'val': (2, -4, None)})
s.wire(j2.p('S'), (116, 48), (116, 52))
s.pwr('GND', 116, 52)
k1 = s.add("K1", "Relay:G6K-2", "G6K-2F-Y 5VDC", 60, 64, rot=180,
           fields={'ref': (-20, -3, None), 'val': (-20, -1, None)})
s.wire(j1.p('T'), (52, 52), k1.p('6'))
s.wire(k1.p('3'), (60, 46), j2.p('T'))
for pin, kind, nm in (('5', 'global', 'FX_IN'), ('7', 'local', 'BYPASS'), ('4', 'global', 'FX_OUT'), ('2', 'local', 'BYPASS')):
    x, y = k1.p(pin)
    s.wire((x, y), (x, y + 2))
    if kind == 'global':
        s.label('global', nm, x, y + 2, 270, 'output' if nm == 'FX_IN' else 'input')
    else:
        s.label('local', nm, x, y + 2, 270)
q1 = s.add("Q1", "Transistor_BJT:MMBT3904", "MMBT3904", 88, 80, mirror='y', fields={'ref': (-6, -1, 'right'), 'val': (-6, 1, 'right')})
s.wire(k1.p('8'), (68, 54), (74, 54), (86, 54), q1.p('3'))
s.wire((74, 54), (74, 61))
s.add("D5", "Device:D", "1N4148W", 74, 64, rot=90, fields=Rr)
s.wire(k1.p('1'), (68, 72), (74, 72), (74, 67))
s.wire((68, 72), (68, 75))
s.pwr('+5V', 68, 75, rot=180)
s.pwr('GND', *q1.p('2'))
rq = s.add("R27", "Device:R", "4.7k", 97, 80, rot=90)
s.wire(q1.p('1'), (94, 80))
s.add("R28", "Device:R", "100k", 94, 83, fields=NV)
s.pwr('GND', 94, 86)
s.wire(rq.p('2'), (106, 80))
s.label('global', 'FX_ON', 106, 80, 0, 'input')
s.text("Relay off = BYPASS: the input jack\nconnects straight to the output jack\nthrough both NC contacts (BYPASS).\nWith no power the relay is off, so a\ndead pedal still passes the guitar.\nRelay on = effect: input -> FX_IN,\nFX_OUT -> output.  R28 keeps it in\nbypass until the firmware starts.", 10, 72, size=1.0)
s.text("To avoid clicks, the firmware\nturns Boost down for a few ms\nwhile the relay switches.", 100, 58, size=1.0)
sw2 = s.add("SW2", "Switch:SW_Push", "Footswitch", 40, 104, fields={'ref': (0, -4, None), 'val': (0, 3, None)})
s.add("R29", "Device:R", "10k", 30, 101, fields=NV)
s.pwr('+3V3', 30, 98)
s.add("C34", "Device:C", "100n", 26, 107, fields=NV)
s.pwr('GND', 26, 110)
s.wire(sw2.p('1'), (30, 104), (26, 104), (22, 104))
s.label('global', 'FOOTSW', 22, 104, 180, 'output')
s.wire(sw2.p('2'), (48, 104), (48, 107))
s.pwr('GND', 48, 107)
s.text("Soft-touch momentary switch on the panel, wired to a 2-pin\nJST PH connector.  Read by the firmware; C34 filters\nnoise picked up on the switch wires.", 10, 117, size=1.0)

# =====================================================================
# Controls board: Boost knob
# =====================================================================
sk = Sheet("Tap Changer - Boost Knob", 2, "Boost endless knob (encoder with push switch)")
s = sk
ENC = "PEC11R-4220F-S0024"
s.box(8, 38, 74, 78, "S: BOOST KNOB")
cx, cy = 44, 60
enc = s.add("SW1", "Device:RotaryEncoder_Switch", ENC, cx, cy, fields={'ref': (0, -7, None), 'val': (-2, 11, None)})
s.wire(enc.p('C'), (cx - 9, cy))
s.pwr('GND', cx - 9, cy, rot=270, hide_val=True)
s.add("R30", "Device:R", "10k", cx - 14, cy - 5)
s.add("C37", "Device:C", "10n", cx - 19, cy - 5, fields=NV)
s.pwr('+3V3', cx - 14, cy - 8)
s.pwr('GND', cx - 19, cy - 8, rot=180, hide_val=True)
s.wire(enc.p('A'), (cx - 14, cy - 2), (cx - 19, cy - 2), (cx - 24, cy - 2))
s.label('global', "BOOST_A", cx - 24, cy - 2, 180, 'output')
s.add("R31", "Device:R", "10k", cx - 14, cy + 5)
s.add("C38", "Device:C", "10n", cx - 19, cy + 5, fields=NV)
s.pwr('+3V3', cx - 14, cy + 8, rot=180)
s.pwr('GND', cx - 19, cy + 8)
s.wire(enc.p('B'), (cx - 14, cy + 2), (cx - 19, cy + 2), (cx - 24, cy + 2))
s.label('global', "BOOST_B", cx - 24, cy + 2, 180, 'output')
s.add("R32", "Device:R", "10k", cx + 12, cy - 5)
s.pwr('+3V3', cx + 12, cy - 8)
s.wire(enc.p('S1'), (cx + 12, cy - 2), (cx + 18, cy - 2))
s.label('global', "BOOST_SW", cx + 18, cy - 2, 0, 'output')
s.wire(enc.p('S2'), (cx + 8, cy + 2), (cx + 8, cy + 5))
s.pwr('GND', cx + 8, cy + 5)
s.text("Encoder: Bourns PEC11R type, 24 steps per turn with a push switch\n"
       "(20 mm shaft, as on Flashover).  No end stops: the LED ring shows the setting.\n"
       "A and B have 10k pull-ups and 10n caps (0.1 ms) to smooth contact bounce;\n"
       "the push switch is debounced in firmware.\n"
       "Holding the footswitch and pressing the knob at power-up enters recovery mode,\n"
       "in place of Flashover's Soft/Hard button.\n"
       "All signals go to the microcontroller on the main board through J8.", 8, 84, size=1.0)

# =====================================================================
# Controls board: LED ring (same driver as Flashover, most outputs unused)
# =====================================================================
sl = Sheet("Tap Changer - LED Ring", 3, "IS31FL3236A LED driver: the 11-LED Boost ring and the On LED")
s = sl
s.box(4, 34, 64, 132, "T: LED DRIVER")
drv = s.add("U7", "Driver_LED:IS31FL3236A-TQ", "IS31FL3236A", 40, 88,
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
# OUT1-11 = Boost ring (Flashover's Gain ring outputs), OUT36 = On (as Flashover); the rest unused
names = {i: f"B{i}" for i in range(1, 12)}
names[36] = 'ON'
for i in range(1, 37):
    x, y = drv.p(dp[f"OUT{i}"])
    if i in names:
        s.wire((x, y), (x + 3, y))
        s.label('local', names[i], x + 3, y, 0)
    else:
        s.nc((x, y))
rows = [("U: BOOST RING", [f"B{i}" for i in range(1, 12)], 6),
        ("V: ON LED", ['ON'], 17)]
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
s.text("The ring: 11 LEDs around the knob, from about 7 o'clock (1) to 5 o'clock (11),\n"
       "lit as a bar and dimmed for dark stages using the driver's PWM.\n"
       "The outputs match Flashover's Gain ring (OUT1-11) and On LED (OUT36),\n"
       "so the firmware carries over.  OUT12-35 are unused.\n\n"
       "Flashover's driver is kept for the first prototype; a smaller chip\n"
       "can be looked at for production.  The driver runs from +5V so blue or\n"
       "white LEDs have enough headroom; its I2C inputs accept 3.3V logic.\n"
       "The ring is a cheap place to try the acrylic light-pipe insert\n"
       "(Construction Research.docx).  LED color is still to be chosen.", 66, 88, size=1.0)
s.box(146, 34, 188, 62, "W: LED BUS PULL-UPS")
for x, ref, net in ((156, "R35", "LED_SDA"), (168, "R36", "LED_SCL")):
    s.add(ref, "Device:R", "4.7k", x, 44)
    s.pwr('+3V3', x, 41)
    s.wire((x, 47), (x, 49))
    s.label('global', net, x, 49, 270, 'bidirectional' if net == 'LED_SDA' else 'input')

# =====================================================================
# Board connectors (main board J7 <-> control board J8)
# =====================================================================
LINK_PINS = {1: '+5V', 2: 'GND', 3: '+3V3', 4: 'GND', 5: 'BOOST_A', 6: 'BOOST_B',
             7: 'BOOST_SW', 8: 'LED_EN', 9: 'LED_SDA', 10: 'LED_SCL', 11: 'LINK_LED', 12: 'GND'}


def board_connector(s, ref, value, cx, cy):
    j = s.add(ref, "Connector_Generic:Conn_02x06_Odd_Even", value, cx, cy,
              fields={'ref': (1, -7, None), 'val': (1, 9, None)})
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


sj = Sheet("Tap Changer - Board Connector", 8, "J7: connector to the control board (TapChanger_Controls)")
s = sj
s.box(40, 40, 140, 90, "X: CONTROL BOARD CONNECTOR")
board_connector(s, "J7", "To control board", 88, 62)
s.text("2x6 socket, 2.54 mm.  The control board's J8 header plugs in from above.\n"
       "It carries +5V (LED driver), +3V3 (pull-ups), the knob signals,\n"
       "the LED driver bus and the Link LED.", 42, 80, size=1.0)
s.box(144, 40, 188, 70, "Y: MOUNTING")
for ref, x in (("H1", 156), ("H2", 172)):
    s.add(ref, "Mechanical:MountingHole", "M3", x, 54, fields={'ref': (3, -1, 'left'), 'val': (3, 1, 'left')}).no_bom = True
s.text("M3 holes for the standoffs\nthat hold the control board.", 146, 63, size=1.0)

sk2 = Sheet("Tap Changer Controls - Board Connector", 4, "J8: connector to the main board, and the Link LED")
s = sk2
s.box(8, 36, 104, 100, "X: MAIN BOARD CONNECTOR")
board_connector(s, "J8", "To main board", 56, 60)
for i, net in enumerate(('+5V', '+3V3', 'GND')):
    x = 16 + 14 * i
    s.pwr(net, x, 88)
    s.wire((x, 88), (x + 4, 88))
    s.pwr('FLAG', x + 4, 88)
s.text("2x6 header, 2.54 mm, on the underside;\nplugs into J7 on the main board.\n"
       "Power comes from the main board (flags).", 10, 94, size=1.0)
s.box(108, 36, 176, 66, "Z: LINK LED")
s.label('global', 'LINK_LED', 114, 46, 180, 'input')
rl = s.add("R37", "Device:R", "330", 122, 46, rot=90)
s.wire((114, 46), rl.p('1'))
dl = s.add("D18", "Device:LED", "Link", 132, 46, rot=180, fields={'ref': (0, -3, None), 'val': (0, 3, None)})
s.wire(rl.p('2'), dl.p('2'))
s.wire(dl.p('1'), (138, 46), (138, 49))
s.pwr('GND', 138, 49)
s.text("Lit while the pedal is linked to Switchyard.\nDriven straight from GPIO20 (about 4 mA for a red LED).", 110, 58, size=1.0)
s.box(108, 70, 176, 100, "Y: MOUNTING")
for ref, x in (("H3", 120), ("H4", 136)):
    s.add(ref, "Mechanical:MountingHole", "M3", x, 84, fields={'ref': (3, -1, 'left'), 'val': (3, 1, 'left')}).no_bom = True
s.text("M3 holes, lined up with H1 and H2 on the main board.", 110, 93, size=1.0)

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


root = Sheet("Tap Changer", 1, "Tap Changer clean boost - Linework series - main board")
subs = [  # (sheet, file, name, x, y, pins)
    (sa, "audio.kicad_sch", "Audio", 16, 40, []),
    (sp, "power.kicad_sch", "Power and Bias", 66, 40, []),
    (sb, "bypass.kicad_sch", "Bypass", 116, 40, []),
    (sc, "controller.kicad_sch", "Controller", 16, 72, []),
    (sd, "digital_pot.kicad_sch", "Digital Pot", 66, 72, []),
    (sm, "midi.kicad_sch", "MIDI", 116, 72, []),
    (sj, "board_connector.kicad_sch", "Board Connector", 166, 72, []),
]
sheet_syms = make_root(root, "TapChanger", subs)
root.text("TAP CHANGER", 24, 24, size=3.0, bold=True)
root.text("Clean boost pedal, Linework series.  Main board, version 0.1 (schematic draft, not yet built).\n"
          "One non-inverting op-amp stage with a digital pot in its feedback: Boost from about +1 dB to +20 dB.\n"
          "Values match the LTspice model in pedals/boost/sim/TapChanger_circuit.inc.\n"
          "Power, controller, MIDI and bypass are copied from Flashover, with the same GPIO numbers.", 24, 30)
root.text("Signal flow:  input jack (Bypass)  ->  Audio  ->  output jack (Bypass)", 16, 62)
root.text("Two boards, as on Flashover.  This main board holds the audio circuit, controller, digital pot,\n"
          "MIDI, bypass and all five jacks.  The Boost knob, its LED ring and the panel LEDs are on the\n"
          "control board (project TapChanger_Controls), which plugs into J7 on the Board Connector sheet.\n"
          "The enclosure isn't chosen yet (Hammond 125B is the first to check), so no board layout yet.", 16, 96)
root.text("Footprints: surface-mount for assembly, through-hole for jacks, the encoder, panel LEDs\n"
          "and the regulator (laid flat).  Audio-path capacitors (C1, C2, C9) must be C0G or film, not X7R.", 16, 118)

root2 = Sheet("Tap Changer Controls", 1, "Tap Changer control board - Boost knob, LED ring and panel LEDs")
subs2 = [
    (sk, "boost_knob.kicad_sch", "Boost Knob", 16, 40, []),
    (sl, "led_ring.kicad_sch", "LED Ring", 66, 40, []),
    (sk2, "board_connector.kicad_sch", "Board Connector", 116, 40, []),
]
sheet_syms2 = make_root(root2, "TapChanger_Controls", subs2)
root2.text("TAP CHANGER CONTROLS", 24, 24, size=3.0, bold=True)
root2.text("Control board for Tap Changer, version 0.1.  It sits behind the top panel:\n"
           "the Boost endless knob with its 11-LED ring, and the On and Link LEDs.\n"
           "It plugs into J7 on the main board (project TapChanger) through J8 on its underside.", 24, 30)


def path_of(sh):
    return f"/{sh.root.uuid}/{sh.sym_uuid}"


# =====================================================================
# Footprints (as Flashover's, where the part is the same)
# =====================================================================
FP_BY_REF = {
    'C1': 'Capacitor_SMD:C_1206_3216Metric',      # film or C0G
    'C9': 'Capacitor_SMD:C_1210_3225Metric',
    'C3': 'Capacitor_SMD:CP_Elec_5x5.4', 'C6': 'Capacitor_SMD:CP_Elec_5x5.4',
    'C7': 'Capacitor_SMD:CP_Elec_6.3x5.4',
    'R25': 'Resistor_SMD:R_2010_5025Metric',
    'D3': 'Diode_SMD:D_SMA',
    'D17': 'LED_THT:LED_D3.0mm', 'D18': 'LED_THT:LED_D3.0mm',   # panel LEDs poke through the enclosure
    'U1': 'Package_SO:SOIC-8_3.9x4.9mm_P1.27mm',
    'U2': 'Package_SO:TSSOP-14_4.4x5mm_P0.65mm',
    'U3': 'Package_TO_SOT_THT:TO-220-3_Horizontal_TabDown',
    'U4': 'Package_DFN_QFN:QFN-56-1EP_7x7mm_P0.4mm_EP3.2x3.2mm',
    'U5': 'Package_SO:SOIC-8_5.3x5.3mm_P1.27mm',
    'U6': 'Package_TO_SOT_SMD:SOT-23-5',
    'U7': 'Package_QFP:TQFP-48-1EP_7x7mm_P0.5mm_EP4.11x4.11mm',
    'U8': 'Package_DIP:SMDIP-6_W7.62mm',
    'K1': 'Relay_SMD:Relay_DPDT_Omron_G6K-2F-Y',
    'Y1': 'Crystal:Crystal_SMD_3225-4Pin_3.2x2.5mm',
    'Q1': 'Package_TO_SOT_SMD:SOT-23',
    'J1': 'Connector_Audio:Jack_6.35mm_Neutrik_NRJ4HF_Horizontal',
    'J2': 'Connector_Audio:Jack_6.35mm_Neutrik_NRJ4HF_Horizontal',
    'J3': 'Connector_BarrelJack:BarrelJack_CUI_PJ-102AH_Horizontal',
    'J4': 'Connector_JST:JST_SH_SM03B-SRSS-TB_1x03-1MP_P1.00mm_Horizontal',
    'J5': 'Connector_Audio:Jack_3.5mm_CUI_SJ1-3523N_Horizontal',
    'J6': 'Connector_Audio:Jack_3.5mm_CUI_SJ1-3523N_Horizontal',
    'J7': 'Connector_PinSocket_2.54mm:PinSocket_2x06_P2.54mm_Vertical',
    'J8': 'Connector_PinHeader_2.54mm:PinHeader_2x06_P2.54mm_Vertical',   # on the underside
    'SW1': 'GreywireStudio:RotaryEncoder_Bourns_PEC11R-4xxxF-Sxxxx_Vertical',
    'SW2': 'Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal',      # footswitch is wired off-board
    'H1': 'MountingHole:MountingHole_3.2mm_M3', 'H2': 'MountingHole:MountingHole_3.2mm_M3',
    'H3': 'MountingHole:MountingHole_3.2mm_M3', 'H4': 'MountingHole:MountingHole_3.2mm_M3',
}
FP_BY_LIB = {
    'Device:R': 'Resistor_SMD:R_0805_2012Metric',
    'Device:C': 'Capacitor_SMD:C_0805_2012Metric',
    'Device:D': 'Diode_SMD:D_SOD-123',
    'Device:LED': 'LED_SMD:LED_0805_2012Metric',
}
missing = []
for sh in (sa, sp, sc, sd, sm, sb, sk, sl, sj, sk2):
    for c in sh.comps.values():
        if c.power:
            continue
        c.footprint = FP_BY_REF.get(c.ref) or FP_BY_LIB.get(c.lib_id, '')
        if not c.footprint:
            missing.append(c.ref)
if missing:
    print("NO FOOTPRINT:", sorted(set(missing)))


def write_project(out, project, root, subs, syms):
    os.makedirs(out, exist_ok=True)
    kigen.PROJECT = project
    for sh, fn, *_ in subs:
        bad = sh.check_dangling()
        if bad:
            print("DANGLING on", sh.title, bad)
        txt = sh.render(path_of(sh)).replace('(date "2026-10-09")', f'(date "{DATE}")')
        with open(os.path.join(out, fn), 'w', encoding='utf-8', newline='\n') as fh:
            fh.write(txt)
    txt = root.render(f"/{root.uuid}", root_extra='\n'.join(syms), is_root=True).replace('(date "2026-10-09")', f'(date "{DATE}")')
    with open(os.path.join(out, project + ".kicad_sch"), 'w', encoding='utf-8', newline='\n') as fh:
        fh.write(txt)
    pro = json.load(open(os.path.join(GS1, "GS1.kicad_pro"), encoding='utf-8'))
    pro["meta"]["filename"] = project + ".kicad_pro"
    pro["schematic"]["top_level_sheets"] = [{"filename": project + ".kicad_sch", "name": "Root", "uuid": root.uuid}]
    pro["schematic"]["used_designators"] = ""
    pro["sheets"] = [[root.uuid, "Root"]] + [[sh.sym_uuid, n] for sh, _, n, *_ in subs]
    import rules
    rules.apply(pro)   # same design rules as Flashover
    # U1B's output drives VB directly (no series resistor; see the Power sheet), so the
    # check must accept an op-amp output on a flagged power net. Pin types: 1 = output, 8 = power output.
    pm = pro["erc"]["pin_map"]
    pm[1][8] = pm[8][1] = 0
    json.dump(pro, open(os.path.join(out, project + ".kicad_pro"), 'w', encoding='utf-8', newline='\n'), indent=2)
    shutil.copy(os.path.join(GS1, "sym-lib-table"), os.path.join(out, "sym-lib-table"))
    open(os.path.join(out, "fp-lib-table"), "w", encoding="utf-8", newline="").write("\n".join([
        "(fp_lib_table", "  (version 7)",
        """  (lib (name "GreywireStudio")(type "KiCad")(uri "${KIPRJMOD}/../../../../libraries/GreywireStudio.pretty")"""
        """(options "")(descr "GreywireStudio shared footprints"))""", ")", ""]))
    print("written", out, sorted(os.listdir(out)))


write_project(KI + "TapChanger", "TapChanger", root, subs, sheet_syms)
write_project(KI + "TapChanger_Controls", "TapChanger_Controls", root2, subs2, sheet_syms2)
