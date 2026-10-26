# Macintosh 640x400 renderer notes — v13.0

## v13.0 native palace wall compositor

Original CLUT 2000 and wall routines were recovered from the supplied HFS disk.
The private Mac RNG generates room colors in CLUT 76..83 without touching
random_seed. Native wall fills/details follow original CODE 3:$0B72 and CODE
4:$5CB6/$5EE6 with the existing native floor registration bridge. See
V13_PALACE_RENDERER.md for evidence, exact geometry and regeneration commands.

BACK/MID/FORE support native_entry_fill (use_physical=2), drawing an RGB rect
before image lookup while preserving viewport clips and dirty rectangles.
Bottom fills route into MID during airborne overlays; face/detail entries
stay FORE. Image entries keep modes 0/1. FORETABLE_MAX=600 accommodates dense
walls and above-room entries. seg008.o depends on mac_palace_palette.h.

200 palace checks, all 5,503 architecture checks and 1,050 floor checks pass.
The independently generated wall fixtures cover four room seeds and 983,040
pixels. All 124 shared dungeon/climb/fall captures are unchanged; palace wall
backgrounds intentionally change. Native PAKs and gameplay logic are preserved.

## v12.3 linked slab/debris redraw

`redraw_at_cur_mob()` now requests room composition for native linked-room
changes. A slab landing in the left room's last column changes its right-side
debris component, which can be visible in the current room. The same-room
dirty-tile path cannot address that other-room index. Holding or stopped gates
exposed the stale edge; moving gate redraws masked it.

The native visibility helper limits this refresh to current/linked rooms.
Same-room dirty redraws, other graphics modes and slab physics/room-coordinate
bridges remain unchanged. Combined real mob/trigger/gate simulations test full
and incremental composition across room boundaries; see the current v12 report.

## v12.2 native XOR, reflection foreground and chomper blood event

`draw_native_image_physical_blit()` now honors `blitters_3_xor` via a clipped
ARGB working rectangle at exact physical coordinates. Only nontransparent
source RGB is XORed; destination alpha, caller clip/blend state and native
origins are retained. Dirty rectangles round outward into logical pixels.
This restores the existing shadow/union operation instead of alpha-blitting
the second colored body pass. Ordinary native and black-mask paths are unchanged.

Native reflection submission explicitly assigns its mirror cell and marks
that cell's FORE. The glass diagonals/frame must draw after the reflected
body even when the real body is in another cell. No reflection/gameplay X
transform or sprite offsets are changed.

`chomped()` reports its blood-bit transition through the shared visible native
architecture check. That event occurs after tile animation, so observing only
`animate_tile()` misses it. This refreshes native blood slices immediately;
death/collision/HP/animation state remain the original game logic.

The new real-renderer checks cover XOR math and clipping, mirror reflection
in both directions, actual jump/shadow handoff and escape, actor/chomper cycles
and death, and left/right/above-room gate edges. See the current v12 test report.

## v12.1 gate depth, refresh side effects and native exit bounds

Gate FORE follows `draw_gate_fore()`: 0x33 moving bottom bars plus full 0x34
slices, excluding the BACK floor wedge, combined post and partial top slices.
This prevents a floor-shadow strip when passing through a gate.

Native same-room `redraw_screen(0)` must not reset `exit_room_timer` or clear
keyboard buffers. Repeated pose-triggered recomposition previously kept the
timer at 2 and blocked room exits until the animation stopped. Real room
changes retain entry behavior and have priority over `need_full_redraw`.

The fixed exit clip is the closed panel's SHPD rectangle, not CELL_TOP (which
created a 13/16-native-pixel header gap). A shared geometry helper maps the
unchanged gameplay phase 0..43 to the whole panel height, in physical pixels;
it is fully retracted at 43. This supersedes the v12.0 clip and visual lift.
`mac_get_leveldoor_clip()` derives the late stair-frame top/right clip from
the exit beside the actor in his own room/row, without mutating tile lookup
state or trusting stale legacy globals. The native draw also initializes
those globals. All actor and gameplay animation positions/timing are preserved.

See `V12_ANIMATED_ARCHITECTURE_TESTS.md` for 2,554 architecture checks, the
unchanged floor/climb/fall regressions and current playtest captures.

