# Make the JLCPCB order files for the breakout board: gerbers + drill (zipped), BOM and
# placement (CPL), into ../Flashover_Breakout/fab/. Run after route.sh, and check DRC first.
cd "$(dirname "$0")" && . ./env.sh
P=Flashover_Breakout
B="$KD/$P/$P.kicad_pcb"
OUT="$KD/$P/fab"
rm -rf "$OUT" && mkdir -p "$OUT/gerbers"
"$K" pcb export gerbers --layers F.Cu,B.Cu,F.Paste,B.Paste,F.SilkS,B.SilkS,F.Mask,B.Mask,Edge.Cuts \
    --subtract-soldermask -o "$OUT/gerbers/" "$B" >/dev/null
"$K" pcb export drill --format excellon --excellon-separate-th --generate-map --map-format gerberx2 \
    -o "$OUT/gerbers/" "$B" >/dev/null
(cd "$OUT/gerbers" && python -c "import zipfile,os; z=zipfile.ZipFile('../${P}_gerbers.zip','w',zipfile.ZIP_DEFLATED); [z.write(f) for f in sorted(os.listdir('.'))]")
rm -rf "$OUT/gerbers"
"$K" pcb export pos --side front --format csv --units mm --exclude-dnp -o "$OUT/pos.csv" "$B" >/dev/null
python - "$OUT" <<'PY'
import csv, sys
out = sys.argv[1]
# LCSC part numbers for JLCPCB assembly. The pin headers are left for hand soldering.
LCSC = {
    "MCP45HV51-104": ("C637118", "MCP45HV51-104E/ST"),
    "MCP45HV51-103": ("C637117", "MCP45HV51-103E/ST"),
    "IS31FL3236A-QFLS2": ("C246443", "IS31FL3236A-QFLS2-TR"),
    "100n": ("C14663", "100nF 50V X7R 0603"),
    "1u": ("C15849", "1uF 25V X5R 0603"),
    "4.7k": ("C23162", "4.7k 1% 0603"),
    "15k": ("C22809", "15k 1% 0603"),
    "100k": ("C25803", "100k 1% 0603"),
}
rows = list(csv.DictReader(open(out + "/pos.csv", encoding="utf-8")))
groups = {}
cpl = []
for r in rows:
    ref, val, fp = r["Ref"], r["Val"], r["Package"]
    if ref.startswith("J"):
        continue                                    # pin headers: soldered by hand
    groups.setdefault((val, fp), []).append(ref)
    cpl.append([ref, f"{float(r['PosX']):.3f}mm", f"{float(r['PosY']):.3f}mm", "Top", f"{float(r['Rot']):g}"])
with open(out + "/Flashover_Breakout_bom.csv", "w", newline="", encoding="utf-8") as fh:
    w = csv.writer(fh)
    w.writerow(["Comment", "Designator", "Footprint", "LCSC Part #"])
    for (val, fp), refs in sorted(groups.items()):
        lc, desc = LCSC[val]
        w.writerow([desc, ",".join(sorted(refs, key=lambda s: (s[0], int(s[1:])))), fp, lc])
with open(out + "/Flashover_Breakout_cpl.csv", "w", newline="", encoding="utf-8") as fh:
    w = csv.writer(fh)
    w.writerow(["Designator", "Mid X", "Mid Y", "Layer", "Rotation"])
    w.writerows(sorted(cpl, key=lambda r: (r[0][0], int(r[0][1:]))))
print("BOM lines:", len(groups), " placed parts:", len(cpl))
PY
rm -f "$OUT/pos.csv"
ls -la "$OUT"
