"""Fan out surface-mount pads on plane nets: a short track and a via down to the inner plane.

usage: python fanout.py <project>      (KiCad's bundled Python; run after gen_pcb.py, before routing)
"""
import math
import sys
import pcbnew

import rules

from paths import KI
proj = sys.argv[1]
fn = KI + proj + "/" + proj + ".kicad_pcb"
b = pcbnew.LoadBoard(fn)
mm, FM = pcbnew.ToMM, pcbnew.FromMM

plane_nets = {z.GetNetname() for z in b.Zones() if z.GetLayer() in (pcbnew.In1_Cu, pcbnew.In2_Cu)}
VIA_D, VIA_DRILL, CL = 0.6, 0.3, rules.CLEARANCE + 0.05
WIDTH = {"GND": 0.4, "+5V": 0.4}

# obstacles: every pad as a box (mm), plus vias/tracks we add as we go
pads = []
for fp in b.GetFootprints():
    for p in fp.Pads():
        bb = p.GetBoundingBox()
        pads.append((p, p.GetNetname(), mm(bb.GetLeft()), mm(bb.GetTop()), mm(bb.GetRight()), mm(bb.GetBottom())))
edge = pcbnew.SHAPE_POLY_SET()
b.GetBoardPolygonOutlines(edge, True)
inner = edge.CloneDropTriangulation()
inner.Inflate(-FM(rules.EDGE + VIA_D / 2), pcbnew.CORNER_STRATEGY_ROUND_ALL_CORNERS, FM(0.01))
new_vias, new_segs = [], []


def box_dist(x, y, l, t, r, bt):
    dx = max(l - x, 0, x - r)
    dy = max(t - y, 0, y - bt)
    return math.hypot(dx, dy)


def seg_dist(px, py, ax, ay, bx, by):
    vx, vy = bx - ax, by - ay
    L = vx * vx + vy * vy
    u = 0 if L == 0 else max(0, min(1, ((px - ax) * vx + (py - ay) * vy) / L))
    return math.hypot(px - ax - u * vx, py - ay - u * vy)


def free(net, pad, vx, vy, sx, sy, w):
    if not inner.Contains(pcbnew.VECTOR2I(FM(vx), FM(vy))):
        return False
    for p, n, l, t, r, bt in pads:
        if p is pad or n == net:
            continue
        if box_dist(vx, vy, l, t, r, bt) < VIA_D / 2 + CL:
            return False
        for k in range(1, 6):          # sample the stub track
            x, y = sx + (vx - sx) * k / 6, sy + (vy - sy) * k / 6
            if box_dist(x, y, l, t, r, bt) < w / 2 + CL:
                return False
    for x, y, n in new_vias:
        if math.hypot(vx - x, vy - y) < VIA_D + (CL if n != net else 0.1):
            return False
    for ax, ay, bx, by, n, ww in new_segs:
        if n != net and seg_dist(vx, vy, ax, ay, bx, by) < VIA_D / 2 + ww / 2 + CL:
            return False
    return True


