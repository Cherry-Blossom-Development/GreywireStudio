"""Draw a 1:1 printable drilling template for the Flashover's Hammond 1590BB box.

usage: python drill_template.py      (any Python 3 with matplotlib; not KiCad's Python)

Writes ../Flashover_drill_template.pdf (US Letter, two pages):
  1. the top panel, looking down at the pedal, with the knob, LED, button and footswitch holes
  2. the three jack walls, each seen from outside the box
Knob, LED and button positions are read from the control board, so the template follows the board.
Jack positions are the ones in FLASHOVER_STATUS.md's drilling guide.  Print at 100% ("Actual size").
"""
import math
import re

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from matplotlib.patches import Circle, Rectangle

from paths import KI

OX, OY = 100.0, 50.0                      # pedal corner on the KiCad page (see gen_pcb.py)
BOX_W, BOX_L, BOX_H = 94.0, 119.5, 34.0   # 1590BB outside: across, front-to-back, height
PAGE_W, PAGE_H = 215.9, 279.4             # US Letter, mm

KNOB_HOLE = 7.5        # PEC11R M7 bushing
LED_HOLE = 3.2         # 3 mm LED, or a light pipe (not decided)
BUTTON_HOLE = 5.0      # 6 mm tact switch plunger; check against the cap
FOOTSWITCH = (47.0, 102.0)
FOOTSWITCH_HOLE = 12.0  # usual footswitch size; check against the part
KNOB_MAX = 20.0        # largest knob that leaves the LED ring showing (ring radius 12.6)
ENC_SHAFT = (7.5, 2.5)  # shaft centre from pad A in the PEC11R footprint

# (name, across from the pedal's left edge or down from the jack wall, depth from the panel face,
#  hole, outside pocket or None) -- the drilling guide in FLASHOVER_STATUS.md
TOP_WALL = [("OUTPUT", 21.4, 14.9, 11.5, None), ("9V DC", 47.8, 14.9, 8.0, 13.0),
            ("INPUT", 75.5, 14.9, 11.5, None)]
MIDI_OUT = ("MIDI OUT", 72.5, 18.9, 6.5, 11.0)   # left wall
MIDI_IN = ("MIDI IN", 72.5, 18.9, 6.5, 11.0)     # right wall


def footprints(fn):
    """{reference: (pedal x, pedal y)} of each footprint's pad centre (or encoder shaft)."""
    out = {}
    for blk in open(fn, encoding="utf-8").read().split("\n\t(footprint ")[1:]:
        ref = re.search(r'\(property "Reference" "([^"]+)"', blk).group(1)
        at = re.search(r"\n\t\t\(at ([-\d.]+) ([-\d.]+)(?: ([-\d.]+))?\)", blk)
        x, y, rot = float(at.group(1)), float(at.group(2)), float(at.group(3) or 0)
        if "RotaryEncoder" in blk.split("\n", 1)[0]:
            lx, ly = ENC_SHAFT
        else:
            pads = [(float(a), float(b)) for a, b in
                    re.findall(r"\n\t\t\(pad \"[^\"]*\" (?:thru_hole|smd)[^\n]*\n\t\t\t\(at ([-\d.]+) ([-\d.]+)", blk)]
            if not pads:
                continue
            lx = sum(p[0] for p in pads) / len(pads)
            ly = sum(p[1] for p in pads) / len(pads)
        a = math.radians(rot)
        out[ref] = (x + lx * math.cos(a) + ly * math.sin(a) - OX,
                    y - lx * math.sin(a) + ly * math.cos(a) - OY)
    return out


def page():
    fig = plt.figure(figsize=(PAGE_W / 25.4, PAGE_H / 25.4))
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_xlim(0, PAGE_W)
    ax.set_ylim(PAGE_H, 0)              # y down, in mm: one unit is one millimetre on paper
    ax.axis("off")
    return fig, ax


def hole(ax, x, y, d, label=None, dx=0, dy=None, ls="-", color="black", size=7):
    ax.add_patch(Circle((x, y), d / 2, fill=False, lw=0.6, ls=ls, ec=color))
    c = min(d / 2 + 1.5, 4)
    ax.plot([x - c, x + c], [y, y], lw=0.25, color=color)
    ax.plot([x, x], [y - c, y + c], lw=0.25, color=color)
    if label:
        ax.text(x + dx, y + (d / 2 + 2.5 if dy is None else dy), label, ha="center", va="top",
                fontsize=size, color=color)


