"""Generate the Flashover breakout board's KiCad project (schematic): the three digital pots and
the LED driver from the main board, on a small board with two rows of pins for a breadboard.

It is for the stage 2-5 breadboard (FLASHOVER_BREADBOARD_STAGE2.md): those chips only come in
tiny surface-mount packages, so JLCPCB solders them onto this board. Same chips, same
addresses, same supporting parts and the same part references as the main board, so the
firmware runs unchanged. Two differences:
  - the LED driver is the QFN-44 package (IS31FL3236A-QFLS2), which JLCPCB stocks; the main
    board has the TQFP-48 (IS31FL3236A-TQLS4), the same chip
  - each pot has 100 nF decoupling on VL and V+ (C101-C106)

Run with any Python 3:  python gen_breakout.py   (then gen_breakout_pcb.py with KiCad's Python)
"""
import os, json, shutil

import kigen
from kigen import Sheet, UK, mm, px, py, f, font
from paths import KI

DATE = "2026-11-05"
PROJECT = "Flashover_Breakout"
POWER = ('+9V', '+5V', '+3V3', 'GND')
DIRS = {0: (-1, 0), 180: (1, 0), 90: (0, 1), 270: (0, -1)}   # outward direction of a pin
POT = "GreywireStudio:MCP45HV51"
VERT = {'ref': (2, -1, 'left'), 'val': (2, 1, 'left')}


def term(s, c, num, net, n=3):
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
        rot = {(0, 1): 0 if gnd else 180, (0, -1): 180 if gnd else 0,
               (-1, 0): 270 if gnd else 90, (1, 0): 90 if gnd else 270}[(dx, dy)]
        s.pwr(net, *e, rot=rot, hide_val=gnd and dx != 0)
    else:
        ang = {(-1, 0): 180, (1, 0): 0, (0, 1): 270, (0, -1): 90}[(dx, dy)]
        s.label('local', net, *e, ang)


def cap(s, ref, val, x, y, top, bottom='GND'):
    """Vertical capacitor (or resistor) between two power nets, pins at y - 3 and y + 3."""
    lib = "Device:C" if ref.startswith('C') else "Device:R"
    s.add(ref, lib, val, x, y, fields=VERT)
    s.pwr(top, x, y - 3)
    s.pwr(bottom, x, y + 3)


s = Sheet("Flashover Breakout", 2, "Digital pots and LED driver on a breadboard-friendly board, for the stage 2-5 breadboard")

# ---------------------------------------------------------------- digital pots
POTS = [("U7", "MCP45HV51-104", "GAIN", 0, 0), ("U8", "MCP45HV51-103", "TONE", 0, 1),
        ("U9", "MCP45HV51-103", "LEVEL", 1, 0)]
for i, (ref, val, name, a1, a0) in enumerate(POTS):
    x0 = 4 + 38 * i
    s.box(x0, 30, x0 + 38, 84, f"{'ABC'[i]}: {name} POT ({ref})")
    cx, cy = x0 + 22, 52
    u = s.add(ref, POT, val, cx, cy, unit=2, fields={'ref': (5, -9, 'left'), 'val': (5, -7, 'left')})
    term(s, u, '1', '+3V3')
    term(s, u, '14', '+9V', 6)      # staggered, so the two supply names don't overlap
    term(s, u, '9', 'GND')
    term(s, u, '10', 'GND')
    term(s, u, '2', 'SCL')
    term(s, u, '4', 'SDA')
    term(s, u, '5', '+3V3' if a0 else 'GND')
    term(s, u, '3', '+3V3' if a1 else 'GND')
    term(s, u, '6', 'GND')          # WLAT low: a new setting takes effect at once
    term(s, u, '8', '+3V3')         # SHDN high: normal operation
    term(s, u, '7', 'NC')
    p = s.add(ref, POT, val, x0 + 12, 74, unit=1, hide_val=True, fields={'ref': (-2, 1, 'right'), 'val': (-2, 3, 'right')})
    term(s, p, '13', f"{name}_A", 2)
    term(s, p, '12', f"{name}_W", 2)
    term(s, p, '11', f"{name}_B", 2)
    cap(s, f"C{101 + i}", "100n", x0 + 24, 78, "+3V3")
    cap(s, f"C{104 + i}", "100n", x0 + 32, 78, "+9V")
    s.text(f"I2C address 0x{0x3C + 2 * a1 + a0:02X} (A1={a1}, A0={a0})", x0 + 1, 33, size=1.0)

s.box(4, 88, 42, 112, "D: POT BUS PULL-UPS")
for x, ref, net in ((14, "R21", "SDA"), (28, "R22", "SCL")):
    r = s.add(ref, "Device:R", "4.7k", x, 98, fields=VERT)
    s.pwr('+3V3', x, 95)
    s.wire((x, 101), (x, 104))
    s.label('local', net, x, 104, 270)
s.text("V+ = 9V (the pots' audio side swings 0-9V), V- = GND,\nVL = 3.3V logic.  C101-C106 decouple VL and V+.",
       44, 92, size=1.0)

