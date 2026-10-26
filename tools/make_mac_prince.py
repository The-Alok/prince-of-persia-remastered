#!/usr/bin/env python3
"""
Build data/res_mac_prince.pak from data/res_snes_alt.pak.

Mac Prince = the SNES Alt pack with the Prince recoloured: light-blue turban,
red vest, white trousers, tan skin and orange shoes.

Only the Prince (KID) sprites change. Each KID sprite carries the same 16-colour
palette, so the palette is edited in place and a few pixels are remapped so the
shoes get their own colours. The file size and entry table stay identical.
"""
import os
import struct
import sys

entry_struct = struct.Struct('<16s4sHHHBBII')
header_struct = struct.Struct('<8sII')

# palette index -> RGB (multiples of 4, so the 6-bit KID .pal stays in sync)
NEW = {
    1: (0xa0, 0x50, 0x20), 2: (0xd0, 0x78, 0x38), 3: (0xf0, 0xa4, 0x64),  # skin
    4: (0xb0, 0xb0, 0xb8), 6: (0xf8, 0xf8, 0xf8),                          # trousers
    5: (0xd8, 0x80, 0x2c), 9: (0xa0, 0x50, 0x10),                          # shoes (9 = dark)
    7: (0x78, 0xb8, 0xf8), 8: (0x30, 0x68, 0xd0),                          # turban
    13: (0xe8, 0x30, 0x28), 14: (0xb0, 0x18, 0x18),                        # vest
    15: (0xe8, 0x30, 0x28), 10: (0xb0, 0x18, 0x18),                        # sash -> vest
}


def remap_pixels(px, w, h):
    # Sword steel used indices 9 and 11; move it to grey/white to free index 9 for the shoes.
    px = [4 if v == 9 else 6 if v == 11 else v for v in px]
    out = px[:]
    # Shoe pixels dither index 5 with index 4. A 4 touching a 5 is a shoe pixel -> dark shoe colour.
    for y in range(h):
        for x in range(w):
            if px[y * w + x] == 4:
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    X, Y = x + dx, y + dy
                    if 0 <= X < w and 0 <= Y < h and px[Y * w + X] == 5:
                        out[y * w + x] = 9
                        break
    return out


def main():
    base = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data')
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(base, 'res_snes_alt.pak')
    dst = sys.argv[2] if len(sys.argv) > 2 else os.path.join(base, 'res_mac_prince.pak')

    buf = bytearray(open(src, 'rb').read())
    magic, version, count = header_struct.unpack_from(buf, 0)
    assert magic == b'POPSPAK1' and version == 1, 'not a POPSPAK1 file'

    sprites = 0
    for i in range(count):
        folder, ext, res_id, w, h, ncol, flags, off, size = entry_struct.unpack_from(
            buf, header_struct.size + i * entry_struct.size)
        if folder.rstrip(b'\0') != b'KID':
            continue
        ext = ext.rstrip(b'\0')
        blob = bytearray(buf[off:off + size])
        if ext == b'png':
            assert ncol == 16 and size == 48 + w * h
            for idx, rgb in NEW.items():
                blob[idx * 3:idx * 3 + 3] = bytes(rgb)
            blob[48:] = bytes(remap_pixels(blob[48:], w, h))
            sprites += 1
        elif ext == b'pal':
            # DOS-style 6-bit palette; the 16 VGA colours start 4 bytes in.
            for idx, rgb in NEW.items():
                blob[4 + idx * 3:4 + idx * 3 + 3] = bytes(v // 4 for v in rgb)
        buf[off:off + size] = blob

    open(dst, 'wb').write(buf)
    print('Recoloured %d Prince sprites -> %s' % (sprites, dst))


if __name__ == '__main__':
    main()
