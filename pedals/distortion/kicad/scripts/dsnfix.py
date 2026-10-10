"""Fix up a Specctra DSN exported by KiCad's Python API before Freerouting sees it:
- net classes and clearance from rules.py (the API doesn't load the project's classes)
- optional inset boundary, so tracks keep rules.EDGE away from the board edge."""
import fnmatch
import re

import rules


def fix(fn, boundary=None):
    s = open(fn, encoding='utf-8').read()
    cl = round(rules.CLEARANCE * 1000)
    s = re.sub(r"\(clearance 200(\)| )", rf"(clearance {cl}\1", s)
    if boundary:
        a = s.index("(path pcb 0", s.index("(boundary"))
        b = s.index(")", a)
        pts = "  ".join(f"{x:.1f} {y:.1f}" for x, y in boundary + boundary[:1])
        s = s[:a] + "(path pcb 0  " + pts + s[b:]
    a = s.index("(class kicad_default")
    b = s.index("(circuit", a)
    names = re.findall(r'"[^"]*"|[^\s()]+', s[a + len("(class kicad_default"):b])
    groups = {n: [] for n, _, _ in rules.CLASSES}
    keep = []
    for n in names:
        bare = n.strip('"')
        for cname, _, pats in rules.CLASSES:
            if any(fnmatch.fnmatchcase(bare, p) for p in pats):
                groups[cname].append(n)
                break
        else:
            keep.append(n)
    s = s[:a] + "(class kicad_default " + " ".join(keep) + "\n      " + s[b:]
    a = s.index("(class kicad_default")
    end = s.index(")", s.index(")", s.index(f"(clearance {cl})", a)) + 1)   # end of (rule ...), then of (class ...)
    extra = ""
    for cname, w, _ in rules.CLASSES:
        if groups[cname]:
            extra += (f"\n    (class {cname.replace(' ', '_')} {' '.join(groups[cname])}\n"
                      f"      (circuit\n        (use_via \"Via[0-3]_600:300_um\")\n      )\n"
                      f"      (rule\n        (width {round(w * 1000)})\n        (clearance {cl})\n      )\n    )")
    s = s[:end + 1] + extra + s[end + 1:]
    open(fn, 'w', encoding='utf-8').write(s)
    return {k: [n.strip('"') for n in v] for k, v in groups.items()}