# ---------------------------------------------------------------- LED driver
s.box(119, 24, 166, 120, "E: LED DRIVER (U10)")
d = s.add("U10", "GreywireStudio:IS31FL3236A-QF", "IS31FL3236A-QFLS2", 140, 75,
          fields={'ref': (8, -41, 'left'), 'val': (8, -39, 'left')})
pn = {v[3]: k for k, v in d.pins.items() if v[3] != 'GND'}
x, y = d.p(pn['VCC'])
s.wire((x, y), (x, y - 8), (x - 12, y - 8), (x - 19, y - 8))
s.pwr('+5V', x, y - 8)
for cx, ref, val in ((x - 12, "C39", "1u"), (x - 19, "C40", "100n")):   # below the +5V wire, clear of the chip
    s.add(ref, "Device:C", val, cx, y - 5, fields=VERT)
    s.pwr('GND', cx, y - 2)
term(s, d, '17', 'GND')
term(s, d, pn['SDA'], 'LED_SDA', 4)
term(s, d, pn['SCL'], 'LED_SCL', 4)
term(s, d, pn['AD'], 'GND', 2)                      # AD at ground: address 0x3C, on its own bus
x, y = d.p(pn['~{SDB}'])
s.wire((x, y), (x - 4, y), (x - 8, y))
s.label('local', 'LED_EN', x - 8, y, 180)
s.add("R34", "Device:R", "100k", x - 4, y + 3, fields={'ref': (-2, -1, 'right'), 'val': (-2, 1, 'right')})
s.pwr('GND', x - 4, y + 6)
x, y = d.p(pn['R_EXT'])
s.wire((x, y), (x - 4, y))
s.add("R33", "Device:R", "15k", x - 4, y + 3, fields={'ref': (-2, -1, 'right'), 'val': (-2, 1, 'right')})
s.pwr('GND', x - 4, y + 6)
for n in range(1, 37):
    term(s, d, pn[f"OUT{n}"], f"OUT{n}", 2)
s.text("R33 sets the LED current:\n58.5 x 1.3V / 15k = about 5 mA.\nR34 keeps the LEDs off until\nthe firmware raises LED_EN.",
       100, 100, size=1.0)
s.box(44, 98, 76, 120, "F: LED BUS PULL-UPS")
for x, ref, net in ((52, "R35", "LED_SDA"), (66, "R36", "LED_SCL")):
    s.add(ref, "Device:R", "4.7k", x, 106, fields=VERT)
    s.pwr('+3V3', x, 103)
    s.wire((x, 109), (x, 112))
    s.label('local', net, x, 112, 270)

# ---------------------------------------------------------------- pins
J1 = {1: '+9V', 2: 'GND', 3: 'GAIN_A', 4: 'GAIN_W', 5: 'GAIN_B', 6: 'TONE_A', 7: 'TONE_W', 8: 'TONE_B',
      9: 'LEVEL_A', 10: 'LEVEL_W', 11: 'LEVEL_B', 12: 'GND'}
# The LED outputs are in the order they leave the chip, so the board's tracks fan straight out
# with no crossings: row 1 has OUT18 down to OUT1, row 2 has OUT19 up to OUT36. (Read round the
# board like a chip: OUT1 at the right end of row 1, along to OUT18, then OUT19 at the left of
# row 2 along to OUT36.) Pins 13-30 of both rows line up.
J1.update({p: f"OUT{31 - p}" for p in range(13, 31)})
J2 = {1: '+3V3', 2: 'GND', 3: 'SDA', 4: 'SCL', 5: 'GND', 6: 'LED_EN', 7: 'LED_SDA', 8: 'LED_SCL',
      9: 'GND', 10: 'GND', 11: 'GND', 12: '+5V'}     # 5V next to the outputs: a clear path from the driver
J2.update({p: f"OUT{p + 6}" for p in range(13, 31)})
for ref, pins, cx, title in (("J1", J1, 186, "G: PINS, ROW 1"), ("J2", J2, 214, "H: PINS, ROW 2")):
    s.box(cx - 16, 20, cx + 6, 94, title)
    j = s.add(ref, "Connector_Generic:Conn_01x30", "Row " + ref[1], cx, 55,
              fields={'ref': (0, -31, None), 'val': (0, 33, None)})
    for k, net in pins.items():
        term(s, j, str(k), net, 3)
s.box(170, 96, 224, 120, "I: POWER IN")
for i, net in enumerate(('+9V', '+5V', '+3V3', 'GND')):
    x = 175 + 12 * i
    s.pwr(net, x, 106)
    s.wire((x, 106), (x + 4, 106))
    s.pwr('FLAG', x + 4, 106)
s.text("All power comes in on the pins\nfrom the breadboard (flags).", 172, 112, size=1.0)

