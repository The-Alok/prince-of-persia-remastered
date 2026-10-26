# SDLPoP Macintosh 640x400 Renderer — Project Handoff

## Mission

Recreate the Macintosh COLOR presentation of Prince of Persia inside SDLPoP while keeping SDLPoP's proven gameplay, timing, collision, replay, and level-state logic intact.

The target architecture is:

```text
SDLPoP 320x200 gameplay / collision / timing
                +
Macintosh-native graphics metadata and composition
                ↓
true 640x400 framebuffer
```

This is no longer a simple graphics pack. `mac_native_full` is a Macintosh-aware rendering mode.

## Current baseline: v1.0.0 — Windows/Linux public release preparation

The GitHub project is now a standalone repository under
The-Alok/prince-of-persia-remastered. At the user's request, it left the
NagyD/SDLPoP fork network on 2026-10-26. Git commit history, SDLPoP attribution,
original license/copyright headers, the existing star, and v1.0.0 release/assets
were preserved. GitHub reports fork=false and no parent. The README/description
now lead with the Macintosh HD port, native 640x400 renderer and 2x sprite
support; see CREDITS.md for the engine and graphics-reference authors. The public
version is 1.0.0; Windows and Linux package builders and complete attribution
are in tools/release/, CREDITS.md and README.upstream.md. Runtime downloads
contain data/libraries/notices; exact source is published at the matching tag.
See doc/RELEASE_VALIDATION.md for platform evidence and limits. Original disk
and extracted application resources remain local and are excluded from Git.

Window title/welcome now use Prince of Persia Remastered v1.0.0, with
Macintosh-version text, controls and a short SDLPoP credit. Display branding
is defined separately from SDLPOP_VERSION in config.h. Startup capture:
screenshots/v18.0/welcome-remastered.png. Keep the existing dismissal flow.

V18 consolidates the live native-full predicate, fixes the missing-DAT stat read,
temporary SHPL palette and empty-directory leaks, and converted-audio ownership.
Decoded audio samples are unchanged; free an active sample only after detaching
it from the SDL callback. Necessary v11 exceptions and original v17 sword data
are preserved. See V18_RELEASE_PREPARATION.md and PAK_NATIVE_ORIGINS.md.

At the user's request, the final menu order is Mac Native FULL v1.0.0
(default/index 0), DOS, SNES 1 (former Mac Prince/SNES Alt 2), SNES 2 (former
SNES Alt), SNES 3 (former SNES). Internal IDs/filenames remain stable; INI
startup stays mac_native_full.
Removed Mac Color/Mac Native PAKs are reference-only under
tools/mac_reference/legacy-packs/; builders/validator/hash manifest follow them.
The menu label is Mac Native FULL v1.0.0. The local archive in dist/
has passed file checks, an extracted-source rebuild and separate headless
launcher startup (screenshots/v18.0/package-verification.json). The build has zero warnings;
26,275 visual checks plus retained climb/fall and 237 lifecycle checks pass.
All 1,157 shared v17 PNGs are unchanged. Focused Memcheck reports zero errors
and zero definite/indirect leaks, with possible/reachable allocations separately
recorded. Native source packs remain byte-identical. Release packaging remains local;
Steam Deck testing, full played run and outstanding live checks remain open.

## V17.0 — original sword data and world audit complete

The user confirmed v16 damage/healing, pause/resume and intro fades, then
requested an original-Mac sword audit before v17. Original DATA/ZERO recovers
51 sword entries at A5-$4CCC; CODE3:$583E..$58C6 converts body actor X to screen
space before adding native X/Y attachment offsets. All 282 Kid/guard frame
records match the original. Native-full now uses generated mac_sword_table.h
and that original conversion order; v10 guessed calibration tables are removed.
Other packs retain the DOS attachment path. See MAC_SWORD_REFERENCE_AUDIT.md.

V17 covers 14 bundled levels, 332 populated room records, 1,328 real-map
animation steps and 14 actual entry frames. It found/fixed palace exit-left
res237's one-column backward overhang during adjacent torch refreshes and
dungeon big-pillar-top FORE res291's one-row cap above the next cell. Both
repairs restore native drawing dependencies; art, origins, gameplay, collision,
AI, timing, sequence data and replays stay unchanged. Preserve the scoped cap
FORE clip and the previous v11 wall-corner clip.

Build/full suite pass: 4,368 combat + 3,362 world + 18,545 earlier counted
checks = 26,275, plus retained climb/fall. There are 322 independent source
combat composites, both facings and five X phases/edges, plus valid sequence
playback. Pre-fix combat fails 2,874 static checks; removing only the two new
redraw dependencies fails 48 real-map checks. All 489 shared v16.0 PNGs are
unchanged, and both native PAK hashes match. Normal-executable headless startup
passes separately. New combat/edge fixes still need live confirmation.

See V17_WHOLE_GAME_VISUAL_REGRESSION.md and screenshots/v17.0/ for limits,
captures, source evidence, the 27-category scene index and playtest instructions.
V17 implementation/automated audit is complete; v18 cleanup/release is next.
This is not a complete played run or blanket original-Mac motion certification.

## v16.0 — HUD/UI audited; reported live checks user-confirmed

The user confirmed damage/healing, pause/resume and intro fades work as intended.
Other retained manual checks and original-Mac UI comparisons remain separate.

V16 corrects the native-full Kid full/empty flask indices: original SHPD image
217/218 means resources617/618, zero-based slots216/217. The old extra slot
selected empty618 for full and actor619 for empty. Native icon pixels now
match source art; HP, damage/healing and blink scheduling are unchanged.

Native RGB fades use the supplied fade-in source and full-screen static fade-out
snapshots rather than the 640x384 gameplay buffer. Cutscene fade-out keeps the
live animated source. RGB formats/pitch, alpha, physical clipping and interrupted
restoration are handled independently. The existing 64-step/four-level RGB curve
and wait calls are retained; native completion follows RGB progress instead of
unrelated DOS palette rows. Other packs retain their previous paths.

Build and full suite pass: 687 UI + 17,858 earlier counted checks = 18,545,
plus retained climb/fall. All 467 shared v15.1 PNGs are unchanged; new HUD/fade
captures cover these fixes. A pre-rebuild v15.1 negative probe failed 137 checks.
Both native PAK checksums match. Separate normal-executable headless startup
ran until its controlled timeout. See V16_HUD_AND_UI.md and screenshots/v16.0/
for coverage, source evidence and live checks. SDLPoP font/menu design is
retained; whole-Mac UI/motion fidelity is not asserted. V17 whole-game visual
regression is next. Prior live confirmations/pending checks stay separate.

## v15.1 — story background corrected, live playtest pending

The user found the intro story panel black instead of blue. Native SHPD40's
frame centre is transparent; the DOS opaque frame draw exposed its stored
black RGB. STORY_FRAME now fills the native target before its transparent art:
intro RGB(16,0,96), #100060 (original CLUT23, CODE3:$0D3A/$0DAA -> $0ED6).
The existing ending selector keeps RGB(128,0,0), #800000 (CLUT16/$263C).
The main title's backdrop, text anchors, gameplay and source assets stay intact.

