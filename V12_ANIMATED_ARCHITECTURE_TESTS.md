# v12.3 Animated Architecture — IN PROGRESS

Current game baseline is v13.0. This document retains the v12.3 regression
record. All 5,503 architecture and 1,050 floor checks still pass with the new
palace wall compositor; palace background pixels intentionally change. See
V13_PALACE_RENDERER.md. Live v12.3 edge-case confirmation and full original-Mac
motion review were not inferred from the request to continue.


## Current combined slab/architecture pass — 2026-10-26

The user confirmed v12.2 good. The next integration pass found a linked-room
debris bug: a slab landing on normal floor in the left room's last column
could leave the old floor edge on the current screen. It was hidden while a
gate was actively redrawing the room. Native neighbor slab-state changes now
request visible-room composition. Gameplay/physics/assets are unchanged.

```bash
make -C src -j2 CC=/usr/bin/gcc
python3 tools/mac_native_renderer/test_climb_overlay.py --floors --architecture --output screenshots/v12.3
sha256sum -c MAC_NATIVE_SHA256SUMS.txt
./prince
```

| Current verification | Result |
| --- | --- |
| Architecture checks | 5,503 passed; 0 failures |
| v11 floor checks | 1,050 passed; 0 failures |
| Existing climb/fall checks | Passed |
| Individual captures shared with v12.2 | All 241 pixel-identical |
| Both native PAK checksums | Match baseline |

The 2,304 added integration frames cover four gate phases, three landing tile
types, four viewing arrangements, 24 simulation steps and both environments:

- Gates: opening, hold/countdown, slow closing and accelerating fast closing.
- Landing surfaces: normal floor, raise plate and drop plate, with real links
  from the plate to the gate and real `do_mobs()` / `process_trobs()` ordering.
- Views: same room, lower room while the slab arrives from above, camera
  following it across the vertical link, and a slab in the left linked room.

Across both environments, these include 880 full and 1,424 incremental frames.
Every frame is compared with a clean render, and the mob array must remain
unchanged by rendering. Each slab must finish falling into debris/stuck state.
Three further tests ensure linked native slab changes request refresh while
unrelated rooms and the DOS graphics mode keep their previous behavior.

The pre-fix audit found four failures (332 dungeon / 394 palace differing
pixels) in holding/closed gate scenes. `screenshots/v12.3/slab-edge-comparison.png`
shows the corrected debris overhang and was visually reviewed. That folder
also contains individual frames and `audit-results.txt`.

The combined integration checks are complete. Live confirmation of this
edge case and original-Mac motion comparisons remain before full v12 sign-off;
v13 palace rendering follows. Restart the existing `./prince` executable for
**Mac Native FULL v12.3 (Slab and room-edge redraw)**.

## Historical v12.2 mirror/chomper pass — user-confirmed

## Current mirror/chomper pass — 2026-10-26

The user confirmed all four v12.1 gate/exit fixes. This pass found and fixed
three further native-renderer issues:

1. The shadow's XOR pass was ignored by the physical blitter, producing another
   colored Prince. Native XOR now respects transparency, exact physical
   positions and the caller's clip while preserving destination alpha.
2. Mirror reflections did not always request their glass/frame foreground,
   allowing the reflection to overpaint those pixels when Kid moved into the
   next cell. Each native reflection now requests its own mirror-cell FORE.
3. The chomper's blood bit changes after tile animation, outside the original
   pose-change check. The actual death event now reports that native visual
   change, refreshing all blood art immediately.

No actor/gameplay coordinates, collision, health rules, animation timing or
native PAK assets were changed. The shadow retains SDLPoP's existing two-pass
effect; this is not original-Mac reference sign-off.

```bash
make -C src -j2 CC=/usr/bin/gcc
python3 tools/mac_native_renderer/test_climb_overlay.py --floors --architecture --output screenshots/v12.2
sha256sum -c MAC_NATIVE_SHA256SUMS.txt
./prince
```

| Current verification | Result |
| --- | --- |
| Architecture checks | 3,196 passed; 0 failures |
| v11 floor checks | 1,050 passed; 0 failures |
| Existing climb/fall checks | Passed |
| Shared individual runtime captures vs v12.1 | All 181 pixel-identical |
| Both native PAK checksums | Match baseline |

