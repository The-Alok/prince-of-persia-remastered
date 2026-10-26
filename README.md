# Prince of Persia Remastered v1.0.1

**A Macintosh HD graphics port for Windows and Linux, with a custom native
640×400 renderer and 2× high-resolution sprite support.**

Prince of Persia Remastered is an independent, unofficial fan project maintained
by [The-Alok](https://github.com/The-Alok). It brings the original Macintosh
colour artwork, sprite origins and scene composition to a modified version of
[SDLPoP by Dávid Nagy and contributors](https://github.com/NagyD/SDLPoP).

## Gameplay screenshots

### Mac Native FULL

**Dungeon — Level 1**

![Mac Native FULL gameplay in the Level 1 dungeon](doc/images/gameplay-level1-mac.png)

**Palace — Level 5**

![Mac Native FULL gameplay in the palace](doc/images/gameplay-mac.png)

### Other graphics packs

Each grid shows **DOS, SNES 1, SNES 2 and SNES 3**, in that order.

**Dungeon — Level 1**

![DOS and all three SNES packs in the same Level 1 dungeon scene](doc/images/graphics-packs-level1.png)

**Palace — Level 5**

![DOS and all three SNES packs in the same palace scene](doc/images/graphics-packs.png)


The custom renderer uses a **640×400 physical framebuffer** over SDLPoP's
**320×200 logical gameplay canvas**. “2×” describes that rendering scale; the
Macintosh sprites use their original native pixels and SHPD origins.
The project focuses on this Macintosh presentation, its asset handling and
platform builds while retaining SDLPoP's gameplay foundation.

## Macintosh graphics: use your own disk

The current source tree contains the SDLPoP-based port and extraction tools,
but does **not bundle the Macintosh graphics PAK, extracted Macintosh sprites,
application resource forks or pixel-reference fixtures**. Generate the optional
Macintosh pack locally from your own Macintosh colour edition of Prince of
Persia. The generated files are ignored by Git and excluded from new packages.

Use a copy you are entitled to use, containing the Macintosh colour application
and `Persia(COLOR)` graphics file, with its Macintosh resource forks preserved.
The launcher accepts **HFS, DSK, DC42, HQX, SIT and ISO** files. It detects raw
classic HFS, Disk Copy 4.2 and Apple Partition Map images from their contents;
BinHex/StuffIt archives unpack automatically in memory, including nested disks.
ISO support covers classic HFS/ISO bridges and ISO9660 discs containing a game
disk or files with AppleDouble resource forks. A DOS-only CD, HFS+ volume or
incomplete archive cannot supply the Macintosh artwork. Password-protected
archives, StuffIt X and installer-specific compression are unsupported.
No game files are downloaded, mounted or executed.

### Start with the launcher

1. Extract the complete game download into a folder you can write to.
2. Copy one supported game image/archive into the same folder as `prince.exe`
   (Windows) or `prince` (Linux). Or use **Browse for image** in the launcher.
   If several are present, Browse lets you select the one to import.
3. Open `prince.exe` on Windows, or run `./prince` on Linux. The launcher finds
   the image and prepares the graphics automatically, then opens the game.
   Later launches go straight to the game when the graphics are already ready.

The setup window appears only when graphics need preparing. Its status lights
show whether a disk image is selected and whether the graphics are ready. It
remembers a browsed image and reuses the prepared graphics automatically. The
disk image is read-only; nothing is mounted, downloaded or executed from it.

The game executable lives in `runtime/`; open the launcher in the main folder.
**Play DOS Mode** starts the game without importing Macintosh graphics. You can
switch packs in the game's Visual settings.

Without an imported Mac pack, the game builds and runs with the bundled DOS
and SNES packs. The first successful import starts in Macintosh mode; later
launches remember your in-game graphics choice. The screenshots above illustrate
the optional Macintosh pack alongside the other graphics options.

**Historical release notice:** the previously published v1.0.0 downloads and
older Git history still contain Macintosh assets until their separate cleanup
is reviewed and completed. The new launcher is available from the current source and local v1.0.1 builds;
the historical v1.0.0 downloads do not yet include it. Their cleanup and a new
public release remain separate steps.


## Next release: v1.0.1

The v1.0.1 builds below are the next release targets. They include automatic
Macintosh setup, HQX/SIT/ISO import, direct startup after setup, the F7/F8 display
controls, and corrected background-brick layering while falling.
They are not yet published on [Releases](https://github.com/The-Alok/prince-of-persia-remastered/releases);
the historical v1.0.0 downloads remain separate until release cleanup is approved.

| Platform | Download | Start |
| --- | --- | --- |
| Windows x64 | `prince-of-persia-remastered-v1.0.1-windows-x86_64.zip` | Extract everything, then double-click `prince.exe` |
| Linux x64 | `prince-of-persia-remastered-v1.0.1-linux-x86_64.tar.gz` | Extract, then run `./launch.sh` or `./prince` |

Windows includes the required SDL runtime DLLs; keep them beside `prince.exe`.
Linux uses the system SDL2 and SDL2_image libraries. On Debian/Ubuntu:

```sh
sudo apt install libsdl2-2.0-0 libsdl2-image-2.0-0
```

The Linux binary's exact glibc/library requirements are recorded in its
`BUILD_INFO.json`. This build requires glibc 2.38 or newer. On older systems,
build from source. Steam Deck compatibility has not been certified.

## What this version adds

- Original Macintosh artwork and SHPD origins at native resolution.
- Native dungeon/palace composition, climbing masks, foreground depth and
  interactive floor, gate, exit, mirror and chomper rendering.
- Original-data sword attachment placement for the Prince and guard families.
- Native cutscene composition, story-card colours, health flasks and RGB fades.
- Four bundled packs: **DOS**, **SNES 1**, **SNES 2**, **SNES 3**.
- Optional locally generated **Mac Native FULL v1.0.1**, first in the menu
  and preferred at startup when installed.

F7 cycles picture shaders: **Pixel** (nearest-neighbour), **Sharp** (crisp
integer prescaling), **Soft** (bilinear), and **Smooth HQ** (Scale3x edge
smoothing). F8 cycles the installed graphics packs during play or pause. The
pause menu shows the current shader and pack. Both choices are also available
in Settings → Visuals and are remembered between launches. Display shortcuts
do not skip story screens, advance animation or change movement/collision.

SDLPoP gameplay, collision, animation timing and replay logic are preserved.


## Quick controls

- Arrow keys: move, jump/climb and crouch.
- Shift: grab, careful steps and sword action.
- Esc: menu/settings; F6/F9: quicksave/quickload.
- Ctrl+Tab: record a replay; Tab on the title screen: replay browser.
- **F7**: cycle picture shaders — Pixel → Sharp → Soft → Smooth HQ.
- **F8**: cycle installed graphics packs while playing or paused.
- Alt+Enter: fullscreen toggle.

Edit `SDLPoP.ini` or use Settings → Visuals. The default remains
`graphics_pack = mac_native_full`. Internal pack IDs are retained for
compatibility even though the three SNES menu labels were renamed.

## Build from source

Linux requires a C compiler, make, pkg-config and SDL2/SDL2_image development
headers. On Debian/Ubuntu:

```sh
sudo apt install build-essential pkg-config libsdl2-dev libsdl2-image-dev
make -C src -j2 CC=/usr/bin/gcc
./prince
```

Windows can be built with MinGW in an MSYS2 MINGW64 shell, or cross-compiled
with the extracted Fedora MinGW toolchain:

```sh
python tools/release/build_windows.py
# Or, from Linux:
python3 tools/release/build_windows.py --toolchain .build/toolchain \
  --sdl2 .build/SDL2-2.32.10/x86_64-w64-mingw32 --output .build/windows-release
```

The Windows script keeps its objects separate from Linux and copies the
runtime DLL dependency closure, PNG codec and applicable SDL3 compatibility
runtime. See its docstring for prerequisites. The exact source for release
v1.0.0 is the [v1.0.0 tag](https://github.com/The-Alok/prince-of-persia-remastered/tree/v1.0.0).

<details>
<summary>Optional Macintosh renderer tests</summary>

Developers can still use `tools/build_mac_graphics_pack.py` for a manual import.
HQX/SIT/ISO developer imports use the built native launcher; raw HFS imports
remain available through the independent Python reader. The format/safety tests do not
require game assets:

```sh
python3 tools/test_mac_disk_import.py
python3 tools/test_mac_archives.py
python3 tools/test_launcher.py
# After building, with locally generated Macintosh resources:
python3 tools/mac_native_renderer/test_display_controls.py
```

For image/renderer regression, install Pillow and generate private fixtures
from your disk into the initially empty, Git-ignored `tools/mac_reference/`
directory. This also generates the optional Mac pack:

```sh
python3 -m pip install Pillow
python3 tools/build_mac_graphics_pack.py "/path/to/PoP1.dsk" \
  --test-assets tools/mac_reference
make -C src -j2 CC=/usr/bin/gcc
python3 tools/mac_native_renderer/test_climb_overlay.py \
  --floors --architecture --palace --props --combat --world --ui --cutscenes
```

Existing nonempty fixture directories are not overwritten. These local files
contain extracted game resources and must not be committed or redistributed.

</details>

## Issues and contributions

This repository is the home for this Macintosh port's rendering, graphics-pack,
scaling, asset-loading and Windows/Linux build work. Report issues with your
platform, graphics pack, level/room if known, facing direction and a screenshot
or recording. Gameplay regressions observed in this version are welcome too.

## AI assistance disclaimer

This remaster was created with AI assistance from **ChatGPT and Codex**, including
help with coding, debugging, testing and documentation. **The-Alok** directed
development and playtested the game. Original game artwork and upstream code
remain credited to their respective creators below.

## Credits and license

This is a modified version of [SDLPoP](https://github.com/NagyD/SDLPoP) by
**Dávid Nagy (NagyD)** and contributors. SDLPoP remains the engine and gameplay
foundation. Thanks to **Falcury** for SDL2, replays, quicksave, menus and other
features, and to all upstream contributors listed below.

**[chanunnaki's SDLPoP-PSP](https://github.com/chanunnaki/SDLPoP-PSP)** supplied
the graphics-pack implementation/reference used during this project's earlier
work. Thanks to chanunnaki for the enhanced PSP port and pack integration, and
to **striga** and **sharkwouter** for the earlier PSP scaffolding.

Earlier platform ports also credit **PSPDEV, samo79, BeWorld, Capehill and the
SDL2 AmigaOS4 contributors, salass00, kas1e, guillaume**, and **os4coding.net**.

Graphics-pack integration is separate from authorship of the artwork:

- **Mac Native FULL**: original Macintosh artwork and SHPD origins from the
  Macintosh game. This project's renderer does not claim authorship of that
  artwork. Locally generated test fixtures retain the native Mac dimensions,
  transparency and origins recorded in their generated manifest.
- **SNES 1 / SNES 2**: based on the former SNES Alt pack, sourced from
  **Maurice Kaltofen (mk1995)**'s SNES-PC-set. SNES 1 additionally uses this
  project's Prince recolouring.
- **SNES 3**: former SNES pack, sourced from **KJ**'s SNES Remnants. The
  referenced credits also acknowledge **brain, PoP_MoreSNES, Total Pack 3.0,
  tV2 and programmer**.

See the [PSP project's graphics credits](https://github.com/chanunnaki/SDLPoP-PSP#graphics-packs--custom-level-authors)
for its full attribution and mod references.

**Jordan Mechner and the original Prince of Persia teams**, including the
Macintosh artwork/port contributors, created the original game. This is an
unofficial fan project maintained by [The-Alok](https://github.com/The-Alok).

<details>
<summary>Original SDLPoP author and contributor list</summary>

Author:

* NagyD (David from `forum.princed.org`)

Contributors:

* Andrew (bug reports)
* htamas (inspiration, dungeon wall drawing algorithm, bug reports)
* Norbert (EndeavourAccuracy on GitHub) (bug reports, suggestions, improved gamepad support)
* musa (bug reports)
* Eugene (bug reports)
* StaticReturn (Mac OS X: Makefile (for older SDL1 version), bug reports)
* Poirot (ecalot on GitHub) (Mac OS X: Now compatible with Falcury SDL2 port)
* kees (bugfixes)
* Falcury
    * porting to SDL2
    * quicksave improvements
    * replay files
    * SDLPoP.ini: added basic support and constantly adding new options
    * mod folders
    * fake tiles
    * readable sequence table
    * CMake support
    * pause menu
    * MIDI support
    * and various other bugfixes, improvements, additions
* segra (segrax on GitHub) (Joystick support, resizable window)
* DarkPrince (bug reports)
* Andrey Vasilkin / digi@os2.snc.ru (eComStation (OS/2) support)
* mfn (fixed a small bug when `USE_MIXER` is undefined)
* diddledani (Visual C++/NMake support)
* zaps166 (small Makefile fixes)
* usineur (faster music loading)
* yaqxsw (icon)
* SergioMartin86
* dstarosta
* Ryzee119
* berikv
* xyproto
* BarrOff
* icculus
* thrimbor
* vanfanel
* asppsa
* akien-mga
* mbeijen
* alebcay
* ckorn
* Rondom
* yuv422
* dive

Usernames on the list above refer to either GitHub users or users from `forum.princed.org`.

</details>

Smooth HQ implements the [Scale3x algorithm](https://www.scale2x.it/algorithm)
from the Scale2x project by Andrea Mazzoleni. The four picture-filter styles
were referenced against the user-supplied PoP2 Mac build; its code and game
assets are not bundled with this project.

The built-in StuffIt decoder adapts routines from **libxad** by Dirk Stöcker,
Dik T. Winter and Matthew T. Russotto, under LGPL-2.1-or-later. Its original
notices, corresponding source and local changes are in
[`src/third_party/libxad/`](src/third_party/libxad/); downloads include the license.

Source is **GPL-3.0-or-later**; see [COPYING](COPYING) and the original copyright
and license notices preserved in the source files. Original game artwork and
other game data retain their original ownership; this project does not
relicense them.

[SDL (Simple DirectMedia Layer)](https://www.libsdl.org/) is distributed under
the [zlib license](https://www.zlib.net/zlib_license.html). SDL_image, Nuked OPL3
and stb_vorbis retain their own notices and licenses. Windows downloads include
notices for their bundled runtime libraries.
