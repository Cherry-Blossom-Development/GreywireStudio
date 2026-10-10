"""Small schematic writer for KiCad 10 (.kicad_sch).  Coordinates in grid units (1.27 mm)."""
import uuid as _uuid
from kisym import get_symbol, pins, xform, dump, Q

G = 1.27
PROJECT = "Flashover"
from paths import LIB
GW_LIB = LIB + "GreywireStudio.kicad_sym"


import random as _random
_rng = _random.Random(20261009)
NS = _uuid.UUID('6f1c2c55-6a6e-4f53-9d55-0b7b6c3c1a10')


def U():
    """Repeatable UUIDs, so regenerating gives the same file."""
    return str(_uuid.UUID(int=_rng.getrandbits(128), version=4))


def UK(key):
    """Stable UUID for a named object (symbols, sheets), so the board keeps its links."""
    return str(_uuid.uuid5(NS, key))


def mm(v):
    return round(v * G, 4)


OX, OY = 8, 6


def px(v):
    return mm(v + OX)


def py(v):
    return mm(v + OY)


def f(v):
    s = f"{v:.4f}".rstrip('0').rstrip('.')
    return '0' if s == '-0' else s


def font(size=1.27, bold=False, italic=False):
    s = f"(font (size {f(size)} {f(size)})"
    if bold:
        s += " (bold yes)"
    if italic:
        s += " (italic yes)"
    return s + ")"


class Comp:
    def __init__(self, sheet, ref, lib_id, value, x, y, rot=0, mirror=None, unit=1,
                 fields=None, hide_ref=False, hide_val=False, power=False):
        self.sheet, self.ref, self.lib_id, self.value = sheet, ref, lib_id, value
        self.x, self.y, self.rot, self.mirror, self.unit = x, y, rot, mirror, unit
        self.fields, self.hide_ref, self.hide_val, self.power = fields or {}, hide_ref, hide_val, power
        lib, sym = lib_id.split(':')
        self.tree = sheet.need(lib, sym)
        self.pins = pins(self.tree, unit)

    def p(self, num):
        px, py = self.pins[num][0] / G, self.pins[num][1] / G
        return xform(px, py, self.x, self.y, self.rot, self.mirror)

    def field_pos(self, which):
        """(x, y, justify) in grid units."""
        if which in self.fields:
            dx, dy, j = self.fields[which]
            return self.x + dx, self.y + dy, j
        short = self.lib_id.split(':')[1]
        horiz = self.rot in (90, 270)
        if short in ('R', 'C', 'C_Polarized'):
            if horiz:
                return (self.x, self.y - (4 if which == 'ref' else 2), None)
            return (self.x + 2, self.y + (-1 if which == 'ref' else 1), 'left')
        if short in ('D', 'D_Schottky'):
            if horiz:
                return (self.x + 2, self.y + (-1 if which == 'ref' else 1), 'left')
            return (self.x, self.y - (4 if which == 'ref' else 2), None)
        if short == 'R_Potentiometer':
            return (self.x - 2, self.y + (-1 if which == 'ref' else 1), 'right')
        # library default, transformed
        for e in self.tree:
            if isinstance(e, list) and e[0] == 'property' and e[1][1] == ('Reference' if which == 'ref' else 'Value'):
                at = next(a for a in e if isinstance(a, list) and a[0] == 'at')
                eff = dump(next(a for a in e if isinstance(a, list) and a[0] == 'effects'))
                j = 'left' if 'justify left' in eff else ('right' if 'justify right' in eff else None)
                px, py = float(at[1]) / G, float(at[2]) / G
                x, y = xform(px, py, self.x, self.y, self.rot, self.mirror)
                if self.rot == 180:
                    j = {'left': 'right', 'right': 'left'}.get(j, j)
                return x, y, j
        return self.x, self.y, None

    def write(self, path):
        out = []
        a = f"(at {f(px(self.x))} {f(py(self.y))} {self.rot})"
        mir = f"\n\t\t(mirror {self.mirror})" if self.mirror else ""
        out.append(f"\t(symbol\n\t\t(lib_id \"{self.lib_id}\")\n\t\t{a}{mir}\n\t\t(unit {self.unit})\n\t\t(body_style 1)\n"
                   f"\t\t(exclude_from_sim no)\n\t\t(in_bom {'no' if self.power or getattr(self, 'no_bom', False) else 'yes'})\n\t\t(on_board {'no' if self.power else 'yes'})\n"
                   f"\t\t(in_pos_files yes)\n\t\t(dnp no)\n\t\t(uuid \"{UK(f'{PROJECT}/{self.sheet.title}/{self.ref}/{self.unit}')}\")")

        def prop(name, val, x, y, j, hide, ang=(self.rot if self.rot in (90, 270) else 0)):
            jj = f" (justify {j})" if j else ""
            hh = "\n\t\t\t(hide yes)" if hide else ""
            return (f"\t\t(property \"{name}\" \"{val}\"\n\t\t\t(at {f(px(x))} {f(py(y))} {ang}){hh}\n\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n"
                    f"\t\t\t(effects {font()}{jj})\n\t\t)")
        rx, ry, rj = self.field_pos('ref')
        vx, vy, vj = self.field_pos('val')
        if self.rot in (90, 270) or self.mirror == 'y':
            # justification on rotated/mirrored symbols is unreliable, so center the text
            def centre(x, j, txt):
                w = len(txt) * 0.9
                return (x + w / 2 if j == 'left' else x - w / 2 if j == 'right' else x), None
            rx, rj = centre(rx, rj, self.ref)
            vx, vj = centre(vx, vj, self.value)
        out.append(prop("Reference", self.ref, rx, ry, rj, self.hide_ref))
        out.append(prop("Value", self.value, vx, vy, vj, self.hide_val))
        out.append(prop("Footprint", getattr(self, "footprint", ""), self.x, self.y, None, True))
        out.append(prop("Datasheet", "", self.x, self.y, None, True))
        for num in self.pins:
            out.append(f"\t\t(pin \"{num}\"\n\t\t\t(uuid \"{U()}\")\n\t\t)")
        out.append(f"\t\t(instances\n\t\t\t(project \"{PROJECT}\"\n\t\t\t\t(path \"{path}\"\n\t\t\t\t\t(reference \"{self.ref}\")\n"
                   f"\t\t\t\t\t(unit {self.unit})\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)")
        return '\n'.join(out)


