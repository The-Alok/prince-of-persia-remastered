#!/usr/bin/env python3
"""Linux climb/fall renderer regression: run after `make -C src` from the project root.

Uses the real game object files, native packs, SDL dummy video/audio, and a
separate temporary executable. Optional --output saves climb frames 135..149
and ledge-fall frames 102..106.
"""
import argparse
import os
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def check_asset_registration():
    pack = (ROOT / 'data/res_mac_native_full.pak').read_bytes()
    header = struct.Struct('<8sII')
    entry = struct.Struct('<16s4sHHHBBII')
    magic, version, count = header.unpack_from(pack)
    assert magic == b'POPSPAK1' and version == 1
    images, origins = {}, {}
    for i in range(count):
        folder, ext, rid, w, h, colors, flags, off, size = entry.unpack_from(pack, header.size + i * entry.size)
        folder, ext = folder.rstrip(b'\0').decode(), ext.rstrip(b'\0').decode()
        if folder not in ('VDUNGEON', 'VPALACE') or rid not in (*range(220, 228), 241):
            continue
        data = pack[off:off + size]
        if ext == 'org':
            origins[folder, rid] = struct.unpack('<hh', data)
        elif ext == 'png':
            palette = [data[j:j + 3] for j in range(0, colors * 3, 3)]
            images[folder, rid] = (w, h, flags, palette, data[colors * 3:])
    for folder in ('VDUNGEON', 'VPALACE'):
        bw, bh, _, bpal, bpix = images[folder, 241]
        bx, by = origins[folder, 241]
        for rid in range(220, 228):
            w, h, flags, palette, pix = images[folder, rid]
            mx, my = origins[folder, rid]
            for y in range(h):
                for x in range(w):
                    color = pix[y * w + x]
                    if flags & 1 and color == 0:
                        continue
                    xx, yy = x + bx - mx, y + by - my
                    assert 0 <= xx < bw and 0 <= yy < bh, (folder, rid, x, y)
                    assert palette[color] == bpal[bpix[yy * bw + xx]], (folder, rid, x, y)
    print('PASS: all 16 dungeon/palace masks exactly register with their floor-top pixels', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, help='directory for rendered comparison frames')
    parser.add_argument('--floors', action='store_true', help='also audit interactive-floor transitions')
    parser.add_argument('--architecture', action='store_true', help='also audit gates, exits, chompers and mirrors')
    parser.add_argument('--palace', action='store_true', help='also check native palace walls against recovered Mac fixtures')
    parser.add_argument('--props', action='store_true', help='also audit potion types, pickups, torches and static props')
    parser.add_argument('--cutscenes', action='store_true', help='also audit native Princess room, hourglass, actors and story cards')
    parser.add_argument('--ui', action='store_true', help='also audit health, bottom text, menus, dialogs and RGB fades')
    parser.add_argument('--combat', action='store_true', help='compare native pickup/combat swords with recovered Mac composition')
    parser.add_argument('--world', action='store_true', help='audit and capture the real room maps and entries across levels 1..14')
    parser.add_argument('--valgrind-log', type=Path, help='run the test executable under Valgrind and save its memory report')
    parser.add_argument('--timeout', type=int, default=180, help='test process timeout in seconds (default: 180)')
    parser.add_argument('--lifecycle', action='store_true', help='run a short resource/pack/fade lifetime audit instead of pixel suites')
    args = parser.parse_args()
    check_asset_registration()
    makefile = (ROOT / 'src/Makefile').read_text()
    obj_line = next(line for line in makefile.splitlines() if line.startswith('OBJ ='))
    objects = [ROOT / 'src' / name for name in obj_line.split('=', 1)[1].split()]
    if not all(path.exists() for path in objects):
        raise SystemExit('Build the game first: make -C src')
    cflags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', 'sdl2', 'SDL2_image'], text=True))
    libs = shlex.split(subprocess.check_output(['pkg-config', '--libs', 'sdl2', 'SDL2_image'], text=True))
    with tempfile.TemporaryDirectory(prefix='sdlpop-climb-test-') as tmp:
        binary = Path(tmp) / 'climb-test'
        subprocess.run(['/usr/bin/gcc', '-std=c99', '-Wall', '-D_GNU_SOURCE=1', f'-I{ROOT / "src"}',
                        *cflags, str(Path(__file__).with_suffix('.c')),
                        str(Path(__file__).with_name('test_interactive_floors.c')), *map(str, objects),
                        '-Wl,--wrap=draw_tables', '-Wl,--wrap=draw_native_image_physical_blit',
                        '-Wl,--wrap=show_dialog',
                        *libs, '-lm', '-o', str(binary)], check=True)
        env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', SDL_RENDER_DRIVER='software')
        if args.lifecycle:
            env['SDLPOP_LIFECYCLE_AUDIT'] = '1'
        if args.floors:
            env['SDLPOP_FLOOR_AUDIT'] = '1'
        if args.architecture:
            env['SDLPOP_ARCHITECTURE_AUDIT'] = '1'
        if args.palace:
            env['SDLPOP_PALACE_AUDIT'] = '1'
        if args.props:
            env['SDLPOP_PROPS_AUDIT'] = '1'
        if args.cutscenes:
            env['SDLPOP_CUTSCENE_AUDIT'] = '1'
        if args.ui:
            env['SDLPOP_UI_AUDIT'] = '1'
        if args.combat:
            env['SDLPOP_COMBAT_AUDIT'] = '1'
        if args.world:
            env['SDLPOP_WORLD_AUDIT'] = '1'
        if args.output:
            args.output.mkdir(parents=True, exist_ok=True)
            env['SDLPOP_CLIMB_TEST_OUTPUT'] = str(args.output.resolve())
        command = [str(binary), 'megahit', '1', 'mute']
        if args.valgrind_log:
            args.valgrind_log.parent.mkdir(parents=True, exist_ok=True)
            command = ['valgrind', '--tool=memcheck', '--leak-check=full',
                       '--show-leak-kinds=definite', '--errors-for-leak-kinds=definite',
                       '--error-exitcode=99', f'--log-file={args.valgrind_log.resolve()}', *command]
        subprocess.run(command, cwd=ROOT, env=env, check=True, timeout=args.timeout)


if __name__ == '__main__':
    main()
