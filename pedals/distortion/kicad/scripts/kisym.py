"""Minimal KiCad S-expression parser and symbol-library helpers."""
import math, os, re

LIBDIR = r"C:\Users\dalla\AppData\Local\Programs\KiCad\10.0\share\kicad\symbols"


def tokenize(s):
    i, n = 0, len(s)
    while i < n:
        c = s[i]
        if c in ' \t\r\n':
            i += 1
        elif c == '(' or c == ')':
            yield c
            i += 1
        elif c == '"':
            j = i + 1
            buf = []
            while s[j] != '"':
                if s[j] == '\\':
                    buf.append(s[j:j + 2])
                    j += 2
                else:
                    buf.append(s[j])
                    j += 1
            yield ('"', ''.join(buf))
            i = j + 1
        else:
            j = i
            while j < n and s[j] not in ' \t\r\n()':
                j += 1
            yield s[i:j]
            i = j


def parse(s):
    stack = [[]]
    for t in tokenize(s):
        if t == '(':
            stack.append([])
        elif t == ')':
            x = stack.pop()
            stack[-1].append(x)
        else:
            stack[-1].append(t)
    return stack[0][0]


class Q(str):
    """Quoted string."""


def dump(x, ind=0):
    if isinstance(x, list):
        simple = all(not isinstance(e, list) for e in x)
        if simple:
            return '(' + ' '.join(atom(e) for e in x) + ')'
        out = '(' + ' '.join(atom(e) for e in x if not isinstance(e, list))
        for e in x:
            if isinstance(e, list):
                out += '\n' + '\t' * (ind + 1) + dump(e, ind + 1)
        return out + '\n' + '\t' * ind + ')'
    return atom(x)


def atom(e):
    if isinstance(e, tuple):
        return '"' + e[1] + '"'
    if isinstance(e, Q):
        return '"' + str(e).replace('"', '\\"') + '"'
    return str(e)


def name(x):
    return x[1][1] if isinstance(x[1], tuple) else x[1]


_libcache = {}


def load_lib(lib):
    if lib not in _libcache:
        path = lib if os.path.isabs(lib) else os.path.join(LIBDIR, lib + '.kicad_sym')
        with open(path, encoding='utf-8') as f:
            tree = parse(f.read())
        _libcache[lib] = {name(e): e for e in tree if isinstance(e, list) and e[0] == 'symbol'}
    return _libcache[lib]


def get_symbol(lib, sym, libpath=None):
    """Return a flattened copy of a library symbol, named 'lib:sym'."""
    import copy
    syms = load_lib(libpath or lib)
    s = copy.deepcopy(syms[sym])
    ext = [e for e in s if isinstance(e, list) and e[0] == 'extends']
    if ext:
        parent = copy.deepcopy(syms[name(ext[0])])
        pname = name(parent)
        # child properties override parent's
        props = {name(e): e for e in s if isinstance(e, list) and e[0] == 'property'}
        new = [parent[0], ('"', sym)]
        for e in parent[2:]:
            if isinstance(e, list) and e[0] == 'property' and name(e) in props:
                new.append(props.pop(name(e)))
            elif isinstance(e, list) and e[0] == 'symbol':
                e[1] = ('"', name(e).replace(pname, sym, 1))
                new.append(e)
            else:
                new.append(e)
        # insert remaining child-only properties before first sub-symbol
        idx = next(i for i, e in enumerate(new) if isinstance(e, list) and e[0] == 'symbol')
        for p in props.values():
            new.insert(idx, p)
            idx += 1
        s = new
    s[1] = ('"', f'{lib}:{sym}')
    return s


def pins(symtree, unit=1, body=1):
    """{number: (x, y, angle)} in library coordinates (y up)."""
    out = {}
    for sub in symtree:
        if isinstance(sub, list) and sub[0] == 'symbol':
            m = re.match(r'.*_(\d+)_(\d+)$', name(sub))
            u, b = int(m.group(1)), int(m.group(2))
            if u not in (0, unit) or b not in (0, body):
                continue
            for p in sub:
                if isinstance(p, list) and p[0] == 'pin':
                    at = next(e for e in p if isinstance(e, list) and e[0] == 'at')
                    num = next(e for e in p if isinstance(e, list) and e[0] == 'number')
                    nm = next(e for e in p if isinstance(e, list) and e[0] == 'name')
                    out[name(num)] = (float(at[1]), float(at[2]), float(at[3]), name(nm), p[1])
    return out


def xform(px, py, x, y, rot, mirror=None):
    """Library pin point -> schematic point for a symbol at (x, y, rot)."""
    if mirror == 'y':
        px = -px
    elif mirror == 'x':
        py = -py
    a = math.radians(rot)
    rx = px * math.cos(a) - py * math.sin(a)
    ry = px * math.sin(a) + py * math.cos(a)
    return (round(x + rx, 3), round(y - ry, 3))
