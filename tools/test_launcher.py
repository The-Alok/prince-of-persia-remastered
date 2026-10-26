#!/usr/bin/env python3
"""Exercise the real SDL launcher with controlled input, no game assets in tests.

Optional --disk exercises extraction from a privately supplied Macintosh image.
Captures are actual launcher framebuffer PNGs, not mockups. Requires Linux,
a C compiler, pkg-config, SDL2 and SDL2_image development libraries.
"""
import argparse
import hashlib
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HARNESS = r'''
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int launcher_entry(int,char**);
int __real_SDL_PollEvent(SDL_Event*);
void __real_SDL_RenderPresent(SDL_Renderer*);
SDL_Window* __real_SDL_CreateWindow(const char*,int,int,int,int,Uint32);
static int frames,stage,offset,ready,saved,quit_sent;
SDL_Window* __wrap_SDL_CreateWindow(const char*t,int x,int y,int w,int h,Uint32 flags){const char*folder=getenv("UI_CAPTURE_DIR");if(folder){char marker[4096];SDL_snprintf(marker,sizeof(marker),"%s/window-opened.txt",folder);FILE*f=fopen(marker,"w");if(f){fputs("1",f);fclose(f);}}const char*m=getenv("UI_TEST_MODE");if(m&&!strcmp(m,"other-small")){w/=2;h/=2;}return __real_SDL_CreateWindow(t,x,y,w,h,flags);}
static void capture(SDL_Surface *s,const char *name) {
 const char *folder=getenv("UI_CAPTURE_DIR");if(!folder)return;
 char path[4096];SDL_snprintf(path,sizeof(path),"%s/%s.png",folder,name);IMG_SavePNG(s,path);
}
void __wrap_SDL_RenderPresent(SDL_Renderer*r) {
 int w,h;SDL_GetRendererOutputSize(r,&w,&h);
 SDL_Surface*s=SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_RGBA32);
 if(s&&SDL_RenderReadPixels(r,NULL,s->format->format,s->pixels,s->pitch)==0){
  Uint32 pixel;memcpy(&pixel,(char*)s->pixels+(208*h/560)*s->pitch+(49*w/800)*4,4);
  Uint8 red,green,blue,alpha;SDL_GetRGBA(pixel,s->format,&red,&green,&blue,&alpha);
  ready=red==80&&green==218&&blue==151;
  if(frames==1)capture(s,"initial");
  if(stage==1&&frames==3&&getenv("UI_TEST_MODE")&&!strcmp(getenv("UI_TEST_MODE"),"browse"))capture(s,"browse");
  if(ready&&!saved){capture(s,"ready");saved=1;}
  if(frames==9&&!ready)capture(s,"error-or-missing");
 }
 if(s)SDL_FreeSurface(s);
 __real_SDL_RenderPresent(r);frames++;
}
static int click(SDL_Event*e,int x,int y){memset(e,0,sizeof(*e));e->type=SDL_MOUSEBUTTONUP;e->button.button=SDL_BUTTON_LEFT;e->button.x=x;e->button.y=y;return 1;}
static int key(SDL_Event*e,SDL_Keycode k,int mod){memset(e,0,sizeof(*e));e->type=SDL_KEYDOWN;e->key.keysym.sym=k;e->key.keysym.mod=mod;return 1;}
int __wrap_SDL_PollEvent(SDL_Event*e) {
 const char *mode=getenv("UI_TEST_MODE"),*path=getenv("UI_TEST_DISK");
 if(mode&&!strcmp(mode,"browse")){
  if(stage==0&&frames>=2){stage=1;return click(e,100,330);}
  if(stage==1&&frames>=4){stage=2;return key(e,SDLK_a,KMOD_CTRL);}
  if(stage==2){if(path&&path[offset]){memset(e,0,sizeof(*e));e->type=SDL_TEXTINPUT;size_t n=strlen(path+offset);if(n>31)n=31;memcpy(e->text.text,path+offset,n);offset+=n;return 1;}stage=3;return key(e,SDLK_RETURN,0);}
 }
 if(mode&&(!strcmp(mode,"other")||!strcmp(mode,"other-small"))){
  if(stage==0&&frames>=2){stage=1;return click(e,650,500);} /* Disabled Play must do nothing. */
  if(stage==1&&frames>=5){stage=2;return click(e,100,500);}
 }else if(ready&&frames>=8&&stage!=20){stage=20;return click(e,650,500);}
 if(!quit_sent&&((mode&&!strcmp(mode,"error")&&frames>=12)||frames>400)){quit_sent=1;memset(e,0,sizeof(*e));e->type=SDL_QUIT;return 1;}
 return __real_SDL_PollEvent(e);
}
int main(int argc,char**argv){return launcher_entry(argc,argv);}
'''
STUB = r'''
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(int argc,char**argv){char cwd[4096];FILE*f=fopen(getenv("UI_TEST_RESULT"),"w");if(!f)return 9;
fprintf(f,"cwd=%s\n",getcwd(cwd,sizeof(cwd)));for(int i=0;i<argc;i++)fprintf(f,"arg%d=%s\n",i,argv[i]);fclose(f);return 0;}
'''
IMPORT = r'''
#include "mac_asset_import.h"
#include <stdio.h>
int main(int argc,char**argv){char e[512];if(argc!=3)return 2;int ok=mac_import_disk(argv[1],argv[2],e,sizeof(e));if(!ok)fprintf(stderr,"%s\n",e);return ok?0:1;}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disk', type=Path)
    parser.add_argument('--archive', type=Path, action='append', default=[])
    parser.add_argument('--output', type=Path, default=ROOT/'screenshots/launcher-v1.0.1')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'sdl2', 'SDL2_image'], text=True))
    with tempfile.TemporaryDirectory(prefix='launcher-tests-', dir=ROOT/'.build') as temporary:
        work = Path(temporary)
        (work/'harness.c').write_text(HARNESS)
        (work/'engine.c').write_text(STUB)
        (work/'import.c').write_text(IMPORT)
        compiler = ['/usr/bin/gcc', '-D_GNU_SOURCE=1', '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-I'+str(ROOT/'src')]
        subprocess.run([*compiler, '-Dmain=launcher_entry', '-c', str(ROOT/'src/launcher.c'), '-o', str(work/'launcher.o'), *flags], check=True)
        subprocess.run([*compiler, str(work/'harness.c'), str(work/'launcher.o'), str(ROOT/'src/mac_asset_import.c'), str(ROOT/'src/mac_archive.c'), '-Wl,--wrap=SDL_RenderPresent', '-Wl,--wrap=SDL_PollEvent', '-Wl,--wrap=SDL_CreateWindow', '-o', str(work/'ui'), *flags], check=True)
        subprocess.run([*compiler, str(work/'engine.c'), '-o', str(work/'engine')], check=True)
        subprocess.run([*compiler, str(work/'import.c'), str(ROOT/'src/mac_asset_import.c'), str(ROOT/'src/mac_archive.c'), '-o', str(work/'importer'), *flags], check=True)
        total = 0

        def game(name):
            folder = work/name
            (folder/'data').mkdir(parents=True)
            (folder/'runtime').mkdir()
            shutil.copy2(work/'ui', folder/'prince')
            shutil.copy2(work/'engine', folder/'runtime/prince-engine')
            return folder

        def run(folder, mode, disk=None, expect_game=True):
            nonlocal total
            captures = output/folder.name
            captures.mkdir(exist_ok=True)
            result = folder/'result.txt'
            window_marker = captures/'window-opened.txt'
            window_marker.unlink(missing_ok=True)
            env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', SDL_RENDER_DRIVER='software',
                       UI_TEST_MODE=mode, UI_TEST_DISK=str(disk or ''), UI_CAPTURE_DIR=str(captures),
                       UI_TEST_RESULT=str(result), PATH='/no-interpreter-needed', HOME=str(work))
            subprocess.run([str(folder/'prince')], cwd=work, env=env, check=True, timeout=20)
            assert result.exists() == expect_game
            if expect_game:
                text = result.read_text()
                assert f'cwd={folder}' in text and f'arg0={folder}/prince' in text
                if mode == 'cached':
                    assert 'arg1=' not in text and not window_marker.exists()
                else:
                    assert f'arg1=graphics={"dos" if mode.startswith("other") else "mac_native_full"}' in text
                    assert window_marker.exists()
            total += 1

        missing = game('missing-image')
        run(missing, 'other')
        assert not (missing/'data/res_mac_native_full.pak').exists()
        run(game('small-window'), 'other-small')
        bad = game('invalid-image')
        (bad/'bad.dsk').write_bytes(b'not a Macintosh disk')
        run(bad, 'error', expect_game=False)
        assert not (bad/'data/res_mac_native_full.pak').exists()
        ambiguous = game('multiple-images')
        (ambiguous/'one.dsk').write_bytes(b'one')
        (ambiguous/'two.hfs').write_bytes(b'two')
        run(ambiguous, 'error', expect_game=False)

        # Parser rejection, original/output preservation, and temporary cleanup.
        for data in (b'', b'\0'*4096, b'bad image', b'ER\x02\0'+bytes(4092)):
            source = work/'bad.dsk'; target = work/'not-created.pak'
            source.write_bytes(data)
            result = subprocess.run([str(work/'importer'), str(source), str(target)], capture_output=True)
            assert result.returncode == 1 and not target.exists() and source.read_bytes() == data
            total += 1

        if args.disk:
            from build_mac_graphics_pack import graphics_from_disk, pack_bytes
            from test_mac_disk_import import dc42, partitioned
            image = args.disk.read_bytes()
            expected = pack_bytes(graphics_from_disk(image)[0])
            source = work/'my image ü.hfs'
            source.write_bytes(image)
            source.chmod(0o444)
            for name, content in (('raw.hfs', image), ('raw.dsk', image), ('diskcopy.dsk', dc42(image)), ('partition.hfs', partitioned(image))):
                disk = work/name; target = work/(name+'.pak'); disk.write_bytes(content)
                subprocess.run([str(work/'importer'), str(disk), str(target)], check=True)
                assert target.read_bytes() == expected and disk.read_bytes() == content
                total += 1
            # Existing output is never replaced, even when extraction succeeds.
            protected = work/'keep.pak'; protected.write_bytes(b'preserve me')
            result = subprocess.run([str(work/'importer'), str(source), str(protected)], capture_output=True)
            assert result.returncode == 1 and protected.read_bytes() == b'preserve me'
            assert not list(work.glob('*.tmp'))
            total += 1
            browsed = game('browse-image')
            run(browsed, 'browse', source)
            assert (browsed/'data/res_mac_native_full.pak').read_bytes() == expected
            before = (browsed/'data/res_mac_native_full.pak').stat().st_mtime_ns
            (browsed/'result.txt').unlink()
            run(browsed, 'cached')
            assert (browsed/'data/res_mac_native_full.pak').stat().st_mtime_ns == before
            automatic = game('auto-detect-image')
            (automatic/'PoP1.dsk').write_bytes(image)
            run(automatic, 'automatic')
            assert (automatic/'data/res_mac_native_full.pak').read_bytes() == expected
            assert hashlib.sha256(args.disk.read_bytes()).digest() == hashlib.sha256(image).digest()
        for index, archive in enumerate(args.archive):
            folder = game(f'archive-{index}')
            shutil.copy2(archive, folder/archive.name)
            run(folder, 'automatic')
            pack = folder/'data/res_mac_native_full.pak'
            assert pack.exists()
            if args.disk:
                assert pack.read_bytes() == expected
            run(folder, 'cached')
        print(f'PASS: {total} launcher/import scenarios; actual UI captures: {output}')


if __name__ == '__main__':
    main()