Build and full suite pass: 1,101 cutscene plus 16,757 earlier counted checks
(17,858 total), plus retained climb/fall. Of 457 PNGs shared with v15.0, only
the four story cards change. Both native PAK checksums match. Independent
fixtures now cover both story colours, border pixels and title-background
isolation. See V15_CUTSCENE_COMPOSITION.md and screenshots/v15.1/.
Live confirmation remains pending; v16 HUD/UI is next.

## v15.0 — cutscenes implemented, live playtest pending

The cutscene pass restores the native PV prop group 960 (962..975), separate
from background 951. Native-full no longer overlays the DOS bed or uses DOS
hourglass/pillar frames. Flames, hourglass/sand and the foreground pillar use
original CODE 3 anchors with SHPD origins. Native frames recompose the retained
background, clearing transparent animation trails while preserving star colours,
existing torch RNG calls, actors and cutscene scheduling. Cutscenes own all
640x400 pixels; a borrowed 640x384 gameplay buffer is restored on return.
Story cards use their native insets and text pixels. Other packs keep their
existing paths. Source artwork, disk, PAKs and gameplay are unchanged.

Build and automated suite pass: 1,093 cutscene, 8,681 props, 1,523 palace/pickup,
5,503 architecture and 1,050 floor checks (17,850 total), plus retained
climb/fall. All 409 PNGs shared with v14.0 are unchanged. Native PAK checksums
match. A separate headless startup stayed running until its controlled timeout.
These are automated checks, not live visual confirmation. See
V15_CUTSCENE_COMPOSITION.md and screenshots/v15.0/ for source evidence,
comparison images and playtests. V16 HUD/UI is next. Earlier v14/v13.1/v12.3
live checks and original-Mac motion review remain open separately.

## v14.0 — props implemented, live playtest pending

The requested prop audit found four renderer defects: potion effect bits were
read using Mac encoding instead of SDLPoP encoding; the DOS-sized sprite table
left native green/blue bubble frames unloaded; a native dungeon skeleton edge
could repaint one neighbouring pixel; and native lattice bypassed the existing
high-jump foreground rule. Native art selection, group-150 loading (33 images),
skeleton neighbour refresh, and lattice FORE routing now correct those paths.
Gameplay, pickup effects, collision, timing, origins, source art and PAKs stay
unchanged. The life bottle is tall and has no bubbles, per the original Mac
CODE 4:$41BC/$42F8; feather/invert use green and hurt/open blue bubbles.

Build and full suite pass: 8,681 props, 1,523 palace/pickup, 5,503 architecture,
1,050 floor (16,757 total), plus retained climb/fall. All 288 captures shared
with v13.1 are unchanged. Both native PAK checksums match. The actual skeleton
wake and lattice opaque-pixel occlusion are checked; a temporary old-lattice
probe fails 25 checks, while the corrected build passes. See
V14_PROPS_AND_PICKUPS.md and screenshots/v14.0/ for coverage, evidence, source
asset limits and playtest instructions. V14 implementation/automated audit is
complete; live playtest remains. V15 follows in the current-baseline section. Earlier
unconfirmed v13.1/v12.3 live edge checks and original-Mac motion review remain
tracked separately.

## v13.1 palace implementation/audit complete

The user confirmed v13.0 works as expected. Final fake-wall/linked-room checks
found a native palace wall stripe crossing two cells: refreshing either could
erase or repaint the other's decoration. `mac_expand_background_redraws()`
expands those background requests before the row pass, preserving BACK order.
It uses visible fake-wall state and respects the no-blue bit; FORE/actor-only
requests keep their semantics. Real pickup tests also found a two-pixel native
sword outline in the next floor cell. Sword refreshes restore that cell, and
`do_pickup()` erases/restores the old sword edge before its state disappears.
Both changes are native-only and preserve gameplay and assets.

Final build/checks: 1,523 palace/pickup, 5,503 architecture and 1,050 floor,
plus retained climb/fall checks; zero failures. All 279 captures shared with
v13.0 are pixel-identical. Native PAK hashes are unchanged. See
V13_PALACE_RENDERER.md and screenshots/v13.1/ for evidence and exact coverage.
V13 implementation/automated audit is complete; new v13.1 fixes still need
live confirmation. Next is v14 props/pickups. Broader original-Mac comparisons
and the separately unconfirmed v12.3 live edge case remain release tasks.

## v13.0 original palace compositor — user-confirmed

The user supplied Mac-Color-Native-Graphics/ and then original PoP1_spf.dsk.
Read-only extraction recovered original CLUT 2000 (201 entries), data/resource
forks and CODE segments. All 820 source PNGs/origins independently match the
disk (3,835,931 pixels). Both existing native packs rebuild byte-identically.

Native palace walls now use the recovered room-color generator (CODE 3:$0B72),
private Mac RNG (CODE 2:$223E), fills/detail composition (CODE 4:$5CB6/$5EE6),
and original CLUT 76..83. Gameplay random_seed, coordinates and timing remain
unchanged. Native RGB fill entries use mode 2 in BACK/MID/FORE; sprites stay
modes 0/1. Foreground capacity is 600 for dense wall scenes. The established
three-logical-pixel native registration bridge remains. The missing opaque
wall faces are restored behind their original SHPD crack/detail overlays.

Build and checks pass: 200 palace, 5,503 architecture and 1,050 floor checks.
Four complete wall scenes match independent pixel fixtures. All 124 shared
dungeon/climb/fall captures remain pixel-identical; 100 shared palace captures
change intentionally. Source disk and native PAK checksums remain unchanged.
See V13_PALACE_RENDERER.md and screenshots/v13.0/ for exact source locations,
recovery/build commands, validation, comparison captures and pending scope.

The user confirmed this baseline works as expected. The v13.1 section above
records the completed fake-wall/boundary audit and remaining live/reference
limits. V12 regressions are retained.

## v12.3 regression baseline — retained

The user confirmed v12.2 good and authorized continuing the plan. The combined
falling-slab/architecture audit adds four gate phases (opening, holding, slow
closing, fast closing), three landings (floor, opener, closer), four views
(same room, lower-room view, following the slab across a vertical room link,
and a slab in the linked room on the left), 24 real simulation steps each,
and both environments. It calls `do_mobs()` then `process_trobs()` in game order;
plates use real trigger links to the gate. It compares consecutive frames
with clean composition and asserts rendering leaves the mob array unchanged.

This reproduced four failures: when the gate was holding/stopped and a slab
landed on normal floor in the left room's last column, the new debris edge
was not refreshed in the current view (332 dungeon / 394 palace pixels).
An actively moving gate happened to conceal the problem by recomposing the
room. `redraw_at_cur_mob()` only handled changes within `drawn_room`.

