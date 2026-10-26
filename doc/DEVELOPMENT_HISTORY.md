# Prince of Persia Remastered — v1.0.0

The window title and welcome screen identify this version as **Prince of Persia
Remastered v1.0.0**, with Macintosh graphics information and quick controls.
The welcome screen retains a short SDLPoP engine credit.

V18 prepares a local Linux release candidate, fixes resource-lifetime defects,
and consolidates native-mode checks. Gameplay, decoded audio samples and native
art/origins are retained. See V18_RELEASE_PREPARATION.md, RELEASE_NOTES.md and
PAK_NATIVE_ORIGINS.md for verification, packaging and remaining live checks.

Graphics menu order: **Mac Native FULL v1.0.0 (default), DOS, SNES 1, SNES 2, SNES 3**.
SNES 1 is the former SNES Alt 2/Mac Prince; SNES 2 is the former SNES Alt;
SNES 3 is the former SNES. Their settings IDs/filenames remain unchanged.
Mac Color and Mac Native 640x400 are no longer active options. Their development
reference copies are under tools/mac_reference/legacy-packs/. The active choice
remains graphics_pack = mac_native_full. Close the old game and restart ./prince.

The exact Fedora binary requirements are recorded in the packaged BUILD_INFO.json;
Steam Deck testing and full-game sign-off remain pending. No public release has
been published. Earlier milestone notes below are retained as history.

## V17.0 — Combat Reference and World Audit

V17 replaces native sword-placement guesses with the original Macintosh
attachment table, recovered from the supplied disk. Pickup, attack, parry,
draw/sheath and stepping are checked for Prince/guard art in both directions.
The level-wide sweep also fixes two thin redraw edges beside palace exits and
above dungeon pillars. Gameplay, collision, AI, timing and assets are unchanged.

Build/startup and 26,275 counted checks pass, plus retained climb/fall tests.
The new sweep covers 332 room maps and 14 entry frames across Levels 1–14;
all 489 earlier shared captures remain unchanged. See
MAC_SWORD_REFERENCE_AUDIT.md, V17_WHOLE_GAME_VISUAL_REGRESSION.md and
screenshots/v17.0/combat-comparison.png. Restart `./prince` for v17.0.
New fixes await live playtesting; v18 cleanup/release is next.

## v16.0 — HUD and UI

V16 fixes the Kid's full/empty health-flask selection and native RGB fades.
Fade-in uses its supplied image; fade-out includes the entire screen and HUD,
while cutscene actors/flames keep animating. Health, gameplay and original
assets are unchanged. The retained SDLPoP menus/text now have automated coverage.

Build/startup checks and 18,545 counted renderer checks pass, plus retained
climb/fall checks. All 467 shared v15.1 PNGs are unchanged. See V16_HUD_AND_UI.md
and screenshots/v16.0/hud-comparison.png. Restart `./prince` to playtest the
new build. The user confirmed damage/healing, pause/resume and intro fades.

## v15.1 — Native Cutscenes

V15.1 restores the intro story panel's original dark blue background (#100060).
The native frame is transparent in the middle and now draws over an explicit
colour fill. Ending cards retain the red selector; the main title is unchanged.
Build and 17,858 automated checks pass, plus retained climb/fall checks.
Only four story-card captures differ from v15.0. See screenshots/v15.1/ and
V15_CUTSCENE_COMPOSITION.md. Restart `./prince` to playtest the new build.

## v15.0 — Native Cutscenes

V15 restores native Princess-room props, attached flames, the hourglass and
foreground pillar, removes the duplicate DOS bed overlay, and gives cutscenes
their full 640x400 buffer. Story cards use the native text pixels and insets.
Gameplay, animation timing and original assets are unchanged.

The rebuilt game and 17,850 counted checks pass, plus retained climb/fall checks.
All 409 PNGs shared with v14.0 are unchanged. See V15_CUTSCENE_COMPOSITION.md
and screenshots/v15.0/cutscene-comparison.png. V15.0 awaits live playtesting;
v16 HUD/UI is next. Close the old game window and restart `./prince` to load
this build.

## v14.0 — Native Props and Pickups

