"""One-off: swap the control board's encoders to the Bourns PEC11R footprint, keeping the routing.

usage: python swap_encoders.py      (KiCad's bundled Python; KiCad must be closed)
"""
import pcbnew

import rules
from paths import KI, LIB

NAME = "RotaryEncoder_Bourns_PEC11R-4xxxF-Sxxxx_Vertical"
VALUE = "PEC11R-4220F-S0024"
fn = KI + "Flashover_Controls/Flashover_Controls.kicad_pcb"
b = pcbnew.LoadBoard(fn)
base = pcbnew.FootprintLoad(LIB + "GreywireStudio.pretty", NAME)   # load once: a second load fails
for ref in ("SW1", "SW2", "SW3"):
    old = b.FindFootprintByReference(ref)
    new = pcbnew.FOOTPRINT(base)
    new.SetFPID(pcbnew.LIB_ID("GreywireStudio", NAME))
    new.SetReference(ref)
    new.SetValue(VALUE)
    new.SetPath(old.GetPath())
    new.SetAttributes(old.GetAttributes())
    b.Add(new)
    new.SetOrientation(old.GetOrientation())
    new.SetPosition(old.GetPosition())
    nets = {p.GetNumber(): p.GetNet() for p in old.Pads()}
    for p in new.Pads():
        if p.GetNumber() in nets:
            p.SetNet(nets[p.GetNumber()])
    b.Remove(old)
    print(ref, "swapped")
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
pcbnew.SaveBoard(fn, b)
rules.apply_file(fn[:-len(".kicad_pcb")] + ".kicad_pro")