For native linked-room slab changes, that helper now requests visible-room
composition. This covers the actual overhang without interpreting the other
room's tile index as a current dirty-cell index. Its existing same-room path,
DOS/other-pack behavior, unrelated-room behavior, physics and the room-crossing
Y bridge remain unchanged. Three scope tests cover those exclusions.

Final build and audits pass: 5,503 architecture checks and 1,050 floor checks.
The 2,304 new combined frames exercise 880 full and 1,424 incremental redraws;
all 241 individual captures shared with v12.2 are pixel-identical. Native PAK
checksums match. Captures/log: `screenshots/v12.3/`; the visually reviewed
`slab-edge-comparison.png` shows the old edge versus the landed debris.
The combined integration audit is covered; v12 stays open for this edge-case
playtest and original-Mac motion comparisons. v13 palace rendering is next.

## v12.2 baseline — user-confirmed mirror/chomper fixes

The user confirmed all four v12.1 issues fixed and authorized continuing.
The next pass audited mirror reflection, jump/shadow handoff, actor/chomper
redraws, actual chomper blood/death and linked gate edges.

Three renderer defects were reproduced and corrected:
- Native physical blits ignored `blitters_3_xor`, drawing the shadow as a second
  colored Prince. `draw_native_xor()` now XORs RGB in the clipped physical
  rectangle, preserving target alpha and ignoring transparent source pixels.
  It handles indexed and RGBA input, negative/odd origins, caller clips/blend
  state and logical dirty rectangles. The existing two-pass shadow/union
  operation, native anchors and gameplay state are retained.
- A reflection did not request its mirror-cell FORE. When Kid's body was in
  the next cell, clean and consecutive draws differed at glass/border pixels.
  Native reflection submission now assigns its mirror cell and requests that
  foreground each time. Reflection transform and actor positions are unchanged.
- Chomper blood is set by `chomped()` after `process_trobs()`, so the v12 pose
  check could not observe that transition. `mac_architecture_changed()` is
  shared with this event and requests native visible-room composition on the
  actual blood change. The before/clean death fixture differed by 889 pixels
  in each environment. Health, death animation and collision are unchanged.

Build and tests pass: 3,196 architecture checks and 1,050 floor checks. All
181 captures shared with v12.1 are pixel-identical, including the complete
climb/fall and stair sequences. Both PAK checksums match. The 642 added checks
include 24 XOR cases, 28 reflection states, standing shadow, actual jump
handoff plus ten real shadow steps, 290 actor/chomper cases and 288 linked gate
edge states. A one-shot mirror handoff is tested by its real flag/health/state
effects; subsequent stable frames use clean-versus-incremental pixel checks.

The initial test incorrectly compared the handoff-consuming frame with a
second draw after its flag had been consumed; that was a fixture difference,
not a gameplay defect. The death fixture also now renders the already-closed
chomper before triggering death, isolating the real blood transition. Preserve
these distinctions when extending the tests.

Captures/log: `screenshots/v12.2/`, including the visually reviewed
`mirror-chomper-comparison.png`. The user confirmed this pass good. The combined
slab/architecture pass follows above. No native assets were edited.

## v12.1 baseline — user-confirmed gate/exit fixes

The user reported four playtest issues with six screenshots: a dark diagonal
strip below a gate, delayed horizontal room changes, a black gap below exit
headers in dungeon/palace, and the Prince vanishing during the stair entry.

Corrections:
- `mac_draw_gate_parts(1)` now matches legacy FORE behavior: only moving bottom
  bars (0x33) and full slices (0x34). The previous shared BACK/FORE path submitted
  the baked floor wedge 0x2F in FORE when closed, and combined post piece 0x32
  when raised, plus the BACK-only top partial slice. The wedge repainted the
  floor after its top surface and left a dark strip after crossing the gate.
- v12.0 architecture recomposition called `redraw_screen(0)`, which reset
  `exit_room_timer` to 2 every time. A moving gate could keep room exits blocked
  until animation stopped. Native same-room refresh now preserves that timer
  and all four keyboard-buffer clearing sites. Real room-entry behavior and
  the existing brief dark transition remain unchanged. `different_room` takes
  priority over a simultaneous full-redraw request in `draw_game_frame()`.
- CELL_TOP is 13 dungeon / 16 palace physical pixels below the panel's closed
  SHPD top, so v12.0's clip cut a black gap below the lintel. The fixed clip now
  uses the closed panel's native top/bottom. Phase 0..43 maps to panel height
  (107 dungeon / 110 palace physical pixels), retracting fully at the original
  gameplay opening time. This supersedes v12.0's unscaled visual lift. Pixel
  motion is kept in physical coordinates, retaining odd native pixels.
- The native path did not populate `leveldoor_ybottom/right`, leaving stair
  frames 224..228 clipped against stale/zero bounds. Native drawing now sets
  them; `clip_char()` additionally resolves the actor's nearest exit in his
  room/row using `mac_get_leveldoor_clip()`, so another exit cannot overwrite
  his clip. The top follows the panel bottom and the right uses the opening's
  native bound. Earlier stair poses and all gameplay positions remain unchanged.

Verification: build passed; 2,554 architecture checks and 1,050 floor checks
pass. Added all 48 gate poses with actor-priority FORE; actual stair sequence
217..228 and its level-completion command in all rows/environments; poisoned
old clip globals; input/timer preservation; simultaneous room/full redraw;
and real left/right room exits while a gate animates. Captures show gradual
occlusion through frame 227 and full occlusion behind the jamb by frame 228.
All 20 climb/fall images remain pixel-identical to v11.19; native PAK checksums
match. Captures/log: `screenshots/v12.1/`; supplied references are preserved in
`screenshots/development-audits/v12.1-reported/`. The user confirmed all four fixes.
Mirror reflection/shadow and remaining architecture interactions keep v12 open.

## v12.0 baseline (superseded exit clip/lift; initial redraw pass retained)

The first animated-architecture pass is implemented and built. Gates, exit
doors and chompers now request buffered visible-room recomposition when their
native rendered pose changes. Native transparent/tall slices exceeded the DOS
dirty-tile regions; the initial audit reproduced 982 sequential-versus-clean
render mismatches across 2,358 checks. Mirror appearance/removal uses the same
recomposition, including the real level-4 event when the spawning exit is in
another room. Unrelated rooms, other graphics packs and gate substeps within
one rendered pose do not request this extra redraw.

Exit resource 233 is clipped to the fixed opening: the top is native CELL_TOP,
and the bottom derives from its closed SHPD anchor and image height. This
prevents the moving panel from rising above the carved lintel. Optional top
and bottom physical clip fields on BACK/FORE entries preserve the caller's
SDL clip. The passage, panel and stairs remain BACK even during generic
airborne overlays, following legacy `draw_leveldoor()` ordering. Opening
range 0..43, gate/chomper state progression and all actor coordinates remain
unchanged. Native assets are unchanged.

