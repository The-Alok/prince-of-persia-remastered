# V16.0 — HUD and UI audit

Implementation and automated audit complete, 2026-10-26. The user confirmed
damage/healing, pause/resume and intro fades work as intended. Other manual
checks below remain separate; this confirmation does not imply a complete
original-Mac UI comparison.
The rebuilt executable labels the selected pack **Mac Native FULL v16.0
(HUD and UI)**. V17 whole-game visual regression is next.

## Fixes

Native-full full/empty Kid flasks were one sprite slot too far forward.
`get_image()` uses zero-based indices, whereas the original SHPD identifiers
are one-based. CODE 3:$05CC/$05D2 selects images217/218 in group400, corresponding
to resources617/618 and table slots216/217. The previous native branch selected
empty618 for full health and actor619 for an empty flask. The corrected branch
uses the real full/empty art. Source PNG composition verifies the entire icon,
including the bottom/right edges. Health values, damage, healing and blink
scheduling are unchanged. Other graphics packs retain their existing paths.

The existing placement is retained: logical x=8*i, y=192 maps to physical
x=16*i, y=384. Native icons are 16x15 physical pixels and fit the last 16 rows.
Guard icons start at physical x=624 and step left by 16. These HUD screen
positions use 320x200 logical coordinates, not the 280-wide actor X conversion.
No actor coordinates, origins, collision or replay logic change.

Native RGB fades now use their actual source image and handle surface dimensions,
pitch, pixel format, alpha and the destination clip separately. The old RGB
loop ignored the fade-in source argument, read the global gameplay buffer,
and stopped at its 384-row height. A fade-out could leave the bottom 16 physical
rows visible. It also depended on unrelated DOS palette rows for completion.

Native fade-in borrows its supplied source and restores that same source on
completion/interruption. Static fade-out owns a full 640x400 snapshot of the
visible screen, including the HUD. Animated cutscene fade-out borrows the live
cutscene buffer, so actors and flames keep moving. RGB fades use the existing
64 steps/four RGB levels per step and wait calls; completion follows that RGB
curve instead of unused DOS palette values. Native fades leave the global UI
palette intact and never subtract from alpha. Snapshot ownership is released
on completion/interruption. Legacy palette fade paths remain available to
other packs.

## Evidence and checks

Original assets are preserved at the workspace root. The independent health
oracle loads `KID/0617_macgroup400.png`, `KID/0618_macgroup400.png` and each
guard family's `0751` PNG directly. No PAK or source-art changes are required.

The new UI suite has **687 passing checks**:

| Area | Coverage |
| --- | --- |
| Kid health | Every current/max pair for max1..10, all full/empty source pixels, damage redraw and blink |
| Guard health | Guard, Fat, Shadow and Vizier native icons; current0..10, damage, right edge, low-health blink |
| Visibility | Skeleton/mouse health suppressed; shadow suppression outside level12 retained |
| Bottom text | Level, time, pause, quicksave, expired-time and copy-protection messages; footer bounds and erase |
| Menus | Real pause/settings drawing, footer transparency, overlay composition, untouched base image and close restoration |
| Dialogs | Real dialog frame/text and exact peel restoration; no interactive keyboard wait in this test |
| RGB fades | All64 steps in/out; independent source, full400 rows, odd/padded pitch, RGB24/ABGR32/ARGB32, alpha, clips, interruption and palette restoration |
| Cutscene fades | Actual Princess/Jaffar sequences, flames and hourglass during64-step in/out; current composition through final screen edges; return to gameplay buffer |
| Transition | Actual left-to-right transition reaches physical column639 and row399 |
| Scope/state | Legacy fade routing retained; UI draw state/RNG checked; temporary scene fixtures restore their own actor/star/flame state |

A negative probe linked the new public-API checks with the unchanged v15.1
game objects before rebuilding: **137 failures in 271 checks**. These comprised
66 health-image/redraw failures and 71 fade-source/footer/completion failures.
The probe skips format-mismatch cases that would be unsafe in the old raw-byte
loop. The corrected full suite passes without those restrictions. See
`screenshots/v16.0/baseline-negative-probe.txt` and the before captures under
`screenshots/development-audits/v16-before/`.

Full regression:

```bash
make -C src -j2 CC=/usr/bin/gcc
python3 tools/mac_native_renderer/test_climb_overlay.py \
  --floors --architecture --palace --props --ui --cutscenes \
  --output screenshots/v16.0
sha256sum -c MAC_NATIVE_SHA256SUMS.txt
```

All **18,545 counted checks** pass: 687 UI, 1,101 cutscene, 8,681 props,
1,523 palace/pickup, 5,503 architecture and 1,050 floor, plus retained
climb/fall checks. All 467 PNGs shared with v15.1 are unchanged. Those earlier
captures cover room/cutscene composition; the newly added UI captures exercise
the health and fade fixes. Both native PAK checksums match.

The normal executable builds successfully. Its separate SDL dummy-driver
startup ran until a deliberate three-second timeout (exit124), with only the
unavailable haptic-device warning. Build/startup checks do not confirm live
visuals. Logs, pixel-comparison JSON, individual UI captures and
`hud-comparison.png` are saved in `screenshots/v16.0/`.

## Live playtest and limits

Close the old game and launch `./prince` again to load v16.0.

1. Start with three health points: all three Kid flasks should contain red
   liquid. Take damage: only lost points should show the empty bottle, without
   an actor fragment. Check the last point blinking, then healing.
2. Fight a guard: check the icons at the far right and their removal after
   damage/defeat. Recheck a Fat guard, Shadow on level12, and Jaffar when available.
3. Pause/resume and open Settings. Check the menu, footer message, mouse/key
   selection and restoration of the room. Trigger a quicksave/quickload message.
4. Watch title/Princess fades and story-card transitions. Check the bottom edge
   clears with the rest of the picture and moving actors/flames stay animated.
5. If copy protection is enabled, check its message/dialog and dismiss it.

This milestone preserves SDLPoP's existing font/menu design and HUD registration.
The source-art checks establish native icon fidelity; they do not establish
full original-Mac system UI, typography, menu layout or motion equivalence.
Menu input/navigation and actual in-game timing still need live observation.
Earlier pending live/reference checks remain recorded in the prior milestone
notes; continuing the plan is not treated as their playtest confirmation.
