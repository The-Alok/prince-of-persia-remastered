#!/usr/bin/env python3
"""Build the optional Macintosh pack from your own colour Prince of Persia disk.

Accepts classic-HFS .hfs/.dsk volumes, Disk Copy 4.2 and Apple Partition Maps.
Reads the image without mounting it, executing it, or downloading anything.
Only graphics and their origins are extracted; the port retains its DOS data.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import io
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/mac_native_renderer'))
from extract_mac_reference import bounded, hfs_files, resource_entries

HEADER = struct.Struct('<8sII')
ENTRY = struct.Struct('<16s4sHHHBBII')
GROUPS = {
    40: 'TITLE', 50: 'TITLE', 150: 'PRINCE', 400: 'KID', 700: 'PRINCE',
    800: 'PV', 850: 'PV', 900: 'PV', 950: 'PV', 960: 'PV',
    1200: 'VDUNGEON', 1360: 'VDUNGEON', 1750: 'GUARD',
    2200: 'VPALACE', 2360: 'VPALACE', 2750: 'SKEL',
    3750: 'FAT', 4750: 'SHADOW', 5750: 'VIZIER',
}
FIELDS = ['mac_group', 'folder', 'resource_id', 'width', 'height',
          'origin_x', 'origin_y', 'file']


def decoder_module():
    path = ROOT / 'tools/mac_color_port/decode_mac_pop_reference.py'
    spec = importlib.util.spec_from_file_location('mac_disk_decoder', path)
    decoder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(decoder)
    return decoder


def graphics_from_disk(image):
    """Return pack entries and optional PNG source records, entirely from disk."""
    decoder = decoder_module()
    files = hfs_files(image, include=lambda f: f['type'] == 'APPL' or f['name'] == 'Persia(COLOR)')
    applications = []
    for file in files:
        if file['type'] == 'APPL':
            resources = resource_entries(file['rsrc'])
            if ('clut', 2000) in resources:
                applications.append(resources)
    colors = [f for f in files if f['name'] == 'Persia(COLOR)']
    if len(applications) != 1 or len(colors) != 1:
        raise ValueError('expected one colour Prince of Persia application (clut 2000) and Persia(COLOR); this disk/release is unsupported')
    palette, _, _ = decoder.parse_clut(applications[0]['clut', 2000]['data'])
    resources = resource_entries(colors[0]['rsrc'])
    data = colors[0]['data']
    entries, records = [], []
    for group, folder in GROUPS.items():
        if ('SHPD', group) not in resources or ('SHPT', group) not in resources:
            raise ValueError(f'unsupported colour resources: missing SHPD/SHPT {group}')
        offset, compressed, size = struct.unpack('>III', resources['SHPD', group]['data'])
        if size > 64 * 1024 * 1024:
            raise ValueError('unreasonable decompressed graphic size')
        source = bounded(data, offset, compressed or size)
        decoded = decoder.presage_lzss(source, size) if compressed else source
        table = resources['SHPT', group]['data']
        if len(table) % 4:
            raise ValueError('invalid SHPT table length')
        offsets = struct.unpack('>' + 'I' * (len(table) // 4), table)
        for slot, at in enumerate(offsets):
            if at == 0xFFFFFFFF:
                continue
            w, h, ox, oy = struct.unpack('>4h', bounded(decoded, at, 8))
            if not 0 < w <= 4096 or not 0 < h <= 4096:
                raise ValueError('invalid native image dimensions')
            pixels, alpha, _ = decoder.decode_presage_v1(decoded[at + 8:], w, h)
            colors_rgb, indices, rgba = [(0, 0, 0)], bytearray(), bytearray()
            color_indices = {}
            transparent = False
            for index, opaque in zip(pixels, alpha):
                if not opaque:
                    transparent = True
                    indices.append(0)
                    rgba.extend(b'\0\0\0\0')
                    continue
                if index >= len(palette):
                    raise ValueError('graphic references a missing palette entry')
                rgb = tuple(palette[index])
                if rgb not in color_indices:
                    color_indices[rgb] = len(colors_rgb)
                    colors_rgb.append(rgb)
                if len(colors_rgb) > 255:
                    raise ValueError('graphic needs more colours than the PAK format supports')
                indices.append(color_indices[rgb])
                rgba.extend((*rgb, 255))
            rid = group % 1000 + slot
            payload = bytes(c for rgb in colors_rgb for c in rgb) + indices
            entries.append((folder, 'png', rid, w, h, len(colors_rgb), 2 | int(transparent), payload))
            entries.append((folder, 'org', rid, 0, 0, 0, 0, struct.pack('<hh', ox, oy)))
            records.append((dict(zip(FIELDS, (group, folder, rid, w, h, ox, oy,
                                             f'{folder}/{rid:04d}_macgroup{group}.png'))), bytes(rgba)))
    if len(records) != 820:
        raise ValueError(f'unsupported colour release: expected 820 images, found {len(records)}')
    return entries, records


def pack_bytes(entries):
    entries = sorted(entries, key=lambda e: (e[0].lower(), e[2], e[1].lower()))
    offset = HEADER.size + ENTRY.size * len(entries)
    table, payload = bytearray(), bytearray()
    for folder, ext, rid, w, h, colors, flags, data in entries:
        table += ENTRY.pack(folder.encode().ljust(16, b'\0'), ext.encode().ljust(4, b'\0'),
                            rid, w, h, colors, flags, offset, len(data))
        payload += data
        offset += len(data)
    return HEADER.pack(b'POPSPAK1', 1, len(entries)) + table + payload


def write_pack(path, content, force=False):
    """Validate before replacing, and never silently overwrite a different pack."""
    if path.exists():
        if path.read_bytes() == content:
            return
        if not force:
            raise ValueError(f'{path} already exists and differs; back it up or pass --force')
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=path.parent, prefix=path.name + '.', delete=False) as target:
        temporary = Path(target.name)
        try:
            target.write(content)
            target.close()
            os.replace(temporary, path)
        finally:
            temporary.unlink(missing_ok=True)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('disk', type=Path, help='your Macintosh colour-game .hfs or .dsk image')
    parser.add_argument('--output', type=Path, default=ROOT / 'data/res_mac_native_full.pak')
    optional = parser.add_mutually_exclusive_group()
    optional.add_argument('--source-output', type=Path, help='also write local PNGs/origins (requires Pillow; empty directory)')
    optional.add_argument('--test-assets', type=Path, help='also regenerate local renderer fixtures in an empty directory (requires Pillow)')
    parser.add_argument('--force', action='store_true', help='replace an existing different output pack')
    args = parser.parse_args(argv)
    try:
        if args.disk.resolve() == args.output.resolve():
            raise ValueError('input disk and output pack must be different files')
        if args.disk.stat().st_size > 2 * 1024 ** 3:
            raise ValueError('disk image exceeds the supported 2 GiB limit')
        source_output = args.source_output or (args.test_assets / 'source-images' if args.test_assets else None)
        for path in (args.source_output, args.test_assets):
            if path and path.exists() and any(path.iterdir()):
                raise ValueError('source-output/test-assets must be empty; existing files will not be overwritten')
        image_class = None
        if source_output:
            from PIL import Image
            image_class = Image
        entries, records = graphics_from_disk(args.disk.read_bytes())
        content = pack_bytes(entries)
        write_pack(args.output, content, args.force)
        if source_output:
            source_output.mkdir(parents=True, exist_ok=True)
            manifest = io.StringIO(newline='')
            writer = csv.DictWriter(manifest, fieldnames=FIELDS)
            writer.writeheader()
            for record, rgba in records:
                path = source_output / record['file']
                path.parent.mkdir(parents=True, exist_ok=True)
                image_class.frombytes('RGBA', (record['width'], record['height']), rgba).save(path)
                writer.writerow(record)
            (source_output / 'manifest.csv').write_text(manifest.getvalue())
        if args.test_assets:
            scripts = ROOT / 'tools/mac_native_renderer'
            subprocess.run([sys.executable, str(scripts / 'extract_mac_reference.py'),
                            str(args.disk), '--output', str(args.test_assets)], check=True)
            subprocess.run([sys.executable, str(scripts / 'build_palace_wall_reference.py'),
                            '--source', str(source_output), '--reference', str(args.test_assets)], check=True)
            subprocess.run([sys.executable, str(scripts / 'build_combat_reference.py'),
                            '--source', str(source_output), '--reference', str(args.test_assets),
                            '--output', str(args.test_assets / 'combat-oracle')], check=True)
        print(f'Built 820 Macintosh images with native origins: {args.output}')
        print(f'SHA-256: {hashlib.sha256(content).hexdigest()}')
        print('Disk read-only; no downloads. Macintosh pack is ready to select in Visuals.')
        return 0
    except (OSError, ValueError, KeyError, struct.error, ImportError, subprocess.CalledProcessError) as error:
        print(f'Macintosh extraction failed: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
