"""Place the Flashover breakout board (2 layers), ready for autorouting.

usage (KiCad's Python):  python gen_breakout_pcb.py
Needs build/Flashover_Breakout.xml (kicad-cli sch export netlist --format kicadxml).
Then: bash route.sh Flashover_Breakout   and   bash breakout_fab.sh

Coordinates are in mm from pin 1 of J1. The two pin rows are 17.78 mm (0.7") apart, like a
Raspberry Pi Pico, so the board straddles a breadboard's centre gap: J1 along the top edge,
J2 along the bottom, both with pin 1 on the left. Every pin is named on the silkscreen.
"""
import xml.etree.ElementTree as ET
import pcbnew

from paths import KI, LIB, BUILD
import rules

FPDIR = r"C:/Users/dalla/AppData/Local/Programs/KiCad/10.0/share/kicad/footprints/"
PROJECT = "Flashover_Breakout"
OX, OY = 100.0, 80.0                     # where pin 1 of J1 sits on the KiCad page
ROW = 17.78
PITCH = 2.54
NPIN = 30
X1, X2, Y1, Y2 = -2.0, (NPIN - 1) * PITCH + 2.0, -6.0, ROW + 6.0     # board outline

V = lambda x, y: pcbnew.VECTOR2I(pcbnew.FromMM(OX + x), pcbnew.FromMM(OY + y))

# short pin names for the silkscreen (the schematic has the full names)
SILK_J1 = ["9V", "GND", "GA", "GW", "GB", "TA", "TW", "TB", "LA", "LW", "LB", "GND"] + [str(31 - p) for p in range(13, 31)]
SILK_J2 = ["3V3", "GND", "SDA", "SCL", "GND", "EN", "LSDA", "LSCL", "GND", "GND", "GND", "5V"] + [str(p + 6) for p in range(13, 31)]

t = ET.parse(BUILD + PROJECT + ".xml").getroot()
comps, nets = {}, {}
for c in t.find('components'):
    comps[c.get('ref')] = dict(value=c.findtext('value'), fp=c.findtext('footprint'),
                               path=c.find('sheetpath').get('tstamps') + c.find('tstamps').text)
for n in t.find('nets'):
    for nd in n.findall('node'):
        nets[(nd.get('ref'), nd.get('pin'))] = n.get('name')

b = pcbnew.CreateEmptyBoard()
b.SetCopperLayerCount(2)
tb = b.GetTitleBlock()
tb.SetTitle("Flashover breakout board")
tb.SetCompany("GreywireStudio")
tb.SetRevision("0.1")
tb.SetDate("2026-11-05")
netinfo = {}
for name in sorted(set(nets.values())):
    ni = pcbnew.NETINFO_ITEM(b, name)
    b.Add(ni)
    netinfo[name] = ni
fps = {}
for ref, c in comps.items():
    lib, name = c['fp'].split(':')
    fp = pcbnew.FootprintLoad((LIB if lib == "GreywireStudio" else FPDIR) + lib + ".pretty", name)
    fp.SetFPID(pcbnew.LIB_ID(lib, name))
    fp.SetReference(ref)
    fp.SetValue(c['value'])
    fp.SetPath(pcbnew.KIID_PATH(c['path']))
    b.Add(fp)
    for pad in fp.Pads():
        net = nets.get((ref, pad.GetNumber()))
        if net:
            pad.SetNet(netinfo[net])
    fp.Value().SetVisible(False)
    fps[ref] = fp


def place(ref, x, y, rot=0.0):
    fp = fps[ref]
    fp.SetOrientationDegrees(rot)
    fp.SetPosition(V(x, y))


def pad_xy(ref, num):
    p = next(p for p in fps[ref].Pads() if p.GetNumber() == num).GetPosition()
    return pcbnew.ToMM(p.x) - OX, pcbnew.ToMM(p.y) - OY


def text(s, x, y, size=0.9, rot=0.0, layer=pcbnew.F_SilkS, just=pcbnew.GR_TEXT_H_ALIGN_CENTER, mirror=False):
    tx = pcbnew.PCB_TEXT(b)
    tx.SetText(s)
    tx.SetLayer(layer)
    tx.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(size), pcbnew.FromMM(size)))
    tx.SetTextThickness(pcbnew.FromMM(0.15))
    tx.SetHorizJustify(just)
    tx.SetTextAngleDegrees(rot)
    tx.SetMirrored(mirror)
    tx.SetPosition(V(x, y))
    b.Add(tx)


