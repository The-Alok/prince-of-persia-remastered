#!/usr/bin/env python3
"""Compare every supplied PNG/origin with a fresh decode of the original disk forks."""
import csv
import importlib.util
import json
from pathlib import Path
import struct
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def main():
    spec = importlib.util.spec_from_file_location('mac_decoder', ROOT / 'tools/mac_color_port/decode_mac_pop_reference.py')
    decoder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(decoder)
    reference = ROOT / 'tools/mac_reference'
    forks = reference / 'hfs-forks'
    resources = decoder.parse_rsrc(forks / 'Prince of Persia__Persia(COLOR).rsrc')
    app = decoder.parse_rsrc(forks / 'Prince of Persia__Prince of Persia.rsrc')
    palette, _, _ = decoder.parse_clut(app['clut', 2000]['data'])
    source = ROOT.parent / 'Mac-Color-Native-Graphics'
    with (source / 'manifest.csv').open(newline='') as f:
        rows = list(csv.DictReader(f))
    data = (forks / 'Prince of Persia__Persia(COLOR).data').read_bytes()
    groups = {}
    pixels_checked = 0
    for row in rows:
        group = int(row['mac_group'])
        if group not in groups:
            off, compressed, size = struct.unpack('>III', resources['SHPD', group]['data'])
            raw = data[off:off + (compressed or size)]
            decoded = decoder.presage_lzss(raw, size) if compressed else raw
            table = resources['SHPT', group]['data']
            offsets = struct.unpack('>' + 'I' * (len(table) // 4), table)
            groups[group] = decoded, offsets
        decoded, offsets = groups[group]
        slot = int(row['resource_id']) - group % 1000
        at = offsets[slot]
        w, h, ox, oy = struct.unpack_from('>4h', decoded, at)
        assert (w, h, ox, oy) == tuple(int(row[key]) for key in ('width', 'height', 'origin_x', 'origin_y')), row['file']
        pixels, alpha, _ = decoder.decode_presage_v1(decoded[at + 8:], w, h)
        with Image.open(source / row['file']) as image:
            png = image.convert('RGBA').tobytes()
        for i, (color, opaque) in enumerate(zip(pixels, alpha)):
            assert (png[i * 4 + 3] != 0) == bool(opaque), (row['file'], i, 'alpha')
            if opaque:
                assert tuple(png[i * 4:i * 4 + 3]) == tuple(palette[color]), (row['file'], i, 'RGB')
        pixels_checked += w * h
    report = {'images_and_origins_verified': len(rows), 'pixels_verified': pixels_checked,
              'native_groups': len(groups), 'clut_entries': len(palette)}
    (reference / 'source-verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: original disk independently reproduces {len(rows)} PNGs/origins, {pixels_checked} pixels')


if __name__ == '__main__':
    main()
