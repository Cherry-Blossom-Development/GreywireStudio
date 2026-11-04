"""Generate the Switchyard KiCad project (schematic only, one board).

Reuses Flashover's schematic writer (pedals/distortion/kicad/scripts/kigen.py). The controller
and MIDI circuits are copied from Tap Changer (pedals/boost/kicad/scripts/gen_tapchanger.py),
which copied them from Flashover. The GPIO numbers and the part references match the stage 1
breadboard (pedals/controller/SWITCHYARD_BREADBOARD_STAGE1.md), so the breadboard firmware
carries over.

Run with any Python 3:  python gen_switchyard.py
"""
import os, sys, json, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
FLASH = os.path.normpath(os.path.join(HERE, "../../../distortion/kicad/scripts"))
sys.path.insert(0, FLASH)
import kigen
from kigen import Sheet, Comp, U, UK, mm, px, py, f, font

KI = os.path.normpath(os.path.join(HERE, "..")).replace("\\", "/") + "/"     # pedals/controller/kicad/
GS1 = os.path.normpath(os.path.join(FLASH, "../GS1"))
DATE = "2026-11-04"

NV = {'ref': (-2, -1, 'right'), 'val': (-2, 1, 'right')}   # fields to the left of a vertical part
Rr = {'ref': (2, -1, 'left'), 'val': (2, 1, 'left')}
POWER = ('+9V', '+5V', '+3V3', 'GND')

# outward direction of a pin, for parts placed without rotation or mirroring
DIRS = {0: (-1, 0), 180: (1, 0), 90: (0, 1), 270: (0, -1)}


def term(s, c, num, net, n=3, shape='passive', kind='global'):
    """Short wire out of a pin, ending in a power symbol, a label, or a no-connect."""
    x, y = c.p(num)
    if net == 'NC':
        s.nc((x, y))
        return
    dx, dy = DIRS[int(c.pins[num][2])]
    e = (x + dx * n, y + dy * n)
    s.wire((x, y), e)
    if net in POWER:
        gnd = net == 'GND'
        if dy == 1:
            rot = 0 if gnd else 180
        elif dy == -1:
            rot = 180 if gnd else 0
        elif dx == -1:
            rot = 270 if gnd else 90
        else:
            rot = 90 if gnd else 270
        s.pwr(net, *e, rot=rot, hide_val=gnd and dx != 0)
    else:
        ang = {(-1, 0): 180, (1, 0): 0, (0, 1): 270, (0, -1): 90}[(dx, dy)]
        s.label(kind, net, *e, ang, shape)


# =====================================================================
# Power: 9V input, 5V converter, 5V power select
# =====================================================================
sp = Sheet("Switchyard - Power", 2, "9V input, 9V-to-5V converter, and the choice between 9V power and the phone's USB power")
s = sp
s.box(8, 36, 56, 76, "A: 9V INPUT")
s.box(56, 36, 140, 76, "B: 9V TO 5V CONVERTER")
s.box(140, 36, 196, 76, "C: 9V SENSE")
s.box(56, 78, 140, 112, "D: 5V POWER SELECT")
# J4 mirrored so pin 2 (+) is on top, as on Tap Changer: pin 2 -> D1 anode, pin 1 -> GND
j4 = s.add("J4", "Connector:Barrel_Jack", "9V DC", 14, 50, mirror='x',
           fields={'ref': (-2, -6, None), 'val': (-2, 5, None)})
d1 = s.add("D1", "Device:D_Schottky", "SS14", 28, 48, rot=180,
           fields={'ref': (0, -4.5, None), 'val': (0, -3, None)})
s.wire(j4.p('2'), d1.p('2'))
s.wire(j4.p('1'), (22, 52), (22, 54), (22, 58))
s.pwr('GND', 22, 58)
s.wire((22, 54), (28, 54))
s.pwr('FLAG', 28, 54)
s.wire(d1.p('1'), (34, 48), (40, 48), (46, 48))
s.pwr('+9V', 34, 48)
s.add("C5", "Device:C_Polarized", "100u", 40, 51)
s.pwr('GND', 40, 54)
s.pwr('FLAG', 46, 48)
s.text("Center-negative 2.1 mm jack, as on every pedal.\nD1 protects against a reversed adapter.\nThe breadboard uses a 1N5817 (same part, leaded).", 10, 66, size=1.0)
# 5V buck converter (AP63205: fixed 5V, values from its datasheet, Table 3)
u3 = s.add("U3", "Regulator_Switching:AP63205WU", "AP63205WU", 80, 52,
           fields={'ref': (0, -7, None), 'val': (0, -5.5, None)})