# ---------------------------------------------------------------- pin rows
for ref, y in (("J1", 0.0), ("J2", ROW)):
    for rot in (0, 90, 180, 270):          # the rotation that runs pins 1..30 left to right
        place(ref, 0, y, rot)
        xe, ye = pad_xy(ref, str(NPIN))
        if abs(ye - y) < 0.01 and xe > 1:
            break
    fps[ref].Reference().SetVisible(False)
for k in range(NPIN):
    x = k * PITCH
    text(SILK_J1[k], x, -1.75, rot=90, just=pcbnew.GR_TEXT_H_ALIGN_LEFT)       # reads upward, above the top row
    text(SILK_J2[k], x, ROW + 1.75, rot=90, just=pcbnew.GR_TEXT_H_ALIGN_RIGHT) # below the bottom row

# ---------------------------------------------------------------- parts
# Pots in the upper left, near their pins on row 1; their bus pull-ups by row 2's SDA and SCL.
for i, ref in enumerate(("U7", "U8", "U9")):
    xc = 6.5 + 8.0 * i
    place(ref, xc, 6.0, 90)
    place(f"C{101 + i}", xc - 3.6, 8.9, 90)     # VL, beside pin 1
    place(f"C{104 + i}", xc - 3.6, 3.4, 270)    # V+, beside pin 14, 9V pad on top (pre-routed below)
    fps[ref].Reference().SetPosition(V(xc, 6.0))
    fps[ref].Reference().SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(0.8), pcbnew.FromMM(0.8)))
    fps[ref].Reference().SetLayer(pcbnew.F_Fab)
    text(("U7 GAIN", "U8 TONE", "U9 LEVEL")[i], xc, 10.6, size=0.8)
for ref, x in (("R21", 3 * PITCH - 0.9), ("R22", 3 * PITCH + 0.9), ("R35", 6 * PITCH), ("R36", 7 * PITCH)):
    place(ref, x, 13.4, 90)
# The LED driver turned a quarter clockwise, centred over pins 13-30: its outputs then leave in
# the same order as the pins (see gen_breakout.py), and its control pins face right.
XC = (12 + 29) / 2 * PITCH
place("U10", XC, ROW / 2, 270)
fps["U10"].Reference().SetLayer(pcbnew.F_Fab)
px1, py1 = pad_xy("U10", "1")
assert px1 > XC + 1.5 and py1 < ROW / 2 - 2, f"U10 pin 1 at {px1:.2f}, {py1:.2f}: expected top right"
# Ground under the LED driver, done before routing: its exposed pad is boxed in by the pins round
# it, so four vias take it down to the ground fill on the back; and the three pins that go to
# ground (17 and 39 GND, 37 AD) get a short track straight inward onto that pad.
gnd = netinfo["GND"]
for dx in (-0.6, 0.6):
    for dy in (-0.6, 0.6):
        via = pcbnew.PCB_VIA(b)
        via.SetPosition(V(XC + dx, ROW / 2 + dy))
        via.SetDrill(pcbnew.FromMM(0.2))
        via.SetWidth(pcbnew.FromMM(0.5))
        via.SetNet(gnd)
        b.Add(via)
# stitching vias tie the front and back ground fills together, in the copper-free margins
for x, y in ((-1.0, ROW / 2), (X2 - 1.0, ROW / 2 - 3), (X2 - 1.0, ROW / 2 + 3),
             (17.0, Y1 + 1.0), (47.0, Y1 + 1.0), (32.0, Y2 - 1.0), (62.0, Y2 - 1.0)):
    via = pcbnew.PCB_VIA(b)
    via.SetPosition(V(x, y))
    via.SetDrill(pcbnew.FromMM(0.3))
    via.SetWidth(pcbnew.FromMM(0.6))
    via.SetNet(gnd)
    b.Add(via)
for num in ("17", "37", "39"):
    x, y = pad_xy("U10", num)
    rx, ry = x - XC, y - ROW / 2
    if abs(rx) > abs(ry):
        end = (XC + (1.0 if rx > 0 else -1.0), y)
    else:
        end = (x, ROW / 2 + (1.0 if ry > 0 else -1.0))
    tr = pcbnew.PCB_TRACK(b)
    tr.SetStart(V(x, y))
    tr.SetEnd(V(*end))
    tr.SetWidth(pcbnew.FromMM(0.2))
    tr.SetLayer(pcbnew.F_Cu)
    tr.SetNet(gnd)
    b.Add(tr)
for ref, x, y, r in (("R33", XC + 6.6, ROW / 2 - 0.7, 0), ("C40", XC + 6.6, ROW / 2 + 1.0, 0),
                     ("C39", XC + 9.4, ROW / 2 + 0.3, 90), ("R34", XC + 11.6, ROW / 2 + 0.3, 90)):
    place(ref, x, y, r)


