# V1.0.0 — cleanup and local release preparation

2026-10-26. Cleanup and local release-candidate tooling are implemented. The user requested removal of
Mac Color and Mac Native 640x400 from the active pack list, and renamed Mac
Prince to SNES Alt 2, then requested this final order: Mac Native FULL v1.0.0
(default), DOS, SNES 1 (former SNES Alt 2/Mac Prince), SNES 2 (former SNES Alt),
SNES 3 (former SNES). Internal settings IDs and PAK filenames stay stable.
The current renderer remains `mac_native_full`, now index 0 as well as the INI
default. Lifecycle assertions verify all five ordered ID/label pairs and startup
selection, then switch through all five packs eight times.

The window title is now **Prince of Persia Remastered - v1.0.0**.
The welcome screen describes this Macintosh graphics version, shows the menu,
save/load and replay controls, links the local README and retains a short
SDLPoP engine credit. `REMASTERED_NAME`/`REMASTERED_VERSION` in `src/config.h`
define the display identity; the upstream `SDLPOP_VERSION` remains separate.
The real startup text draw was captured in `screenshots/v18.0/welcome-remastered.png`;
the SDL window title was checked in the same run. The input wait/controller
dismissal behavior is retained. No native artwork or gameplay changes.

## Cleanup and ownership

`uses_mac_native_full()` in `src/pak.c` evaluates the live selection. Existing
native-full call sites now use it; it is not cached across menu pack switches.
The legacy DOS path and native image flags/origins remain separate concerns.
V17 already removed the obsolete guessed sword corrections. V11's documented
climb registration and outline exceptions remain necessary and are retained.

The cleanup fixes three prior compiler warnings: an overlong menu description,
a truncation-prone resource-folder copy and a redundant stack-array NULL check.
The save-path storage is now initialized for a missing HOME/override, and bounded
by its own capacity. Ordinary save locations are unchanged.

Memcheck found actual ownership defects, reproduced in the retained pre-fix logs:

- Missing DAT fallback read `stat` output without checking success.
- Sprite loading leaked each temporary 100-byte SHPL palette after copying it.
- Directory enumeration leaked an open handle when no matching extension existed.
- Converted audio allocated an unused sample-sized tail plus a separate sample
  buffer twice the byte length. The latter was never freed. Allocate the header
  and exact sample byte count separately; free the samples with their header.
  If that sample is playing, detach it from the SDL callback before freeing it.

Audio interpolation, scheduling and resulting bytes are unchanged: 31 converted
buffers have the same FNV1a `8e4ffd9d` before and after cleanup. The lifecycle
fixture tests repeated pack switches, owned/borrowed fade sources, interrupted
fades, no-HOME save fallback and freeing an active sample. It frees its sound
resources before exit. It is compiled only into the temporary test executable.

## Checks

The retained visual suite has 26,275 counted checks plus climb/fall checks.
Final logs/captures are under `screenshots/v18.0/`. All 26,275 checks pass,
plus retained climb/fall and 237 lifecycle checks. All 1,157 PNGs shared with
v17 have identical decoded RGB pixels and dimensions. The forced build has
zero compiler warnings. Both native PAK hashes match, the moved stable pack
passes structural/pixel validation, and all 820 full-pack images rebuild
byte-identically from the preserved source folder. Lifecycle checks are separate
from the visual count. Compare decoded RGB pixels with v17, not only PNG file bytes.

```sh
make -B -C src -j2 CC=/usr/bin/gcc
python3 tools/mac_native_renderer/test_climb_overlay.py \
  --floors --architecture --palace --props --combat --world --ui --cutscenes \
  --output screenshots/v18.0
python3 tools/mac_native_renderer/test_climb_overlay.py --lifecycle \
  --valgrind-log screenshots/v18.0/lifecycle-memory-results.txt
sha256sum -c MAC_NATIVE_SHA256SUMS.txt
python3 tools/mac_native_renderer/validate_mac_native_pack.py
```

The focused lifecycle Memcheck run has zero definite/indirect leaks and zero
reported errors after cleanup. Possible-loss and reachable allocations from the
remaining game/SDL state are reported separately; this is not a whole-process
leak-free certification. The final run reports 7,676,224 possible-loss bytes
and 8,176,490 reachable bytes in the remaining game/SDL state. The earlier long UI/cutscene memory run is retained as
pre-cleanup evidence and is not the final memory result. Full pixel checks cover
fade output independently; the lifecycle run checks allocation ownership.

Performance logs measure this Fedora host under SDL's dummy software driver.
The existing three-gate benchmark excludes timing waits. Whole-suite elapsed
time and peak RSS include the compiler/test process and image output; they are
not game FPS or a Steam Deck benchmark. The branding regression gate mean is 0.325 ms;
the earlier timed full output-producing run took 40.40 s and peaked at 133,292 KiB RSS. Do not extrapolate them to hardware.

## Package and release limits

`tools/release/build_release.py` creates one `.tar.gz` archive in `dist/`, with
the executable, active data, source, fixtures, notes, selected captures and a
SHA-256 inventory. Reference-only legacy packs stay outside the active data
folder. The builder refuses an existing archive name. The workspace remains
the sole development tree. Extracted package verification uses a temporary folder.

Live pending: v17 sword/door/pillar checks, v12.3 linked slab edge, v13.1 wall
and pickup edges, separate props/cutscene checks, a complete played run and
original-Mac motion comparisons. V16 damage/healing, pause/resume and intro fades
remain user-confirmed. Steam Deck startup/controller/display/performance testing
and a compatible-runtime build are pending. Local Fedora startup is separate
from those live checks. No public release or blanket original-Mac fidelity claim.

## Local archive verification

The local v1.0.0 archive was built and extracted into a temporary directory
with spaces in its path. All inventoried files pass SHA-256 verification;
the included source rebuilds with zero warnings. Exactly five active PAKs are
present. Both the workspace executable and extracted launcher run under dummy
SDL drivers until a controlled three-second timeout. This is startup evidence,
not live visual playtesting. See screenshots/v18.0/package-verification.json
and package-source-build-results.txt. The temporary extraction was removed;
the source workspace and dist/ archive remain. No external release was published.