Final results: 2,374 architecture checks, 1,050 floor checks and the existing
climb/fall suite pass; the 20 climb/fall PNGs are pixel-identical to v11.19.
The three-gate offscreen rebuild benchmark averaged 0.308 ms here; this is
not an end-to-end frame-time measurement. Captures and the full log are in
`screenshots/v12.0/`. See `V12_ANIMATED_ARCHITECTURE_TESTS.md` for commands,
coverage and pending actor interactions, level-exit and mirror playtests.
v12 remains open. v11 remains closed at v11.19.

## v11.19 baseline — v11 COMPLETE

The user authorized checking and closing the remaining v11 cases after
confirming the climb and ledge-fall fixes. The closure audit passed 1,050
floor checks across dungeon and palace graphics, retained all climb/fall
regressions, and reviewed representative generated runtime captures.

One remaining issue was found and fixed: the dungeon below-left wall corner
(VWALL res362) extends one physical row below the floor front. A full room
draw covers that row with the lower wall's FORE, but partial upper-floor
redraws repainted it. This caused the same 46-pixel wall-border difference
during spike, plate and debris updates. The native physical back/fore draw
now clips that corner at `physical_y + origin_y + 2`, preserving the floor
front's existing boundary, the image origin, and the caller's clip. Palace
corners already end at the boundary. Static composition remains unchanged.
The clip applies only when the current column has a real wall FORE below.
Each back/fore entry records an optional native bottom clip; ordinary entries
initialize it to -1. The below-row cache has eleven entries so the current
tile below is available even at column nine. Native lower wall-side redraws
request the higher-priority corner above, preserving the full-room overlap
order and synthetic left-level border pixels.

The audit initially reproduced 50 failures attributable to this one shared
seam; all pass after the fix. Coverage includes static-to-moving loose-slab
component alignment, animation redraws versus clean renders, viewport edges,
linked horizontal rooms and a loose slab crossing into the room below.
See `V11_CLOSURE_REPORT.md` and `screenshots/v11.19/` for reproducible commands,
counts, captures and remaining whole-game scope. v11 is closed; v12 animated
architecture is now in progress above.

## v11.18 baseline (native generic airborne overlay restored)

Playtest status: the user confirmed the v11.18 ledge-fall fix after testing,
following confirmation of the completed climb fixes through v11.17. These
reported defects are closed. v11 still needs its broader interactive-floor
playtest matrix: loose-floor shake/fall and boundaries, spike cycles,
pressure-plate transitions/edge combinations, debris, and floor seams.
These are validation targets; no new defect in those cases is confirmed by
this session. The recommended next focused pass is loose floors, especially
the last shake frame -> first falling frame and row/room crossings.

The user confirmed the v11.17 climb fixes and moved to a fall-off-ledge bug.
The user explicitly confirmed the intended composition: the Prince should
disappear behind the pillar as he drops, rather than remain visible in front
of it. Preserve that depth order; do not move the actor around the pillar.
The five supplied stills show KID513..517 / frames 102..106. A canonical
fixture reproduces the trousers through the floor and the torso/foot overlap
with the pillar using unchanged actor coordinates.

Cause: native `redraw_needed()` folded `redraw_frames2` into a full BACK tile
redraw, losing the legacy `draw_other_overlay()` MID phase. Before this fix,
the frame-104 fixture had only the actor in MID; environment pixels therefore
could not occlude it at the ledge plane.

`mac_draw_other_overlay_exact()` now mirrors the legacy empty-left / empty-
two-columns-left conditions. It submits the native counterparts of
`draw_tile2()`'s components in MID, preserving SHPD anchors and floor-state
selection. For the second condition it also submits BACK components and sets
`tile_object_redraw` to 0xFF so the previous-cell actor is redrawn in the same
order as the legacy renderer. A scoped `mac_back_to_mid` routing flag lets the
existing native component helpers emit exact MID entries, then resets to zero.
This phase excludes foreground pieces and keeps full redraws, climb masks,
and pillar FORE as separate operations. No actor offset, gameplay, collision,
timing, or asset changes are involved.

Tests now include 42 airborne overlay cases (seven tile states, three rows,
both facing flags), the second-neighbor actor ordering, continuous-floor
non-overlay behavior, 40 climb/fall actor anchor checks, and clean versus
consecutive fall renders. The frame-104 actor must precede the floor in MID,
and its old white-trouser pixel at (236,114) must equal the native floor pixel.
The canonical fixture also probes the pillar seam. Existing climb checks are
retained. Captures are under `screenshots/v11.18/`. Playtest both ledge sides,
wall/pillar supports and room boundaries before declaring this subsystem closed.
The build and all tests passed. All 15 canonical climb captures are pixel-
identical to v11.17, and both native PAK checksums remain unchanged. The full
five-pose before/after sheet is `screenshots/v11.18/fall-sequence-comparison.png`.

## v11.17 baseline (frame-138 line and frame-144 dot covered)

The user playtested v11.16 and inspected its saved captures, then reported a
remaining vertical line in frame 138 and a black dot by the shoe in frame 144.
Both are additional one-column mask-boundary leaks. Extend
`mac_draw_climb_mask_edge()` to native res221 (frame 138, image index 0x14)
and res227 (frame 144, image index 0x1A). For res221 only, start the copied
floor column four physical rows below the mask top to preserve the visible
elbow/arm pixels. Res227 uses the full mask-height column. Other masks and
all v11.15 actor anchors retain their behavior.

The canonical fixture changes only (226,107..109) in frame 138 and (215,112)
in frame 144: three black pixels and one black dot become registered floor
pixels. All other 135..149 captures are pixel-identical to v11.16. Assets and
gameplay are unchanged. Build and prior regressions passed. New edge checks
failed on v11.16 before the fix; all 120 edge cases now pass, including the
preserved elbow rows and untouched adjacent column. Direct fixture checks
validate both marks. Consecutive renders 138..146 match clean captures at
all four old/new outline probe locations. Captures and comparison preview
are under `screenshots/v11.17/`; in-game motion remains for user playtest.

## v11.16 baseline (remaining climb outline column covered)

The user confirmed v11.15 was nearly fixed and reported a thin black vertical
line under the Prince. Pixel inspection of the captured native poses found
that masks res222..224 (frames 139..141) stop one physical column before the
DOS-positioned body's outline. In the canonical fixture the leaked columns
are x225 in frames 139/140 and x233 in frame 141. The matching frame-141 line
contains seven black pixels. This is an occlusion-width issue; do not move the
already calibrated v11.15 actor to hide it.

`mac_draw_climb_mask_edge()` runs immediately after those three exact native
MID masks. It restores a one-pixel-wide strip from native floor-top res241,
positioned using the mask/base SHPD origins. A temporary SDL clip limits the
draw to that strip intersected with the existing viewport, then restores the
clip. The original PAK images and their metadata remain untouched. Ordinary
floor seams, all actor anchors, v11.15 mask anchors, pillar foreground, and
gameplay are unchanged.

