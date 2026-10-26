#!/usr/bin/env python3
"""Check presentation filters and real SDL F7/F8 input without player saves."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    captures = ROOT/'screenshots/v1.0.1-controls/display'
    captures.mkdir(parents=True, exist_ok=True)
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'sdl2', 'SDL2_image'], text=True))
    line = next(line for line in (ROOT/'src/Makefile').read_text().splitlines() if line.startswith('OBJ ='))
    objects = [str(ROOT/'src'/name) for name in line.split('=', 1)[1].split()]
    with tempfile.TemporaryDirectory(prefix='pop-display-test-') as work:
        binary = Path(work)/'display-test'
        subprocess.run(['/usr/bin/gcc', '-D_GNU_SOURCE=1', '-std=c99', '-Wall', '-I'+str(ROOT/'src'),
                        str(Path(__file__).with_suffix('.c')), *objects,
                        '-Wl,--wrap=draw_tables', '-Wl,--wrap=SDL_RenderPresent',
                        '-Wl,--wrap=load_ingame_settings', '-o', str(binary), *flags, '-lm'], check=True)
        env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', SDL_RENDER_DRIVER='software',
                   DISPLAY_CAPTURE_DIR=str(captures), HOME=work, SDLPOP_SAVE_PATH=work)
        command = [str(binary), 'megahit', '1', 'mute', 'seed=1']
        subprocess.run(command, cwd=ROOT, env=env, check=True, timeout=180)
    print(f'Actual presentation/menu captures: {captures}')


if __name__ == '__main__':
    main()
