#!/usr/bin/env python3
"""Package Windows/Linux release downloads; source is provided at the matching Git tag."""
import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import platform
import re
import subprocess
import tarfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
VERSION = re.search(r'^#define REMASTERED_VERSION "([^"]+)"', (ROOT/'src/config.h').read_text(), re.M)[1]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def command(*args):
    return subprocess.check_output(args, cwd=ROOT, text=True, stderr=subprocess.STDOUT)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', choices=['linux', 'windows'], default='linux')
    parser.add_argument('--windows-runtime', type=Path, default=ROOT/'.build/windows/runtime')
    parser.add_argument('--output', type=Path, default=ROOT/'dist')
    parser.add_argument('--source-ref', help='exact source commit/tag; defaults to HEAD in a Git checkout')
    args = parser.parse_args()
    if not re.fullmatch(r'\d+\.\d+\.\d+', VERSION):
        raise SystemExit('Expected a public semantic release version.')
    source_ref = args.source_ref
    if not source_ref:
        if not (ROOT/'.git').exists():
            raise SystemExit('Pass --source-ref with the matching source commit/tag when packaging a source export.')
        source_ref = command('git', 'rev-parse', 'HEAD').strip()
    if 'graphics_pack = mac_native_full' not in (ROOT/'SDLPoP.ini').read_text():
        raise SystemExit('Native FULL must be the startup default.')
    contents = {}
    executable_names = set()

    def add(path, relative=None):
        if path.is_file():
            contents[relative or path.relative_to(ROOT).as_posix()] = path.read_bytes()

    # Explicit public-document list: ignored local development notes must not
    # enter downloads merely because they still exist in the working tree.
    for name in ('SDLPoP.ini', 'COPYING', 'README.md'):
        add(ROOT/name)
    for name in ('gameplay-level1-mac.png', 'gameplay-mac.png',
                 'graphics-packs.png', 'graphics-packs-level1.png', 'welcome.png'):
        add(ROOT/'doc/images'/name)
    # The launcher contains the importer. Python tools stay in the source tree;
    # neither an interpreter nor the player's generated Mac pack is bundled.
    # A checked inventory prevents local Mac imports/dumps (or user mods) from
    # leaking into an archive, even if they sit beside the bundled resources.
    for line in (ROOT/'tools/release/bundled_data.sha256').read_text().splitlines():
        if not line or line.startswith('#'):
            continue
        expected, relative = line.split(maxsplit=1)
        path = ROOT/relative
        if not path.resolve().is_relative_to((ROOT/'data').resolve()):
            raise SystemExit(f'Invalid bundled-data path: {relative}')
        if path.name.startswith(('res_mac_native', 'res_mac_color')) or '_macgroup' in path.name:
            raise SystemExit(f'Macintosh import cannot be bundled: {relative}')
        if digest(path.read_bytes()) != expected:
            raise SystemExit(f'Bundled DOS/SNES data changed: {relative}')
        add(path)
    for name in ('mods/mods.txt', 'replays/replays.txt'):
        add(ROOT/name)
    metadata = {'version': VERSION, 'platform': f'{args.platform}-x86_64',
                'source': f'https://github.com/The-Alok/prince-of-persia-remastered/tree/{source_ref}',
                'upstream': 'https://github.com/NagyD/SDLPoP',
                'graphics_reference': 'https://github.com/chanunnaki/SDLPoP-PSP',
                'macintosh_graphics': 'not bundled; launcher imports your own Macintosh colour-game disk',
                'live_full_game_and_steam_deck_tests': 'pending; build/startup checks are separate from native playthrough testing'}
    if args.platform == 'linux':
        if platform.system() != 'Linux' or platform.machine() != 'x86_64':
            raise SystemExit('Linux x64 packaging requires a Linux x64 build host.')
        if f'v{VERSION}' not in command(str(ROOT/'prince'), '--version'):
            raise SystemExit('Rebuild prince with the current version before packaging.')
        add(ROOT/'prince')
        if not (ROOT/'runtime/prince-engine').is_file():
            raise SystemExit('Build both the launcher and runtime/prince-engine with make -C src.')
        add(ROOT/'runtime/prince-engine')
        add(ROOT/'tools/release/launch.sh', 'launch.sh')
        executable_names.update(('prince', 'runtime/prince-engine', 'launch.sh'))
        dependencies = command('ldd', 'prince') + command('ldd', 'runtime/prince-engine')
        if 'not found' in dependencies:
            raise SystemExit('A runtime library is missing.')
        versions = sorted(set(re.findall(r'GLIBC_([0-9.]+)', command('readelf', '--version-info', 'prince') + command('readelf', '--version-info', 'runtime/prince-engine'))),
                          key=lambda value: tuple(map(int, value.split('.'))))
        metadata.update(build_os=platform.freedesktop_os_release().get('PRETTY_NAME'),
                        required_glibc_symbol_versions=versions,
                        dynamic_dependencies=dependencies.splitlines(), bundled_sdl_libraries=False)
    else:
        runtime = args.windows_runtime.resolve()
        binary = runtime/'prince.exe'
        if not binary.exists() or binary.read_bytes()[:2] != b'MZ':
            raise SystemExit('Build the Windows runtime first with build_windows.py.')
        report = runtime/'BUILD_INFO_WINDOWS.json'
        if not (runtime/'runtime/prince-engine.exe').is_file() or not report.is_file() or json.loads(report.read_text()).get('version') != VERSION:
            raise SystemExit('Rebuild the Windows launcher and engine for this version with build_windows.py.')
        for path in runtime.rglob('*'):
            if path.is_file():
                relative = path.relative_to(runtime)
                if path.name in ('prince.exe', 'BUILD_INFO_WINDOWS.json') or relative.as_posix() == 'runtime/prince-engine.exe' or path.suffix.lower() == '.dll' or relative.parts[0] == 'licenses':
                    add(path, relative.as_posix())
        contents['launch.bat'] = b'@echo off\r\ncd /d "%~dp0"\r\nstart "" "%~dp0prince.exe" %*\r\n'
        metadata.update(bundled_runtime='See BUILD_INFO_WINDOWS.json and licenses/', native_windows_playtest='pending')
    contents['BUILD_INFO.json'] = (json.dumps(metadata, indent=2)+'\n').encode()
    contents['SHA256SUMS'] = ''.join(f'{digest(contents[name])}  {name}\n' for name in sorted(contents)).encode()
    base = f'prince-of-persia-remastered-v{VERSION}-{args.platform}-x86_64'
    args.output.mkdir(parents=True, exist_ok=True)
    archive = args.output/(base+('.zip' if args.platform == 'windows' else '.tar.gz'))
    with archive.open('xb') as raw:
        if args.platform == 'windows':
            with zipfile.ZipFile(raw, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as target:
                for name in sorted(contents):
                    entry = zipfile.ZipInfo(f'{base}/{name}', (2026,1,1,0,0,0))
                    entry.compress_type = zipfile.ZIP_DEFLATED
                    entry.external_attr = 0o644 << 16
                    target.writestr(entry, contents[name])
        else:
            with gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=0) as compressed:
                with tarfile.open(fileobj=compressed, mode='w|') as target:
                    for name in sorted(contents):
                        entry = tarfile.TarInfo(f'{base}/{name}')
                        entry.size = len(contents[name])
                        entry.mode = 0o755 if name in executable_names else 0o644
                        target.addfile(entry, io.BytesIO(contents[name]))
    archive.with_name(archive.name+'.sha256').write_text(f'{digest(archive.read_bytes())}  {archive.name}\n')
    print(f'{archive}\n{len(contents)} files, {archive.stat().st_size:,} bytes')


if __name__ == '__main__':
    main()
