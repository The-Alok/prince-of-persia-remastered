# Prince of Persia Remastered v1.0.0

First public release for **Windows x64** and **Linux x64**. This release packages
the Macintosh renderer developed through the v11–v18 milestones under a new
public version number. The SDLPoP engine version is credited separately.

## Included

- Native 640×400 Macintosh dungeon/palace artwork, origins and palette composition.
- Climb/fall masking, pillar depth, floor-state, gate/exit, mirror/shadow and
  chomper fixes retained from the development milestones.
- Sword pickup, draw/sheath, attack and parry placement from original Mac data,
  checked for Prince/guard-family art in both directions.
- Native props, Princess room/cutscenes, blue story panels, health icons and fades.
- Resource-lifetime cleanup and a version-specific title/welcome screen.
- Graphics menu: Mac Native FULL v1.0.0 (default), DOS, SNES 1, SNES 2, SNES 3.

## Downloads

- `prince-of-persia-remastered-v1.0.0-windows-x86_64.zip`: extract all files and
  run `prince.exe`; required runtime DLLs/notices are included.
- `prince-of-persia-remastered-v1.0.0-linux-x86_64.tar.gz`: extract and run
  `./launch.sh`. System SDL2/SDL2_image and glibc 2.38+ are required for this
  Fedora-built binary. Source builds support other compatible Linux systems.

Each archive contains a file inventory in `SHA256SUMS` and build information;
the release also provides a checksum file for both downloads. Source for the
exact release is available from the v1.0.0 Git tag and GitHub source archives.

## Validation and limits

The retained visual suite contains 26,275 counted checks plus climb/fall checks;
the lifecycle suite passes 237 checks and covers the five ordered menu choices, startup default,
40 pack switches and 48 fade allocations/restorations. Original-data combat
checks use 322 independent composites; world coverage includes 332 populated
room maps and 14 actual entry poses. Build/startup and visual/reference results
are recorded separately in the development notes.

The Windows binary is cross-compiled and checked under Wine; a full native
Windows playthrough remains pending. The Wine host has no working WASAPI audio
endpoint, so native Windows sound/input checks remain open. See
[doc/RELEASE_VALIDATION.md](doc/RELEASE_VALIDATION.md). Steam Deck testing, a complete played run
and wider original-Mac motion comparisons remain open. The user confirmed
Macintosh damage/healing, pause/resume and intro fades during development.

## Credits

Built on [SDLPoP](https://github.com/NagyD/SDLPoP) by Dávid Nagy and contributors.
Thanks to [chanunnaki's SDLPoP-PSP](https://github.com/chanunnaki/SDLPoP-PSP) for
the graphics-pack reference, and to the original game and mod/art contributors.
See `CREDITS.md`, `README.upstream.md`, `COPYING` and bundled library notices.