Validation: build passed; all prior mask/actor checks passed, plus 72 rendered
edge cases validating exact floor pixels, an untouched neighboring column,
and restored clip bounds. The new pixel test failed on v11.15 before the fix.
The canonical 135..149 captures differ from v11.15 only at 13 pixels in frame
139, 15 in frame 140, and seven in frame 141, each within a single column.
Consecutive animation (with peel restoration and no room reset) also matches
clean renders at both old line columns through the reported 144..146 finish.
Captures and before/after preview are in `screenshots/v11.16/`. User motion
playtest remains the final visual check.

## v11.15 baseline (native climb masks aligned with the floor top)

v11.15 corrects the remaining climb-mask Y anchor. All eight native masks are
pixel-identical cutouts of floor-top resource 241 when aligned by their SHPD
origins. Their anchor must therefore match the floor base, not the DOS overlay
baseline: `draw_main_y + 1 + MAC_FLOOR_TOP_ANCHOR`, plus one logical pixel for
`tiles_5_stuck`. The old anchor was 3 logical / 6 physical pixels too low.

The supplied port screenshot matches frame 141 / KID552 at (41,30) in the crop.
Recomposing the native floor, actor, res224 mask at the old anchor, floor front,
and pillar foreground reproduces every pixel in the actor/floor comparison
region. The reference depicts the adjacent pose, KID553 / frame 142. Keep the
`base + image_index + 1` resource-loading convention in mind when naming poses.
The corrected rendered comparison also exposed a body-anchor mismatch: after
aligning the native floor in the original Mac reference, KID553 is about six
native pixels above the port's pose. `draw_mid()` now lifts only the displayed
post-row-change climb phase (141..144 / KID552..555) by three logical pixels,
using the same floor-top bridge as the mask. It selects by native KID image
index (151..154), so both mirrored directions receive the same vertical lift.
Earlier hanging poses, later crouching/standing poses, Char/Kid coordinates,
animation, collision and the gameplay frame table are unchanged. This removes
the trousers entering the floor plane and the frame-142 foot emerging below
the floor lip in the rendered regression fixture. Full in-game motion still
needs user playtest, especially the 140->141 and 144->145 transitions.

v11.15 preserves v11.13 foreground depth and v11.14 mask selection.
`SDLPOP_MAC_FLOOR_DEBUG=1` now prints
`role=climb-overlay` with the selected mask resource and exact coordinates.

Validation: `python3 tools/mac_native_renderer/test_climb_overlay.py --output
screenshots/v11.15` validates all 16 dungeon/palace mask cutouts, 192 overlay
placements (eight frames, four tile types, three rows, both directions), and
30 body placements (135..149 in both directions), including unchanged Kid
state and neighboring pose anchors. The harness links actual game object
files into a temporary executable using SDL dummy drivers, and captures the
complete right-facing 135..149 sequence. The normal `prince` has no test hooks.

The project began from `PoP1_spf.dsk`. Macintosh SHPD/SHPT graphics were decoded and packed into SDLPoP PAK files. Native art retains its original dimensions, transparency, palette, and SHPD `origin_x/origin_y` metadata.

### Stable foundations already implemented

- 640x400 framebuffer with original gameplay coordinates retained.
- Legacy/DOS art scales 2x; native Mac art draws 1:1.
- Native SHPD origins are stored in the PAK as `org` sidecars.
- Room/environment compositor can place images in exact physical pixels.
- Macintosh-style environment draw order is used in `mac_native_full`.
- Raised-gate collision regression caused by physical sprite height was fixed.
- Native HUD flask mapping was fixed.
- Stale offscreen-buffer room ghosts were fixed.
- Room rendering is clipped above the HUD strip.
- Native actor body images use SHPD pivots with mirrored-origin support.
- Princess-room duplicated foreground pillar overlay was removed for native PV art.
- Princess-room flames have a native physical attachment path.
- Native title compositor is working: ornamental frame, transparent palace art, centered logo/text.
- v10.15 completed the actor/combat attachment pass.
- v11.0 introduced the state-driven interactive-floor compositor and origin-aware falling loose-floor path.
- v11.2 keeps those anchors unchanged and fixes native spike retraction ghost pixels by expanding only the spike dirty/wipe height (`0x21` → `0x25` logical pixels) for `mac_native_full`.
- v11.2 also expands pressure-plate state-change redraw coverage (`0x11` → `0x14` logical pixels) only in `mac_native_full`, preventing stale native top/right pixels from surviving a button transition and forming a doubled dark lip.
- v11.8 (Claude-corrected baseline) fixes the pressure-plate artifacts at their actual redraw source: the below-left corner is always restored for a stuck plate, while `draw_tile_wipe()` is clamped to `draw_main_y` in `mac_native_full` so button-state wipes no longer erase the wall row below the floor.
- v11.9 restored the missing climb-up midtable redraw semantics, and v11.10 separated floor-overlay and foreground redraw phases. v11.10-v11.12 then made an incorrect assumption and suppressed the native dungeon pillar foreground during pull-up. The supplied original Mac reference proves the opposite layer order: the pillar/wall face must remain in FORE and hide the Prince's lower leg/foot behind the masonry. v11.13 removes that suppression while keeping the useful overlay/foreground phase separation. Actor coordinates and collision remain unchanged.
- v11.14 replaces the remaining DOS climb-overlay wedges with the Macintosh-native eight-frame sequence at environment resources 220..227. Frame 143 now uses native `res226` instead of DOS `res232`; SHPD origins are honored through `mac_add_mid_anchor_exact()`. This removes the coarse grey slab that was covering the trousers while leaving v11.13 pillar foreground depth intact.

## v10 actor/combat change

SDLPoP actor X coordinates are internally based on a 280-wide coordinate space and are converted to 320-wide screen X with:

```c
calc_screen_x_coord(x) = x * 320 / 280;
```

Before v10, sword attachment did this:

```text
actor x280 → convert to x320 → add sword_tbl.x
```

That under-scaled the attachment distance relative to the native actor pivot. v10 now does:

```text
actor x280 → add sword_tbl.x in actor space → convert complete anchor to x320
```

The old Prince-only `-3/-1` render bias is removed. The corrected math applies uniformly to Prince, guard, skeleton-family actors and both directions.

Optional diagnostics:

```bash
SDLPOP_MAC_ACTOR_DEBUG=1 ./prince
```

This logs character ID, animation frame, sword frame/resource, direction, actor-space X, attachment X, final screen X, and Y.

## Coordinate models

### Environment/native piece placement

```text
physical_x = logical_anchor_x * 2 - origin_x
physical_y = logical_anchor_y * 2 - origin_y
```

Important Mac large/color anchors:

```text
CELL_TOP     = -63
FLOOR_TOP    =  -4
FLOOR_BOTTOM =  -1
```

### Mirrored actor origin

```text
mirrored_origin_x = image_width - 1 - origin_x
```

Gameplay positions are never rewritten into physical pixels.

## Remaining roadmap

### v10 — Actor + combat compositor  [COMPLETED at v10.15]

Goals:
- Correct sword attachment coordinate space.
- Validate Prince facing left/right.
- Validate guard swords.
- Validate skeleton-family actors.
- Preserve mirrored SHPD origins.
- Provide frame/resource debug logging.

