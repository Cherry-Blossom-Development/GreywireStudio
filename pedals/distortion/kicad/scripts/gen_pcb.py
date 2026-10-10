"""Generate the Flashover main and control boards (placement only, no routing).

Run with KiCad's bundled Python.  Coordinates below are 'pedal coordinates' in mm:
x across the 1590BB from the left wall (0..94), y down from the top (jack) wall (0..119.5),
looking at the top panel.  Both boards have their components facing the panel.
"""
import math, sys
import xml.etree.ElementTree as ET
import pcbnew

FPDIR = r"C:/Users/dalla/AppData/Local/Programs/KiCad/10.0/share/kicad/footprints/"
from paths import KI, BUILD as SP
OX, OY = 100.0, 50.0          # where the pedal's top-left corner sits on the KiCad page

V = lambda x, y: pcbnew.VECTOR2I(pcbnew.FromMM(OX + x), pcbnew.FromMM(OY + y))
mm = pcbnew.ToMM

# ---------------------------------------------------------------- enclosure
BOX_W, BOX_H = 94.0, 119.5            # Hammond 1590BB outside
BX1, BX2 = 5.0, 89.0                  # board left/right edges (walls are about 3 mm)
BY_TOP, BY_BOT = 4.0, 88.0            # main board top/bottom; footswitch sits at y = 102
NOTCH = 6.5                           # clears the lid-screw bosses in the top corners
CTL_TOP = 30.0                        # control board starts below the 1/4" jack bodies
JACK_FRONT = BY_TOP                   # jack bodies line up with the board's top edge

KNOBS = {'SW1': (22.0, 46.0), 'SW2': (47.0, 68.0), 'SW3': (72.0, 46.0)}   # Gain, Tone, Level shafts
RING_R = 12.6                         # clears the encoder's courtyard
RINGS = {'SW1': range(7, 18), 'SW2': range(18, 29), 'SW3': range(29, 40)}  # D7..D39
PANEL = {'D40': (16.0, 68.0), 'SW4': (22.0, 76.0), 'D41': (28.0, 68.0),  # Soft, button, Hard
         'D42': (47.0, 85.3), 'D45': (72.0, 68.0)}                         # On (above the footswitch), Link
HOLES = {'H1': (9.0, 62.0), 'H2': (85.0, 62.0), 'H3': (9.0, 62.0), 'H4': (85.0, 62.0)}
FOOTSWITCH = (47.0, 102.0)


def load_netlist(fn):
    t = ET.parse(fn).getroot()
    comps = {}
    for c in t.find('components'):
        sp = c.find('sheetpath').get('tstamps')
        ts = c.find('tstamps').text
        comps[c.get('ref')] = dict(value=c.findtext('value'), fp=c.findtext('footprint'), path=sp + ts)
    nets = {}
    for n in t.find('nets'):
        for nd in n.findall('node'):
            name = n.get('name')
            if name.startswith(('unconnected-(', 'Net-(')):
                name = name.replace('/', '{slash}')   # KiCad escapes '/' inside pin names
            nets[(nd.get('ref'), nd.get('pin'))] = name
    return comps, nets