V14 corrects potion effect/art selection, loads the missing native green/blue
bubble frames, repairs a skeleton edge refresh and preserves lattice foreground
occlusion with the high-jump setting enabled. Potions, sword pickup, torches,
skeleton waking, debris, balconies and lattice have automated renderer coverage.
Gameplay and original assets are unchanged.

The rebuilt game and 16,757 counted checks pass, plus retained climb/fall checks.
All 288 captures shared with v13.1 are unchanged. See V14_PROPS_AND_PICKUPS.md
and screenshots/v14.0/props-comparison.png. V14.0 live confirmation remains
open; its automated coverage is retained in v15.

## v13.1 — Palace and Pickup Redraw Edges

The first native palace wall compositor is implemented using the original
CLUT and drawing routines recovered from the supplied PoP1_spf.dsk. It adds
missing wall-face colors and SHPD brick details, with a private rendering RNG
that preserves gameplay randomness. Original assets and disk are unchanged.
All 820 exported images/origins match a fresh disk decode.

The user confirmed v13.0 works as expected. The final audit fixed neighbouring
cell redraws for the palace wall stripe and the floor sword's two-pixel outline.
The build, 1,523 palace/pickup checks, 5,503 architecture checks and 1,050 floor
checks pass. Four full wall scenes match independent fixtures; all 279 captures
shared with v13.0 are unchanged. See V13_PALACE_RENDERER.md and
screenshots/v13.1/. V13 implementation/automated audit is complete; these new
edge fixes await live playtest. V14 expands this coverage as described above. Broader original-Mac
scene comparisons remain open.

### v12.3 combined slab/architecture pass

The user confirmed v12.2. Combined slab, moving-gate, pressure-plate and room
checks found one remaining native redraw bug: debris created in the linked
room to the left could leave the old floor edge visible on the current screen.
Neighbor slab-state changes now request visible-room composition; unrelated
rooms and other graphics packs keep their prior path. Slab physics, trigger
behavior and gameplay timing are unchanged.

The build, 5,503 architecture checks and 1,050 floor checks pass. The new 2,304
combined frames cover opening/holding/slow/fast gates, floor/opener/closer
landings and same/lower/switching/linked-left views; three extra checks verify
refresh scope. All 241 individual captures shared with v12.2 are unchanged.
See `screenshots/v12.3/slab-edge-comparison.png` and the current v12 test report.

### v12.2 mirror and chomper pass

All four v12.1 gate/exit issues are user-confirmed fixed. This pass restores
the shadow's native XOR operation, keeps mirror glass/frame foreground over
the reflection on every redraw, and refreshes native chomper blood art when a
death changes that state after tile animation. Gameplay health, collision and
timing logic remain unchanged.

The build, 3,196 architecture checks and 1,050 floor checks pass. All 181
captures shared with v12.1 remain pixel-identical. New scenes cover reflection
in both directions, the actual mirror jump/shadow handoff, shadow escape,
actors crossing chomper cycles, actual death/blood, and linked-room gate edges.
See `screenshots/v12.2/mirror-chomper-comparison.png` and
`V12_ANIMATED_ARCHITECTURE_TESTS.md`. Live v12.2 playtest remains open.

### v12.1 playtest corrections

Gate foreground now contains only moving bars, removing the dark floor wedge
left after passing a gate. Native same-room redraws preserve the room-exit
timer and keyboard buffers, so repeated gate redraws cannot postpone room
crossings. Actual room transitions take priority over full redraw requests.

Exit clipping uses the panel's closed SHPD bounds instead of CELL_TOP, removing
the black header gap in dungeon and palace. Visual phase 0..43 now retracts the
whole native-height panel. The Prince's final stair frames resolve clipping
from the exit beside him rather than stale globals, keeping him visible as he
ascends and passes behind the jamb. Gameplay animation/timing remains unchanged.

The build and 2,554 architecture checks pass, including both room-crossing
directions during gate animation and stair frames 217..228 plus level-sequence
completion. All 1,050 floor checks pass; 20 climb/fall captures remain identical.
See `screenshots/v12.1/` and `V12_ANIMATED_ARCHITECTURE_TESTS.md`.

### v12.0 first architecture pass