class Sheet:
    pwr_count = [0]
    flg_count = [0]

    def __init__(self, title, page, comment=""):
        self.uuid = UK('sheet/' + title)
        self.title, self.page, self.comment = title, page, comment
        self.libs = {}
        self.comps = {}
        self.items = []      # extra s-expr strings
        self.wires = []      # ((x1,y1),(x2,y2))
        self.labelpts = []
        self.ncs = []

    def need(self, lib, sym):
        key = f"{lib}:{sym}"
        if key not in self.libs:
            self.libs[key] = get_symbol(lib, sym, GW_LIB if lib == 'GreywireStudio' else None)
        return self.libs[key]

    # ---- parts ----
    def add(self, ref, lib_id, value, x, y, rot=0, **kw):
        key = ref if ref not in self.comps else f"{ref}.{kw.get('unit', 1)}"
        c = Comp(self, ref, lib_id, value, x, y, rot, **kw)
        self.comps[key] = c
        return c

    def pwr(self, kind, x, y, rot=0, hide_val=None):
        lib_id = {'+9V': 'power:+9V', 'GND': 'power:GND', '+5V': 'power:+5V', '+3V3': 'power:+3V3',
                  'VB': 'GreywireStudio:VB', 'FLAG': 'power:PWR_FLAG'}[kind]
        if kind == 'FLAG':
            Sheet.flg_count[0] += 1
            ref = f"#FLG{Sheet.flg_count[0]:02d}"
            val = 'PWR_FLAG'
        else:
            Sheet.pwr_count[0] += 1
            ref = f"#PWR{Sheet.pwr_count[0]:02d}"
            val = kind
        c = Comp(self, ref, lib_id, val, x, y, rot, hide_ref=True, power=True,
                 hide_val=(kind == 'FLAG') if hide_val is None else hide_val)
        if rot in (90, 270):
            # sideways symbol: put the name beyond the end of the symbol
            d = -1 if (rot == 90) == (kind != 'GND') else 1
            c.fields = {'val': (d * (3 + len(val) * 0.45), 0, None), 'ref': (d * 6, 2, None)}
        if rot == 180:
            # keep the name readable below the inverted symbol
            c.fields = {'val': (0, 3.2, None), 'ref': (0, 5, None)}
        self.comps[ref] = c
        return c

    def P(self, key, pin):
        return self.comps[key].p(pin)

    # ---- wiring ----
    def wire(self, *pts):
        pts = [tuple(p) for p in pts]
        for a, b in zip(pts, pts[1:]):
            if a[0] != b[0] and a[1] != b[1]:
                raise ValueError(f"diagonal wire {a}->{b} on {self.title}")
            if a != b:
                self.wires.append((a, b))

    def nc(self, pt):
        self.ncs.append(tuple(pt))

    def label(self, kind, name, x, y, angle=0, shape='input'):
        just = {0: 'left', 180: 'right', 90: 'left', 270: 'right'}[angle]
        self.labelpts.append((x, y))
        if kind == 'local':
            self.items.append(f"\t(label \"{name}\"\n\t\t(at {f(px(x))} {f(py(y))} {angle})\n\t\t(effects {font()} (justify {just} bottom))\n\t\t(uuid \"{U()}\")\n\t)")
        elif kind == 'hier':
            self.items.append(f"\t(hierarchical_label \"{name}\"\n\t\t(shape {shape})\n\t\t(at {f(px(x))} {f(py(y))} {angle})\n"
                              f"\t\t(effects {font()} (justify {just}))\n\t\t(uuid \"{U()}\")\n\t)")
        elif kind == 'global':
            self.items.append(f"\t(global_label \"{name}\"\n\t\t(shape {shape})\n\t\t(at {f(px(x))} {f(py(y))} {angle})\n"
                              f"\t\t(fields_autoplaced yes)\n\t\t(effects {font()} (justify {just}))\n\t\t(uuid \"{U()}\")\n"
                              f"\t\t(property \"Intersheetrefs\" \"${{INTERSHEET_REFS}}\"\n\t\t\t(at {f(px(x))} {f(py(y))} 0)\n"
                              f"\t\t\t(effects {font()} (justify {just}) (hide yes))\n\t\t)\n\t)")

    def text(self, s, x, y, size=1.27, bold=False, just='left'):
        s = s.replace('"', '\\"').replace('\n', '\\n')
        self.items.append(f"\t(text \"{s}\"\n\t\t(exclude_from_sim no)\n\t\t(at {f(px(x))} {f(py(y))} 0)\n"
                          f"\t\t(effects {font(size, bold)} (justify {just}))\n\t\t(uuid \"{U()}\")\n\t)")

    def box(self, x1, y1, x2, y2, title=None):
        self.items.append(f"\t(rectangle\n\t\t(start {f(px(x1))} {f(py(y1))})\n\t\t(end {f(px(x2))} {f(py(y2))})\n"
                          f"\t\t(stroke (width 0) (type dash))\n\t\t(fill (type none))\n\t\t(uuid \"{U()}\")\n\t)")
        if title:
            self.text(title, x1 + 1, y1 + 1.5, size=1.524, bold=True)

    # ---- connectivity helpers ----
    def all_pins(self):
        pts = []
        for c in self.comps.values():
            # stacked pins (same number of points) count once
            pts.extend(set(c.p(n) for n in c.pins))
        return pts

    def junctions(self):
        def on_interior(p, seg):
            (x1, y1), (x2, y2) = seg
            if x1 == x2 == p[0]:
                return min(y1, y2) < p[1] < max(y1, y2)
            if y1 == y2 == p[1]:
                return min(x1, x2) < p[0] < max(x1, x2)
            return False
        pinpts = self.all_pins()
        cand = set()
        for a, b in self.wires:
            cand.add(a)
            cand.add(b)
        out = []
        for p in cand:
            n = sum((a == p) + (b == p) for a, b in self.wires)
            n += pinpts.count(p)
            n += 2 * sum(on_interior(p, s) for s in self.wires)
            if n >= 3:
                out.append(p)
        # sanity: a pin must never sit in the middle of a wire
        for p in pinpts:
            for s in self.wires:
                if on_interior(p, s):
                    raise ValueError(f"pin at {p} lies inside wire {s} on {self.title}")
        return out

    def check_dangling(self):
        """Wire ends that touch nothing (no pin, other wire, label or nc)."""
        pinpts = set(self.all_pins())
        bad = []
        for a, b in self.wires:
            for p in (a, b):
                others = sum((x == p) + (y == p) for x, y in self.wires) - 1
                mid = any((x[0] == y[0] == p[0] and min(x[1], y[1]) < p[1] < max(x[1], y[1])) or
                          (x[1] == y[1] == p[1] and min(x[0], y[0]) < p[0] < max(x[0], y[0])) for x, y in self.wires)
                if not (p in pinpts or others or mid or p in self.labelpts):
                    bad.append(p)
        return bad

    # ---- output ----
    def render(self, path, root_extra="", is_root=False):
        o = [f"(kicad_sch\n\t(version 20260306)\n\t(generator \"eeschema\")\n\t(generator_version \"10.0\")\n"
             f"\t(uuid \"{self.uuid}\")\n\t(paper \"A4\")"]
        o.append(f"\t(title_block\n\t\t(title \"{self.title}\")\n\t\t(date \"2026-10-09\")\n\t\t(rev \"0.1\")\n"
                 f"\t\t(company \"GreywireStudio\")\n\t\t(comment 1 \"{self.comment}\")\n\t)")
        o.append("\t(lib_symbols")
        for t in self.libs.values():
            o.append('\t\t' + dump(t, 2))
        o.append("\t)")
        for p in self.junctions():
            o.append(f"\t(junction\n\t\t(at {f(px(p[0]))} {f(py(p[1]))})\n\t\t(diameter 0)\n\t\t(color 0 0 0 0)\n\t\t(uuid \"{U()}\")\n\t)")
        for p in self.ncs:
            o.append(f"\t(no_connect\n\t\t(at {f(px(p[0]))} {f(py(p[1]))})\n\t\t(uuid \"{U()}\")\n\t)")
        for a, b in self.wires:
            o.append(f"\t(wire\n\t\t(pts\n\t\t\t(xy {f(px(a[0]))} {f(py(a[1]))}) (xy {f(px(b[0]))} {f(py(b[1]))})\n\t\t)\n"
                     f"\t\t(stroke\n\t\t\t(width 0)\n\t\t\t(type default)\n\t\t)\n\t\t(uuid \"{U()}\")\n\t)")
        o.extend(self.items)
        for c in self.comps.values():
            o.append(c.write(path))
        o.append(root_extra)
        if is_root:
            o.append(f"\t(sheet_instances\n\t\t(path \"/\"\n\t\t\t(page \"1\")\n\t\t)\n\t)")
        o.append("\t(embedded_fonts no)\n)\n")
        return '\n'.join(x for x in o if x)