## v12.0 animated architecture (in progress)

`animate_tile()` retains the original gameplay animation functions and modifier
writes. A native-only post-check requests `need_full_redraw` when a visible
gate pose (`MIN(modifier,188)/4`), exit panel position or chomper art/blood state
changes. The normal buffered room redraw reconstructs transparent/tall native
art outside the DOS dirty rectangles. Linked neighboring rooms are included
because their artwork can overhang the viewport; unrelated rooms are skipped.
Mirror changes through `redraw_at_trob()` and the actual level-4 spawn also
request recomposition. This does not alter animation speed, collision or actors.

Native exit panel 0x21/resource 233 stays in BACK with its passage and optional
stairs, including during generic airborne overlays. It lifts behind a fixed
clip: top = `(draw_main_y + 1 + MAC_CELL_TOP_ANCHOR) * 2`, bottom = closed
panel's SHPD-derived physical top + image height. Its moving SHPD anchor and
gameplay opening range remain unchanged. BACK/FORE entries have optional
`physical_clip_top` and `physical_clip_bottom` (-1 for none); drawing intersects
the saved SDL clip and restores it. The v11.19 wall-corner bottom clip remains.

The audit checks 2,374 architecture cases and all 1,050 v11 floor cases. Saved
climb/fall images are unchanged. See `V12_ANIMATED_ARCHITECTURE_TESTS.md` for
coverage and the remaining live actor-interaction/mirror/exit checks.

## v11.19 floor milestone closure

The dungeon wall corner res362 has height 25 and origin_y 22, extending one
physical row past the floor-front boundary. Clip its native physical draw
before that lower wall FORE row (`physical_y + origin_y + 2`, exclusive).
This makes partial redraws match full composition without shifting assets or
repainting the lower wall. The same clipping is a no-op for the palace corner
that already ends at the boundary. The existing SDL clip is restored afterward.
Only a real wall FORE below enables this per-entry clip; synthetic room-border
corners stay intact. Lower wall-side updates also redraw the higher-priority
corner above in the bottom-to-top pass. The extra below-row cache entry covers
the last column. All 20 existing climb/fall captures remain pixel-identical.

The `--floors` test suite checks 1,050 cases, including exact redraw pixels
and loose-floor component alignment. v11 is complete; the broader palace
wall implementation remains a separate v13 milestone.

## v11.18 generic airborne MID phase

`redraw_frames2` now invokes `mac_draw_other_overlay_exact()` rather than a
full background redraw. Existing native components are routed to MID only
during this pass, preserving their exact SHPD coordinates. The legacy
empty-neighbor conditions, optional BACK copy and previous-cell actor redraw
are retained. Foreground, full-tile and dedicated climb-overlay phases stay
separate. This fixes floor/pillar overdraw during ledge-fall frames 102..106
without altering actor placement or gameplay.

## v11.17 additional outline boundaries

The edge helper now includes res221 / frame 138 below its first four physical
rows, preserving the elbow, and res227 / frame 144 for the stray shoe dot.
Each restores one column from registered floor-top art. Canonical captures
change three pixels in frame 138 and one in frame 144; actor anchors, native
assets, and all other poses are unchanged. Tests cover 120 edge cases, both
reported marks, and consecutive 138..146 frames at all outline probes.

## v11.16 climb outline coverage

Masks res222..224 leave one column of the port's actor outline beyond their
right edge. The physical MID draw restores that column from floor-top res241
with SHPD-origin alignment. The helper clips the draw to one physical pixel
and the existing viewport, then restores the clip. This preserves native
texture and all v11.15 actor/mask anchors without changing the PAK assets.

The renderer test now checks exact pixels for 72 edge cases, unchanged next
column and clip bounds, and matching clean/incremental renders at the former
line columns through consecutive finish frames 144..146.

## v11.15 climb floor-top registration

Native climb masks share the floor-base anchor (`draw_main_y + 1 - 4`, plus
one logical pixel for a stuck plate). Their previous DOS overlay anchor was
six physical pixels too low. All eight masks in both dungeon and palace packs
match the floor-top pixels exactly at the corrected SHPD alignment.