Native gate, exit-door and chomper pose changes now recompose the buffered
visible room, removing stale slices left by the DOS tile-sized redraw bands.
Mirror appearance/removal also refreshes its full native extent, including the
level-4 gameplay spawn. The moving exit panel is clipped behind its fixed
lintel and stays in BACK with the passage/stairs, matching legacy layering.
Gameplay coordinates, animation timing and native assets are unchanged.

The initial pass passed 2,374 architecture checks and all 1,050 v11 floor
checks. The 20 climb/fall captures remain pixel-identical to v11.19. See
`V12_ANIMATED_ARCHITECTURE_TESTS.md` for results and remaining live playtests.
v12 is open; this is not whole-game or original-Mac visual sign-off.

```bash
make -C src -j2 CC=/usr/bin/gcc
python3 tools/mac_native_renderer/test_climb_overlay.py --floors --architecture --output screenshots/v12.0
./prince
```

Close an older running game before launching the rebuilt `prince`.

### v11.19 closure audit

v11 is closed following 1,050 dungeon/palace floor checks, the existing
climb/fall regressions, and visual review of representative runtime captures.
The audit found and fixed a one-native-pixel wall-border repaint under exposed
dungeon floors: wall-corner res362 extended into the lower wall's foreground
row during partial redraws. Its draw is now clipped to the floor front while
preserving its SHPD anchor and restoring the caller's viewport clip.

See `V11_CLOSURE_REPORT.md` for coverage, results and limits. Run the complete
suite with:

```bash
make -C src -j2
python3 tools/mac_native_renderer/test_climb_overlay.py --floors --output screenshots/v11.19
```

v12 animated architecture started with the v12.0 pass above.

### v11.18 falling past the ledge/pillar

The native renderer had treated `redraw_frames2` as a full background tile
redraw. The legacy renderer instead uses `draw_other_overlay()` to put the
appropriate environment components in MID after an airborne actor. Losing
that phase let the Prince's trousers/torso draw through the floor plane and
parts of the pillar while falling from a ledge (frames 102..106).

The native path now reuses the exact native components and SHPD anchors in
that MID phase. It preserves the legacy exposed-neighbor rule, including the
previous-cell actor redraw, and keeps pillar foreground separate. Actor
positions, collision, timing, native assets, and the v11.17 climb work are
unchanged. The test runner now saves both climb and fall sequences:

```bash
python3 tools/mac_native_renderer/test_climb_overlay.py --output screenshots/v11.18
```

### v11.17 frame-138 tail / frame-144 shoe dot

The floor-edge repair now also covers the three-pixel outline tail in frame
138 and the single stray shoe-outline pixel in frame 144. For native mask
221, the extra column begins four physical rows below the mask top so the
visible elbow pixels remain untouched. Mask 227 receives the same one-column
coverage as the v11.16 masks. Actor placement, sprite assets, and gameplay
remain unchanged. Canonical captures differ from v11.16 by just four pixels.

The renderer regression checks 120 edge cases and consecutive frames
138..146, including direct checks at both reported marks. Save captures with
`python3 tools/mac_native_renderer/test_climb_overlay.py --output screenshots/v11.17`.

### v11.16 remaining vertical outline

Native climb masks 222..224 ended one physical column before the port's body
outline, leaving a thin black line inside the floor. The native MID draw now
restores only that extra column from floor-top resource 241 at its exact SHPD
alignment. This preserves texture, the v11.15 actor anchors, and the original
mask assets. The helper intersects the existing viewport clip and restores it
after drawing. No gameplay, actor movement, collision, or pillar depth changes.

The renderer regression now checks that column's pixels in 72 cases and
advances the full sequence without resetting the room, confirming that frames
144..146 match clean renders at the former line positions.

### v11.15 climb mask registration

The native climb masks (environment resources 220..227) are exact cutouts of
the normal floor-top image, resource 241. v11.14 placed them at the DOS overlay
baseline, six native pixels below the floor top. This left the Prince visible
inside the sloping surface during pull-up.