def scale_bar(ax, x, y):
    ax.plot([x, x + 100], [y, y], lw=0.8, color="black")
    for t in (0, 50, 100):
        ax.plot([x + t, x + t], [y - 2, y + 2], lw=0.8, color="black")
    ax.text(x + 50, y + 3, "This line must measure exactly 100 mm. If it doesn't, reprint at 100% "
            "(\"Actual size\", not \"Fit to page\").", ha="center", va="top", fontsize=7.5)


def panel_page(pdf, fp):
    fig, ax = page()
    x0, y0 = (PAGE_W - BOX_W) / 2, 38.0
    P = lambda x, y: (x0 + x, y0 + y)
    ax.text(PAGE_W / 2, 12, "Flashover: top panel drilling template (1:1)", ha="center", fontsize=14,
            weight="bold")
    ax.text(PAGE_W / 2, 19, "Looking down at the pedal. Jack wall at the top. Hammond 1590BB, "
            "119.5 x 94 mm.", ha="center", fontsize=8.5)
    ax.text(PAGE_W / 2, 24, "Cut along the outline, tape it to the panel, and centre-punch each "
            "cross before drilling.", ha="center", fontsize=8.5)
    ax.add_patch(Rectangle(P(0, 0), BOX_W, BOX_L, fill=False, lw=0.9))
    ax.text(*P(BOX_W / 2, -1.5), "jack wall (back): Output  .  9V DC  .  Input", ha="center",
            va="bottom", fontsize=7)
    ax.text(*P(-1.5, MIDI_OUT[1]), "MIDI Out", ha="right", va="center", fontsize=7, rotation=90)
    ax.text(*P(BOX_W + 1.5, MIDI_IN[1]), "MIDI In", ha="left", va="center", fontsize=7, rotation=270)
    ax.text(*P(BOX_W / 2, BOX_L + 1.5), "front (player side)", ha="center", va="top", fontsize=7)

    for ref, name in (("SW1", "GAIN"), ("SW2", "TONE"), ("SW3", "LEVEL")):
        kx, ky = fp[ref]
        hole(ax, *P(kx, ky), KNOB_HOLE, f"{name}  {KNOB_HOLE:g} mm", dy=-18.5)   # above the ring
        ax.add_patch(Circle(P(kx, ky), KNOB_MAX / 2, fill=False, lw=0.4, ls=(0, (1, 2)), ec="0.5"))
    rings = [f"D{n}" for n in range(7, 40)]
    for r in rings:
        hole(ax, *P(*fp[r]), LED_HOLE, color="0.45")
    for ref, name in (("D40", "SOFT"), ("D41", "HARD"), ("D45", "LINK"), ("D42", "ON")):
        hole(ax, *P(*fp[ref]), LED_HOLE, name)
    hole(ax, *P(*fp["SW4"]), BUTTON_HOLE, f"SOFT/HARD button\n{BUTTON_HOLE:g} mm (check the cap)")
    hole(ax, *P(*FOOTSWITCH), FOOTSWITCH_HOLE, f"FOOTSWITCH\n{FOOTSWITCH_HOLE:g} mm (check the part)")

    ly = y0 + BOX_L + 12
    notes = [
        "Black circles: holes to drill, at the size written beside them. The cross marks the centre.",
        f"Grey circles: the 33 ring LEDs, 11 around each knob. Panel holes ({LED_HOLE:g} mm) or light pipes: "
        "not decided yet.",
        f"Dotted circles: the largest knob ({KNOB_MAX:g} mm across) that still leaves the LED ring visible.",
        f"Small LEDs (Soft, Hard, Link, On): {LED_HOLE:g} mm holes for 3 mm LEDs.",
        "Positions come from the control board file. Check them against the real board before drilling.",
    ]
    for i, n in enumerate(notes):
        ax.text(x0 - 8, ly + i * 5, "- " + n, fontsize=7.5)
    scale_bar(ax, (PAGE_W - 100) / 2, ly + len(notes) * 5 + 6)
    pdf.savefig(fig)
    return fig


