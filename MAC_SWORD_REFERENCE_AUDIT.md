# Native sword pickup and combat reference audit

Completed before the v17 sweep, 2026-10-26. Included in playable v17.0.
The user confirmed v16 damage/healing, pause/resume and intro fades first.
The new sword placement still awaits live confirmation.

## Original evidence and correction

The original application on `PoP1_spf.dsk` contains a dedicated Macintosh
sword table at A5-$4CCC. Expanding DATA/ZERO as the original initialization
routine does recovers 51 four-byte records: sprite index, signed X/Y offsets
and a padding byte. CODE 3:$583E..$58C6 selects that table. It converts the
body anchor from 280-wide actor X to 320-wide screen X **before** adding the
facing-relative sword offset. Y is added to the body anchor directly.

The original Kid and guard frame tables are at A5-$556C and A5-$4FC6.
All 241 Kid and 41 guard records match SDLPoP's frame data exactly, including
sprite selection, sword index, dx/dy and flags. This establishes that the
recovered relative weapon offsets apply to these same body poses.

The renderer previously used DOS sword offsets in actor space, then added
frame-specific guessed screen offsets from the v10 experiments. Those could
look close but differed from the original table and depended on X rounding.
Native-full now uses generated `src/mac_sword_table.h`, with the original
conversion order. The old native calibration tables are removed. DOS and the
stable graphics pack keep their existing DOS attachment path.

Example original versus DOS table offsets, in logical screen pixels:

| Sword table entry | Mac X/Y | DOS X/Y | Example use |
| --- | --- | --- | --- |
| 16 | (9,-34) | (13,-35) | Prince parry |
| 15 | (21,-22) | (28,-23) | Prince ready/stepping |
| 23 | (13,-16) | (7,-13) | Prince low strike |
| 8 | (16,-18) | (16,-19) | Guard ready |
| 35 | (-2,-33) | (7,-32) | First sword pickup pose |
| 38 | (7,-26) | (14,-29) | Pickup transition |

Screen anchors use 320x200 logical coordinates. The native renderer doubles
these to physical 640x400 pixels, then subtracts each sprite's SHPD origin.
For right-facing art it mirrors both the pixels and origin. Body and weapon
object sorting retains the original higher-anchor-before-body composition.
Gameplay coordinates, collision, damage, AI, timing, frame/sequence tables,
replays, native source PNGs and installed PAKs are unchanged.

## Reproduce and verification

```bash
python3 tools/mac_native_renderer/build_combat_reference.py \
  --header src/mac_sword_table.h
make -C src -j2 CC=/usr/bin/gcc
python3 tools/mac_native_renderer/test_climb_overlay.py --combat \
  --output screenshots/combat-review
```

Normal builds depend on the generated header; they do not need the disk,
Capstone or the extraction scripts. The generator reads original data and
source PNGs and writes an independent pixel oracle under
`tools/mac_reference/combat-oracle/`, along with provenance and case metadata.
That oracle is separate from the C renderer and does not use its calibration
or runtime drawing helpers.

All **4,368 combat checks pass**:

- 322 independent body/weapon composites: 64 Kid, 52 Guard, 52 Fat, 52 Vizier,
  50 Shadow-art and 52 Skeleton-art cases. These include both facing directions.
- Five X positions per case cover conversion rounding and both viewport edges.
  Anchors and entire composited pixel images must match; actor state is checked.
- Actual valid Prince/guard sequence playback covers pickup, draw/sheath,
  strikes, parries, strike-after-parry, forward and backward steps. Frames
  without a weapon must submit no sword.
- The v16.0 object files fail **2,874 of 3,220** static placement/pixel checks
  against the same independent oracle. The corrected renderer passes.

The original Shadow SHPD group omits resource768 (inactive frame166); the
generator records that absent slot instead of inventing native artwork.
Shadow-family pixel fixtures use its native art with ordinary guard composition;
the real shadow XOR/reflection/handoff path remains covered separately by the
architecture suite. There is no newly invented guard pickup animation; the
actual sword pickup sequence belongs to the Prince.

Evidence in `screenshots/v17.0/`: `combat-comparison.png`, individual
`combat-*` captures, `combat-reference.json`, `combat-negative-probe.txt`
and the combined `audit-results.txt`. Original pre-fix captures remain under
`screenshots/development-audits/v16-combat-before/`.

This verifies placement against original program data and decoded art at the
retained actor anchor. It is not a recording/emulator comparison of complete
Mac motion, timing or every overlapping pair of actors. Live check: pick up
the sword, draw/sheath, step both ways, attack and parry against a guard, then
repeat with the Prince on the opposite side. Observe hand/hilt contact and
foreground occlusion. Fat guards, Jaffar, Shadow and Skeleton remain useful
release playtests.
