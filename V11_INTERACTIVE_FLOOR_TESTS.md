# v11.19 Interactive Floor / Occlusion Regression — CLOSED

Later v17.0 edge correction: native dungeon big-pillar-top foreground res291
starts one physical row above its logical cell (originY125). Refreshing the
upper floor now restores only that cap overlap in FORE, clipped at the cell
boundary. The rest of the lower pillar and all v11 climb/airborne masks and
the v11.19 wall-corner clip remain unchanged. See V17_WHOLE_GAME_VISUAL_REGRESSION.md.

v11 closed after the user authorized the remaining audit. The full renderer
suite passed 1,050 floor checks in dungeon and palace modes, plus the retained
climb/fall tests. Representative runtime captures were visually reviewed.
The shared one-pixel exposed-floor/wall-border redraw issue found during the
audit was fixed in v11.19. See `V11_CLOSURE_REPORT.md` for the final coverage
table and scope; the sections below remain historical regression references.

```bash
python3 tools/mac_native_renderer/test_climb_overlay.py --floors --output screenshots/v11.19
```

Earlier v11.18 status: the user confirmed the climb fixes through v11.17 and the v11.18
ledge-fall fix in play. Keep those scenes as regression checks. Remaining
v11 validation targets are loose floors, spikes, plate transitions and edge
combinations, debris, floor seams, and row/room boundaries. Prioritize the
loose-floor shake-to-fall transition next; no additional defect is currently
confirmed by this session.

## v11.18 current airborne regression

Step off the reported ledge and inspect frames 102..106. The floor plane and
pillar should occlude the overlapping body pixels; trousers, torso and feet
must not overwrite the masonry or floor surface. The user confirmed he should
disappear behind the pillar during this fall. The native airborne overlay now
follows the legacy MID draw order. No actor positions changed.
Recheck a ledge at the opposite side, a wall-supported ledge, a depressed plate
edge, loose floor, an airborne room transition, and the completed climb suite.

```bash
python3 tools/mac_native_renderer/test_climb_overlay.py --output screenshots/v11.18
```

The runner now also saves `fall-frame-102.png` through `fall-frame-106.png`
and checks their sequential redraws. Final in-game visual playtest remains.

## v11.17 final-pixel regression retained

Check frame 138's elbow/body area: the three-pixel vertical tail below the
elbow should be gone, while the arm and upper outline stay intact. In frame
144, the isolated black dot beside the shoe should be gone, with the shoe
outline itself intact. Recheck the whole climb up/down and the v11.16 fixes.
Actor placement and gameplay remain unchanged.

```bash
python3 tools/mac_native_renderer/test_climb_overlay.py --output screenshots/v11.17
```

The new fixture should differ from v11.16 only at those four pixels. The
regression also checks the old/new outline locations across consecutive
138..146 frames without resetting the room.

## v11.16 outline regression retained

Revisit the reported climb in both directions, especially the finish through
frames 144..146. No thin black outline should remain inside the floor under
the Prince. Resources 222..224 now cover one additional physical column using
the registered floor-top pixels. Actor placement remains exactly as v11.15.
Check the whole sequence in climb-up and climb-down, along with a depressed
plate edge and normal movement behind the pillar.

After building, run:

```bash
python3 tools/mac_native_renderer/test_climb_overlay.py --output screenshots/v11.16
```

This includes exact edge-pixel/clip checks and consecutive frames 144..146
without resetting the room, so old pixels must be restored correctly. The
original sprite assets, floor texture, and neighboring column are preserved.

## v11.15 climb regression retained

The native climb masks now share the floor-top anchor, moving them six physical
pixels upward from v11.14. Capture frames 141 (KID552/res224) and 142
(KID553/res225), then check the entire 137..144 sequence in both climb-up and
climb-down. The visible trousers should meet the actual sloping edge rather
than a second diagonal inside the floor. The mask texture must register with
the static floor. Test normal floor, pillar, torch floor, and depressed/stuck
plate edges; a depressed plate keeps its one-logical-pixel lower anchor.

The displayed body poses 141..144 also move six native pixels upward to match
the original Mac reference. Check for a smooth 140->141 row change and a smooth
144->145 return to the regular anchor, in both climb directions. Earlier and
later body poses retain their anchors. Gameplay position and collision are
unchanged. Check ordinary movement behind pillars and a left-facing climb at
the opposite ledge; pillar foreground remains enabled.
With floor debugging enabled, `role=climb-overlay` should report an anchor Y
three logical pixels above the old baseline (two above for a stuck plate).
Historical sections below describe earlier iterations; this section defines
the current mask-registration expectation. Final visual playtest is required.

