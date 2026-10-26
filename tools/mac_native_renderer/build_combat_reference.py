#!/usr/bin/env python3
"""Recover original Mac sword offsets and independent body/weapon pixel fixtures.

CODE 3:$583E..$58C6 selects the A5 -$4CCC sword table (51 four-byte records),
converts the body anchor to screen coordinates, then applies facing-relative
screen offsets. CODE 3:$40E8 selects the six-byte actor frame records.
Only generated headers/fixtures are written; source disk/PNGs/PAKs are read-only.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import struct
from PIL import Image
from extract_mac_reference import resource_entries

ROOT = Path(__file__).resolve().parents[2]


def recover():
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
    code = resources['CODE', 3]['data']
    assert code[0x587C:0x5880] == bytes.fromhex('49edb334')
    assert code[0x5896:0x589A] == bytes.fromhex('4ead052a')
    assert code[0x58A6:0x58AA] == bytes.fromhex('4ebae97e')
    assert code[0x58AA:0x58AE] == bytes.fromhex('102c0002')
    jump = resources['CODE', 0]['data']
    jump_offset = struct.unpack_from('>I', jump, 12)[0]
    entry = 16 + 0x52A - jump_offset - 2
    assert struct.unpack_from('>H', jump, entry)[0] + 4 == 0x59A4
    assert struct.unpack_from('>H', jump, entry + 4)[0] == 4
    assert resources['CODE', 4]['data'][0x59AE:0x59B4] == bytes.fromhex('e78081fc0007')  # signed x*8/7
    table = [struct.unpack_from('>BbbB', expanded, below - 0x4CCC + i * 4) for i in range(51)]
    frames = {
        'kid': [struct.unpack_from('>BBbbBB', expanded, below - 0x556C + i * 6)[:5] for i in range(241)],
        'guard': [struct.unpack_from('>BBbbBB', expanded, below - 0x4FC6 + i * 6)[:5] for i in range(41)],
    }
    # The port's frame records must agree before using a Mac relative attachment.
    c_source = (ROOT / 'src/seg006.c').read_text()
    for name, key in [('frame_table_kid', 'kid'), ('frame_tbl_guard', 'guard')]:
        part = c_source.split('const frame_type ' + name + '[] = {')[1].split('};', 1)[0]
        parsed = []
        for line in part.splitlines():
            match = re.search(r'\{([^}]+)\}', line)
            if match:
                parsed.append(tuple(sum(int(v.strip(), 0) for v in q.split('|'))
                                    for q in match[1].split(',') if q.strip()))
        assert parsed == frames[key], f'{key} frame metadata diverged from original Mac'
    return fork, table, frames


def screen_x(x):
    return (1 if x >= 0 else -1) * (abs(x) * 8 // 7)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'tools/mac_reference/combat-oracle')
    parser.add_argument('--header', type=Path)
    args = parser.parse_args()
    fork, table, frames = recover()
    args.output.mkdir(parents=True, exist_ok=True)
    source = ROOT.parent / 'Mac-Color-Native-Graphics'
    rows = {(r['folder'], int(r['resource_id'])): r for r in csv.DictReader((source / 'manifest.csv').open())}
    cases = []
    absent = []
    families = [('KID', 400, 'kid', 0), ('GUARD', 750, 'guard', 149),
                ('FAT', 750, 'guard', 149), ('VIZIER', 750, 'guard', 149),
                ('SHADOW', 750, 'guard', 149), ('SKEL', 750, 'guard', 149)]
    for folder, group, key, first in families:
        for index, frame in enumerate(frames[key]):
            image, sword, dx, dy, flags = frame
            sword_index = sword & 63
            if not sword_index or table[sword_index][0] == 255 or image == 255:
                continue
            sword_id, sx, sy, pad = table[sword_index]
            assert pad == 0
            if (folder, group + image + 1) not in rows:
                absent.append({'folder': folder, 'frame': index + first, 'body_resource': group + image + 1})
                continue
            for right in (0, 1):
                direction, sign = (0, 1) if right else (255, -1)
                raw_x = (114 + sign * dx) * 2 - 116
                if (flags ^ direction) < 128:
                    raw_x += 1
                bx, by = screen_x(raw_x), 168 + dy
                wx, wy = bx + sign * sx, by + sy
                canvas = Image.new('RGBA', (640, 384), (27, 23, 51, 255))
                # Object sort draws the higher sword anchor before the body.
                for asset_folder, rid, x, y in [('PRINCE', 701 + sword_id, wx, wy), (folder, group + image + 1, bx, by)]:
                    record = rows[asset_folder, rid]
                    art = Image.open(source / record['file']).convert('RGBA')
                    ox, oy = int(record['origin_x']), int(record['origin_y'])
                    if right:
                        ox = art.width - 1 - ox
                        art = art.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
                    canvas.alpha_composite(art, (x * 2 - ox, y * 2 - oy))
                name = f'{folder.lower()}-{index + first:03d}-{"right" if right else "left"}.png'
                canvas.convert('RGB').save(args.output / name)
                cases.append({'folder': folder, 'frame': index + first, 'right': right,
                              'body_resource': group + image + 1, 'sword_frame': sword_index,
                              'sword_resource': 701 + sword_id, 'body_x280': raw_x,
                              'body_x320': bx, 'body_y': by, 'sword_x320': wx,
                              'sword_y': wy, 'image': name})
    # Packed independent oracle for the C harness; not the runtime header layout.
    with (args.output / 'cases.bin').open('wb') as out:
        for case in cases:
            out.write(struct.pack('<10h', [f[0] for f in families].index(case['folder']),
                                  case['frame'], case['right'], case['body_resource'],
                                  case['sword_resource'], case['body_x280'], case['body_x320'],
                                  case['body_y'], case['sword_x320'], case['sword_y']))
    report = {'source_sha256': hashlib.sha256(fork.read_bytes()).hexdigest(),
              'source': 'CODE3:$583E..$58C6, DATA/ZERO A5-$4CCC; frame records A5-$556C/-$4FC6; original SHPD origins',
              'x_conversion': 'CODE0 jump $52A -> CODE4:$59A4, signed (x*8)/7 before sword offset',
              'frame_records_equal_port': {k: len(v) for k, v in frames.items()},
              'sword_table': table, 'cases': cases,
              'absent_native_body_slots': absent,
              'limits': 'Reference composition at the retained actor anchor; not original-Mac motion/timing or an emulator video comparison.'}
    (args.output / 'reference.json').write_text(json.dumps(report, indent=2) + '\n')
    if args.header:
        lines = ['/* Generated by build_combat_reference.py from original Mac DATA/ZERO.',
                 ' * CODE 3:$583E..$58C6; screen offsets AFTER actor X conversion. */',
                 '#ifndef MAC_SWORD_TABLE_H', '#define MAC_SWORD_TABLE_H',
                 'static const sword_table_type mac_sword_tbl[51] = {']
        lines += [f'    {{{rid:3d}, {x:3d}, {y:3d}}}, /* {i} */' for i, (rid, x, y, _) in enumerate(table)]
        lines += ['};', '#endif', '']
        args.header.write_text('\n'.join(lines))
    print(f'PASS: recovered 51 native sword entries, all 282 actor records match; generated {len(cases)} source composites')


if __name__ == '__main__':
    main()
