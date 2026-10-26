#!/usr/bin/env python3
"""Independent pixel fixtures from the recovered Mac wall routine constants.

Reference locations: CODE 2:$223E RNG; CODE 3:$0B72 room colors;
CODE 4:$5CB6 body fills/details and $5EE6 bottom fills/details.
This uses the supplied disk's extracted CLUT and original PNGs, not C output.
"""
import argparse
import csv
import json
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]


class MacRandom:
    def __init__(self, seed):
        self.seed = seed

    def next(self, maximum):
        old = self.seed
        low = ((old & 0xFFFF) ^ 0x569A) + (old >> 16)
        self.seed = ((low & 0xFFFF) * 0x6A59) & 0xFFFFFFFF
        sample = (self.seed + old) & 0xFFFF
        return (sample * (maximum + 1)) >> 16


def room_colors(room):
    rng = MacRandom(room)
    rng.next(1)
    rows = []
    for row in range(3):
        courses = []
        for course in range(4):
            colors = []
            while len(colors) < 11:
                color = 76 + (4 if course % 2 else 0) + rng.next(3)
                if not colors or color != colors[-1]:
                    colors.append(color)
            courses.append(colors)
        rows.append(courses)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT.parent / 'Mac-Color-Native-Graphics')
    parser.add_argument('--reference', type=Path, default=ROOT / 'tools/mac_reference')
    parser.add_argument('--header', type=Path, help='also regenerate the C eight-color palette header')
    args = parser.parse_args()
    palette_info = json.loads((args.reference / 'clut-2000.json').read_text())
    palette = {entry['value']: tuple(entry['rgb8']) for entry in palette_info['color_specs']}
    with (args.source / 'manifest.csv').open(newline='') as f:
        pieces = {int(row['resource_id']): row for row in csv.DictReader(f) if row['folder'] == 'VPALACE'}
    out = args.reference / 'wall-oracle'
    out.mkdir(exist_ok=True)
    all_colors = {}
    for room in (1, 4, 12, 24):
        colors = room_colors(room)
        all_colors[room] = colors
        canvas = Image.new('RGB', (640, 384), 'black')
        draw = ImageDraw.Draw(canvas)
        for row in range(3):
            main_y, bottom_y = row * 63 + 59, row * 63 + 62
            for col in range(10):
                fills = [
                    (0, main_y - 40, 32, 20, colors[row][0][col]),
                    (0, main_y - 19, 16, 21, colors[row][1][col]),
                    (16, main_y - 19, 16, 21, colors[row][1][col + 1]),
                    (0, main_y, 8, 19, colors[row][2][col]),
                    (8, main_y, 24, 19, colors[row][2][col + 1]),
                    (0, bottom_y, 32, 3, colors[row][3][col]),
                ]
                for dx, bottom, width, height, color in fills:
                    x, y = col * 64 + dx * 2, (bottom + 1 - height) * 2
                    draw.rectangle((x, y, x + width * 2 - 1, y + height * 2 - 1), fill=palette[color])
                rng = MacRandom(room + row * 10 + col)
                details = [(15 + rng.next(2), 0, bottom_y)]
                details += [(base + rng.next(2), 24 if i == 0 else 0, main_y + dy)
                            for i, (base, dy) in enumerate(zip((3, 6, 9, 12), (-53, -34, -13, 0)))]
                for image_id, dx, anchor_y in details:
                    record = pieces[360 + image_id]
                    with Image.open(args.source / record['file']) as image:
                        rgba = image.convert('RGBA')
                        xy = (col * 64 + dx * 2 - int(record['origin_x']), anchor_y * 2 - int(record['origin_y']))
                        canvas.paste(rgba, xy, rgba)
        canvas.save(out / f'room-{room}.png')
        (out / f'room-{room}.rgb').write_bytes(canvas.tobytes())
    (out / 'room-colors.json').write_text(json.dumps(all_colors, indent=2) + '\n')
    if args.header:
        lines = ['/* Generated from original Mac application clut 2000, indices 76..83.',
                 f" * Resource SHA-256: {palette_info['source_resource_sha256']} */",
                 '#ifndef MAC_PALACE_PALETTE_H', '#define MAC_PALACE_PALETTE_H',
                 'static const Uint32 mac_palace_rgb[8] = {']
        for index in range(76, 84):
            r, g, b = palette[index]
            lines.append(f'    0x{r:02X}{g:02X}{b:02X}, /* CLUT {index} */')
        lines.extend(['};', '#endif', ''])
        args.header.write_text('\n'.join(lines))
    print('Generated four independent 640x384 native wall fixtures and indexed room-color tables')


if __name__ == '__main__':
    main()