Automated renderer check (after building):

```bash
python3 tools/mac_native_renderer/test_climb_overlay.py --output screenshots/v11.15
```

Use `graphics_pack = mac_native_full`. The v11 source changes rendering/redraw coverage only; gameplay/collision/timing should behave exactly as v10.15.

## Diagnostic mode

```bash
SDLPOP_MAC_FLOOR_DEBUG=1 ./prince
```

`MAC_FLOOR` lines report `tile`, `mod`, component `role`, 1-based environment `image`, Mac resource ID, logical anchor, SHPD origin and final physical X/Y. Disable the variable for ordinary play because room redraws can produce a lot of output.

## Required visual checks

1. **Normal floor** — inspect long runs of floor and a floor ending beside empty space. The top/front/right pieces should form one clean edge with no doubled dark wedge.
2. **Drop button (red)** — inspect idle and pressed states. Pressing must not create a two-tile-looking/double-width dark lip.
3. **Depressed/stuck drop button** — compare one with floor continuing to its right and one with empty/non-floor space to its right. Both continuing-floor and exposed-edge contexts keep the depressed plate's native `0x23` edge. A stuck/depressed plate must **not** receive the generic black `0x2A` mask; the below-left corner is still redrawn beneath transparent parts of `0x23`.
4. **Raise button (green)** — inspect the idle plate, its left-edge variant beside empty space, and the transition after it is pressed.
5. **Loose floor shake** — watch every visible shake frame for base/front/right alignment. There should be no seam opening between the three components.
6. **Loose floor fall transition** — capture the final static/shake frame and first falling frame. At the same `curmob.y` baseline, base/front/right placement should not jump downward.
7. **Falling loose floor** — follow it through each row and across a room boundary. It must stay intact and must not leave stale fragments.
8. **Debris / broken floor** — inspect ordinary debris and debris-with-torch, including exposed right edges. Torch/sconce layering must remain present.
9. **Spikes** — inspect retracted, rising, fully raised and retracting frames. The static floor components and animated spike layers should stay registered. v11.3 retains the v11.2 fix that expands the Mac-native spike wipe from `0x21` to `0x25` logical pixels so tall-frame tips are fully erased while retracting; no detached bright needles should remain above a retracted plate.
10. **Regression** — recheck one v10.15 combat scene, title, Princess room and HUD boundary to ensure the floor work did not disturb prior milestones.

## What to send for a remaining defect

A screenshot at 640x400 plus the nearby `MAC_FLOOR` terminal lines is enough to identify the exact tile state/component. Include the room/level if easy to reproduce.

## v11.3 pressure-plate edge/redraw check

Mac Native FULL now uses a 0x14 logical-pixel wipe for button state changes instead of the DOS 0x11 band. This is intentionally renderer-only and exists to clear the highest native pressure-plate top/right pixels before the new state is composited.

Retest both raise and drop buttons in these contexts:

- button beside normal floor;
- button beside empty space;
- two adjacent buttons;
- press and release while the Prince is standing on the seam.

There should be no stale upper lip or double-width dark shadow after either transition.


## v11.3 exposed depressed-button edge

A runtime capture showed that a depressed/stuck drop button beside empty space was still selecting the normal-floor right wedge (0x2A). The depressed top is 2 native pixels lower, so the normal wedge reads as an oversized/double-width dark shadow. Mac Native FULL now keeps the state-native depressed edge (0x23) at its own anchor for both continuing-floor and exposed-edge contexts.

Retest a depressed drop button immediately beside a gap. The right edge should remain a short depressed-plate lip rather than expanding into the normal-floor wedge.


## v11.4 exposed-edge shadow regression
Retest a depressed/stuck pressure plate immediately left of an empty tile, especially when a wall or floor exists below-left. There must be only one visible ledge diagonal. The below-left corner must not show through beneath the state-specific pressure-plate wedge. `SDLPOP_MAC_FLOOR_DEBUG=1` should report `right-mask` followed by the real `right` component for that exposed edge.
## v11.5 exact-native black-mask regression
The v11.4 compositor order was correct, but the native physical draw path ignored `blitters_9_black` and rendered the `0x2A` mask as a second colored wedge. v11.5 makes that physical mask genuinely black while preserving exact SHPD-origin placement.