Exit criteria:
- Sword touches the hand in normal guard stance, attack, parry, retreat and advance frames in both directions.
- Guards retain correct attachment.
- No HUD bleed or actor clipping regressions.

If only isolated frames remain wrong after the coordinate-space fix, add a **small Mac-specific per-frame exception table**, not a global offset.

### v11 — Interactive floor compositor  [COMPLETED at v11.19]

Audit separately:
- normal floor
- raise button
- drop button
- depressed/stuck button
- loose floor
- falling loose floor
- broken floor/debris
- spikes
- floor edge and overhang

Goal: eliminate pressure-plate double-width shadows, floor seams and animation jumps by using the correct Mac component/anchor for each tile state rather than broad Y corrections.

v11.8–v11.10 implementation status:
- Corrected v11.8 supersedes the v11.6/v11.7 stuck-plate corner experiment. A stuck plate still skips the generic `0x2A` black mask, but the below-left corner is always restored so transparent pixels under the `0x23` edge do not expose wiped black background.
- Corrected v11.8 clamps Mac-native tile wipes to the floor front (`draw_main_y`) instead of the DOS `draw_bottom_y`, preventing the pressure-plate redraw from erasing three logical rows of the wall below.
- v11.9 restored the missing climb-up midtable overlay for frames 137..144.
- v11.10 corrects its draw semantics: that overlay flag no longer forces `mac_draw_tile_exact()`, preventing native foretable pieces from being redundantly re-submitted; for the exposed dungeon pillar case, the tall pillar fore-strip is explicitly suppressed when the same tile is using the dedicated climb overlay, preventing it from over-occluding the Prince.

v11.5 implementation status:
- v11.3 corrects the exposed depressed/stuck-button edge: native Mac state 0x05 now keeps right component 0x23 beside empty/non-floor neighbors instead of inheriting the normal-floor 0x2A wedge. The old substitution produced the screenshot-confirmed double-width dark shadow.
- v11.4 restores the missing exposed-ledge black occlusion pass between the below-left corner and the state-specific right edge. Without that mask, the lower wall/corner diagonal remained visible beneath the pressure-plate wedge, creating the screenshot-confirmed doubled/double-layer shadow. The mask uses 0x2A only as a black silhouette; the visible depressed edge remains 0x23.
- v11.5 fixes the physical native-image draw path so `blitters_9_black` is actually honored. Before this fix, exact/native entries ignored their blitter and always alpha-blitted the source image, so v11.4's intended black `0x2A` occlusion silhouette appeared as a second colored floor wedge. Native physical black masks are now converted to an alpha-preserving black silhouette at the same 640x400 physical origin; all other native physical blits keep their previous alpha path.
- base/right/front IDs and anchors are grouped per floor state;
- exposed-edge duplicate floor wedges are removed in the native compositor;
- depressed drop-button edge selection is neighbor-aware;
- loose-floor static frames use the same state table;
- falling loose floors use SHPD-origin physical placement derived from the same anchor origin;
- `SDLPOP_MAC_FLOOR_DEBUG=1` logs floor component/resource/anchor/origin data.
- Runtime spike captures confirmed that the native assets/anchors are correct, but the DOS-era 0x21 wipe left 1–5 physical pixels of tall spike tips above the cleared band. v11.2 uses a 0x25 wipe only for Mac Native FULL spike animation; gameplay state/timing/collision are untouched.

The local Fedora renderer audit and runtime capture review completed at
v11.19; see `V11_CLOSURE_REPORT.md`. Device-specific and whole-game regression
coverage remains part of the later regression/release milestones.

### v12 — Animated architecture  [IN PROGRESS at v12.3]

Initial redraw pass, user-confirmed gate/exit corrections, mirror/shadow and
chomper redraw fixes are implemented; automated actor and linked gate-edge
checks pass and the user confirmed v12.2. Combined slab/architecture checks
also pass at v12.3. Its linked debris-edge playtest and original-Mac motion
comparisons remain open. See `V12_ANIMATED_ARCHITECTURE_TESTS.md`.

Audit:
- gates closed/open/transition
- exit door and stairs
- chompers
- mirror
- loose-floor fall
- moving gate slices

Goal: static and animated states must use the same native anchor model with no visual jumps or gameplay-coordinate leakage.

### v13 — Palace renderer [IMPLEMENTATION/AUTOMATED AUDIT COMPLETE at v13.1]

Original CLUT wall blocks and SHPD details are implemented, with independent
pixel fixtures and fake-wall/room-boundary checks. V13.0 is user-confirmed;
v13.1 edge fixes await live playtest. Original-Mac scene comparisons stay open
as release validation. See `V13_PALACE_RENDERER.md`.

### v14 — Props and pickups [IMPLEMENTED/AUDITED at v14.0; live playtest pending]

The full prop pass and fixes are recorded in V14_PROPS_AND_PICKUPS.md.
All 8,681 new checks pass; original artwork/PAKs and gameplay stay unchanged.
The earlier wall-adjacent pickup regressions remain in the v13 suite.

Audit:
- potions/bubbles
- sword pickup
- skeleton props
- torches/sconces
- debris
- balconies
- lattice

Goal: correct origin, composition and foreground/background layer for all room objects.

### v15 — Cutscenes [IMPLEMENTED/AUDITED at v15.0; live playtest pending]

Native prop mapping, baked-background composition, foreground depth, full-screen
buffers and story-card placement are corrected. All 1,093 cutscene checks pass,
including actual actor sequences and independent source PNG composition.
See `V15_CUTSCENE_COMPOSITION.md` for source locations, limits and live checks.

Audit each cutscene as its own composition instead of assuming DOS layer structure:
- Princess room
- Jaffar scenes
- hourglass
- story cards
- ending scenes

Important lesson: Mac backgrounds can already contain architecture/fixtures that DOS draws separately.

### v16 — HUD / UI [IMPLEMENTED/AUDITED at v16.0; live playtest pending]

Native flask selection and RGB fade sources/footer are corrected. All 687 new
UI checks pass, including native icon pixel fixtures, real pause/settings and
dialog composition, actual animated cutscene fades and the screen transition.
See `V16_HUD_AND_UI.md` for retained font/menu scope and manual checks.

Audit:
- Kid health flasks
- guard health
- pause menu
- bottom text
- transitions/fades
- screen-edge clipping
- copy protection/dialogs if enabled

### v17 — Whole-game visual regression [AUTOMATED SWEEP COMPLETE at v17.0]

V17 covers every populated room record across Levels 1–14 and real entry poses,
with retained transient-state fixtures and the requested source-data sword
audit. See `V17_WHOLE_GAME_VISUAL_REGRESSION.md`. New fixes await live playtest;
a complete played run and original-Mac motion comparison remain release tasks.

Create a repeatable screenshot suite across Levels 1–14. Minimum canonical scenes:

