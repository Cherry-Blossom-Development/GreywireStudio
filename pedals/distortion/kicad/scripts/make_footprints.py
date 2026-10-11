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