class Board:
    def __init__(self, project, title, layers=2):
        self.project = project
        self.b = pcbnew.CreateEmptyBoard()
        self.b.SetCopperLayerCount(layers)
        tb = self.b.GetTitleBlock()
        tb.SetTitle(title)
        tb.SetCompany("GreywireStudio")
        tb.SetRevision("0.1")
        tb.SetDate("2026-10-09")
        self.comps, self.nets = load_netlist(SP + project + ".xml")
        self.netinfo = {}
        for name in sorted(set(self.nets.values())):
            ni = pcbnew.NETINFO_ITEM(self.b, name)
            self.b.Add(ni)
            self.netinfo[name] = ni
        self.fps = {}
        for ref, c in self.comps.items():
            lib, name = c['fp'].split(':')
            fp = pcbnew.FootprintLoad(FPDIR + lib + ".pretty", name)
            fp.SetFPID(pcbnew.LIB_ID(lib, name))
            fp.SetReference(ref)
            fp.SetValue(c['value'])
            fp.SetPath(pcbnew.KIID_PATH(c['path']))
            self.b.Add(fp)
            for pad in fp.Pads():
                net = self.nets.get((ref, pad.GetNumber()))
                if net:
                    pad.SetNet(self.netinfo[net])
            attrs = fp.GetAttributes()
            if ref.startswith('H'):
                attrs |= pcbnew.FP_EXCLUDE_FROM_BOM
            else:
                attrs &= ~pcbnew.FP_EXCLUDE_FROM_BOM
            fp.SetAttributes(attrs)
            self.fps[ref] = fp
        self.placed = set()

    # ---- placement helpers ----
    def place(self, ref, x, y, rot=0.0, back=False):
        fp = self.fps[ref]
        if back and not fp.IsFlipped():
            fp.Flip(fp.GetPosition(), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
        fp.SetOrientationDegrees(rot)
        fp.SetPosition(V(x, y))
        self.placed.add(ref)

    def place_centre(self, ref, x, y, rot=0.0):
        """Place so the courtyard centre lands on (x, y)."""
        self.place(ref, x, y, rot)
        bb = self.crt(ref)
        cx, cy = (bb[0] + bb[2]) / 2, (bb[1] + bb[3]) / 2
        self.place(ref, 2 * x - cx, 2 * y - cy, rot)

    def crt(self, ref):
        fp = self.fps[ref]
        layer = pcbnew.B_CrtYd if fp.IsFlipped() else pcbnew.F_CrtYd
        poly = fp.GetCourtyard(layer)
        bb = poly.BBox() if poly.OutlineCount() else fp.GetBoundingBox(False)
        return (mm(bb.GetLeft()) - OX, mm(bb.GetTop()) - OY, mm(bb.GetRight()) - OX, mm(bb.GetBottom()) - OY)

    def pack(self, refs, x1, y1, x2, y2, gap=0.6, label=""):
        """Shelf-pack footprints (orientation 0) into a region, tallest first."""
        refs = [r for r in refs if r not in self.placed]
        sizes = {}
        for r in refs:
            self.place(r, 0, 0)
            bb = self.crt(r)
            sizes[r] = (bb[2] - bb[0], bb[3] - bb[1], -bb[0], -bb[1])
        refs.sort(key=lambda r: (-sizes[r][1], -sizes[r][0], r))
        x, y, shelf = x1, y1, 0.0
        overflow = []
        for r in refs:
            w, h, ox, oy = sizes[r]
            if x + w > x2:
                x, y, shelf = x1, y + shelf + gap, 0.0
            if y + h > y2:
                overflow.append(r)
                continue
            self.place(r, x + ox, y + oy)
            x += w + gap
            shelf = max(shelf, h)
        if overflow:
            print(f"  {self.project} {label}: no room for {overflow}")

    def outline(self, pts, closed=True):
        for a, b in zip(pts, pts[1:] + (pts[:1] if closed else [])):
            s = pcbnew.PCB_SHAPE(self.b)
            s.SetShape(pcbnew.SHAPE_T_SEGMENT)
            s.SetLayer(pcbnew.Edge_Cuts)
            s.SetWidth(pcbnew.FromMM(0.1))
            s.SetStart(V(*a))
            s.SetEnd(V(*b))
            self.b.Add(s)

    def fab_line(self, a, b, layer=pcbnew.Dwgs_User):
        s = pcbnew.PCB_SHAPE(self.b)
        s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetLayer(layer)
        s.SetWidth(pcbnew.FromMM(0.15))
        s.SetStart(V(*a))
        s.SetEnd(V(*b))
        self.b.Add(s)

    def fab_circle(self, c, r, layer=pcbnew.Dwgs_User):
        s = pcbnew.PCB_SHAPE(self.b)
        s.SetShape(pcbnew.SHAPE_T_CIRCLE)
        s.SetLayer(layer)
        s.SetWidth(pcbnew.FromMM(0.15))
        s.SetCenter(V(*c))
        s.SetEnd(V(c[0] + r, c[1]))
        self.b.Add(s)

    def note(self, text, x, y, layer=pcbnew.Dwgs_User, size=1.2):
        t = pcbnew.PCB_TEXT(self.b)
        t.SetText(text)
        t.SetLayer(layer)
        t.SetPosition(V(x, y))
        t.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(size), pcbnew.FromMM(size)))
        t.SetTextThickness(pcbnew.FromMM(size * 0.15))
        t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_LEFT)
        self.b.Add(t)

    def enclosure(self):
        """Enclosure, footswitch and knob positions on the User.Drawings layer for reference."""
        box = [(0, 0), (BOX_W, 0), (BOX_W, BOX_H), (0, BOX_H)]
        for a, b in zip(box, box[1:] + box[:1]):
            self.fab_line(a, b)
        self.fab_circle(FOOTSWITCH, 6.0)
        self.note("Footswitch", FOOTSWITCH[0] - 5, FOOTSWITCH[1] + 9)
        self.note("Hammond 1590BB outline (outside), viewed from the top panel", 0, BOX_H + 4)
        for c in KNOBS.values():
            self.fab_circle(c, 8.0)

    def pads_abs(self, ref):
        return {p.GetNumber(): (round(mm(p.GetPosition().x) - OX, 3), round(mm(p.GetPosition().y) - OY, 3))
                for p in self.fps[ref].Pads()}

    def plane(self, layer, net, pts):
        """Copper pour over the given outline (filled after routing)."""
        z = pcbnew.ZONE(self.b)
        z.SetLayer(layer)
        z.SetNet(self.netinfo[net])
        z.SetZoneName(f"{net} plane")
        z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
        z.SetMinThickness(pcbnew.FromMM(0.2))
        z.SetThermalReliefGap(pcbnew.FromMM(0.3))
        z.SetThermalReliefSpokeWidth(pcbnew.FromMM(0.4))
        z.SetLocalClearance(pcbnew.FromMM(0.25))
        ol = z.Outline()
        ol.NewOutline()
        for x, y in pts:
            ol.Append(pcbnew.FromMM(OX + x), pcbnew.FromMM(OY + y))
        self.b.Add(z)
        return z

    def save(self):
        missing = sorted(set(self.fps) - self.placed)
        if missing:
            print(f"  {self.project}: unplaced {missing}")
        fn = KI + self.project + "/" + self.project + ".kicad_pcb"
        pcbnew.SaveBoard(fn, self.b)
        import rules
        rules.apply_file(fn[:-len(".kicad_pcb")] + ".kicad_pro")   # SaveBoard resets the rules
        print("saved", fn)


