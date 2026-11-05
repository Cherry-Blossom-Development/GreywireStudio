"""Write Flashover's custom footprints into libraries/GreywireStudio.pretty, by editing copies of
KiCad's own footprints (KiCad's Python can't save footprint files on this install).

usage: python make_footprints.py      (any Python 3)

- RotaryEncoder_Bourns_PEC11R-4xxxF-Sxxxx_Vertical: the Bourns PEC11R with switch. Same pins as the
  Alps EC11E footprint, but the mounting legs are 13.2 mm apart, not 11.2, with a 2.6 x 1.8 mm slot
  (Bourns PEC11R datasheet, PEC11R-4xxxF-Sxxxx recommended PCB layout).
"""
import os
import re

from paths import LIB

KIFP = "C:/Users/dalla/AppData/Local/Programs/KiCad/10.0/share/kicad/footprints/"
OUT = LIB + "GreywireStudio.pretty/"
os.makedirs(OUT, exist_ok=True)


def rename(s, old, new, descr):
    s = s.replace(f'(footprint "{old}"', f'(footprint "{new}"', 1)
    s = s.replace(f'(property "Value" "{old}"', f'(property "Value" "{new}"', 1)
    s = re.sub(r'\(descr "[^"]*"\)', f'(descr "{descr}")', s, count=1)
    return s


def sub1(s, a, b):
    assert s.count(a) == 1, a
    return s.replace(a, b)


def write(name, s):
    open(OUT + name + ".kicad_mod", "w", encoding="utf-8", newline="\n").write(s)
    print("wrote", name)


# ---------------------------------------------------------------- PEC11R encoder
src = "RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm"
s = open(KIFP + "Rotary_Encoder.pretty/" + src + ".kicad_mod", encoding="utf-8").read()
name = "RotaryEncoder_Bourns_PEC11R-4xxxF-Sxxxx_Vertical"
s = rename(s, src, name, "Bourns PEC11R-4xxxF-Sxxxx 12 mm incremental encoder with push switch, vertical shaft, "
                         "https://www.bourns.com/docs/Product-Datasheets/PEC11R.pdf")
# shaft at (7.5, 2.5); legs 13.2 mm apart -> y = 2.5 -/+ 6.6
for old_y, new_y in (("-3.1", "-4.1"), ("8.1", "9.1")):
    s = sub1(s, f'(pad "MP" thru_hole rect\n\t\t(at 7.5 {old_y})\n\t\t(size 3.2 2)\n\t\t(drill oval 2.8 1.5)',
             f'(pad "MP" thru_hole rect\n\t\t(at 7.5 {new_y})\n\t\t(size 3.2 2.4)\n\t\t(drill oval 2.8 2)')
# courtyard grows to take the wider legs (y -4.6 .. 9.6 -> -5.8 .. 10.8)
s = re.sub(r"\((start|end) (-?[\d.]+) -4\.6\)", r"(\1 \2 -5.8)", s)
s = re.sub(r"\((start|end) (-?[\d.]+) 9\.6\)", r"(\1 \2 10.8)", s)
write(name, s)


# ---------------------------------------------------------------- IS31FL3236A, QFN-44 5 x 5 mm
# The LED driver in the package JLCPCB stocks (IS31FL3236A-QFLS2-TR). Land pattern from the
# Lumissil datasheet (rev. I, "Recommended land pattern, QFN-44"): 0.25 x 0.80 mm pads at 0.4 mm
# pitch, 11 a side, inner edges 4.0 mm apart; exposed pad 2.45 x 2.45 mm (the datasheet's thin tabs
# out to the corners are left off). Pin 1 at the top of the left side, counting anticlockwise.
# Body outline, silkscreen and courtyard come from KiCad's QFN-40 5 x 5 mm, the same body.
src = "QFN-40-1EP_5x5mm_P0.4mm_EP3.6x3.6mm"
s = open(KIFP + "Package_DFN_QFN.pretty/" + src + ".kicad_mod", encoding="utf-8").read()
name = "QFN-44-1EP_5x5mm_P0.4mm_EP2.45x2.45mm"
s = rename(s, src, name, "QFN, 44 pin, 5 x 5 mm, 0.4 mm pitch, IS31FL3236A-QFLS2 "
                         "(https://www.lumissil.com/assets/pdf/core/IS31FL3236A_DS.pdf, recommended land pattern)")
s = s.replace(src, name)
head = s[:s.index('\t(pad ')]
pads = []
P = 0.4
C = 2.4                     # pad centre: inner edge 2.0 + half of 0.8
# The end pads of neighbouring sides would touch at the corners. As in the datasheet's pattern,
# they are trimmed there: their inner edge stops at 2.15 and the inner corner is chamfered,
# leaving 0.21 mm between them.
CHAMFER = {0: "top_right", 10: "bottom_right", 11: "top_left", 21: "top_right",
           22: "bottom_left", 32: "top_left", 33: "bottom_left", 43: "bottom_right"}
for i in range(44):
    side, k = divmod(i, 11)
    o = (k - 5) * P
    end = i in CHAMFER
    c, ln = (2.475, 0.65) if end else (C, 0.8)
    if side == 0:
        at, size = (-c, o), (ln, 0.25)           # left, top to bottom
    elif side == 1:
        at, size = (o, c), (0.25, ln)            # bottom, left to right
    elif side == 2:
        at, size = (c, -o), (ln, 0.25)           # right, bottom to top
    else:
        at, size = (-o, -c), (0.25, ln)          # top, right to left
    extra = f'\t\t(chamfer_ratio 0.5)\n\t\t(chamfer {CHAMFER[i]})\n' if end else ''
    pads.append(f'\t(pad "{i + 1}" smd roundrect\n\t\t(at {at[0]:g} {at[1]:g})\n\t\t(size {size[0]:g} {size[1]:g})\n'
                f'\t\t(layers "F.Cu" "F.Mask" "F.Paste")\n\t\t(roundrect_rratio 0.25)\n{extra}\t)')
pads.append('\t(pad "45" smd rect\n\t\t(at 0 0)\n\t\t(size 2.45 2.45)\n\t\t(property pad_prop_heatsink)\n'
            '\t\t(layers "F.Cu" "F.Mask")\n\t\t(zone_connect 2)\n\t)')
for x in (-0.62, 0.62):                           # paste on the exposed pad in four windows, about 64%
    for y in (-0.62, 0.62):
        pads.append(f'\t(pad "" smd roundrect\n\t\t(at {x:g} {y:g})\n\t\t(size 0.98 0.98)\n'
                    f'\t\t(layers "F.Paste")\n\t\t(roundrect_rratio 0.25)\n\t)')
write(name, head + '\n'.join(pads) + '\n\t(embedded_fonts no)\n)\n')
