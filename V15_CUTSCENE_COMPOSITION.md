# V15.1 — Native cutscene composition

Implementation and automated audit complete, 2026-10-26. Live playtest pending.
The current executable uses `mac_native_full`; the menu label is
**Mac Native FULL v15.1 (Native cutscenes)**. V16 HUD/UI is next.

## V15.1 story background correction

The user reported that intro text appeared over black instead of blue. The
SHPD 40 frame's centre is transparent. Drawing it with the DOS opaque blitter
painted the transparent pixels' stored black RGB; the old DOS palette-index-14
change does not supply an underlay for the native frame.

The native-full STORY_FRAME path now fills the physical 640x400 target with
the selected story colour, then draws the transparent native frame and text.
CODE 3:$0D3A/$0DAA pass CLUT index23 to the fill routine at $0ED6; original
CLUT2000 entry23 is RGB(16,0,96), `#100060`. The existing ending selector uses
entry16, RGB(128,0,0), `#800000`, matching CODE 3:$263C. The main palace title
has its own drawing path and retains its black backdrop. Source palettes and
PAKs are unchanged. Logical story anchors and 280-wide actor X are untouched.

The build and full suite pass: **1,101 cutscene + 16,757 earlier checks =
17,858**, plus retained climb/fall checks. Independent source PNG fixtures
now compose all four cards over both blue and red fills, compare opaque border
pixels, and verify the main title remains unchanged after either selection.
Of 457 PNGs shared with v15.0, 453 are identical; only the four story cards
change. Both native PAK checksums match. Headless startup is checked separately
from live playtest. See `screenshots/v15.1/` for captures, source evidence,
build/test logs and `v15-regression-comparison.json`.

To verify live: close the old game, restart `./prince`, and let the intro text
appear. The inner panel should be dark blue around the white lettering while
the ornamental border stays intact. V15.1 live confirmation remains pending.

The sections below preserve the v15.0 implementation and audit record.

## What changed

The Princess room was using a native background with DOS prop-table layout.
Mac SHPD 950 contains only background 951. The hourglass, sand, foreground
pillar and small room flames belong to SHPD 960 (resources 962..975), which
the previous loader did not map into the room's sprite slots. The native-full
loader now uses that group, with 15 slots including the background. It requires
the final native sidecar; other packs retain their existing table layout.

The native background already contains the bed, carpet and architecture.
The separate DOS bed overlay is omitted. Correct native room flames replace
the general dungeon torch art, and the hourglass and sand use their own SHPD
origins. The native foreground pillar is drawn after the actors.

Native frames are recomposed from the retained background before actors and
foreground, clearing transparent flame/sand/actor trails. The existing star
colours are restored without adding RNG calls or advancing their animation.
Cutscenes use the full 640x400 buffer and viewport; gameplay's 640x384 room
buffer is restored when returning from a scene entered between levels.

The four story cards now use their native SHPD insets and native text pixels.
Their logical anchors are unchanged. DOS monochrome recolouring previously
replaced the native text white and omitted its inset.

Gameplay, actor sequences, cutscene scheduling, collision, timing, replay logic,
original PNGs/origins, disk and PAKs are unchanged. Actor X remains SDLPoP's
280-wide coordinate system; prop anchors below use 320x200 logical screen
coordinates, doubled to 640x400 physical pixels before subtracting SHPD origins.

## Original-Mac evidence

Read-only application resource extraction under `tools/mac_reference/` and the
user-supplied `Mac-Color-Native-Graphics/` provide these anchors:

| Element | Original source | Logical anchor | Physical top-left |
| --- | --- | --- | --- |
| Room background 951 | SHPD 950 | (0,0) | (0,0) |
| Foreground pillar 962 | CODE 3:$1D30..$1D3E | (240,167) | (473,219) |
| Left/right flames 973..975 | CODE 3:$2512..$2576, DATA/ZERO | (78,121), (290,121) | (156,213), (580,213) |
| Hourglass 963..969 | CODE 3:$2578..$25F6 | (152,168) | (297,284) |
| Sand 970..972 | Same hourglass routine | (160,164) | (313,313), (313,312), (313,313) |
| Absence/marry/hail cards 42..44 | CODE 3:$0D42/$0DB2/$2644 | (24,25) | (71,51) |
| Credits card 45 | CODE 3:$0E3E | (24,26) | (66,50) |

The flame X values come from initialized A5 globals at -$56E8. Expanding
DATA/ZERO as CODE 1:$005A..$006A does recovers the six words
`9,36,6,2,1,6`: left/right xh, xl and initial frame. Native flame selection
uses the three available poses while retaining SDLPoP's existing two torch
RNG calls and frame scheduling. No guessed flame offsets are used.

The foreground pillar uses the recovered original anchor. It is a separately
drawn source overlay; it is not asserted to be pixel-identical to the pillar
already baked into background 951. Whole-Mac motion/timing equivalence and
original star placement are outside this audit.

Reproduce source evidence with:

```bash
python3 tools/mac_native_renderer/audit_cutscene_sources.py \
  --output screenshots/v15.0/cutscene-source-evidence.json
```

## Verification

Build passes with `make -C src -j2 CC=/usr/bin/gcc`. A separate normal-executable
startup under SDL dummy video/audio stayed running until a deliberate
three-second timeout (exit 124); only the unavailable haptic-device warning
was reported. This is a startup check, not a visual playtest.

Full automated regression passes:

```bash
python3 tools/mac_native_renderer/test_climb_overlay.py \
  --floors --architecture --palace --props --cutscenes \
  --output screenshots/v15.0
sha256sum -c MAC_NATIVE_SHA256SUMS.txt
```

- 1,093 cutscene checks: source PNG composition, all hourglass/sand states,
  both sprite variants, native loader scope, four story cards, full-screen
  presentation, and buffer restoration for title/gameplay entry paths.
- 459 actual sequence ticks cover Princess, Jaffar, mouse, embrace, ending and
  expired-hourglass poses. Every tick checks consecutive versus clean rendering
  and preserves actor state, torch RNG and frame clocks across rendering.
  68 additional checks compare every opaque pillar pixel while actors overlap.
- Retained suites: 8,681 props, 1,523 palace/pickup, 5,503 architecture and
  1,050 floor checks. Total counted checks: **17,850**, zero failures, plus
  the retained climb/fall checks.
- All **409 shared PNG captures** from v14.0 have identical decoded RGB pixels
  and dimensions. Newly named cutscene captures are compared separately with
  their independent source composition.
- Both native PAK checksums still match `MAC_NATIVE_SHA256SUMS.txt`.

Evidence is in `screenshots/v15.0/`: `audit-results.txt`, `build-results.txt`,
`startup-results.txt`, `v14-regression-comparison.json`, source evidence,
individual scene captures and `cutscene-comparison.png`. The comparison's
before image is a saved pre-change runtime capture; the after image uses the
new renderer. Story-card before captures use the retained stable-pack drawing
branch with the same loaded art.

## Live playtest

Close the running game and launch `./prince` again. A running program keeps
the build it loaded at startup.

1. Let the title sequence run: check the story text, Princess/Jaffar scene,
   hourglass, attached flames and foreground pillar. Look for trails or jumps.
2. Watch later Princess/mouse interludes and the ending when available. Check
   actors passing the right pillar and the final embrace.
3. Check the expired-time scene and return to gameplay: the room and HUD should
   redraw normally, without remnants from the bottom of the cutscene.

V15 live confirmation remains open, along with previously tracked v14 props,
v13.1 edge fixes, the v12.3 linked-room edge case and original-Mac motion review.
Earlier v13.0, v12.1 and v12.2 user confirmations remain recorded separately.