made = missed = 0
for fp in b.GetFootprints():
    cx, cy = mm(fp.GetPosition().x), mm(fp.GetPosition().y)
    for p in fp.Pads():
        net = p.GetNetname()
        if net not in plane_nets or p.GetAttribute() != pcbnew.PAD_ATTRIB_SMD:
            continue
        px, py = mm(p.GetPosition().x), mm(p.GetPosition().y)
        bb = p.GetBoundingBox()
        hw, hh = mm(bb.GetWidth()) / 2, mm(bb.GetHeight()) / 2
        w = WIDTH.get(net, 0.3)
        out = math.atan2(py - cy, px - cx) if (px, py) != (cx, cy) else 0.0
        dirs = sorted((math.radians(a) for a in range(0, 360, 45)), key=lambda a: abs(math.remainder(a - out, 2 * math.pi)))
        done = False
        for extra in (0.45, 0.8, 1.2, 1.7):
            for a in dirs:
                reach = abs(math.cos(a)) * hw + abs(math.sin(a)) * hh
                vx, vy = px + math.cos(a) * (reach + extra), py + math.sin(a) * (reach + extra)
                if free(net, p, vx, vy, px, py, w):
                    v = pcbnew.PCB_VIA(b)
                    v.SetPosition(pcbnew.VECTOR2I(FM(vx), FM(vy)))
                    v.SetWidth(FM(VIA_D))
                    v.SetDrill(FM(VIA_DRILL))
                    v.SetNet(p.GetNet())
                    b.Add(v)
                    t = pcbnew.PCB_TRACK(b)
                    t.SetStart(p.GetPosition())
                    t.SetEnd(v.GetPosition())
                    t.SetWidth(FM(w))
                    t.SetLayer(pcbnew.F_Cu)
                    t.SetNet(p.GetNet())
                    b.Add(t)
                    new_vias.append((vx, vy, net))
                    new_segs.append((px, py, vx, vy, net, w))
                    made += 1
                    done = True
                    break
            if done:
                break
        if not done:
            missed += 1
            print("  no room for a via at", fp.GetReference(), p.GetNumber(), net)
# exposed pads: a grid of vias inside the pad, and the chip's other ground pins tied straight in to it
for fp in b.GetFootprints():
    eps = [p for p in fp.Pads() if p.GetAttribute() == pcbnew.PAD_ATTRIB_SMD and p.GetNetname() in plane_nets
           and mm(p.GetBoundingBox().GetWidth()) > 2.5 and mm(p.GetBoundingBox().GetHeight()) > 2.5]
    for ep in eps:
        net = ep.GetNetname()
        bb = ep.GetBoundingBox()
        ex, ey = mm(ep.GetPosition().x), mm(ep.GetPosition().y)
        n = 3 if mm(bb.GetWidth()) > 3.6 else 2
        step = 1.2
        for i in range(n):
            for j in range(n):
                v = pcbnew.PCB_VIA(b)
                v.SetPosition(pcbnew.VECTOR2I(FM(ex + (i - (n - 1) / 2) * step), FM(ey + (j - (n - 1) / 2) * step)))
                v.SetWidth(FM(VIA_D))
                v.SetDrill(FM(VIA_DRILL))
                v.SetNet(ep.GetNet())
                b.Add(v)
        missed -= 1
        for p in fp.Pads():
            if p.GetNumber() == ep.GetNumber() or p.GetNetname() != net:
                continue
            if any(abs(mm(t.GetStart().x) - mm(p.GetPosition().x)) < 1e-3 and abs(mm(t.GetStart().y) - mm(p.GetPosition().y)) < 1e-3
                   for t in b.GetTracks() if t.GetClass() == "PCB_TRACK"):
                continue                      # already fanned out
            px, py = mm(p.GetPosition().x), mm(p.GetPosition().y)
            # straight in towards the exposed pad, stopping at its edge
            tx = min(max(px, mm(bb.GetLeft()) + 0.3), mm(bb.GetRight()) - 0.3)
            ty = min(max(py, mm(bb.GetTop()) + 0.3), mm(bb.GetBottom()) - 0.3)
            t = pcbnew.PCB_TRACK(b)
            t.SetStart(p.GetPosition())
            t.SetEnd(pcbnew.VECTOR2I(FM(tx), FM(ty)))
            t.SetWidth(FM(0.25))
            t.SetLayer(pcbnew.F_Cu)
            t.SetNet(p.GetNet())
            b.Add(t)
            missed -= 1
            print("  tied", fp.GetReference(), p.GetNumber(), "to its exposed pad")
pcbnew.SaveBoard(fn, b)
rules.apply_file(fn[:-len(".kicad_pcb")] + ".kicad_pro")
print(f"{proj}: planes {sorted(plane_nets)}, {made} fan-out vias, {missed} pads left for the router")