The original Mac KID553 reference also supports lifting the four displayed
post-row-change climb poses (141..144 / KID552..555) six native pixels. The
lift is applied only in the native KID `draw_mid()` path, before peel bounds
are computed. Mirrored origins and X positions are retained; gameplay state,
frame-table offsets, and all other body poses remain unchanged. The floor
front and pillar FORE layer are unchanged.

See `tools/mac_native_renderer/test_climb_overlay.py` for the real-renderer
regression and optional captured climb sequence. See the current section of
`V11_INTERACTIVE_FLOOR_TESTS.md` for remaining motion playtests.

## Rendering architecture

Gameplay remains logical 320x200. Native Macintosh images render 1:1 into a 640x400 framebuffer. Legacy images scale 2x.

Environment placement uses SHPD origins:

```text
physical_x = logical_anchor_x * 2 - origin_x
physical_y = logical_anchor_y * 2 - origin_y
```

Mirrored actors use:

```text
mirrored_origin_x = image_width - 1 - origin_x
```


## v11.0 interactive floor compositor

The floor path now treats each interactive tile state as a native Macintosh composite rather than applying generic DOS bottom-alignment corrections independently to each draw call. A `mac_floor_state_type` groups the base, right-edge and front resource IDs with their logical Macintosh anchors.

Covered states: normal floor, spikes, depressed/stuck drop button, drop button, raise button, loose-floor animation frames, debris and debris-with-torch.

The exact renderer also removes the redundant generic floor-right wedge from the exposed-edge prepass. State-specific right edges are submitted once. The depressed button keeps a context-sensitive edge rule so exposed/non-floor neighbors use the full normal wedge while a continuing floor uses the depressed seam.

Falling loose floors are now submitted through an origin-aware physical midtable path. `curmob.y` remains a gameplay/legacy baseline; the renderer derives the Mac anchor origin as `obj_y - 2`, then applies the same per-piece anchors as the static compositor. No mob physics or collision values are changed.

Diagnostics:

```bash
SDLPOP_MAC_FLOOR_DEBUG=1 ./prince
```

This prints component IDs, anchors, SHPD origins and physical positions for floor pieces.

## v10 sword attachment correction

SDLPoP actor X exists in a 280-wide coordinate space before `calc_screen_x_coord()` maps it to 320 screen pixels.

The legacy sword path added `sword_tbl[].x` after conversion. With native actor pivots this made the attachment offset inconsistent with the actor anchor.

v10 applies `sword_tbl[].x` with `obj_dx_forward()` before `calc_screen_x_coord()`. Y attachment remains in logical vertical pixels as before.

No gameplay/collision state is changed. Only the object render anchor differs.

The previous Prince-only `-3 X / -1 Y` visual bias has been deleted.

## Debugging

Set:

```bash
SDLPOP_MAC_ACTOR_DEBUG=1
```

Sword frames print enough information to identify a remaining bad attachment by exact animation frame and sword resource.

## Stable prior work retained

- origin-aware Mac environment compositor
- native room draw ordering
- actor SHPD origin/mirroring
- gameplay/HUD clipping
- Princess PV composition fixes
- v9 native title compositor and centered title overlays

Full project state and future milestones are in `HANDOFF_MAC_RENDERER.md`.

## v10.10 combat attachment

Final cleanup for the two remaining Prince low-lunge poses reported after v10.9: frame 155 now receives a dedicated correction, and frame 157 is strengthened.

Pickup frames 229..236 keep their existing calibration. Normal fencing uses a separate backward-relative-to-facing X correction measured from native Mac hand/hilt pixels. This avoids applying pickup-specific offsets to combat and preserves mirroring automatically.


## v10.11 guard hand-lock cleanup

Two isolated guard poses are micro-calibrated in the Mac-only combat exception table: frame 158 (GUARD760 + S705) moves slightly toward the hand; frame 165 (GUARD767 + S709) moves left/up in the reported left-facing capture. The corrections stay facing-relative and do not affect pickup frames or gameplay coordinates.


## v10.12 Prince hand-lock cleanup

Two additional Prince poses are tuned in the same Mac-only combat exception table. Frame 152 (KID576 + S717) receives a very small backward hilt lock toward the fist. Frame 155 (KID579 + S718) moves left/up by 2 logical pixels in the reported right-facing capture so the blade clears the torso and sits above the hand. These corrections remain facing-relative and do not affect pickup frames or gameplay coordinates.

