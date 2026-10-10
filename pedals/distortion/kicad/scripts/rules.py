"""Flashover design rules, written into a .kicad_pro.

KiCad's Python API rewrites the project file with default rules whenever it saves a board,
so this runs after every scripted save as well as from the schematic generator."""
import json
import sys

CLASSES = [("Power", 0.4, ["+9V", "+5V", "VB", "GND", "Net-(D3-A)"]),
           ("Logic power", 0.3, ["+3V3", "*+1V1"])]
CLEARANCE = 0.15
EDGE = 0.3


def apply(pro):
    ns = pro["net_settings"]
    base = dict(ns["classes"][0])
    # small signal vias (0.45 / 0.2 mm) so the router can escape fine-pitch chips; power keeps 0.6 / 0.3
    base.update(name="Default", clearance=CLEARANCE, track_width=0.2, via_diameter=0.45, via_drill=0.2)
    ns["classes"] = [base] + [dict(base, name=n, track_width=w, priority=i, via_diameter=0.6, via_drill=0.3)
                              for i, (n, w, _) in enumerate(CLASSES)]
    ns["netclass_patterns"] = [{"netclass": n, "pattern": p} for n, _, pats in CLASSES for p in pats]
    rules = pro.setdefault("board", {}).setdefault("design_settings", {}).setdefault("rules", {})
    rules.update(min_clearance=CLEARANCE, min_track_width=0.15, min_via_diameter=0.45,
                 min_through_hole_diameter=0.2, min_copper_edge_clearance=EDGE, min_resolved_spokes=1)
    return pro


def apply_file(fn):
    pro = json.load(open(fn, encoding='utf-8'))
    json.dump(apply(pro), open(fn, 'w', encoding='utf-8', newline='\n'), indent=2)


if __name__ == "__main__":
    for fn in sys.argv[1:]:
        apply_file(fn)