def track(layer, net, *pts, w=0.2):
    for a, c in zip(pts, pts[1:]):
        tr = pcbnew.PCB_TRACK(b)
        tr.SetStart(V(*a))
        tr.SetEnd(V(*c))
        tr.SetWidth(pcbnew.FromMM(w))
        tr.SetLayer(layer)
        tr.SetNet(netinfo[net])
        b.Add(tr)


def via_at(x, y, net, d=0.6, drill=0.3):
    v = pcbnew.PCB_VIA(b)
    v.SetPosition(V(x, y))
    v.SetDrill(pcbnew.FromMM(drill))
    v.SetWidth(pcbnew.FromMM(d))
    v.SetNet(netinfo[net])
    b.Add(v)


def net_pad(ref, net):
    p = next(p for p in fps[ref].Pads() if p.GetNetname() == net)
    return pad_xy(ref, p.GetNumber())


# Pre-routed where the autorouter got stuck (it leaves these alone and routes the rest).
# 9V: a bus on the back above row 1, from pin 1; at each pot it comes up through a via in the gap
# between two row 1 pins, to the pot's 9V capacitor and on to its pin 14.
GAPS = (1.5 * PITCH, 4.5 * PITCH, 7.5 * PITCH)       # between pins 2/3, 5/6 and 8/9
track(pcbnew.B_Cu, "+9V", (0, 0), (0, -2.6), (GAPS[-1], -2.6), w=0.3)
for i, ref in enumerate(("U7", "U8", "U9")):
    gx = GAPS[i]
    cx, cy = net_pad(f"C{104 + i}", "+9V")
    px14, py14 = pad_xy(ref, "14")
    via_at(gx, -2.6, "+9V")
    track(pcbnew.F_Cu, "+9V", (gx, -2.6), (gx, cy - 0.9), (cx, cy - 0.3), (cx, cy))
    track(pcbnew.F_Cu, "+9V", (cx, cy), (px14, cy), (px14, py14))
# 5V: from the LED driver's VCC pin past its neighbours (both ground) to C40, and through a via to
# the back, over to C39 and (by the router) on to row 2 pin 12.
x38, y38 = pad_xy("U10", "38")
c40 = net_pad("C40", "+5V")
c39 = net_pad("C39", "+5V")
track(pcbnew.F_Cu, "+5V", (x38, y38), (c40[0] - 0.6, y38), c40)
via_at(x38 + 2.0, y38, "+5V")
track(pcbnew.B_Cu, "+5V", (x38 + 2.0, y38), (c39[0] + 1.0, y38), (c39[0] + 1.0, c39[1] - 0.3))
via_at(c39[0] + 1.0, c39[1] - 0.3, "+5V")
track(pcbnew.F_Cu, "+5V", (c39[0] + 1.0, c39[1] - 0.3), (c39[0] + 0.3, c39[1] - 0.3), c39)

for ref in fps:
    if ref[0] in "RC":
        fps[ref].Reference().SetVisible(False)     # too small to label; the fab drawing has them
# the back: what's on the board, mirrored so it reads from behind
for k, line in enumerate(("FLASHOVER BREAKOUT v0.1  (GreywireStudio, Nov 2026)",
                          "Digital pots on I2C: U7 Gain 0x3C, U8 Tone 0x3D, U9 Level 0x3E (V+ 9V, VL 3.3V)",
                          "LED driver U10 IS31FL3236A at 0x3C on its own bus (VCC 5V, EN high = on)",
                          "Pin rows 0.7 in apart: straddles a breadboard like a Pico")):
    text(line, (X1 + X2) / 2, 3.5 + 3.4 * k, size=1.0, layer=pcbnew.B_SilkS, mirror=True)

# ---------------------------------------------------------------- outline
pts = [(X1, Y1), (X2, Y1), (X2, Y2), (X1, Y2)]
for a, c in zip(pts, pts[1:] + pts[:1]):
    s = pcbnew.PCB_SHAPE(b)
    s.SetShape(pcbnew.SHAPE_T_SEGMENT)
    s.SetLayer(pcbnew.Edge_Cuts)
    s.SetWidth(pcbnew.FromMM(0.1))
    s.SetStart(V(*a))
    s.SetEnd(V(*c))
    b.Add(s)

fn = KI + PROJECT + "/" + PROJECT + ".kicad_pcb"
pcbnew.SaveBoard(fn, b)
rules.apply_file(KI + PROJECT + "/" + PROJECT + ".kicad_pro")
print("saved", fn, f"board {X2 - X1:.1f} x {Y2 - Y1:.1f} mm")
