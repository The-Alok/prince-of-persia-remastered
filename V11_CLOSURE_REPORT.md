# v11 interactive floors — CLOSED at v11.19

Date: 2026-10-25. Mode: `mac_native_full`.

The user confirmed the climb and ledge-fall fixes, then authorized checking
and closing the remaining v11 work. The local runtime renderer audit is
complete. One additional shared redraw seam was found and fixed. The final
build passes all checks below.

## Floor audit

Each animated/changed tile is rendered through the game's incremental redraw
path and compared pixel for pixel with a clean render of the same state.
The harness links the real game objects, loads the actual native PAKs, and
runs SDL software rendering with dummy video/audio drivers. Comparisons cover
the whole offscreen room, so artifacts outside the changing tile also fail.

| Area | Checks | Coverage | Result |
| --- | ---: | --- | --- |
| Loose floors | 174 | Shake states, first moving slab, subsequent falls; exact base/front/right position agreement at the static-to-moving handoff | PASS |
| Spikes | 138 | Complete raise/hold/retract cycles on all three rows | PASS |
| Pressure plates | 150 | Raise/drop press and release, stuck plates; empty/floor/raise/drop/loose neighbors with independent event timers | PASS |
| Debris | 66 | Floor-to-debris, debris with torch, torch animation redraws; all rows | PASS |
| Normal floor seams | 72 | Four floor modifiers; first, middle and last columns; all rows | PASS |
| Viewport/room boundaries | 450 | Spike cycles at left/right edges and overhangs entering a linked room; a moving slab crossing into the room below | PASS |
| **Total** | **1,050** | Both dungeon and palace native graphics | **PASS — zero failures** |

Representative runtime captures were visually reviewed: loose-floor handoff,
raised/retracted spikes, pressed plates, debris/torch and palace plate.

![Representative floor captures](screenshots/v11.19/audit-overview.png)

## Additional regression results

- Native dungeon/palace climb masks still register exactly with their floor-top artwork.
- The existing 192 climb-overlay and 120 mask-edge cases pass.
- All 40 climb/fall actor-placement checks pass, including mirrored origins and unchanged Kid state.
- Consecutive climb frames 138..146 retain clean pixels at all old outline locations.
- All 42 airborne-overlay cases and the previous-cell actor ordering checks pass.
- Consecutive fall frames 102..106 preserve the correct floor/pillar occlusion.
- All 20 canonical climb/fall captures are pixel-identical to v11.18.
- Both native PAKs still match `MAC_NATIVE_SHA256SUMS.txt`.
- The normal playable `prince` executable was rebuilt successfully.

## Issue found during closure

Dungeon wall-corner resource 362 extends one native pixel below the floor
front: height 25, origin_y 22. In a full room composition the lower wall's
foreground covers that row. A partial redraw of the upper floor repainted the
corner over the lower wall, changing 46 pixels in that border. This appeared
in 50 audit transitions involving spikes, plates and debris beside a gap.

v11.19 clips only that native corner draw at the existing floor-front boundary
(`physical_y + origin_y + 2`, exclusive), preserving its SHPD position and
restoring the caller's SDL clip afterward. The clip is recorded on the draw
entry only when the current column has a real wall foreground below; synthetic
left-level borders retain their original pixels. A lower wall-side redraw
also requests the higher-priority corner from the tile above in the same
bottom-to-top pass, preserving the border's overlap order. The below-row cache
now includes all ten current columns, including the rightmost edge.

The palace corner already ends at this boundary. The fix leaves gameplay,
animation timing, actor positions, assets and full static composition
unchanged. All previously failing cases now match clean renders.

## Reproduce

From this source folder:

```bash
make -C src -j2
python3 tools/mac_native_renderer/test_climb_overlay.py --floors --output screenshots/v11.19
sha256sum -c MAC_NATIVE_SHA256SUMS.txt
```

If the system compiler cache requires sandbox access, the local compiler can
be selected explicitly: `make -C src -j2 CC=/usr/bin/gcc`.

The test executable is temporary and has no hooks in the playable game.
Fixtures, captures and comparisons do not change saved games or source PAKs.
Raw results: `screenshots/v11.19/audit-results.txt`.

## Milestone scope

This closes the local native interactive-floor renderer milestone, including
the user-confirmed climbing and falling defects. The audit validates rendering
consistency and component placement; it is not a fresh comparison of every
state against original Macintosh footage. Complete palace-wall authenticity,
the full cutscene/UI suite, every level, and device-specific performance
remain assigned to their later milestones. v12 animated architecture is next
and has not started.