v11.15 gives each mask the same logical anchor as the floor top:
`draw_main_y + 1 + MAC_FLOOR_TOP_ANCHOR`, plus one logical pixel for a depressed
plate. The original Macintosh reference also shows the post-row-change body
pose about six native pixels higher than the port. The displayed KID552..555
poses (frames 141..144) receive that lift in `draw_mid()`, including mirrored
poses. Earlier hanging poses and later standing/crouching poses keep their
anchors. Gameplay actor coordinates, animation, collision, pillar foreground,
and the floor front remain unchanged. Floor diagnostics now include
`role=climb-overlay`.

Build with `make -C src`, then run the Linux renderer regression with:

```bash
python3 tools/mac_native_renderer/test_climb_overlay.py --output screenshots/v11.15
```

This checks dungeon/palace mask pixels, 192 overlay placements, and 30 actor
placements, then renders the full 135..149 sequence without opening a window.
It uses a temporary test executable; `prince` is the normal playable build.


### v11.14 native climb-occluder correction

The remaining grey/pixelated patch across the Prince during the 137..144 climb sequence was traced to the legacy DOS `floor_left_overlay[]` sprites. In `mac_native_full`, frames 137..144 were still drawing DOS resources 232/350/351 on the MID layer. The Macintosh pack already contains eight dedicated frame-specific climb occluders at environment resources **220..227**.

v11.14 maps frames 137..144 directly to native chtab IDs `0x14..0x1B` and submits them with `mac_add_mid_anchor_exact()`, preserving their SHPD origins. For the supplied frame-143 capture this replaces DOS resource 232 with Macintosh resource **226**. The pillar foreground behavior from v11.13 is unchanged.

### v11.13 climb/pillar correction

The original Macintosh reference shows that the vertical pillar/wall face must occlude the Prince's lower body during the pull-up. In the v11.12 port capture, native KID `res552` was drawn over the pillar face, leaving the grey leg and orange foot visible through the masonry. Pixel matching places the actor at the same pose while the native dungeon pillar foreground (`res295`) occupies the overlapping area below the ledge.

v11.13 therefore reverses the v11.10-v11.12 pillar suppression. `redraw_frames_floor_overlay` and `redraw_frames_fore` remain separate compositor phases, but dungeon pillar foreground is again submitted to FORE during climb frames. The dedicated climb MID overlay still handles the thin floor lip. No actor coordinates, collision, climb timing, or pressure-plate logic are changed.

A direct reconstruction of the supplied port frame with `res295` restored over the actor hides exactly the through-pillar leg/foot while leaving the upper-body pull-up pose visible, matching the layering behavior shown in the original Mac reference.

### v11.10 native climb overlay layering fix

The corrected v11.8 baseline is preserved unchanged for pressure plates. v11.9 restored the missing midtable climb occluder, but it still treated `redraw_frames_floor_overlay` as a request for a full `mac_draw_tile_exact()` first. That was wrong: the legacy compositor handles this flag as an overlay-only redraw. The extra full redraw re-submitted ordinary foretable geometry, including the native pillar foreground strip, after the actor and caused the Prince to be over-occluded during climb frames 137..144.

v11.10 removes that coupling. A floor-overlay request now submits only the dedicated climb wedge plus the state-correct floor/front lip to the midtable. If the same tile also needs its ordinary foreground redraw, an exposed dungeon pillar temporarily suppresses its tall native fore-strip for that climb redraw only; otherwise the fore-strip would sit above the midtable and hide most of the Prince. Static pillar depth and all non-climb foreground behavior remain unchanged. Gameplay coordinates, collision and animation timing are unchanged.

### v11.8 pressure-plate black strip / black triangle fix
Two separate renderer-only causes were found by compositing the native `0x23/0x24/0x29/0x2A/0x60` images against runtime captures:

1. **Black triangle.** When a plate changes state, `redraw_tile_height()` wipes the plate cell *and the cell to its right* to black and redraws only those two tiles. v11.7 skipped the below-left corner piece for `tiles_5_stuck`, so nothing refilled the area under the plate's diagonal edge (transparent in `0x23`). v11.8 always submits the below-left corner. The generic `0x2A` black mask is still skipped for stuck plates (`0x23`'s own opaque pixels already cover everything above its diagonal).
2. **Black horizontal strip.** `draw_tile_wipe()` clears down to `draw_bottom_y`, but the Mac floor front ends at `draw_main_y` (3 logical / 6 native rows higher). Those 6 rows are the top of the wall/tile in the row below, which is not redrawn, so each wipe permanently erased its lit top edge. In `mac_native_full` the wipe now stops at `draw_main_y`; its top edge is unchanged.

