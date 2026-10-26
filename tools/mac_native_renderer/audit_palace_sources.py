#!/usr/bin/env python3
"""Validate extracted native source assets and inventory the palace wall pieces.

This is an asset/readiness audit, not an authentic palace compositor test.
It never modifies source PNGs, installed PAKs or the playable game.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import struct

from PIL import Image, ImageDraw
from build_mac_native_pak import image_to_payload, read_pack_entries

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path, help='extracted Mac-Color-Native-Graphics folder')
    parser.add_argument('--output', type=Path, required=True, help='audit report/capture directory')
    args = parser.parse_args()
    source = args.source.resolve()
    manifest = source / 'manifest.csv'
    with manifest.open(encoding='utf-8-sig', newline='') as manifest_file:
        rows = list(csv.DictReader(manifest_file))
    pack_path = ROOT / 'data/res_mac_native_full.pak'
    pack = read_pack_entries(pack_path)
    records = []
    seen = set()
    modes = set()
    for row in rows:
        key = (row['folder'].lower(), int(row['resource_id']))
        if key in seen:
            raise ValueError(f'duplicate manifest key: {key}')
        seen.add(key)
        path = (source / row['file']).resolve()
        if not path.is_relative_to(source):
            raise ValueError(f'image path escapes source folder: {path}')
        with Image.open(path) as image:
            modes.add(image.mode)
            w, h = image.size
        if (w, h) != (int(row['width']), int(row['height'])):
            raise ValueError(f'manifest dimensions differ: {path}')
        if image_to_payload(path.read_bytes()) != pack[(*key, 'png')]:
            raise ValueError(f'source pixels differ from installed PAK: {path}')
        origin = (int(row['origin_x']), int(row['origin_y']))
        if struct.pack('<hh', *origin) != pack[(*key, 'org')][-1]:
            raise ValueError(f'SHPD origin differs from installed PAK: {path}')
        if row['folder'] == 'VPALACE' and (int(row['resource_id']) <= 206 or int(row['resource_id']) >= 360):
            records.append({
                'resource': key[1], 'mac_group': int(row['mac_group']),
                'width': w, 'height': h, 'origin': origin, 'file': row['file'],
            })
    installed_keys = {(f, rid) for f, rid, ext in pack if ext == 'png'}
    if installed_keys != seen:
        raise ValueError('source manifest does not cover exactly the installed full native PAK')
    args.output.mkdir(parents=True, exist_ok=True)
    records.sort(key=lambda record: record['resource'])
    sheet = Image.new('RGB', (1000, ((len(records) + 4) // 5) * 180), '#493C50')
    draw = ImageDraw.Draw(sheet)
    for i, record in enumerate(records):
        x, y = (i % 5) * 200, (i // 5) * 180
        draw.text((x + 5, y + 5), f"Resource {record['resource']} - {record['width']}x{record['height']}", fill='white')
        draw.text((x + 5, y + 20), f"SHPD origin {tuple(record['origin'])}", fill='white')
        with Image.open(source / record['file']) as image:
            rgba = image.convert('RGBA')
            sheet.paste(rgba, (x + 8, y + 45), rgba)
    sheet.save(args.output / 'palace-source-pieces.png')
    report = {
        'source_directory': str(source),
        'source_manifest_sha256': hashlib.sha256(manifest.read_bytes()).hexdigest(),
        'installed_full_pack_sha256': hashlib.sha256(pack_path.read_bytes()).hexdigest(),
        'validated_images_and_origins': len(rows),
        'source_image_modes': sorted(modes),
        'palace_pieces': records,
        'v12_3_wall_mapping_snapshot': {
            'back_arch_sprite': 201,
            'bottom_wall_sprite': 363,
            'fore_by_neighbor_modifier_0_to_3': [368, 370, 366, 364],
            'note': 'These are current renderer selections, not a proposed correct Mac wall mapping.',
        },
        'source_export_limitations': {
            'original_global_clut_indices': 'Not retained by this RGBA PNG export.',
            'original_application_resources': 'Not supplied by the native graphics export.',
            'source_disk_named_by_readme': 'PoP1_spf.dsk',
        },
        'recovered_reference_directory': str(ROOT / 'tools/mac_reference') if (ROOT / 'tools/mac_reference/clut-2000.json').exists() else None,
    }
    (args.output / 'palace-source-audit.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f"PASS: {len(rows)} native source images and SHPD origins match the installed full PAK")
    print('Palace source inventory saved; recovered disk/reference status is recorded separately')


if __name__ == '__main__':
    main()
