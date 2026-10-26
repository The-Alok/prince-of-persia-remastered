#!/usr/bin/env python3
"""Build 640x400 Macintosh graphics packs for SDLPoP.

Usage:
  build_mac_native_pak.py NATIVE_SOURCE OUT_PAK [--full]

NATIVE_SOURCE can be the original ZIP or its extracted folder containing
manifest.csv. Source images and metadata are read without modification.

Default (stable): only resources that the existing res_mac_color.pak replaced
relative to res_dos.pak are emitted. This gives native-resolution Mac characters,
objects, princess/title art, while unsupported Mac wall composition falls back to
SDLPoP/DOS resources.

--full: emit all 820 decoded Macintosh resources for the mac_native_full
compositor. See the renderer milestone notes for verified coverage and limits.

POPSPAK1 flag bits:
  bit 0: transparency / color-key 0
  bit 1: image pixels are already renderer-native (do not 2x-scale)

For each native PNG, v6 also emits a compact sidecar entry with ext='org'.
Its four-byte payload is little-endian int16 origin_x, origin_y from the
Mac SHPD record. Keeping origins inside the same pack avoids a second metadata
file and keeps POPSPAK1 backwards-compatible.
"""
from __future__ import annotations
import csv, io, struct, sys, zipfile
from contextlib import ExitStack
from pathlib import Path, PurePosixPath
from PIL import Image

HEADER = struct.Struct('<8sII')
ENTRY = struct.Struct('<16s4sHHHBBII')
FLAG_TRANSPARENT = 1
FLAG_NATIVE_RES = 2


def read_pack_entries(path: Path):
    b = path.read_bytes()
    magic, version, count = HEADER.unpack_from(b, 0)
    if magic != b'POPSPAK1' or version != 1:
        raise ValueError(f'{path}: unsupported pack')
    out = {}
    for i in range(count):
        vals = ENTRY.unpack_from(b, HEADER.size + i * ENTRY.size)
        folder, ext, rid, w, h, ncol, flags, off, size = vals
        key = (folder.rstrip(b'\0').decode('ascii').lower(), rid, ext.rstrip(b'\0').decode('ascii').lower())
        out[key] = (w, h, ncol, flags, b[off:off+size])
    return out


def stable_keys(repo_root: Path):
    dos = read_pack_entries(repo_root / 'data/res_dos.pak')
    mac = read_pack_entries(repo_root / 'tools/mac_reference/legacy-packs/res_mac_color.pak')
    changed = set()
    for key, value in mac.items():
        if key[2] == 'png' and key in dos and value != dos[key]:
            changed.add((key[0], key[1]))
    return changed


def image_to_payload(data: bytes):
    im = Image.open(io.BytesIO(data)).convert('RGBA')
    colors = [(0, 0, 0)]  # color-key index 0
    color_to_index = {}
    pixels = bytearray()
    has_transparency = False
    for r, g, b, a in im.getdata():
        if a < 128:
            pixels.append(0)
            has_transparency = True
            continue
        key = (r, g, b)
        idx = color_to_index.get(key)
        if idx is None:
            idx = len(colors)
            if idx > 255:
                raise ValueError(f'{im.size}: more than 255 opaque colors')
            color_to_index[key] = idx
            colors.append(key)
        pixels.append(idx)
    pal = bytearray()
    for rgb in colors:
        pal.extend(rgb)
    flags = FLAG_NATIVE_RES | (FLAG_TRANSPARENT if has_transparency else 0)
    return im.width, im.height, len(colors), flags, bytes(pal + pixels)


def main():
    args = [a for a in sys.argv[1:] if a != '--full']
    full = '--full' in sys.argv[1:]
    if len(args) != 2:
        print('usage: build_mac_native_pak.py NATIVE_SOURCE OUT_PAK [--full]')
        raise SystemExit(2)
    src, out = map(Path, args)
    repo_root = Path(__file__).resolve().parents[2]
    allowed = None if full else stable_keys(repo_root)

    with ExitStack() as stack:
        if src.is_dir():
            manifest = stack.enter_context((src / 'manifest.csv').open(encoding='utf-8-sig', newline=''))
            def read_image(name):
                path = (src / name).resolve()
                if not path.is_relative_to(src.resolve()):
                    raise ValueError(f'image path escapes source folder: {name}')
                return path.read_bytes()
        else:
            archive = stack.enter_context(zipfile.ZipFile(src))
            manifest = stack.enter_context(io.TextIOWrapper(
                archive.open('Mac-Color-Native-Graphics/manifest.csv'), encoding='utf-8-sig'))
            def read_image(name):
                return archive.read('Mac-Color-Native-Graphics/' + name)
        rows = list(csv.DictReader(manifest))
        built = []
        for row in rows:
            folder = row['folder']
            rid = int(row['resource_id'])
            if allowed is not None and (folder.lower(), rid) not in allowed:
                continue
            relative = PurePosixPath(row['file'])
            if relative.is_absolute() or '..' in relative.parts:
                raise ValueError(f'invalid image path: {relative}')
            file_name = relative.as_posix()
            w, h, ncol, flags, payload = image_to_payload(read_image(file_name))
            if w != int(row['width']) or h != int(row['height']):
                raise ValueError(f'dimension mismatch for {file_name}')
            built.append((folder, 'png', rid, w, h, ncol, flags, payload))
            origin_payload = struct.pack('<hh', int(row['origin_x']), int(row['origin_y']))
            built.append((folder, 'org', rid, 0, 0, 0, 0, origin_payload))

    built.sort(key=lambda e: (e[0].lower(), e[2], e[1].lower()))
    data_offset = HEADER.size + ENTRY.size * len(built)
    table, payloads = bytearray(), bytearray()
    for folder, ext, rid, w, h, ncol, flags, payload in built:
        fb = (folder.encode('ascii')[:15] + b'\0').ljust(16, b'\0')
        eb = (ext.encode('ascii')[:3] + b'\0').ljust(4, b'\0')
        table += ENTRY.pack(fb, eb, rid, w, h, ncol, flags, data_offset, len(payload))
        payloads += payload
        data_offset += len(payload)

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(HEADER.pack(b'POPSPAK1', 1, len(built)) + table + payloads)
    label = 'full' if full else 'stable'
    print(f'wrote {len(built)//2} {label} native Macintosh images + origin sidecars -> {out} ({out.stat().st_size} bytes)')

if __name__ == '__main__':
    main()
