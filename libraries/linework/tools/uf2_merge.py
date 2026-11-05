"""Merge .uf2 files (the loader and a pedal's firmware) into one, for a blank chip.

usage: python uf2_merge.py out.uf2 in1.uf2 in2.uf2 ...

Each UF2 block carries its own number and the file's block count; the RP2040's boot ROM uses the
count to know when it has everything, so the blocks are renumbered for the merged file.
"""
import struct
import sys

MAGIC0, MAGIC1, MAGIC_END = 0x0A324655, 0x9E5D5157, 0x0AB16F30

blocks = []
for fn in sys.argv[2:]:
    data = open(fn, "rb").read()
    for i in range(0, len(data), 512):
        b = bytearray(data[i:i + 512])
        m0, m1, flags, addr, size, no, total, fam = struct.unpack_from("<8I", b)
        assert m0 == MAGIC0 and m1 == MAGIC1 and struct.unpack_from("<I", b, 508)[0] == MAGIC_END, fn
        blocks.append((addr, b))
blocks.sort(key=lambda x: x[0])
addrs = [a for a, _ in blocks]
assert len(addrs) == len(set(addrs)), "the files overlap"
with open(sys.argv[1], "wb") as out:
    for n, (addr, b) in enumerate(blocks):
        struct.pack_into("<II", b, 20, n, len(blocks))
        out.write(b)
print(f"{sys.argv[1]}: {len(blocks)} blocks, {blocks[0][0]:#x} to {blocks[-1][0] + 256:#x}")