## v10.13 Prince frame 155 hilt lock

The paused low-lunge capture identified Prince frame 155 / KID579 + S718 with debug values `final_x320=67`, `final_y=105`, `combat_dx=9`, `combat_dy=-2`. Inspecting the native Macintosh assets shows KID579 has SHPD origin `(-3,68)` and S718 has origin `(0,8)`. For the right-facing pose both images are horizontally flipped, which puts the S718 hilt on the sword sprite's left edge. At the v10.12 anchor the hilt therefore sits roughly 28 native pixels too far left, hidden underneath the forearm.

Frame 155 is changed from `combat_dx=9, combat_dy=-2` to `combat_dx=-5, combat_dy=-3`. For the reported right-facing pose this moves the sword 14 logical pixels right (28 native pixels) and 1 logical pixel up (2 native pixels), aligning the hilt with the fist while leaving the blade visible beyond the hand. No other combat frame, guard calibration, pickup sequence, gameplay logic, or global sword attachment math changes.

## v10.14 Prince frame-152 follow-up

A paused playtest capture showed S717 still crossing the Prince forearm in frame 152. The Mac-only per-frame correction is changed from `combat_dx=-10, combat_dy=5` to `combat_dx=-8, combat_dy=3`. In the reported right-facing pose this moves the rendered sword 2 logical pixels left and 2 logical pixels up.

## v10.15 Prince frame 152 reference calibration

A direct 640x400 comparison against the supplied correct frame established the remaining error quantitatively. After aligning the Prince body and static room, the v10.14 S717 blade was 16 native pixels right and 6 native pixels low. Since the Mac compositor maps these to 8 logical X pixels and 3 logical Y pixels, the v10.14 exception `(-8,+3)` was the entire error. Frame 152 therefore uses no extra combat correction: `combat_dx=0`, `combat_dy=0`.



### v11.3 depressed pressure-plate edge
The native floor-state table is authoritative for the right component. In particular, `tiles_5_stuck` keeps image `0x23`; do not apply the DOS neighbor fallback to normal floor image `0x2A` at an exposed edge. The Mac component already encodes the depressed plate geometry and using 0x2A creates a broad/double-width shadow.

### v11.4 exposed-ledge occlusion mask
At a visible floor edge the original compositor draws the below-left corner first, then a generic floor-right silhouette in black, then the tile-state right component. v11.0 removed the middle black pass, so the below-left wall/corner diagonal could remain visible under a depressed pressure-plate wedge and look like a second shadow. v11.4 restores that black occlusion pass only when `can_see_bottomleft` is true and `tile_left` has `floor_right`; the final visible right component still comes from the native floor-state table.
### v11.5 native physical black-mask semantics
`mac_native_full` exact entries use 640x400 physical positions so SHPD odd-pixel origins are preserved. Prior to v11.5, `draw_back_fore()` bypassed `draw_image()` for these entries and called `draw_native_image_physical()` directly, which ignored `table_entry->blit`. That made `blitters_9_black` ineffective: the v11.4 floor-right occlusion mask was rendered as the original colored `0x2A` wedge.

v11.5 adds `draw_native_image_physical_blit()`. Ordinary native art still follows the existing alpha/colorkey path. For `blitters_9_black`, the helper creates an alpha-preserving ARGB copy, paints only its RGB channels black, and blits it at the same exact physical coordinate. The visible depressed/stuck edge remains native component `0x23`; the black `0x2A` is only an occlusion silhouette underneath it.

### v11.6 depressed plate beside empty gap
Macintosh gameplay reference footage shows that the exposed depressed/stuck pressure plate does not carry the generic horizontal black floor-right strip. When `tile_left == tiles_5_stuck` and `curr_tile == tiles_0_empty`, `mac_draw_left_floor_overhang()` therefore returns before submitting the below-left corner or the generic `0x2A` black silhouette. `mac_draw_left_other_overhang()` still submits the state-native `0x23` right component at its exact SHPD anchor. Other exposed ledges keep the v11.5 occlusion prepass.

