# Credits

## Prince of Persia Remastered

Project maintained by [The-Alok](https://github.com/The-Alok). This version adds
the native Macintosh graphics renderer, composition fixes and reference audits
to SDLPoP. The original game's gameplay foundation is SDLPoP.

## SDLPoP engine

[Dávid Nagy (NagyD)](https://github.com/NagyD/SDLPoP) created SDLPoP. Thanks to
all SDLPoP contributors, including Falcury for SDL2, replays, quicksave, menus
and other features. The complete upstream documentation/contributor list is
preserved in [README.upstream.md](README.upstream.md). Original copyright and
license notices remain in the sources; see [COPYING](COPYING).

## PSP port and graphics-pack reference

[chanunnaki's SDLPoP-PSP](https://github.com/chanunnaki/SDLPoP-PSP) supplied the
PSP graphics-pack implementation/reference used during this project's earlier
work. Credit to chanunnaki for the enhanced PSP port and pack integration, and
to striga and sharkwouter for the earlier PSP scaffolding.

The pack integration is separate from authorship of the artwork. The PSP
project identifies these graphics sources:

- **SNES 1 / SNES 2**: based on the former SNES Alt pack, sourced from Maurice
  Kaltofen (mk1995)'s SNES-PC-set. SNES 1 additionally uses this project's Prince
  recolouring.
- **SNES 3**: former SNES pack, sourced from KJ's SNES Remnants. The referenced
  credits also acknowledge brain, PoP_MoreSNES, Total Pack 3.0, tV2 and programmer.
- **Mac Native FULL**: original Macintosh artwork and SHPD origins from the
  Macintosh game; original palette/drawing data was checked against the supplied
  source disk. The renderer work here does not claim authorship of that artwork.

See the [PSP project's graphics credits](https://github.com/chanunnaki/SDLPoP-PSP#graphics-packs--custom-level-authors)
for its full attribution and mod references.

## Original game and libraries

Prince of Persia was created by **Jordan Mechner**. Credit to the original
Prince of Persia teams, including the Macintosh artwork/port contributors.
This is an unofficial fan project.

SDLPoP source is GPL-3.0-or-later. Game artwork and other original game data
retain their original ownership; this project does not relicense them. SDL,
SDL_image, Nuked OPL3 and stb_vorbis retain their own notices and licenses.
Windows downloads include notices for their bundled runtime libraries.