s.pwr('+9V', 62, 48)
s.wire((62, 48), (62, 50), (66, 50), (69, 50), u3.p('3'))
s.add("C1", "Device:C", "10u 25V", 66, 53, fields=NV)
s.pwr('GND', 66, 56)
s.wire(u3.p('2'), (69, 54), (69, 50))                 # EN tied to VIN: always on
s.pwr('GND', *u3.p('4'))
s.add("C6", "Device:C", "100n", 94, 49, fields=Rr)       # bootstrap, SW to BST
s.wire(u3.p('6'), (94, 52))
s.wire(u3.p('5'), (90, 50), (90, 46), (94, 46))
l1 = s.add("L1", "Device:L", "4.7u", 104, 46, rot=90, fields={'ref': (0, -4, None), 'val': (0, -2.5, None)})
s.wire((94, 46), l1.p('1'))
s.wire(l1.p('2'), (112, 46), (118, 46), (124, 46), (132, 46))
s.add("C2", "Device:C", "22u", 112, 49)
s.add("C4", "Device:C", "22u", 118, 49)
s.pwr('GND', 112, 52)
s.pwr('GND', 118, 52)
s.wire(u3.p('1'), (88, 60), (124, 60), (124, 46))     # FB senses the 5V output
s.pwr('FLAG', 124, 46)
s.label('global', '5V_BUCK', 132, 46, 0, 'output')
s.text("U3: 9V to 5V switching converter, about 85% efficient.\nUp to about 0.6 A: 0.5 A for the phone's keep-alive charge\nplus about 0.1 A for Switchyard. A linear regulator would\nturn about 2.4 W into heat here, so a switcher is needed.\nL1 and C1, C2, C4, C6 are the datasheet's values for 5V.\nKeep U3, L1 and the capacitors close together.", 58, 68, size=1.0)
# 9V sense: tells the firmware the 9V supply is there, so it may offer the phone a charge
s.label('global', '5V_BUCK', 156, 42, 90, 'input')
s.wire((156, 42), (156, 45))
s.add("R11", "Device:R", "10k", 156, 48)
s.add("R12", "Device:R", "15k", 156, 54)
s.wire((156, 51), (162, 51))
s.pwr('GND', 156, 57)
s.label('global', 'SENSE_9V', 162, 51, 0, 'output')
s.text("About 3V on SENSE_9V (GPIO26, ADC0)\nwhen the 9V supply is plugged in,\n0V when Switchyard runs from the phone.", 142, 64, size=1.0)
# power select: 5V from the converter or from the phone, whichever is there
s.label('global', '5V_BUCK', 96, 88, 180, 'input')
d2 = s.add("D2", "Device:D_Schottky", "SS14", 106, 88, rot=180,
           fields={'ref': (0, -4.5, None), 'val': (0, -3, None)})
s.wire((96, 88), d2.p('2'))
s.label('global', 'VBUS', 96, 98, 180, 'input')
d4 = s.add("D4", "Device:D_Schottky", "SS14", 106, 98, rot=180,
           fields={'ref': (0, 2.5, None), 'val': (0, 4, None)})
s.wire((96, 98), d4.p('2'))
s.wire(d2.p('1'), (116, 88), (124, 88))
s.wire(d4.p('1'), (116, 98), (116, 88))
s.pwr('FLAG', 116, 88)
s.pwr('+5V', 124, 88)
s.add("C7", "Device:C", "10u", 124, 91)
s.pwr('GND', 124, 94)
s.text("+5V (about 4.6V after the diodes) comes from whichever is there:\nthe 9V supply through U3, or the phone through USB.\nThe diodes stop either one feeding back into the other.\n+5V only feeds the 3.3V regulator (Controller sheet).", 58, 104, size=1.0)

# =====================================================================
# USB-C and phone power
# =====================================================================
su = Sheet("Switchyard - USB", 3, "USB-C to the phone: data, the USB-C power controller and the keep-alive charge switch")
s = su
s.box(8, 36, 96, 100, "E: USB-C CONNECTOR")
s.box(96, 36, 196, 100, "F: USB-C POWER CONTROLLER")
s.box(8, 104, 120, 150, "G: KEEP-ALIVE CHARGE SWITCH")
j5 = s.add("J5", "Connector:USB_C_Receptacle_USB2.0_16P", "USB-C", 30, 60,
           fields={'ref': (-10, -16, 'right'), 'val': (-10, -14.5, 'right')})
# VBUS: connector -> ESD chip -> label
s.wire(j5.p('A4'), (52, 48), (60, 48), (88, 48))
s.label('global', 'VBUS', 88, 48, 0, 'bidirectional')
s.add("C11", "Device:C", "4.7u", 52, 51, fields=Rr)
s.pwr('GND', 52, 54)
s.wire(j5.p('A5'), (45, 52))
s.label('local', 'CC1', 45, 52)
s.wire(j5.p('B5'), (45, 54))
s.label('local', 'CC2', 45, 54)
# D- (A7, B7) and D+ (A6, B6): the two rows of the reversible plug are joined
s.wire(j5.p('A7'), (45, 58), (45, 60))
s.wire(j5.p('B7'), (45, 60), (56, 60))
s.wire(j5.p('A6'), (45, 62), (56, 62))
s.wire(j5.p('B6'), (45, 64), (45, 62))
s.nc(j5.p('A8'))
s.nc(j5.p('B8'))
s.pwr('GND', *j5.p('A1'))
s.wire(j5.p('SH'), (24, 82), (20, 82))
s.add("R13", "Device:R", "1M", 20, 85, fields=NV)
s.add("C9", "Device:C", "4.7n", 24, 85, fields=Rr)
s.wire((20, 88), (22, 88), (24, 88))
s.pwr('GND', 22, 88)
u9 = s.add("U9", "Power_Protection:USBLC6-2SC6", "USBLC6-2SC6", 60, 60,
           fields={'ref': (-2, -5, 'right'), 'val': (0, 11, None)})