The 642 added architecture checks cover:

- 24 pixel-level XOR cases: indexed/RGBA sources, transparent nonblack RGB,
  negative and odd positions, clipping, destination alpha/blend preservation
  and applying XOR twice to recover the original pixels.
- 28 standing/jumping mirror states, both facings and seven positions;
  consecutive redraws must equal clean renders without mutating Kid/Guard.
- A standing shadow, actual right-to-left mirror collision flag, reflected
  shadow handoff with original HP changes, and ten real shadow escape steps.
  The one-shot handoff checks state effects; following frames check pixels.
- 290 actor/chomper cases: 24 animation/walking steps in all three rows, both
  facings and environments, plus actual death/blood transitions. The latter
  first renders the closed pose, then triggers death, isolating blood changes.
- 288 moving-gate states at main-room right, linked-left and above-room edges
  across dungeon and palace graphics.

`screenshots/v12.2/mirror-chomper-comparison.png` was visually inspected. It
compares captured pre-fix shadow/reflection/blood cases with current renders.
Individual captures and `audit-results.txt` are in the same directory. These
are controlled fixtures, not recordings of a full live playthrough.

The user confirmed this mirror/shadow/chomper pass good. Combined
falling-slab/architecture transitions are covered by the v12.3 section above.
Menu label: **Mac Native FULL v12.2 (Mirror and chomper redraw)**.

## Historical v12.1 gate/exit corrections — user-confirmed

## Current playtest corrections

The user's six screenshots exposed four issues beyond the v12.0 room-state
audit. All four have code fixes and targeted checks; the user confirmed all four fixed.

| Report | Cause and correction |
| --- | --- |
| 1: shadow strip below gate | FORE repeated the baked floor wedge over the finished floor. Only moving bar images now enter gate FORE, following legacy draw ordering. |
| 2: room change postponed | Every architecture redraw reset the room-exit timer to 2. Native same-room refresh now preserves the timer/input buffers; actual room changes take priority over full redraw requests. |
| 3–5: black exit-header strip | CELL_TOP clipped 13 dungeon / 16 palace physical pixels below the panel's actual top. Use its closed SHPD rectangle; phase 0..43 now retracts the whole native panel height while preserving gameplay timing. |
| 6: stair actor disappears early | Native drawing omitted the clip bounds used for frames 224..228. Native clip lookup now derives the bounds from the exit beside this actor, independently of stale globals. |

The final stair poses remain visible as the Prince climbs inward, then become
progressively occluded by the right jamb. In the canonical dungeon capture,
parts remain visible through frame 227; frame 228 is fully behind the jamb.
This preserves the original actor sequence and positions.

```bash
make -C src -j2 CC=/usr/bin/gcc
python3 tools/mac_native_renderer/test_climb_overlay.py --floors --architecture --output screenshots/v12.1
sha256sum -c MAC_NATIVE_SHA256SUMS.txt
./prince
```

Current results: **2,554 architecture checks, 0 failures; 1,050 floor checks,
0 failures**, plus the existing climb/fall suite. All 20 saved climb/fall
frames are pixel-identical to v11.19; both native PAK checksums match.

New coverage includes all 48 gate poses with actor-priority FORE in both
environments; real stair sequence 217..228 and level-sequence completion in all
rows/environments; visibility checks for the formerly lost poses; deliberately
invalid old clip globals; native timer/input preservation; a simultaneous room
change and full redraw request; and actual room exits in both horizontal
directions during gate animation. The real game redraw/room-exit routines are
used for timing checks. The existing short dark room-transition effect remains.

The generated `screenshots/v12.1/architecture-overview.png` and complete
`exit-stairs-sequence.png` were visually reviewed. The directory also contains
individual dungeon/palace frames and `audit-results.txt`. User screenshots are
preserved under `screenshots/development-audits/v12.1-reported/`.

The four reported scenes are user-confirmed fixed. At v12.1, remaining milestone
scope includes mirror reflection/jump-through/shadow, actor/chomper interactions,
combined room-edge/falling-slab scenes, and original-Mac motion comparisons.
v12 remains open. The menu label is **Mac Native FULL v12.1 (Gate and exit playtest fixes)**.