def by_sheet(board, name):
    t = ET.parse(SP + board.project + ".xml").getroot()
    return [c.get('ref') for c in t.find('components') if c.find('sheetpath').get('names') == f"/{name}/"]


# ===================================================================== connectors
J7_AT, J7_ROT = (63.0, 83.4), 90.0      # main board socket, pin 1 at J7_AT


# ===================================================================== main board
m = Board("Flashover", "Flashover - main board", layers=4)
# 4 layers: signals on the outside, a solid ground plane on In1 under everything, In2 for routing.
# (A +3V3 plane on In2 was tried: losing In2 for routing left more connections unrouted, 70 against 50.)
m.b.SetLayerType(pcbnew.In1_Cu, pcbnew.LT_POWER)
m.b.SetLayerName(pcbnew.In1_Cu, "In1.Cu")
m.plane(pcbnew.In1_Cu, "GND", [(BX1, BY_TOP), (BX2, BY_TOP), (BX2, BY_BOT), (BX1, BY_BOT)])
m.enclosure()

# jacks along the top wall, left to right as seen from above: Output, MIDI Out, DC, MIDI In, Input
jacks = [('J2', 'nrj'), ('J6', 'sj1'), ('J3', 'pj'), ('J5', 'sj1'), ('J1', 'nrj')]
width = {'nrj': 16.84, 'sj1': 12.58, 'pj': 11.58}
left, right = BX1 + NOTCH, BX2 - NOTCH
gap = (right - left - sum(width[k] for _, k in jacks)) / (len(jacks) - 1)
x = left
for ref, kind in jacks:
    if kind == 'nrj':      # nose along +x in the footprint: turn it to face the wall
        m.place(ref, x + 2.63, JACK_FRONT + 16.95, 90)
    elif kind == 'sj1':    # opening along +y
        m.place(ref, x + 6.29, BY_TOP + 4.5, 180)   # footprint's board edge is at y = 4.5
    else:                  # PJ-102AH, opening along +y
        m.place(ref, x + 6.54, JACK_FRONT + 13.7, 180)
    x += width[kind] + gap
print(f"  jack gap {gap:.2f} mm")
# outline: the 3.5 mm jacks bring their own stretch of top edge (with a notch under the plug entry),
# so the top edge stops either side of them
sj = sorted(mm(m.fps[r].GetPosition().x) - OX for r in ('J5', 'J6'))
m.outline([(BX2 - NOTCH, BY_TOP + NOTCH), (BX2, BY_TOP + NOTCH), (BX2, BY_BOT), (BX1, BY_BOT),
           (BX1, BY_TOP + NOTCH), (BX1 + NOTCH, BY_TOP + NOTCH)], closed=False)
top = [BX1 + NOTCH] + [v for x in sj for v in (x - 6.5, x + 6.5)] + [BX2 - NOTCH]
for a, b in zip(top[::2], top[1::2]):
    m.outline([(a, BY_TOP), (b, BY_TOP)], closed=False)