### v11.7 depressed-plate mask rule
Runtime captures showed that testing only `curr_tile == tiles_0_empty` was insufficient. `mac_can_see_bottomleft()` also permits other visually open tile states, so the generic `0x2A` black silhouette could still be inserted beneath a depressed/stuck plate. That silhouette itself contains a full-width top band and a triangular body, matching the reported rectangular black strip plus black triangle.

For `tile_left == tiles_5_stuck`, `mac_draw_left_floor_overhang()` now always skips the generic below-left corner and black-mask prepass. The later state-native `0x23` submission is unchanged and remains colored/translucent Mac art. This rule is renderer-only and does not alter tile state, collision, button timing or level data.

### v11.8 wipe clamp and stuck-plate corner
`draw_tile_wipe()` in `mac_native_full` now ends at `draw_main_y` instead of `draw_bottom_y` (the above-room 3-row wipe is untouched). The Mac floor fronts (`0x2B`, `0x24`, `0x60`, ...) end at physical row `2*draw_main_y+1`; the old bottom reached 6 native rows lower and erased the top edge of the wall/tile in the row below, which is never redrawn after a button transition. `mac_draw_left_floor_overhang()` also submits the below-left corner again for `tiles_5_stuck` (still without the `0x2A` black mask), because the wiped right-hand cell relies on it to fill the area under the `0x23` diagonal.


### v11.9–v11.10 native climb midtable occlusion

`redraw_at_char2()` marks Prince climb frames 137..144 with `redraw_frames_floor_overlay`. The DOS compositor uses that signal to place a small floor-left wedge and a duplicate floor/front component in the midtable, intentionally drawing only those pixels in front of the actor during pull-up. v11.9 restored the missing native midtable overlay, but also forced `mac_draw_tile_exact()` for the same flag. That extra full redraw re-submitted normal foretable geometry, including tall pillar foreground strips, and over-occluded the Prince. v11.10 makes the signal overlay-only again: `mac_draw_floor_overlay_exact()` is submitted without a full tile redraw unless another redraw flag independently requires one. If an exposed dungeon pillar has both a foreground redraw and a climb overlay pending for the same tile, its tall native pillar fore-strip is skipped for that redraw because the dedicated MID-table wedge/front lip is the intended climb occluder. Static/non-climb pillar foreground rendering is unchanged. The v11 Mac floor-state front component and SHPD-aware placement are retained.

### v11.11 foreground redraw separation

`redraw_frames_fore` is kept distinct from full native tile redraws, matching the legacy compositor's final `draw_tile_fore()` phase. v11.11 also suppressed the tall dungeon pillar foreground (`res295`) during dynamic foreground redraws for frames 141..144, after the climb sequence's row jump.

### v11.12 complete climb-overlay pillar suppression

The v11.11 playtest still failed on frame 140. That frame is part of the same 137..144 floor-overlay interval and already has the dedicated MID-table wedge/front lip, so repainting the 19x60 native `res295` pillar strip is redundant and cuts through the native Prince sprite. v11.12 therefore suppresses `res295` during dynamic foreground redraws for all frames 137..144. The suppression is limited to dungeon pillars and only to the dynamic FORE redraw phase; static room composition and non-climb pillar depth are unchanged.

### v11.13 Macintosh climb pillar occlusion correction

The v11.10-v11.12 suppression of dungeon pillar foreground during climb-up was backwards. The supplied Macintosh reference shows the ledge/wall side in front of the Prince's lower body. In the failing port capture, KID `res552` visibly overwrites the pillar face, exposing the grey leg and orange foot through the masonry. v11.13 restores the native pillar foreground (`res295`) to FORE during climb while preserving the separate MID floor-overlay pass. This restores the intended layer stack: room back -> climb MID/floor lip -> Prince -> pillar/wall FORE.


### v11.14 native climb occluders

The Mac-native climb overlay must not use the DOS `floor_left_overlay[]` IDs. Macintosh environment resources 220..227 are eight frame-specific triangular occluders for Prince frames 137..144. They are loaded as chtab IDs `0x14..0x1B` and placed with their SHPD origins through `mac_add_mid_anchor_exact()`. In the supplied frame-143 capture, the old DOS `res232` wedge was matched directly at the offending grey patch; v11.14 replaces it with native `res226`. Pillar `res295` remains a FORE occluder as established in v11.13.