Retest the same depressed/stuck plate immediately left of a gap. Expected result: one visible `0x23` depressed edge, with no teal/colored second wedge under it. With `SDLPOP_MAC_FLOOR_DEBUG=1`, `right-mask` should still precede the real `right` component.

## v11.6 Mac-reference pressure-plate check
Retest a depressed/stuck pressure plate immediately left of an empty tile. Expected Mac-native result: the lowered top meets a single `0x23` triangular end-cap with **no continuous black strip beneath the plate**. `SDLPOP_MAC_FLOOR_DEBUG=1` should show the real `right` component but no `right-mask` for this exact stuck+empty context. Normal exposed floor ledges should still report/use their black occlusion mask.


## v11.7 stuck-plate mask regression
Retest the same depressed/stuck pressure plate in every visually open right-neighbor context, not only literal empty tiles. Expected result: the exposed end is the colored native `0x23` wedge. There must be **no black triangular fill** and **no full-width rectangular black strip** under the plate/ledge. With `SDLPOP_MAC_FLOOR_DEBUG=1`, a stuck plate should report its real `right` component and should never report `right-mask`.

## v11.8 pressure-plate strip / triangle regression
Step on and off a drop button that has a gap to its right and a wall/floor row below.
- While pressed: the area under the plate's diagonal shows the below-left wall corner (no black triangle).
- Pressed and released: the stone block under the plate keeps its lit top edge at the same height as the blocks next to it (no black band between plate front and wall).
- `SDLPOP_MAC_FLOOR_DEBUG=1` still shows no `right-mask` for a stuck plate.
Note: the unpressed drop button front (`0x60`) has two opaque black rows built into its art; those are Mac-native and unchanged.

## v11.10 climb / pillar occlusion regression

Retest frames 137..144 at exposed floor edges and beside pillars. v11.10 removes the accidental full-tile redraw from the floor-overlay flag and suppresses the tall dungeon pillar fore-strip when that same exposed pillar is using the climb overlay. Only the small climb wedge and floor-front lip should pass in front of the Prince. Watch specifically for the torso/head disappearing behind a full pillar face; that must not occur.

Climb up through frames 137..144 at:

- a normal floor edge;
- a dungeon pillar edge;
- a depressed/stuck plate edge;
- a torch-floor edge;
- the big-pillar-top testcase covered by `FIX_BIGPILLAR_CLIMB`.

Expected result: the small floor-left wedge and the floor/front lip pass in front of the Prince exactly during the pull-up frames. The Prince must not show through the floor/pillar lip, and there must be no extra persistent overlay after the climb completes. Actor X/Y, collision and climb timing are unchanged.

## v11.12 full climb-interval pillar regression

Retest the same climb sequence from frames 137..144. Frames 137..140 should retain normal pillar depth while the Prince is still hanging below the ledge. After the row jump, frames 141..144 must no longer have the tall dungeon pillar foreground strip cut through the Prince's torso; only the climb wedge/floor-front lip should occlude him. Also verify ordinary walking/running behind pillars still uses the normal foreground strip.


For v11.12, capture frame 140 specifically. Its horizontal torso with dangling leg must no longer be cut by the tall pillar-front strip. Also verify frames 137..139 and 141..144, then walk/run behind the same pillar to confirm normal non-climb foreground depth is preserved.

## v11.13 climb/pillar reference test

Use the supplied original Macintosh climb screenshot as the visual target. During the `res552`-style bent pull-up pose, the upper torso/head/arm remain visible above the floor, but the grey lower leg and orange foot must be hidden by the vertical pillar/wall face. The pillar face must not disappear behind the actor. Retest the complete 137..144 climb sequence and ordinary movement behind the same pillar.


## v11.14 native climb-overlay regression

Use the supplied original Macintosh screenshot as the reference for frames 137..144. The coarse grey DOS wedge must be gone. In frame 143 specifically, the trousers should be occluded by the small native floor-boundary wedge (`VDUNGEON/VPALACE res226`), not by DOS `res232`. Confirm the lower leg still disappears behind pillar foreground `res295` as in v11.13, and verify all eight climb frames use clean teal Macintosh floor-edge shapes without a grey/pixelated patch.
