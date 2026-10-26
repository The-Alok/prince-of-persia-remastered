#!/usr/bin/env python3
"""Build Windows x64 with MinGW, keeping its objects separate from Linux.

Use --toolchain .build/toolchain for extracted Fedora MinGW RPMs, or run inside
an MSYS2 MINGW64 shell with gcc, binutils, SDL2 and SDL2_image installed.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SYSTEM_DLLS = set('advapi32 bcrypt cfgmgr32 comctl32 comdlg32 crypt32 d3d11 d3d12 d3dcompiler_47 dbghelp dinput8 dnsapi dwmapi dxgi gdi32 hid imm32 iphlpapi kernel32 msvcrt ntdll ole32 oleaut32 opengl32 powrprof propsys psapi rpcrt4 secur32 setupapi shcore shell32 shlwapi user32 userenv ucrtbase usp10 version winhttp wininet winmm winspool ws2_32 wtsapi32'.split())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--toolchain', type=Path)
    parser.add_argument('--sdl2', type=Path, help='official SDL2 MinGW x86_64 directory (include/, lib/, bin/)')
    parser.add_argument('--output', type=Path, default=ROOT / '.build/windows')
    args = parser.parse_args()
    output = args.output.resolve()
    objects = output / 'objects'
    runtime = output / 'runtime'
    objects.mkdir(parents=True, exist_ok=True)
    runtime.mkdir(parents=True, exist_ok=True)
    if args.toolchain:
        prefix = args.toolchain.resolve() / 'usr'
        compiler = str(prefix / 'bin/x86_64-w64-mingw32-gcc')
        windres = str(prefix / 'bin/x86_64-w64-mingw32-windres')
        objdump = str(prefix / 'bin/x86_64-w64-mingw32-objdump')
        sysroot = prefix / 'x86_64-w64-mingw32/sys-root'
        mingw = sysroot / 'mingw'
        cflags = [f'--sysroot={sysroot}', f'-I{mingw / "include/SDL2"}']
        libraries = [f'--sysroot={sysroot}', f'-L{mingw / "lib"}', '-lmingw32', '-lSDL2main', '-lSDL2_image', '-lSDL2', '-lm']
        dll_dirs = [mingw / 'bin']
        license_dirs = list((prefix / 'share/licenses').glob('mingw*'))
    else:
        compiler, windres, objdump = 'gcc', 'windres', 'objdump'
        cflags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', 'sdl2', 'SDL2_image'], text=True))
        libraries = shlex.split(subprocess.check_output(['pkg-config', '--libs', 'sdl2', 'SDL2_image'], text=True)) + ['-lm']
        mingw = Path(shutil.which('gcc')).resolve().parent.parent
        dll_dirs = [mingw / 'bin']
        license_dirs = list((mingw / 'share/licenses').glob('*'))
    if args.sdl2:
        sdl2 = args.sdl2.resolve()
        cflags = [f'-I{sdl2 / "include/SDL2"}', *cflags]
        libraries = [f'-L{sdl2 / "lib"}', *libraries]
        dll_dirs.append(sdl2 / 'bin')
    flags = ['-std=c99', '-O2', '-Wall', '-D_GNU_SOURCE=1', f'-I{ROOT / "src"}', *cflags]
    names = next(line for line in (ROOT / 'src/Makefile').read_text().splitlines() if line.startswith('OBJ =')).split('=', 1)[1].split()

    def compile_one(name):
        subprocess.run([compiler, *flags, '-c', str(ROOT / 'src' / name.replace('.o', '.c')), '-o', str(objects / name)], check=True)

    with ThreadPoolExecutor(max_workers=2) as pool:
        list(pool.map(compile_one, names))
    subprocess.run([windres, '-I', str(ROOT / 'src'), str(ROOT / 'src/icon.rc'), str(objects / 'icon.o')], check=True, cwd=ROOT / 'src')
    binary = runtime / 'prince.exe'
    subprocess.run([compiler, '-mwindows', '-static-libgcc', *[str(objects / name) for name in names],
                    str(objects / 'icon.o'), '-o', str(binary), *libraries], check=True)
    available = {path.name.lower(): path for folder in dll_dirs for path in folder.glob('*.dll')}
    pending, copied, system = [binary], set(), set()
    # These libraries can be loaded by name rather than through PE imports.
    # SDL_image's PNG codec is needed by fallback/font assets. Fedora's SDL2
    # compatibility implementation loads SDL3 dynamically.
    dynamic = ['libpng16-16.dll']
    if not args.sdl2 and (mingw / 'lib/pkgconfig/sdl2-compat.pc').exists():
        dynamic.append('sdl3.dll')
    for name in dynamic:
        if name not in available:
            raise SystemExit(f'Missing dynamically loaded runtime: {name}')
        source = available[name]
        shutil.copy2(source, runtime / source.name)
        copied.add(name)
        pending.append(source)
    while pending:
        path = pending.pop()
        imports = re.findall(r'DLL Name:\s*(\S+)', subprocess.check_output([objdump, '-p', str(path)], text=True))
        for name in imports:
            key = name.lower()
            if key in copied:
                continue
            if key in available:
                source = available[key]
                shutil.copy2(source, runtime / source.name)
                copied.add(key)
                pending.append(source)
            elif key.removesuffix('.dll') in SYSTEM_DLLS or key.startswith(('api-ms-win-', 'ext-ms-win-')):
                system.add(name)
            else:
                raise SystemExit(f'Missing Windows runtime dependency: {name} (imported by {path.name})')
    for folder in license_dirs:
        if folder.is_dir():
            shutil.copytree(folder, runtime / 'licenses' / folder.name, dirs_exist_ok=True)
    if args.sdl2:
        for name in ('COPYING.txt', 'LICENSE.txt'):
            license_file = sdl2.parent / name
            if license_file.exists():
                target = runtime / 'licenses/SDL2'
                target.mkdir(parents=True, exist_ok=True)
                shutil.copy2(license_file, target / name)
    report = {'platform': 'windows-x86_64', 'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
              'bundled_dlls': sorted(copied), 'system_dlls': sorted(system),
              'sdl2_runtime': sdl2.parent.name if args.sdl2 else 'system MinGW SDL2 distribution',
              'native_windows_playtest': 'pending', 'executable': 'prince.exe'}
    (runtime / 'BUILD_INFO_WINDOWS.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