s.wire(u9.p('5'), (60, 48))
s.pwr('GND', *u9.p('2'))
r14 = s.add("R14", "Device:R", "27", 70, 60, rot=90, fields={'ref': (0, -4, None), 'val': (0, -2.5, None)})
r15 = s.add("R15", "Device:R", "27", 70, 62, rot=90, fields={'ref': (6, 1.5, None), 'val': (9, 1.5, None)})
s.wire(u9.p('6'), r14.p('1'))
s.wire(u9.p('4'), r15.p('1'))
s.wire(r14.p('2'), (80, 60))
s.label('global', 'USB_DM', 80, 60, 0, 'bidirectional')
s.wire(r15.p('2'), (80, 62))
s.label('global', 'USB_DP', 80, 62, 0, 'bidirectional')
s.text("The phone is the USB host: it runs the data, and Switchyard\nis a USB MIDI device.  U9 protects the data lines and VBUS\nfrom static.  R14 and R15 are the RP2040's 27 ohm data resistors.\nR13 and C9 tie the shield to ground without a hum path.\nSBU1 and SBU2 aren't used.", 48, 80, size=1.0)
# FUSB302B
u7 = s.add("U7", "Interface_USB:FUSB302BMPX", "FUSB302BMPX", 130, 70,
           fields={'ref': (12, 7, 'left'), 'val': (12, 8.5, 'left')})
term(s, u7, '7', 'SDA', 4, 'bidirectional')
term(s, u7, '6', 'SCL', 4, 'input')
s.wire(u7.p('5'), (116, 72))
s.label('global', 'PD_INT', 116, 72, 180, 'output')
s.add("R16", "Device:R", "10k", 116, 76, fields=NV)
s.wire((116, 72), (116, 73))
s.pwr('+3V3', 116, 79, rot=180)
term(s, u7, '10', 'CC1', 4, kind='local')
term(s, u7, '1', 'CC2', 4, kind='local')
term(s, u7, '2', 'VBUS', 4, 'input')
s.pwr('GND', *u7.p('8'))
# VDD (pins 3, 4) to +3V3 with its capacitors; VCONN (12, 13) unused, tied to GND
s.wire(u7.p('3'), (128, 56), (128, 52))
s.pwr('+3V3', 128, 52)
s.wire((128, 56), (140, 56), (150, 56))
s.add("C10", "Device:C", "100n", 140, 59)
s.add("C12", "Device:C", "1u", 150, 59)
s.pwr('GND', 140, 62)
s.pwr('GND', 150, 62)
s.wire(u7.p('12'), (130, 60), (131, 60), (132, 60), u7.p('13'))
s.pwr('GND', 131, 60, rot=180, hide_val=True)
s.text("U7 handles USB-C's power rules (I2C address 0x22, shares the display's bus).\n"
       "It starts as a power sink, even with no power at all, so the phone powers Switchyard\n"
       "when there's no 9V.  With 9V present the firmware asks the phone for a power role\n"
       "swap: the phone keeps running the data, and Switchyard then supplies VBUS through U8.\n"
       "VCONN (for powered cables) isn't used: the firmware never turns its switch on.\n"
       "PD_INT is open-drain (R16 pull-up).", 98, 88, size=1.0)
# TPS2051C: 5V_BUCK -> VBUS, current limited, off until the firmware turns it on
u8 = s.add("U8", "Power_Management:TPS2051CDBV", "TPS2051C", 60, 120,
           fields={'ref': (0, -6, None), 'val': (0, -4.5, None)})
s.wire(u8.p('5'), (46, 118), (40, 118))
s.label('global', '5V_BUCK', 40, 118, 180, 'input')
s.add("C13", "Device:C", "100n", 46, 121, fields=NV)
s.pwr('GND', 46, 124)
s.wire(u8.p('4'), (50, 122), (50, 126), (50, 130))
s.wire((50, 122), (40, 122))
s.label('global', 'VBUS_EN', 40, 122, 180, 'input')
s.add("R17", "Device:R", "100k", 50, 133, fields=NV)
s.pwr('GND', 50, 136)
s.pwr('GND', *u8.p('2'))
s.wire(u8.p('1'), (76, 118))
s.label('global', 'VBUS', 76, 118, 0, 'output')
s.wire(u8.p('3'), (72, 122), (82, 122))
s.label('global', 'VBUS_FLT', 82, 122, 0, 'output')
s.add("R18", "Device:R", "10k", 72, 125, fields=Rr)
s.pwr('+3V3', 72, 128, rot=180)
s.text("U8 passes 5V to the phone only when VBUS_EN is high (R17 holds it off\nuntil the firmware starts), limits the current, and blocks the phone's\nown 5V from flowing back.  VBUS_FLT goes low on overcurrent.\nKeep-alive charge: 5V at 0.5 A, about 2.5 W.", 10, 144, size=1.0)