## Historical v12.0 audit (exit clip/lift superseded above)

The first rendering pass is implemented. v12 is open for live gameplay and
reference comparisons. The completed v11.19 floor milestone remains closed.

## Fixed in this pass

- Native moving gate slices, exit panels and chomper poses could retain old
  pixels outside the DOS redraw region. Visible native pose changes now
  recompose the buffered room. Gate hold/substeps and unrelated rooms avoid
  unnecessary full recomposition; other graphics packs retain their path.
- Mirror appearance/removal now clears its full native extent, including the
  level-4 gameplay spawn while its triggering exit is in another room.
- The native exit panel lifted above the carved frame. It now moves behind
  the fixed lintel, using native CELL_TOP and SHPD-derived closed bounds.
  Passage, moving panel and stairs stay in BACK during airborne overlays.

Gameplay state progression, collision, actor positions, animation timing and
native PAK assets are unchanged. No exit-motion rescaling was introduced.

## Reproduce

From the source folder:

```bash
make -C src -j2 CC=/usr/bin/gcc
python3 tools/mac_native_renderer/test_climb_overlay.py --floors --architecture --output screenshots/v12.0
sha256sum -c MAC_NATIVE_SHA256SUMS.txt
```

The runner links a separate temporary test executable against the game object
files and uses SDL dummy video/audio. `prince` remains the normal playable
program. Synthetic room fixtures call the real tile animation and renderer;
each incremental result is compared pixel-for-pixel with a clean render of
the same state. The fixture honors the normal full-redraw signal.

## Results on 2026-10-25

| Check | Result |
| --- | --- |
| Architecture audit | 2,374 checks, 0 failures |
| v11 interactive-floor audit | 1,050 checks, 0 failures |
| Existing climb/fall placement, mask and ordering checks | Passed |
| Saved climb 135..149 and fall 102..106 vs v11.19 | All 20 pixel-identical |
| Both native PAK checksums | Match preserved baseline |
| Three-gate room, 200 offscreen rebuilds | Mean 0.308 ms on this host |

Architecture coverage includes dungeon and palace; all three room rows;
gate opening, slow closing and accelerating fast closing; entrance/exit doors
opening and closing; bloody and clean chomper cycles; mirror addition/removal
and the actual level-4 spawn event. Tests also check gate modifier progression,
exit-panel clip bounds and restoration of the SDL clip, and that extra redraw
scheduling stays native-only, pose-based and scoped to visible rooms.

The initial audit reproduced 982 mismatches in 2,358 redraw comparisons before
the fixes. The final count adds panel clipping, mirror-spawn and scheduling
checks. Clean-versus-incremental equality detects stale pixels and inconsistent
layering; it cannot establish fidelity to an original Macintosh recording.
The benchmark measures buffered room composition only, without presentation
or live gameplay, and is not a whole-frame performance guarantee.

Final captures: `screenshots/v12.0/architecture-overview.png`, individual PNGs
and `audit-results.txt`. Earlier experiments are under
`screenshots/development-audits/`. The overview was visually inspected.

## v12.0 pending list (historical; see current status above)

1. Walk/crouch/roll under opening and closing gates in both directions; inspect
   actor depth and gate tips without changing existing collision behavior.
2. Open an exit, approach it, climb the stairs and complete the level. Check
   the entering actor's clipping and the native path's exit clip coordinates
   (`leveldoor_ybottom` / `leveldoor_right`), which this room audit does not cover.
3. Trigger the level-4 mirror; inspect reflection, jumping through it and the
   shadow transition. The audit covers environment appearance, not these actors.
4. Inspect chompers with an actor nearby, including a death/blood transition.
5. Exercise animated architecture at horizontal/vertical room boundaries and
   together with falling slabs. Existing slab/boundary checks remain v11
   regressions; combined architecture/actor interactions need playtesting.
6. Compare gate/exit/chomper motion and static anchors with original Mac footage.

For the updated playable build, close any older game window and run `./prince`
in this same source folder with `graphics_pack = mac_native_full`. The menu
identifies **Mac Native FULL v12.0 (Native architecture redraw)**.