1. title screen
2. Princess room
3. Level 1 start
4. normal floor
5. raise/drop pressure plates
6. gate closed
7. gate opening
8. gate fully raised
9. loose floor static/falling
10. spikes
11. sword pickup
12. Prince fencing left
13. Prince fencing right
14. guard attack/parry
15. skeleton fight
16. mirror
17. chomper
18. potion
19. exit door closed
20. exit door opening
21. exit door open/stairs
22. palace columns
23. balcony
24. lattice
25. pause/HUD
26. room boundary transition

### v18 — Cleanup and release

- Remove obsolete diagnostic nudges/hacks.
- Consolidate Mac-mode checks/helpers.
- Document the PAK origin extension.
- Validate performance and memory management.
- Build Steam Deck/Fedora release package.
- Produce final regression screenshots and release notes.

## Engineering rules for future work

1. **Do not modify gameplay/collision to fix a visual problem** unless physical sprite dimensions have leaked into gameplay by mistake.
2. Prefer original Mac SHPD origins and composition order over screenshot-based offsets.
3. Keep special-case offsets narrowly scoped and temporary.
4. When a composite looks wrong, first verify whether the Mac background already contains that element.
5. Distinguish physical 640x400 pixels from logical 320x200 coordinates explicitly.
6. Validate both facing directions whenever actor mirroring changes.
7. Preserve previous regression scenes before moving to the next subsystem.
8. Do not interpret visually unusual Mac art as a bug without checking the original asset/reference composition.

## Build / test environment

The working ChatGPT container does not have SDL2 / SDL2_image development headers. Source patches and archive structure can be validated here, but final compilation/runtime testing is done on the Steam Deck/Fedora environment.

Build:

```bash
cd src
make clean
make -j$(nproc)
cd ..
./prince
```

Use:

```ini
graphics_pack = mac_native_full
```

For v10 actor diagnostics:

```bash
SDLPOP_MAC_ACTOR_DEBUG=1 ./prince
```

For v11 floor diagnostics:

```bash
SDLPOP_MAC_FLOOR_DEBUG=1 ./prince
```

## Primary source assets

- `PoP1_spf.dsk` — source Macintosh disk image.
- `res_mac_native.pak` — stable native graphics subset.
- `res_mac_native_full.pak` — full native renderer pack.
- `Mac-Color-Native-Graphics/manifest.csv` — extraction metadata used during development.

## Immediate next action for v11 playtest

Use `mac_native_full` and capture the focused cases in `V11_INTERACTIVE_FLOOR_TESTS.md`, especially:
- idle/pressed raise and drop buttons;
- depressed drop button beside floor versus beside an exposed edge;
- loose-floor final shake frame followed by first falling frame;
- falling loose floor crossing a row/room boundary;
- debris and debris-with-torch;
- spike idle/raise/fully-up/retract frames.

If a floor piece is still misplaced, rerun the scene with `SDLPOP_MAC_FLOOR_DEBUG=1` and save the `MAC_FLOOR` lines plus a screenshot. Correct the state/component anchor first; do not change gameplay coordinates.


## v10.3 pickup/sheathing finding

Frames 229..236 are a distinct attachment class, not ordinary fencing. They pair KID resources 560..567 with Mac sword resources 720..727. Their per-frame SHPD origins vary enough that the generic combat attachment leaves visible gaps. v10.3 adds a pickup-only mirrored correction table derived from native hand/hilt alignment. Normal combat math remains unchanged and still needs separate playtest coverage.

## v10.3 pickup recording finding

A Steam Deck recording plus `SDLPOP_MAC_ACTOR_DEBUG=1` exposed an off-by-one diagnostic assumption: `load_sprites_from_file(base)` stores image index `i` from resource `base + i + 1`. Therefore pickup frames 229..236 pair KID 561..568 with PRINCE sword resources 721..728, not 560..567 / 720..727. The v10.1 pickup table had been derived against the wrong pair labels.

The long-lived frame 229 was measured from the recording. Its sword hilt was about 15 native pixels left and 34 native pixels below the extended hand. v10.3 changes only that frame's pickup correction (dx 3->10 logical, dy 18->1 logical) and leaves frames 230..236 unchanged pending frame-specific evidence. Debug output now prints both `body_res` and the corrected `mac_res` IDs.


## v10.3 pickup calibration

Pixel-contact validation of the actual Mac KID 561..568 + PRINCE 721..728 composites found that frame 229 was already attached in v10.2. Remaining detached pickup poses were 230, 234, and 236. v10.3 adjusts only those frames: frame 230 +3 logical Y, frame 234 +3 logical X toward the hand, frame 236 +8 logical X toward the hand and +5 logical Y. Frames 231/232/233/235 were left unchanged.


## v10.4 pickup final calibration
- Reviewed 60 fps recording `Screencast_20261025_031154.webm`.
- Structural pickup drift is gone in v10.3; remaining error is only about 1-2 native pixels.
- Micro-adjusted only pickup frames 229, 230, 234 and 236.
- New dx table: `{11,22,16,15,28,9,3,5}`.
- New dy table: `{1,5,5,2,3,1,-2,3}`.
- Debug logging now prints `corr_dx` and `corr_dy`.
- If this sequence looks connected in play, freeze pickup alignment and move testing to normal combat poses.


## v10.5 pickup frame 233 correction
A paused playtest screenshot was template-matched against the native Mac assets and identified as frame 233 (KID resource 565 + PRINCE sword resource 725). The sword hilt was ~14 native pixels too far right and ~12 px too high relative to the extended hand. Only pickup index 4 / frame 233 was changed: correction dx 28→21, dy 3→9. Other pickup frames remain frozen.


## v10.6 pickup hand-lock calibration
Paused v10.5 screenshots identified two remaining pickup alignment errors precisely:
- frame 232: KID 564 + PRINCE 724. Move sword about 12 native px left and 4 px down. pickup correction dx/dy 15/2 -> 9/4.
- frame 233: KID 565 + PRINCE 725. Move sword about 34 native px left and 18 px up. pickup correction dx/dy 21/9 -> 4/0.
These values were derived from exact native sprite hand/hilt pixel geometry, not motion-frame eyeballing. Other pickup frames remain frozen.

## v10.8 combat hand-lock
Normal combat sword attachment is now calibrated separately from pickup frames 229..236.
Native hand/hilt measurements yielded backward-relative-to-facing X corrections:
- Prince frame 158/170/171: 11 logical px
- Prince frame 163: 11
- Prince frame 164: 10
- Prince frame 165: 9
- Guard/skeleton frame 158: 3
Guard debug body resource IDs were also fixed to use base 751 rather than the KID base 401.
Debug output now includes `combat_dx=`.


## v10.8 combat matrix

The combat attachment pass now has independent X/Y calibration for the captured Prince and guard combat classes: parry, strike/thrust, ready, block, fighting transitions, guard idle, guard attack/recovery, and guard falling/death poses. Pickup frames 229..236 remain isolated and unchanged. Debug output now reports both `combat_dx` and `combat_dy`.


## v10.10 combat lunge cleanup

