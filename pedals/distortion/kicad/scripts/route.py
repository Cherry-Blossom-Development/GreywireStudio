"""Autoroute a board with Freerouting, then clean up, add ground pours and fill.

usage: python route.py <project> export|import
Run with KiCad's bundled Python.  Freerouting runs in between.
"""
import sys
import pcbnew

import dsnfix
import rules

from paths import KI, BUILD as SP
proj, step = sys.argv[1], sys.argv[2]
fn = KI + proj + "/" + proj + ".kicad_pcb"
b = pcbnew.LoadBoard(fn)

if step == "export":
    pcbnew.ExportSpecctraDSN(b, SP + proj + ".dsn")
    # inset the routing boundary so tracks stay rules.EDGE (+ a little) from the board edge
    ps = pcbnew.SHAPE_POLY_SET()
    b.GetBoardPolygonOutlines(ps, True)
    ps.Inflate(-pcbnew.FromMM(rules.EDGE + 0.05), pcbnew.CORNER_STRATEGY_ROUND_ALL_CORNERS, pcbnew.FromMM(0.01))
    ol = ps.Outline(0)
    pts = [(ol.CPoint(i).x / 1000.0, -ol.CPoint(i).y / 1000.0) for i in range(ol.PointCount())]
    groups = dsnfix.fix(SP + proj + ".dsn", pts)
    print("exported; classes:", groups)
    sys.exit(0)

ok = pcbnew.ImportSpecctraSES(b, SP + proj + ".ses")
print("imported", ok)

# remove vias the router left hanging (no track on one of their layers), except GND, which the pours pick up
gnd = b.FindNet("GND")
removed = 0
for t in list(b.GetTracks()):
    if t.GetClass() != "PCB_VIA" or t.GetNetCode() == gnd.GetNetCode():
        continue
    layers = set()
    for u in b.GetTracks():
        if u.GetClass() == "PCB_TRACK" and u.GetNetCode() == t.GetNetCode() and t.GetPosition() in (u.GetStart(), u.GetEnd()):
            layers.add(u.GetLayer())
    for fp in b.GetFootprints():
        for p in fp.Pads():
            if p.GetNetCode() == t.GetNetCode() and p.HitTest(t.GetPosition()):
                layers.update(p.GetLayerSet().CuStack())
    if not layers:
        b.Delete(t)
        removed += 1
print("removed dangling vias:", removed)

# ground pours on every routing layer, clipped to the board outline
bb = b.GetBoardEdgesBoundingBox()
existing = {z.GetLayer() for z in b.Zones()}
layers = [pcbnew.F_Cu, pcbnew.B_Cu] + ([pcbnew.In2_Cu] if b.GetCopperLayerCount() == 4 else [])
for layer in layers:
    if layer in existing:
        continue
    z = pcbnew.ZONE(b)
    z.SetLayer(layer)
    z.SetNet(gnd)
    z.SetZoneName("GND pour")
    z.SetAssignedPriority(0)
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THT_THERMAL)   # SMD pads solid, through-hole pads with spokes
    z.SetMinThickness(pcbnew.FromMM(0.2))
    z.SetThermalReliefGap(pcbnew.FromMM(0.3))
    z.SetThermalReliefSpokeWidth(pcbnew.FromMM(0.4))
    z.SetLocalClearance(pcbnew.FromMM(0.25))
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    ol = z.Outline()
    ol.NewOutline()
    for x, y in ((bb.GetLeft(), bb.GetTop()), (bb.GetRight(), bb.GetTop()),
                 (bb.GetRight(), bb.GetBottom()), (bb.GetLeft(), bb.GetBottom())):
        ol.Append(x, y)
    b.Add(z)
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
pcbnew.SaveBoard(fn, b)
rules.apply_file(KI + proj + "/" + proj + ".kicad_pro")
print("saved with", len(b.Zones()), "zones")