### v11.7 pressure-plate black-mask correction
A second Macintosh comparison showed that the v11.6 exception was too narrow. The generic `0x2A` black floor-right silhouette must never be submitted underneath `tiles_5_stuck`, not only when the neighbor decodes as literal `tiles_0_empty`. Other visually open neighbor states can enter the same overhang path and were still producing the silhouette's full-width black top band plus black triangle. v11.7 skips the generic below-left/mask prepass for every depressed/stuck plate context; the colored native `0x23` component remains the sole exposed edge.

### v11.6 exposed depressed-plate correction
A Macintosh gameplay reference confirms that a depressed/stuck pressure plate immediately left of an empty gap has no continuous black strip under its lowered top. In this exact context, Mac Native FULL now skips both the below-left corner contribution and the generic `0x2A` black occlusion silhouette; the state-native `0x23` right component is the sole exposed edge. Normal floor ledges and other exposed-edge states retain the v11.5 blit-aware black-mask path.

### v11.5 native black-mask correction
The exposed-ledge compositor order introduced in v11.4 was correct, but exact 640x400 native entries bypassed SDLPoP blitter semantics. As a result, the supposed `blitters_9_black` `0x2A` mask was drawn in its original teal floor colors and looked like a second shadow/wedge. v11.5 adds a physical-coordinate blit path that honors the black-mask operation while preserving native alpha and SHPD placement. The visible depressed edge remains `0x23`.

### v11.4 ledge occlusion correction
Mac Native FULL now restores the original black floor-right occlusion pass at exposed ledges. This hides the below-left wall/corner diagonal before the native state-specific right edge is drawn, removing the doubled pressure-plate shadow without changing the visible Mac edge asset, gameplay, collision, or floor height.


v11.0 builds on the completed v10.15 actor/combat pass and moves the Macintosh renderer to the floor-state milestone from `HANDOFF_MAC_RENDERER.md`. Gameplay, collision, timing and level state remain unchanged.

## v11.0 change

Interactive floor tiles now select their Macintosh-native base, right-edge and front components as one state definition with explicit logical anchors. This covers normal floor, spikes, drop/raise buttons, depressed buttons, loose floor frames, debris and debris-with-torch.

The native exact compositor no longer paints a generic floor-right wedge and then a second state-specific wedge at exposed edges. That duplicate submission was especially visible around pressure plates as a widened/doubled dark lip. Depressed drop buttons retain the legacy neighbor rule: their short seam component is used only when a floor continues to the right; an exposed edge uses the full normal floor wedge.

Falling loose floors now use the same SHPD-origin anchor model as their final static/shaking frame. At rest, all three moving pieces resolve to the same base/front/right coordinates as the room compositor, removing the previous three-logical-pixel transition jump.

Floor diagnostics are available with:

```bash
SDLPOP_MAC_FLOOR_DEBUG=1 ./prince
```

The log reports tile state, component role, environment image/resource ID, logical anchor, SHPD origin and final physical coordinate. See `V11_INTERACTIVE_FLOOR_TESTS.md` for the focused playtest matrix.

## Build

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

The menu entry is **Mac Native FULL v17.0 (Combat and world audit)**.

## v11.0 regression focus

1. Normal floor seams and exposed right edges.
2. Raise and drop buttons, both idle and pressed.
3. Depressed/stuck drop buttons beside floor and beside empty/non-floor cells.
4. Loose-floor shake frames and the exact static-to-falling transition.
5. Falling loose floor through room rows/boundaries.
6. Broken floor/debris, including debris with torch.
7. Spikes through their animation frames.
8. Recheck v10.15 sword/combat scenes for regressions.

## v10 actor/combat history

## v10 change

The floating combat sword was traced to a coordinate-space mismatch, not primarily to SHPD origin metadata.