Residual combat issue from v10.9: the Prince low-lunge family still showed detached hilts in two screenshots. Added a new correction for frame 155 (KID579 + S718) and increased the existing frame 157 correction for KID581 + S718.
Seven paused v10.8 playtest captures were treated as a combat regression set. Pickup 229..236 remains frozen. Added/updated native combat corrections:
- Prince 157 / KID 581 / sword 718: dx=4, dy=-2.
- Prince 161 / KID 585 / sword 715: dx=7, dy=2.
- Guard 162 / GUARD 762 / sword 711: dx=6, dy=-2.
- Guard 167 / GUARD 769 / sword 709: dx=2, dy=1.
- Guard 168 / GUARD 770 / sword 713: dx=12, dy=1. This was the largest remaining combat gap.
- Guard 171 / GUARD 772 / sword 705: dx=5, dy=0.
Frames already visually attached in the same regression set were deliberately not changed.


## v10.11 guard hand-lock cleanup

Two remaining guard sword-placement errors were isolated from paused combat captures and corrected only in the Mac native combat exception table:
- Guard frame 158 / GUARD 760 / sword 705: combat dx `2 -> 3`, dy remains `1`. This moves the hilt about 2 native pixels toward the extended hand.
- Guard frame 165 / GUARD 767 / sword 709: combat dx `0 -> -2`, dy `1 -> -1`. For the captured left-facing pose this moves the sword about 4 native pixels left and 4 native pixels up, placing the hilt above/left of the fist correctly.

No gameplay, collision, pickup-frame, Prince, or global sword-anchor math was changed. X corrections remain facing-relative and therefore mirror automatically.

## v10.12 Prince hand-lock cleanup

Two newly reported Prince sword-placement errors were isolated from paused combat captures and corrected only in the Mac native combat exception table:
- Prince frame 152 / KID 576 / sword 717: combat dx `-11 -> -10`, dy remains `5`. This moves the hilt about 2 native pixels back toward the extended fist.
- Prince frame 155 / KID 579 / sword 718: combat dx `7 -> 9`, dy `0 -> -2`. For the captured right-facing pose this moves the sword about 4 native pixels left and 4 native pixels up, placing the blade above the hand instead of intersecting the Prince body.

No gameplay, collision, pickup-frame logic, guard calibration, or global sword-anchor math was changed. X corrections remain facing-relative and therefore mirror automatically.

## v10.13 Prince frame 155 hilt lock

The paused low-lunge capture identified Prince frame 155 / KID579 + S718 with debug values `final_x320=67`, `final_y=105`, `combat_dx=9`, `combat_dy=-2`. Inspecting the native Macintosh assets shows KID579 has SHPD origin `(-3,68)` and S718 has origin `(0,8)`. For the right-facing pose both images are horizontally flipped, which puts the S718 hilt on the sword sprite's left edge. At the v10.12 anchor the hilt therefore sits roughly 28 native pixels too far left, hidden underneath the forearm.

Frame 155 is changed from `combat_dx=9, combat_dy=-2` to `combat_dx=-5, combat_dy=-3`. For the reported right-facing pose this moves the sword 14 logical pixels right (28 native pixels) and 1 logical pixel up (2 native pixels), aligning the hilt with the fist while leaving the blade visible beyond the hand. No other combat frame, guard calibration, pickup sequence, gameplay logic, or global sword attachment math changes.

## v10.14 Prince frame-152 hand-lock follow-up

The new capture identifies the remaining bad pose as `char=0 frame=152 body_res=576 sword_frame=18 mac_res=717 dir=0`, with v10.13 reporting `combat_dx=-10 combat_dy=5`. The sword still intersects the Prince forearm/body. The frame-scoped Mac correction is changed to `combat_dx=-8, combat_dy=3`, moving the rendered sword 2 logical pixels left and 2 logical pixels up in the reported right-facing pose. Frame 155, guards, pickups, global sword-anchor math, collision, and gameplay are unchanged.

## v10.15 Prince frame 152 reference match

The user supplied both a current debug capture and a known-good 640x400 frame for Prince frame 152 / KID576 + S717. The static room aligns at x=0 and y=+1 screenshot pixel; the Prince body aligns at x=-100 and y=+1 because the actor is at a different horizontal position. After that body alignment, the S717 blade in v10.14 is 16 native pixels too far right and 6 native pixels too low compared with the reference. This is exactly 8 logical pixels right and 3 logical pixels down.

Therefore the v10.14 per-frame correction `combat_dx=-8, combat_dy=3` is removed and frame 152 is set to `combat_dx=0, combat_dy=0`, returning that pose to the generic Mac attachment. With the actor/debug values from the supplied capture, the expected sword anchor changes from approximately `final_x320=132 final_y=94` to `final_x320=124 final_y=91`. Frame 155, guard frames, pickup frames, collision and gameplay logic are unchanged.


## v11.11 continuation note

The user's v11.10 climb screenshots showed that `redraw_frames_fore` must remain distinct from a full native tile redraw. v11.11 separated that phase and suppressed the tall dungeon pillar foreground strip (`res295`) during frames 141..144, after the sequence's row jump. This fixed the later-row overdraw path but intentionally left frames 137..140 unchanged.

## v11.12 continuation note

The next playtest isolated the remaining failure to frame 140. Its native Prince pose is the horizontal upper-body / dangling-leg pull-up frame, and it still received the dynamic `res295` pillar foreground because v11.11 began suppression at frame 141. v11.12 suppresses only that tall dynamic dungeon-pillar fore-strip for the entire climb-overlay interval 137..144. The dedicated climb MID overlay (small wedge + floor/front lip) remains the sole intended occluder during those frames. Static room pillars, ordinary movement, actor placement, collision, timing, and the corrected v11.8 pressure-plate work remain unchanged.

## v11.13 correction note

The v11.10-v11.12 pillar suppression was based on a misread of the visual bug and is superseded. In the failing v11.12 capture, the port shows the Prince's grey lower leg and orange foot through the vertical pillar face. The original Macintosh reference hides that lower body behind the ledge/wall side. Pixel matching identifies the current actor pose as native KID `res552`; restoring native dungeon pillar foreground `res295` over the measured overlap removes the through-pillar pixels while preserving the upper-body pose.

Therefore v11.13 keeps `redraw_frames_floor_overlay` overlay-only and `redraw_frames_fore` foreground-only, but **does not suppress pillar foreground during frames 137..144**. The pillar remains a FORE occluder, as in the original composition.

## v11.14 native climb-occluder note

`floor_left_overlay[] = {32,151,151,150,150,151,32,32}` is a DOS compositor table. In native-full those IDs resolve to DOS resources 232/351/350 and are therefore the wrong shapes and resolution for Macintosh 640x400. The Macintosh environment set contains eight dedicated triangular climb masks at resources 220..227, matching the eight frames 137..144 one-for-one.

The native path now uses chtab IDs `0x14..0x1B` and `mac_add_mid_anchor_exact()` at the same logical floor anchor. Do not reintroduce the DOS overlay table into `mac_draw_floor_overlay_exact()`. The legacy `draw_floor_overlay()` path continues to use it for DOS/non-native packs.
