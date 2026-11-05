"""Add IS31FL3236A-QF (the QFN-44 package) to libraries/GreywireStudio.kicad_sym.

KiCad only has the TQFP-48 version (Driver_LED:IS31FL3236A-TQ). This copies it and renumbers the
pins to the QFN-44 package (Lumissil datasheet rev. I, pin description): OUT3-OUT18 = 1-16,
GND = 17 and 39, OUT19-OUT36 = 18-35, SDB 36, AD 37, VCC 38, R_EXT 40, SDA 41, SCL 42,
OUT1 43, OUT2 44, exposed pad 45 (GND). The TQFP's extra GND pins are dropped.
Run once with any Python 3; it does nothing if the symbol is already there.
"""
import copy
import kisym
from paths import LIB

FN = LIB + "GreywireStudio.kicad_sym"
NEW = "IS31FL3236A-QF"

def qfn_number(nm, tq):
    if nm.startswith("OUT"):
        n = int(nm[3:])
        return str(43 + n - 1 if n <= 2 else n - 2 if n <= 18 else n - 1)
    return {"~{SDB}": "36", "AD": "37", "VCC": "38", "R_EXT": "40", "SDA": "41", "SCL": "42",
            "GND": {"4": "17", "18": "39", "19": "45"}.get(tq)}[nm]

text = open(FN, encoding="utf-8").read()
if f'(symbol "{NEW}"' in text:
    print("already there")
    raise SystemExit
src = kisym.get_symbol("Driver_LED", "IS31FL3236A-TQ")   # flattened (it extends IS31FL3236-TQ)
src[1] = ("\"", NEW)
for e in src:
    if isinstance(e, list) and e[0] == "property":
        key = kisym.name(e)
        if key == "Value":
            e[2] = ("\"", NEW)
        elif key == "Footprint":
            e[2] = ("\"", "GreywireStudio:QFN-44-1EP_5x5mm_P0.4mm_EP2.45x2.45mm")
        elif key == "Description":
            e[2] = ("\"", "36-channel LED driver, I2C, QFN-44 5x5 mm (IS31FL3236A-QFLS2)")
    if isinstance(e, list) and e[0] == "symbol":
        e[1] = ("\"", kisym.name(e).replace("IS31FL3236A-TQ", NEW, 1))
        keep = []
        for p in e:
            if isinstance(p, list) and p[0] == "pin":
                nm = kisym.name(next(x for x in p if isinstance(x, list) and x[0] == "name"))
                num = next(x for x in p if isinstance(x, list) and x[0] == "number")
                q = qfn_number(nm, kisym.name(num))
                if q is None:
                    continue                      # a TQFP-only GND pin
                num[1] = ("\"", q)
            keep.append(p)
        e[:] = keep
block = kisym.dump(src, 1)
i = text.rstrip().rfind(")")
open(FN, "w", encoding="utf-8", newline="\n").write(text[:i].rstrip() + "\n\t" + block + "\n)\n")
print("added", NEW)