SDLPoP character X positions use an internal 280-wide actor coordinate system and are converted to 320-wide screen coordinates by `calc_screen_x_coord()`.

Before v10 the sword path effectively did:

```text
actor X280 -> convert to X320 -> add sword_tbl.x
```

v10 instead does:

```text
actor X280 -> add sword_tbl.x -> convert complete anchor to X320
```

This scales the attachment consistently with the body anchor and applies to Prince, guards and skeleton-family actors in both facing directions. The temporary Prince-only v8 sword nudge has been removed.

## Actor diagnostics

Run:

```bash
SDLPOP_MAC_ACTOR_DEBUG=1 ./prince
```

Combat frames with a sword print their character/frame/resource and final attachment coordinates. Use this if a specific fencing pose still looks wrong.

See `HANDOFF_MAC_RENDERER.md` for the full renderer roadmap and architecture handoff.

### v10.3 pickup-frame correction
The actor debug resource labels are now corrected for SDLPoP's `base + image_index + 1` loading convention. Pickup debug output includes both the native body resource and sword resource. Frame 229 receives a recording-derived correction; later pickup frames are intentionally left unchanged until a specific bad frame is observed.

## v10.10 combat lunge cleanup

This pass cleans up the last two Prince low-lunge combat poses still visible in v10.9 screenshots. Prince frame 155 (KID579 + S718) now gets its own correction, and Prince frame 157 (KID581 + S718) gets a stronger backward/upward lock. The pickup sequence 229..236 remains completely independent.


## v10.11 guard hand-lock cleanup

Final frame-scoped Mac combat cleanup for two guard poses: frame 158 receives a 2-native-pixel hilt nudge toward the hand, while frame 165 moves about 4 native pixels left/up in the reported left-facing pose. No gameplay or global attachment math changes.


## v10.12 Prince hand-lock cleanup

Two newly reported Prince low-lunge poses are micro-calibrated in the Mac-only combat exception table. Frame 152 (KID576 + S717) gets a tiny 1-logical-pixel hilt nudge back toward the fist. Frame 155 (KID579 + S718) moves 2 logical pixels left and 2 logical pixels up so the sword sits above the hand instead of intersecting the body. No global sword-anchor math, pickup-frame logic, or gameplay behavior changes.

## v10.13 Prince frame 155 hilt lock

The paused low-lunge capture identified Prince frame 155 / KID579 + S718 with debug values `final_x320=67`, `final_y=105`, `combat_dx=9`, `combat_dy=-2`. Inspecting the native Macintosh assets shows KID579 has SHPD origin `(-3,68)` and S718 has origin `(0,8)`. For the right-facing pose both images are horizontally flipped, which puts the S718 hilt on the sword sprite's left edge. At the v10.12 anchor the hilt therefore sits roughly 28 native pixels too far left, hidden underneath the forearm.

Frame 155 is changed from `combat_dx=9, combat_dy=-2` to `combat_dx=-5, combat_dy=-3`. For the reported right-facing pose this moves the sword 14 logical pixels right (28 native pixels) and 1 logical pixel up (2 native pixels), aligning the hilt with the fist while leaving the blade visible beyond the hand. No other combat frame, guard calibration, pickup sequence, gameplay logic, or global sword attachment math changes.

## v10.14 Prince frame-152 hand-lock follow-up

The remaining Prince frame 152 pose (KID576 + S717) is moved another 2 logical pixels left and 2 logical pixels up from v10.13. The Mac-only correction changes from `combat_dx=-10, combat_dy=5` to `combat_dx=-8, combat_dy=3`, clearing the sword from the forearm/body and placing it above the hand. No other combat frame or gameplay logic changes.

## v10.15 frame 152 reference match

The current 640x400 frame-152 capture was pixel-aligned against the supplied known-good reference. With the Prince body aligned, S717 in v10.14 was 16 native pixels too far right and 6 native pixels too low. Those deltas exactly correspond to the v10.14 combat correction of `dx=-8, dy=3`, so frame 152 now uses `combat_dx=0, combat_dy=0` and falls back to the generic Mac sword attachment. Frame 155 and all guard/pickup calibrations remain unchanged.