def wall(ax, x0, y0, length, title, left_end, right_end, holes):
    """One wall seen from outside, panel face at the top.  holes: (name, from the left end, ...)."""
    ax.text(x0, y0 - 9, title, fontsize=10, weight="bold")
    ax.add_patch(Rectangle((x0, y0), length, BOX_H, fill=False, lw=0.9))
    ax.text(x0 + length / 2, y0 - 1.2, "panel (top) face of the box: line this edge up with it",
            ha="center", va="bottom", fontsize=6.5)
    ax.text(x0 + length / 2, y0 + BOX_H + 1.2, "open side (lid)", ha="center", va="top", fontsize=6.5)
    ax.text(x0 - 1.5, y0 + BOX_H / 2, left_end, ha="right", va="center", fontsize=6.5, rotation=90)
    ax.text(x0 + length + 1.5, y0 + BOX_H / 2, right_end, ha="left", va="center", fontsize=6.5,
            rotation=270)
    for name, along, depth, d, pocket in holes:
        x, y = x0 + along, y0 + depth
        if pocket:
            hole(ax, x, y, pocket, ls=(0, (3, 2)), color="0.4")
        text = f"{name}\n{d:g} mm hole" + (f"\n{pocket:g} mm pocket" if pocket else "")
        hole(ax, x, y, d, text, dy=-(depth - 0.5), size=6.5)
        ax.text(x, y + max(d, pocket or 0) / 2 + 1.0, f"{depth:g} mm down", ha="center", va="top",
                fontsize=6)


def walls_page(pdf):
    fig, ax = page()
    ax.text(PAGE_W / 2, 12, "Flashover: jack wall drilling templates (1:1)", ha="center", fontsize=14,
            weight="bold")
    ax.text(PAGE_W / 2, 19, "Each wall is drawn as you see it from OUTSIDE the box, with the panel "
            "(top) face of the box at the top.", ha="center", fontsize=8.5)
    ax.text(PAGE_W / 2, 24, "Note the corner labels: left and right swap when you walk round the box.",
            ha="center", fontsize=8.5)
    xb = (PAGE_W - BOX_W) / 2
    # behind the pedal, looking at the jack wall: the pedal's right-hand side is on your left
    wall(ax, xb, 50, BOX_W, "Back (jack) wall: stand behind the pedal",
         "INPUT side (right of pedal)", "OUTPUT side (left of pedal)",
         [(n, BOX_W - a, dp, d, pk) for n, a, dp, d, pk in TOP_WALL])
    xs = (PAGE_W - BOX_L) / 2
    # left wall from the left: the back is on your left
    n, a, dp, d, pk = MIDI_OUT
    wall(ax, xs, 118, BOX_L, "Left wall: stand at the pedal's left side", "BACK corner (jacks)",
         "FRONT corner", [(n, a, dp, d, pk)])
    # right wall from the right: the front is on your left
    n, a, dp, d, pk = MIDI_IN
    wall(ax, xs, 186, BOX_L, "Right wall: stand at the pedal's right side", "FRONT corner",
         "BACK corner (jacks)", [(n, BOX_L - a, dp, d, pk)])
    ly = 232
    notes = [
        "Solid circle: the hole. Dashed circle: a shallow, flat-bottomed pocket on the OUTSIDE of the wall "
        "(DC about 2.3 mm deep, MIDI about 1.9 mm),",
        "  so the plug can push fully into the jack through the thick wall. The 1/4\" jacks need no pocket.",
        "Depths are measured from the panel (top) face of the box. The box's height and the lid are drawn "
        "approximately.",
        "Hole positions are from the drilling guide in FLASHOVER_STATUS.md. Check them against the real "
        "boards before drilling.",
    ]
    for i, t in enumerate(notes):
        ax.text(14, ly + i * 4.5, ("- " if not t.startswith("  ") else "  ") + t.strip(), fontsize=7)
    scale_bar(ax, (PAGE_W - 100) / 2, ly + len(notes) * 4.5 + 6)
    pdf.savefig(fig)
    return fig


if __name__ == "__main__":
    fp = footprints(KI + "Flashover_Controls/Flashover_Controls.kicad_pcb")
    out = KI + "Flashover_drill_template.pdf"
    with PdfPages(out) as pdf:
        figs = [panel_page(pdf, fp), walls_page(pdf)]
    for i, f in enumerate(figs, 1):     # previews for checking by eye
        f.savefig(KI + f"scripts/build/drill_template_p{i}.png", dpi=110)
    print("wrote", out)
