#!/usr/bin/env python3
"""Record original cutscene groups and recover flame anchors from Mac DATA/ZERO.

Reads the supplied disk extraction and native PNG manifest without changing them.
DATA expansion implements original CODE 1:$005A..$006A: a zero word is followed
by ZERO's byte count (DBRA enters at its decrement, so no extra byte is added).
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import struct
from extract_mac_reference import resource_entries

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    fork = ROOT / 'tools/mac_reference/hfs-forks/Prince of Persia__Prince of Persia.rsrc'
    resources = resource_entries(fork.read_bytes())
    below = struct.unpack_from('>I', resources['CODE', 0]['data'], 4)[0]
    data, zero = (resources[k, 0]['data'] for k in ('DATA', 'ZERO'))
    expanded = bytearray()
    zi = 0
    for at in range(0, len(data), 2):
        word = data[at:at + 2]
        expanded.extend(word)
        if word == b'\0\0':
            count = struct.unpack_from('>H', zero, zi)[0]
            zi += 2
            expanded.extend(bytes(count))
    assert len(expanded) == below and zi == len(zero)
    values = struct.unpack_from('>6h', expanded, below - 0x56E8)
    assert values == (9, 36, 6, 2, 1, 6)
    anchors = [[(values[i] * 8 + values[i + 2]) * 2, 121 * 2] for i in (0, 1)]
    code = resources['CODE', 3]['data']
    assert code[0x2522:0x2526] == bytes.fromhex('3f3c0079')  # y=121
    assert code[0x2530:0x2532] == bytes.fromhex('d047')      # frame 12 + phase
    assert code[0xD3A:0xD3E] == bytes.fromhex('3f3c0017')   # intro fill CLUT23
    assert code[0xDAA:0xDAE] == bytes.fromhex('3f3c0017')
    assert code[0x263C:0x2640] == bytes.fromhex('3ebc0010') # ending fill CLUT16
    palette_info = json.loads((ROOT / 'tools/mac_reference/clut-2000.json').read_text())
    palette = {e['value']: e['rgb8'] for e in palette_info['color_specs']}
    assert palette[23] == [16, 0, 96] and palette[16] == [128, 0, 0]
    rows = list(csv.DictReader((ROOT.parent / 'Mac-Color-Native-Graphics/manifest.csv').open()))
    groups = {group: [r for r in rows if int(r['mac_group']) == group] for group in (800, 850, 900, 950, 960, 980, 40, 50)}
    result = {
        'application_resource_sha256': hashlib.sha256(fork.read_bytes()).hexdigest(),
        'expanded_a5_below_size': below,
        'flame_data_a5_offset': '-0x56E8', 'flame_data_words': values,
        'flame_logical_anchors': [[x // 2, y // 2] for x, y in anchors],
        'flame_physical_anchors': anchors,
        'flame_source': 'CODE 3:$2512..$2576, DATA/ZERO, SHPD 960 indices 13..15',
        'hourglass_source': 'CODE 3:$2578..$25F6; anchor (152,168), sand (160,164)',
        'pillar_source': 'CODE 3:$1D30..$1D3E; SHPD 960 index 2 at (240,167)',
        'story_source': 'CODE 3:$0D42/$0DB2/$2644/$0E3E; SHPD 40 indices 2..5',
        'story_logical_anchors': [[24, 25], [24, 25], [24, 25], [24, 26]],
        'story_physical_top_left': [[71, 51], [71, 51], [71, 51], [66, 50]],
        'story_background_source': 'CODE 3:$0D3A/$0DAA/$263C -> $0ED6; clut 2000',
        'story_background_rgb': {'intro_clut23': palette[23], 'ending_clut16': palette[16]},
        'group_resources': {str(k): [int(r['resource_id']) for r in v] for k, v in groups.items()},
        'native_bed_group_absent': not groups[980],
        'coordinates': '320x200 logical prop anchors doubled into 640x400 physical pixels, then subtract SHPD origin; actor X remains the existing 280-wide conversion',
        'limits': 'Composition evidence, not complete original-Mac motion/timing equivalence. SDLPoP torch scheduling and star positions retained.',
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(f'PASS: recovered native flame anchors {anchors}; native background/props are separate groups; no native bed overlay')


if __name__ == '__main__':
    main()