# ---------------------------------------------------------------- root sheet
W, H = 40, 16
root = Sheet("Flashover Breakout", 1, "Flashover breakout board - digital pots and LED driver for the breadboard")
s.sym_uuid = UK(PROJECT + "/breakout.kicad_sch")
root_sym = (
    f"\t(sheet\n\t\t(at {f(px(30))} {f(py(50))})\n\t\t(size {f(mm(W))} {f(mm(H))})\n\t\t(exclude_from_sim no)\n"
    f"\t\t(in_bom yes)\n\t\t(on_board yes)\n\t\t(dnp no)\n\t\t(fields_autoplaced yes)\n"
    f"\t\t(stroke\n\t\t\t(width 0.1524)\n\t\t\t(type solid)\n\t\t)\n\t\t(fill\n\t\t\t(color 0 0 0 0.0000)\n\t\t)\n"
    f"\t\t(uuid \"{s.sym_uuid}\")\n"
    f"\t\t(property \"Sheetname\" \"Breakout\"\n\t\t\t(at {f(px(30))} {f(py(50) - 0.7116)} 0)\n\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n"
    f"\t\t\t(effects {font()} (justify left bottom))\n\t\t)\n"
    f"\t\t(property \"Sheetfile\" \"breakout.kicad_sch\"\n\t\t\t(at {f(px(30))} {f(py(50 + H) + 0.5846)} 0)\n\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n"
    f"\t\t\t(effects {font()} (justify left top))\n\t\t)\n"
    f"\t\t(instances\n\t\t\t(project \"{PROJECT}\"\n\t\t\t\t(path \"/{root.uuid}\"\n\t\t\t\t\t(page \"2\")\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)")
root.text("FLASHOVER BREAKOUT BOARD", 24, 24, size=3.0, bold=True)
root.text("For the breadboard prototype (stages 2-5).  The three MCP45HV51 digital pots and the IS31FL3236A\n"
          "LED driver from the Flashover main board, which only come in tiny surface-mount packages, on a\n"
          "small 2-layer board that JLCPCB assembles.  Two rows of 30 pins, 17.78 mm (0.7\") apart like a\n"
          "Raspberry Pi Pico, so it straddles a breadboard's centre gap.", 24, 30)
root.text("Same chips, addresses, supporting parts and references as the main board, so the firmware runs\n"
          "unchanged.  The LED driver here is the QFN-44 package (the main board has the TQFP-48 one).", 24, 72)

# ---------------------------------------------------------------- footprints
FP = {
    'U7': 'Package_SO:TSSOP-14_4.4x5mm_P0.65mm', 'U8': 'Package_SO:TSSOP-14_4.4x5mm_P0.65mm',
    'U9': 'Package_SO:TSSOP-14_4.4x5mm_P0.65mm',
    'U10': 'GreywireStudio:QFN-44-1EP_5x5mm_P0.4mm_EP2.45x2.45mm',
    'J1': 'Connector_PinHeader_2.54mm:PinHeader_1x30_P2.54mm_Vertical',
    'J2': 'Connector_PinHeader_2.54mm:PinHeader_1x30_P2.54mm_Vertical',
}
for c in s.comps.values():
    if c.power:
        continue
    c.footprint = FP.get(c.ref) or {'Device:R': 'Resistor_SMD:R_0603_1608Metric',
                                    'Device:C': 'Capacitor_SMD:C_0603_1608Metric'}[c.lib_id]

# ---------------------------------------------------------------- write
out = KI + PROJECT
os.makedirs(out, exist_ok=True)
kigen.PROJECT = PROJECT
bad = s.check_dangling()
if bad:
    print("DANGLING", bad)
txt = s.render(f"/{root.uuid}/{s.sym_uuid}").replace('(date "2026-10-09")', f'(date "{DATE}")')
open(os.path.join(out, "breakout.kicad_sch"), 'w', encoding='utf-8', newline='\n').write(txt)
txt = root.render(f"/{root.uuid}", root_extra=root_sym, is_root=True).replace('(date "2026-10-09")', f'(date "{DATE}")')
open(os.path.join(out, PROJECT + ".kicad_sch"), 'w', encoding='utf-8', newline='\n').write(txt)
GS1 = KI + "GS1/"
pro = json.load(open(GS1 + "GS1.kicad_pro", encoding='utf-8'))
pro["meta"]["filename"] = PROJECT + ".kicad_pro"
pro["schematic"]["top_level_sheets"] = [{"filename": PROJECT + ".kicad_sch", "name": "Root", "uuid": root.uuid}]
pro["schematic"]["used_designators"] = ""
pro["sheets"] = [[root.uuid, "Root"], [s.sym_uuid, "Breakout"]]
import rules
rules.apply(pro)
json.dump(pro, open(os.path.join(out, PROJECT + ".kicad_pro"), 'w', encoding='utf-8', newline='\n'), indent=2)
shutil.copy(GS1 + "sym-lib-table", os.path.join(out, "sym-lib-table"))
open(os.path.join(out, "fp-lib-table"), "w", encoding="utf-8", newline="").write("\n".join([
    "(fp_lib_table", "  (version 7)",
    """  (lib (name "GreywireStudio")(type "KiCad")(uri "${KIPRJMOD}/../../../../libraries/GreywireStudio.pretty")"""
    """(options "")(descr "GreywireStudio shared footprints"))""", ")", ""]))
print("written", out, sorted(os.listdir(out)))