# =====================================================================
# Controller (copied from Tap Changer; USB now connected, BOOTSEL button added)
# =====================================================================
sc = Sheet("Switchyard - Controller", 4, "RP2040 microcontroller with its 3.3V supply, flash memory, crystal, programming header and boot button")
s = sc
s.box(8, 18, 76, 60, "H: 3.3V SUPPLY")
s.box(76, 18, 166, 124, "I: MICROCONTROLLER")
s.box(8, 64, 40, 98, "K: PROGRAMMING")
s.box(40, 64, 76, 98, "L: FLASH MEMORY")
s.box(8, 100, 76, 124, "J: CRYSTAL")
s.box(8, 126, 76, 150, "M: BOOT BUTTON")
u4 = s.add("U4", "Regulator_Linear:AP2112K-3.3", "AP2112K-3.3", 40, 36)
s.pwr('+5V', 20, 34)
s.wire((20, 34), (26, 34), (30, 34), u4.p('1'))
s.wire(u4.p('3'), (30, 36), (30, 34))
s.add("C18", "Device:C", "1u", 26, 37, fields=NV)
s.pwr('GND', 26, 40)
s.pwr('GND', 40, 42)
s.nc(u4.p('4'))
s.wire(u4.p('5'), (52, 34), (58, 34))
s.add("C19", "Device:C", "1u", 52, 37)
s.pwr('GND', 52, 40)
s.pwr('+3V3', 58, 34)
s.text("+5V comes from the Power sheet (9V or the phone).\nThe AP2112K's input is limited to 6V, which is why\nit runs from +5V and not from 9V.\n+3V3 feeds the microcontroller, flash, display,\nUSB-C controller and MIDI parts.", 10, 52, size=1.0)
mcu = s.add("U1", "MCU_RaspberryPi:RP2040", "RP2040", 110, 80,
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
    s.add(f"C{20 + i}", "Device:C", v, x, 27)
xs11 = [145, 152, 159]
s.wire(*[(x, 24) for x in xs11])
s.label('local', '+1V1', 145, 24, 180)
for i, (x, v) in enumerate(zip(xs11, ['1u', '100n', '100n'])):
    s.add(f"C{29 + i}", "Device:C", v, x, 27)
s.pwr('GND', 159, 30)
s.text("3.3V: C20 on VREG_VIN, one 100n at each of the six IOVDD pins, USB_VDD, ADC_AVDD.   1.1V: C29 on VREG_VOUT, 100n at each DVDD pin.",
       80, 34, size=1.0)
s.wire(mcu.p(pin_of['TESTEN']), (88, 56), (88, 58))
s.pwr('GND', 88, 58)
s.add("R21", "Device:R", "10k", 82, 59, fields=NV)
s.pwr('+3V3', 82, 56)
s.wire(mcu.p(pin_of['RUN']), (82, 62))
for nm, net in (('USB_DM', 'USB_DM'), ('USB_DP', 'USB_DP')):
    x, y = mcu.p(pin_of[nm])
    s.wire((x, y), (x - 4, y))
    s.label('global', net, x - 4, y, 180, 'bidirectional')
for nm in ('XIN', 'XOUT', 'SWCLK', 'SWDIO'):
    x, y = mcu.p(pin_of[nm])
    s.wire((x, y), (x - 4, y))
    s.label('local', nm, x - 4, y, 180)
used = {'GPIO0': ('LOOP_TX', 'output'), 'GPIO1': ('LOOP_RX', 'input'),
        'GPIO2': ('SW_DOWN', 'input'), 'GPIO3': ('SW_UP', 'input'),
        'GPIO4': ('OTHER_TX', 'output'),
        'GPIO6': ('SDA', 'bidirectional'), 'GPIO7': ('SCL', 'output'),
        'GPIO8': ('PD_INT', 'input'), 'GPIO9': ('VBUS_EN', 'output'), 'GPIO10': ('VBUS_FLT', 'input'),
        'GPIO14': ('LED_DOWN', 'output'), 'GPIO15': ('LED_UP', 'output'),
        'GPIO26/ADC0': ('SENSE_9V', 'input')}
for nm, num in pin_of.items():
    if not nm.startswith('GPIO'):
        continue
    x, y = mcu.p(num)
    if nm in used:
        s.wire((x, y), (x + 4, y))
        s.label('global', used[nm][0], x + 4, y, 0, used[nm][1])
    else:
        s.nc((x, y))
s.text("GPIO plan (no-connect = spare)\nSame numbers as the breadboard,\nso its firmware carries over.\n\n"
       "GPIO0       LOOP_TX: MIDI loop out (UART0 TX)\nGPIO1       LOOP_RX: MIDI loop in (UART0 RX)\n"
       "GPIO2-3    SW_DOWN, SW_UP: footswitches\nGPIO4       OTHER_TX: MIDI out for\n                 other brands (UART1 TX)\n"
       "GPIO5       Spare\nGPIO6-7    SDA, SCL (I2C1): display\n                 and USB-C controller\n"
       "GPIO8       PD_INT: USB-C controller\nGPIO9       VBUS_EN: phone charge on\nGPIO10     VBUS_FLT: charge overcurrent\n"
       "GPIO11-13 Spare\nGPIO14-15 LED_DOWN, LED_UP\nGPIO16-25 Spare\nGPIO26     SENSE_9V (ADC0)\nGPIO27-29 Spare", 168, 22, size=1.0)
s.text("USB goes to the phone (USB sheet).\nSwitchyard's own firmware updates arrive over USB.", 122, 119, size=1.0)
u5 = s.add("U5", "Memory_Flash:W25Q16JVSS", "W25Q16JVSS", 66, 82, mirror='y',
           fields={'ref': (-6, -12, None), 'val': (-6, -10, None)})
for a, b in (('~{QSPI_SS}', '1'), ('QSPI_SCLK', '6'), ('QSPI_SD0', '5'), ('QSPI_SD1', '2'), ('QSPI_SD2', '3'), ('QSPI_SD3', '7')):
    if a == '~{QSPI_SS}':
        # break the line so the boot button can join it with a label
        mx, my = mcu.p(pin_of[a])
        s.wire(u5.p(b), (80, my), (mx, my))
        s.label('local', 'QSPI_SS', 80, my)
    else:
        s.wire(u5.p(b), mcu.p(pin_of[a]))
s.pwr('+3V3', *u5.p('8'))
s.pwr('GND', *u5.p('4'))
s.add("C32", "Device:C", "100n", 46, 82, fields=NV)
s.pwr("+3V3", 46, 79)
s.pwr("GND", 46, 85)
s.text("2 MB: firmware, settings and\nthe setlist (two copies)", 42, 94, size=1.0)
s.label('local', 'XIN', 14, 106, 180)
y1 = s.add("Y1", "Device:Crystal_GND24", "12MHz", 33, 106, fields={'ref': (0, -6.5, None), 'val': (0, -4.5, None)})
s.wire((14, 106), (22, 106), y1.p('1'))
s.add("C33", "Device:C", "15p", 22, 109, fields=NV)
s.pwr('GND', 22, 112)
s.pwr('GND', *y1.p('2'))
r22 = s.add("R22", "Device:R", "1k", 47, 106, rot=90)
s.wire(y1.p('3'), (39, 106), r22.p('1'))
s.add("C34", "Device:C", "15p", 39, 109)
s.pwr('GND', 39, 112)
s.wire(r22.p('2'), (56, 106))
s.label('local', 'XOUT', 56, 106, 0)
s.text("ABM8-272-T3, as on the Raspberry Pi Pico", 10, 120, size=1.0)
j6 = s.add("J6", "Connector_Generic:Conn_01x03", "SWD", 16, 80, mirror='y',
           fields={'ref': (-1, -5, None), 'val': (-1, 5, None)})
s.label('local', 'SWCLK', *j6.p('1'))
s.label('local', 'SWDIO', *j6.p('3'))
s.wire(j6.p('2'), (30, 80), (30, 84))
s.pwr('GND', 30, 84)
s.text("For debugging with the\nRaspberry Pi Debug Probe.\nNot needed to load firmware:\nUSB does that.", 10, 89, size=1.0)
s.label('local', 'QSPI_SS', 14, 134, 180)
r23 = s.add("R23", "Device:R", "1k", 26, 134, rot=90)
sw3 = s.add("SW3", "Switch:SW_Push", "BOOTSEL", 40, 134, fields={'ref': (0, -4, None), 'val': (0, 3, None)})
s.wire((14, 134), r23.p('1'))
s.wire(r23.p('2'), sw3.p('1'))
s.wire(sw3.p('2'), (48, 134), (48, 137))
s.pwr('GND', 48, 137)
s.text("Held at power-up, the RP2040 starts as a USB drive for loading\nfirmware (as the Pico's BOOTSEL button).  Only needed for a blank\nchip or a failed update: the firmware can restart into this mode itself.", 10, 143, size=1.0)

# =====================================================================
# MIDI: loop out, loop in, and the out for other brands (copied from Tap Changer)
# =====================================================================
sm = Sheet("Switchyard - MIDI", 5, "MIDI loop in and out, and the MIDI out for other brands, on 3.5 mm TRS jacks (Type A)")
s = sm
s.box(8, 36, 112, 82, "N: MIDI LOOP IN")
s.box(8, 86, 112, 124, "O: MIDI LOOP OUT")
s.box(114, 86, 214, 124, "P: MIDI OUT FOR OTHER BRANDS")
j2 = s.add("J2", "Connector_Audio:AudioJack3", "Loop In", 16, 60, fields={'ref': (0, -6, None), 'val': (0, -4.5, None)})
s.nc(j2.p('S'))
r5 = s.add("R5", "Device:R", "220", 30, 60, rot=90)
s.wire(j2.p('R'), r5.p('1'))
u2 = s.add("U2", "Isolator:H11L1", "H11L1", 60, 62, fields={'ref': (-4, -8, None), 'val': (-4, -6.5, None)})
s.wire(r5.p('2'), (42, 60), u2.p('1'))
s.add("D3", "Device:D", "1N4148W", 42, 63, rot=270, fields=NV)
s.wire(j2.p('T'), (24, 62), (24, 66), (42, 66), (48, 66), (48, 64), u2.p('2'))
s.pwr('+3V3', 60, 52)
s.wire((60, 52), (70, 52))
s.wire((60, 52), u2.p('6'))
s.add("C3", "Device:C", "100n", 70, 55, fields=NV)
s.pwr('GND', 70, 58)
s.pwr('GND', *u2.p('5'))
s.add("R6", "Device:R", "1k", 76, 59)
s.pwr('+3V3', 76, 56)
s.wire(u2.p('4'), (76, 62), (84, 62))
s.label('global', 'LOOP_RX', 84, 62, 0, 'output')
s.text("The end of the loop: the last pedal's MIDI out plugs in here.\n"
       "Type A wiring: Tip = MIDI pin 5, Ring = MIDI pin 4, Sleeve = shield.\n"
       "The sleeve is left unconnected on the input, as the MIDI spec asks, to avoid ground loops.\n"
       "D3 protects the optocoupler if a reversed (Type B) cable is plugged in.", 10, 74, size=1.0)


def midi_out(s, dx, jref, jval, rt, rr, net, note):
    j = s.add(jref, "Connector_Audio:AudioJack3", jval, 90 + dx, 104, rot=180,
              fields={'ref': (2, -8, None), 'val': (2, -6.5, None)})
    rb = s.add(rt, "Device:R", "10", 74 + dx, 102, rot=90)
    s.wire(j.p('T'), rb.p('2'))
    s.wire(rb.p('1'), (60 + dx, 102))
    s.label('global', net, 60 + dx, 102, 180, 'input')
    s.add(rr, "Device:R", "33 0.5W", 80 + dx, 107, fields=NV)
    s.wire(j.p('R'), (80 + dx, 104))
    s.pwr('+3V3', 80 + dx, 110, rot=180)
    s.wire(j.p('S'), (86 + dx, 110))
    s.pwr('GND', 86 + dx, 110)
    s.text(note, 10 + dx, 116, size=1.0)


midi_out(s, 0, "J1", "Loop Out", "R2", "R1", "LOOP_TX",
         "The start of the loop: plugs into the first pedal's MIDI in.\n"
         "3.3V MIDI output, MIDI Association values: 33 ohm from +3.3V to\nthe Ring, 10 ohm from the UART to the Tip.  Sleeve grounded.")
midi_out(s, 106, "J3", "Other Brands", "R4", "R3", "OTHER_TX",
         "Outside the loop: program changes for other brands' MIDI pedals\n"
         "(through their thru jacks or a splitter).  Same circuit as the\nloop out, on UART1, so it never carries loop traffic.")
s.text("Switchyard sends on LOOP_TX and hears the whole loop back on LOOP_RX:\n"
       "every Linework pedal passes on what it receives and adds its own messages\n"
       "(Linework Control Specification).  A gap in the loop shows up as missing\n"
       "Active Sensing, which is how the app finds a broken cable or a dead pedal.", 116, 40, size=1.0)

# =====================================================================
# Panel: footswitches, LEDs and the song.part display
# =====================================================================
sq = Sheet("Switchyard - Panel", 6, "Down and Up footswitches with their LEDs, and the 4-digit song.part display")
s = sq
s.box(8, 36, 52, 100, "Q: FOOTSWITCHES")
s.box(8, 102, 76, 140, "R: FOOTSWITCH LEDS")
s.box(78, 36, 214, 124, "S: SONG.PART DISPLAY")
for i, (ref, val, rp, cf, net, y) in enumerate((("SW1", "Down", "R19", "C14", "SW_DOWN", 50),
                                                 ("SW2", "Up", "R20", "C15", "SW_UP", 74))):
    sw = s.add(ref, "Switch:SW_Push", val, 40, y, fields={'ref': (0, -4, None), 'val': (0, 3, None)})
    s.add(rp, "Device:R", "10k", 30, y - 3, fields=NV)
    s.pwr('+3V3', 30, y - 6)
    s.add(cf, "Device:C", "100n", 26, y + 3, fields=NV)
    s.pwr('GND', 26, y + 6)
    s.wire(sw.p('1'), (30, y), (26, y), (20, y))
    s.label('global', net, 20, y, 180, 'output')
    s.wire(sw.p('2'), (48, y), (48, y + 3))
    s.pwr('GND', 48, y + 3)
s.text("Soft-touch momentary footswitches on the\nlid, each wired to a 2-pin JST PH connector.\nPressing is the only action: holding does\nnothing extra.  The firmware debounces them;\nthe 100n capacitors filter noise picked up\non the switch wires.", 10, 90, size=1.0)
for i, (ref, rr, net, x) in enumerate((("LED1", "R7", "LED_DOWN", 20), ("LED2", "R8", "LED_UP", 50))):
    s.wire((x, 110), (x, 107), (x + 8, 107))
    s.label('global', net, x + 8, 107, 0, 'input')
    s.add(rr, "Device:R", "470", x, 113, fields=NV)
    s.add(ref, "Device:LED", "Red", x, 119, rot=90, fields={'ref': (2, -1, 'left'), 'val': (2, 1, 'left')})
    s.pwr('GND', x, 122)
s.text("3 mm LEDs above each footswitch, lit while\nit's pressed.  Driven straight from the GPIO\npins: about 3 mA each.", 10, 130, size=1.0)
# HT16K33 display driver and the 4-digit display
u6 = s.add("U6", "GreywireStudio:HT16K33", "HT16K33", 110, 86,
           fields={'ref': (4, -21.5, 'left'), 'val': (4, -20, 'left')})
term(s, u6, '19', 'SDA', 4, 'bidirectional')
term(s, u6, '18', 'SCL', 4, 'input')
s.wire(u6.p('20'), (110, 62), (102, 62), (96, 62))
s.pwr('+3V3', 110, 62)
s.add("C16", "Device:C", "100n", 102, 65)
s.add("C17", "Device:C", "4.7u", 96, 65, fields=NV)
s.pwr('GND', 102, 68)
s.pwr('GND', 96, 68)
s.pwr('GND', *u6.p('1'))
SEG = ['SEG_A', 'SEG_B', 'SEG_C', 'SEG_D', 'SEG_E', 'SEG_F', 'SEG_G', 'SEG_DP']
for i in range(8):
    term(s, u6, str({0: 17, 1: 16, 2: 15, 3: 14, 4: 13, 5: 12, 6: 11, 7: 10}[i]), SEG[i], 3, kind='local')
# COM0, 1, 3, 4 drive the digits, as on Adafruit's backpack (COM2 is its colon), so its library works unchanged
COMS = {0: 'DIG1', 1: 'DIG2', 3: 'DIG3', 4: 'DIG4'}
for i in range(8):
    term(s, u6, str(2 + i), COMS.get(i, 'NC'), 3, kind='local')
ds1 = s.add("DS1", "Display_Character:CC56-12SRWA", "CC56-12SRWA", 170, 86,
            fields={'ref': (0, -12, None), 'val': (0, -10.5, None)})
for num, net in (('11', 'SEG_A'), ('7', 'SEG_B'), ('4', 'SEG_C'), ('2', 'SEG_D'), ('1', 'SEG_E'),
                 ('10', 'SEG_F'), ('5', 'SEG_G'), ('3', 'SEG_DP'),
                 ('12', 'DIG1'), ('9', 'DIG2'), ('8', 'DIG3'), ('6', 'DIG4')):
    term(s, ds1, num, net, 3, kind='local')
s.text("Shows song and part, e.g. 03.2 (the decimal point after digit 2).\n"
       "U6 is the chip on the breadboard's Adafruit backpack (product 878), so the\n"
       "breadboard firmware drives it unchanged: ROW0-7 = segments A-G and DP,\n"
       "COM0, COM1, COM3, COM4 = digits 1-4.  U6 limits the segment current itself,\n"
       "so there are no resistors; brightness is set in firmware (16 steps).\n"
       "Runs from 3.3V so its I2C pins match the RP2040.  Fixed I2C address 0x70.\n"
       "DS1 is a 0.56-inch red display, the same size as the breadboard's.", 80, 116, size=1.0)
s.box(52, 36, 76, 70, "T: I2C PULL-UPS")
for x, ref, net in ((58, "R9", "SDA"), (68, "R10", "SCL")):
    s.add(ref, "Device:R", "4.7k", x, 48)
    s.pwr('+3V3', x, 45)
    s.wire((x, 51), (x, 54))
    s.label('global', net, x, 54, 270, 'bidirectional' if net == 'SDA' else 'input')
s.text("One bus:\nU6 (0x70)\nU7 (0x22)", 54, 65, size=1.0)

# =====================================================================
# Mounting holes (on the root sheet's page)
# =====================================================================
sh_ = Sheet("Switchyard - Mounting", 7, "Mounting holes")
s = sh_
s.box(8, 36, 80, 64, "U: MOUNTING")
for ref, x in (("H1", 18), ("H2", 32), ("H3", 46), ("H4", 60)):
    s.add(ref, "Mechanical:MountingHole", "M3", x, 48, fields={'ref': (3, -1, 'left'), 'val': (3, 1, 'left')}).no_bom = True
s.text("M3 holes for the standoffs.  Positions wait for\nthe enclosure layout (1590BB size is the first to check).", 10, 58, size=1.0)

# =====================================================================
# Root sheet
# =====================================================================
W, H = 40, 16


def make_root(root, project, subs):
    syms = []
    for sh, fn, nm, x, y, pl in subs:
        su_ = UK(project + '/' + fn)
        sh.sym_uuid = su_
        sh.file = fn
        sh.root = root
        sh.project = project
        syms.append(
            f"\t(sheet\n\t\t(at {f(px(x))} {f(py(y))})\n\t\t(size {f(mm(W))} {f(mm(H))})\n\t\t(exclude_from_sim no)\n"
            f"\t\t(in_bom yes)\n\t\t(on_board yes)\n\t\t(dnp no)\n\t\t(fields_autoplaced yes)\n"
            f"\t\t(stroke\n\t\t\t(width 0.1524)\n\t\t\t(type solid)\n\t\t)\n\t\t(fill\n\t\t\t(color 0 0 0 0.0000)\n\t\t)\n"
            f"\t\t(uuid \"{su_}\")\n"
            f"\t\t(property \"Sheetname\" \"{nm}\"\n\t\t\t(at {f(px(x))} {f(py(y) - 0.7116)} 0)\n\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n"
            f"\t\t\t(effects {font()} (justify left bottom))\n\t\t)\n"
            f"\t\t(property \"Sheetfile\" \"{fn}\"\n\t\t\t(at {f(px(x))} {f(py(y + H) + 0.5846)} 0)\n\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n"
            f"\t\t\t(effects {font()} (justify left top))\n\t\t)\n"
            f"\t\t(instances\n\t\t\t(project \"{project}\"\n\t\t\t\t(path \"/{root.uuid}\"\n\t\t\t\t\t(page \"{sh.page}\")\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)")
    return syms


root = Sheet("Switchyard", 1, "Switchyard control pedal - Linework series")
subs = [  # (sheet, file, name, x, y, pins)
    (sp, "power.kicad_sch", "Power", 16, 44, []),
    (su, "usb.kicad_sch", "USB", 66, 44, []),
    (sc, "controller.kicad_sch", "Controller", 116, 44, []),
    (sm, "midi.kicad_sch", "MIDI", 16, 72, []),
    (sq, "panel.kicad_sch", "Panel", 66, 72, []),
    (sh_, "mounting.kicad_sch", "Mounting", 116, 72, []),
]
sheet_syms = make_root(root, "Switchyard", subs)
root.text("SWITCHYARD", 24, 24, size=3.0, bold=True)
root.text("Control pedal, Linework series.  Version 0.1 (schematic draft, not yet built).\n"
          "A phone plugs into USB-C; two footswitches step down and up through the setlist and send\n"
          "each Linework pedal its preset around the MIDI loop.  No audio passes through Switchyard.", 24, 30)
root.text("Signal flow:  phone (USB) <-> controller -> MIDI loop out -> pedals -> MIDI loop in -> controller\n"
          "                                                     controller -> MIDI out for other brands", 16, 66)
root.text("One board.  Whether the display and LEDs move to a second board behind the lid, as on Flashover,\n"
          "is decided with the enclosure layout (1590BB size is the first to check, for two footswitches).\n"
          "Same RP2040, flash, 3.3V regulator and MIDI parts as Flashover and Tap Changer.\n"
          "New here: the 9V-to-5V converter, USB-C with its power controller, and the keep-alive charge switch.\n"
          "Part references and GPIO numbers match the stage 1 breadboard (SWITCHYARD_BREADBOARD_STAGE1.md).", 16, 96)
root.text("Footprints: surface-mount for assembly, through-hole for the jacks and panel LEDs.\n"
          "The footswitches are wired off-board to JST connectors.  The display (DS1) sits behind a window in the lid.", 16, 120)


def path_of(sh):
    return f"/{sh.root.uuid}/{sh.sym_uuid}"


# =====================================================================
# Footprints
# =====================================================================
FP_BY_REF = {
    'C5': 'Capacitor_SMD:CP_Elec_6.3x5.4',
    'C1': 'Capacitor_SMD:C_1206_3216Metric', 'C2': 'Capacitor_SMD:C_1206_3216Metric',
    'C4': 'Capacitor_SMD:C_1206_3216Metric', 'C7': 'Capacitor_SMD:C_1206_3216Metric',
    'L1': 'Inductor_SMD:L_Bourns-SRN4018',
    'R1': 'Resistor_SMD:R_2010_5025Metric', 'R3': 'Resistor_SMD:R_2010_5025Metric',
    'D1': 'Diode_SMD:D_SMA', 'D2': 'Diode_SMD:D_SMA', 'D4': 'Diode_SMD:D_SMA',
    'LED1': 'LED_THT:LED_D3.0mm', 'LED2': 'LED_THT:LED_D3.0mm',   # poke through the lid
    'U1': 'Package_DFN_QFN:QFN-56-1EP_7x7mm_P0.4mm_EP3.2x3.2mm',
    'U2': 'Package_DIP:SMDIP-6_W7.62mm',
    'U3': 'Package_TO_SOT_SMD:TSOT-23-6',
    'U4': 'Package_TO_SOT_SMD:SOT-23-5',
    'U5': 'Package_SO:SOIC-8_5.3x5.3mm_P1.27mm',
    'U6': 'Package_SO:SOIC-20W_7.5x12.8mm_P1.27mm',
    'U7': 'Package_DFN_QFN:WQFN-14-1EP_2.5x2.5mm_P0.5mm_EP1.45x1.45mm',
    'U8': 'Package_TO_SOT_SMD:SOT-23-5',
    'U9': 'Package_TO_SOT_SMD:SOT-23-6',
    'Y1': 'Crystal:Crystal_SMD_3225-4Pin_3.2x2.5mm',
    'DS1': 'Display_7Segment:CA56-12SRWA',      # same outline and pins as the CC (common cathode) part
    'J1': 'Connector_Audio:Jack_3.5mm_CUI_SJ1-3523N_Horizontal',
    'J2': 'Connector_Audio:Jack_3.5mm_CUI_SJ1-3523N_Horizontal',
    'J3': 'Connector_Audio:Jack_3.5mm_CUI_SJ1-3523N_Horizontal',
    'J4': 'Connector_BarrelJack:BarrelJack_CUI_PJ-102AH_Horizontal',
    'J5': 'Connector_USB:USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal',
    'J6': 'Connector_JST:JST_SH_SM03B-SRSS-TB_1x03-1MP_P1.00mm_Horizontal',
    'SW1': 'Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal',      # footswitches are wired off-board
    'SW2': 'Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal',
    'SW3': 'Button_Switch_SMD:SW_Push_1P1T_NO_CK_KMR2',
    'H1': 'MountingHole:MountingHole_3.2mm_M3', 'H2': 'MountingHole:MountingHole_3.2mm_M3',
    'H3': 'MountingHole:MountingHole_3.2mm_M3', 'H4': 'MountingHole:MountingHole_3.2mm_M3',
}
FP_BY_LIB = {
    'Device:R': 'Resistor_SMD:R_0805_2012Metric',
    'Device:C': 'Capacitor_SMD:C_0805_2012Metric',
    'Device:D': 'Diode_SMD:D_SOD-123',
}
ALL = (sp, su, sc, sm, sq, sh_)
missing = []
refs = []
for sh in ALL:
    for c in sh.comps.values():
        if c.power:
            continue
        refs.append(c.ref)
        c.footprint = FP_BY_REF.get(c.ref) or FP_BY_LIB.get(c.lib_id, '')
        if not c.footprint:
            missing.append(c.ref)
if missing:
    print("NO FOOTPRINT:", sorted(set(missing)))
dups = sorted({r for r in refs if refs.count(r) > 1 and r != 'U1'})
if dups:
    print("DUPLICATE REFS:", dups)


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
    json.dump(pro, open(os.path.join(out, project + ".kicad_pro"), 'w', encoding='utf-8', newline='\n'), indent=2)
    shutil.copy(os.path.join(GS1, "sym-lib-table"), os.path.join(out, "sym-lib-table"))
    open(os.path.join(out, "fp-lib-table"), "w", encoding="utf-8", newline="").write("\n".join([
        "(fp_lib_table", "  (version 7)",
        """  (lib (name "GreywireStudio")(type "KiCad")(uri "${KIPRJMOD}/../../../../libraries/GreywireStudio.pretty")"""
        """(options "")(descr "GreywireStudio shared footprints"))""", ")", ""]))
    print("written", out, sorted(os.listdir(out)))


write_project(KI + "Switchyard", "Switchyard", root, subs, sheet_syms)