m.outline([(BX1 + NOTCH, BY_TOP + NOTCH), (BX1 + NOTCH, BY_TOP)], closed=False)
m.outline([(BX2 - NOTCH, BY_TOP), (BX2 - NOTCH, BY_TOP + NOTCH)], closed=False)

m.place('J7', *J7_AT, J7_ROT)
m.place('H1', *HOLES['H1'])
m.place('H2', *HOLES['H2'])
m.place('SW5', 12.0, 84.0)                      # footswitch wires
m.place('U3', 27.0, 76.5, 90)                   # 5V regulator laid flat, away from the audio
m.place_centre("D3", 35.0, 18.4)               # reverse-polarity diode right behind the DC jack
# functional groups.  Signal runs right to left: input jack -> gain -> clipping -> tone/output -> output jack
m.pack([r for r in by_sheet(m, "Power and Bias") if r not in ("J3", "U3", "U2")], 29, 20.25, 65, 30.25, label="power")
m.pack(by_sheet(m, "Tone and Output") + ['U2'], 5.5, 30.5, 30, 47, label="tone/output")
m.pack(by_sheet(m, "Bypass"), 31, 30.5, 58, 47, label="bypass")
m.pack(by_sheet(m, "Input and Gain"), 58.5, 30.5, 88.5, 47, label="input/gain")
m.pack(by_sheet(m, "Clipping") + by_sheet(m, "Digital Pots"), 14, 47.5, 80, 61, label="clipping/pots")
m.pack(by_sheet(m, "MIDI"), 62, 64, 81, 81, label="midi")
m.pack(by_sheet(m, "Controller"), 30.5, 61.5, 61.5, 87.5, label="controller")
m.save()

# ===================================================================== control board
c = Board("Flashover_Controls", "Flashover - control board", layers=4)
# 4 layers: ground plane on In1 and a +5V plane on In2 feed the 36 LEDs and the driver by vias
for layer, net in ((pcbnew.In1_Cu, "GND"), (pcbnew.In2_Cu, "+5V")):
    c.b.SetLayerType(layer, pcbnew.LT_POWER)
    c.plane(layer, net, [(BX1, CTL_TOP), (BX2, CTL_TOP), (BX2, BY_BOT), (BX1, BY_BOT)])
c.outline([(BX1, CTL_TOP), (BX2, CTL_TOP), (BX2, BY_BOT), (BX1, BY_BOT)])
c.enclosure()
for ref, (kx, ky) in KNOBS.items():
    c.place(ref, kx - 7.5, ky - 2.5)            # shaft is 7.5, 2.5 from pad A
    for k, d in enumerate(RINGS[ref]):
        th = math.radians(-150 + 30 * k)        # 7 o'clock .. 5 o'clock, clockwise
        c.place(f"D{d}", kx + RING_R * math.sin(th), ky - RING_R * math.cos(th),
                round(-math.degrees(th) / 90) * 90)   # nearest 90 deg: the autorouter only understands square-on pads
for ref, (px_, py_) in PANEL.items():
    if ref.startswith('SW'):
        c.place(ref, px_ - 3.25, py_ - 2.25)    # 6 mm switch: body centre is 3.25, 2.25 from pad 1
    else:
        c.place(ref, px_ - 1.27, py_)           # 3 mm LED: centre is 1.27 mm from pad 1
c.place('H3', *HOLES['H3'])
c.place('H4', *HOLES['H4'])
c.place_centre('U10', 47.0, 45.0, 0)

# J8 on the underside: find the orientation whose pins land on J7's
want = m.pads_abs('J7')
best = None
for rot in (0, 90, 180, 270):
    c.place('J8', *J7_AT, rot, back=True)
    got = c.pads_abs('J8')
    err = max(math.dist(want[n], got[n]) for n in want)
    if best is None or err < best[0]:
        best = (err, rot)
c.place('J8', *J7_AT, best[1], back=True)
print(f"  J8 underside rotation {best[1]}, worst pin offset from J7 {best[0]:.3f} mm")

enc = lambda n: [f"R{23 + 3 * n}", f"R{24 + 3 * n}", f"R{25 + 3 * n}", f"C{33 + 2 * n}", f"C{34 + 2 * n}"]
c.pack(enc(0) + enc(1) + ['R32'], 5.5, 80.5, 34, 88, label="gain/tone encoders")
c.pack(enc(2) + ['R46'], 60, 74, 88.5, 81, label="level encoder")
c.pack(['C39', 'C40', 'R33', 'R34', 'R35', 'R36'], 37.5, 30.5, 57, 37.5, label="driver")
c.save()
