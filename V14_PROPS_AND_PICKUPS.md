# v14.0 props and pickups — implemented, live playtest pending

The requested v14 audit covers potions/bubbles, floor swords and pickup removal,
torches/sconces, skeleton props and waking, debris, balconies and lattice.
The normal game executable is rebuilt. Automated renderer checks pass; this
does not substitute for live playtesting or original-Mac motion comparisons.
V13.0 remains user-confirmed; no new confirmation is inferred for v13.1.

## Corrections

### Potion effect translation and native sprite loading

SDLPoP stores potion effect in bits 3..7 and animation in bits 0..2. The Mac
renderer formerly read the effect directly from the Mac $E0 field, causing
ordinary game modifiers to select the wrong bubbles and bottle. The native
helper now decodes the SDLPoP effect and applies the original Mac art rules:

| SDLPoP effect | Native bottle | Native bubbles |
| --- | --- | --- |
| 0 empty | Small | None |
| 1 health | Small | Red |
| 2 life | Tall | None |
| 3 feather | Small | Green |
| 4 invert | Small | Green |
| 5 hurt | Small | Blue |
| 6 open | Small | Blue |

The original Mac CODE 4:$41BC selects the tall bottle for native type $40;
CODE 4:$42F8 maps bubble colors and suppresses empty/life bubbles. These rules
were recovered from the supplied disk, rather than inferred from DOS bottle
sizes. Dungeon/palace use their respective native bottle images. The existing
eight-phase SDLPoP bubble schedule is retained, including its blank phases;
no new simulation clock or effect encoding is introduced. Nonstandard effect
values retain the red/small fallback rather than indexing past the color map.

The DOS SHPL declares only 23 images in the flame/sword/potion group, while
native SHPD 150 contains 33. Several green/blue images were present in the
unchanged PAK but never loaded. The loader now enlarges only native-full
resource 150 to 33 when the final image's native origin sidecar is present.
A real reload probe verifies the DOS path remains at 23 and the active native
table remains at 33. Original PNGs, disk and both PAKs remain unchanged.

### Skeleton edge refresh

Native dungeon skeleton base res230 is 64 physical pixels wide with origin
X=-1, reaching one column into the next cell. A partial refresh could leave
one wrong pixel there. The existing sword background dependency now covers
the skeleton too, restoring the following cell in its original draw order.
The actual skeleton wake event is also exercised; skeleton state, health,
animation and positions are not changed.

### Lattice foreground with the high-jump setting

The native tile path bypassed the existing `enable_super_high_jump` drawing
rule, leaving lattice base pieces behind the actor. It now routes those pieces
to FORE when the setting is enabled, including the combined lattice/door-top
piece, with their existing native origins/anchors. With the setting disabled,
the original BACK path remains. Gameplay jump/collision rules are unchanged.

Ten checks cover both setting values across five arrangements. Twenty-four
additional checks compare every opaque lattice pixel with the actor-free
background while real standing/crouching/jumping sprites overlap it. A temporary
test executable restoring the old lattice behavior fails 25 checks (five layer
and twenty pixel-occlusion cases); the corrected executable passes all of them.
The temporary probe does not alter the normal source/object files or game.

## Verification

From the active source folder:

```bash
make -C src -j2 CC=/usr/bin/gcc
python3 tools/mac_native_renderer/test_climb_overlay.py --floors --architecture --palace --props --output screenshots/v14.0
sha256sum -c MAC_NATIVE_SHA256SUMS.txt
```

`--props` alone runs the new suite plus retained climb/fall checks. The new
test-only companion is `tools/mac_native_renderer/test_native_props.inc`;
it shares the established clean-versus-consecutive-frame helpers, with no
test hooks compiled into the normal game.

| Prop coverage | Checks |
| --- | ---: |
| Seven effect types, eight phases, two environments: native art selection | 112 |
| Real potion animation and removal: three rows, three columns, four neighbours | 4,536 |
| All nine torch frames plus real animation, debris and linked-left-room views | 2,160 |
| Sword twinkle/removal, including beside another potion and at room edges | 1,224 |
| Static prop single/pair-cell refreshes | 324 |
| Lattice layer selection with high-jump setting off/on | 10 |
| Prop/actor layering and consecutive movement, both facings | 288 |
| Opaque lattice pixel occlusion | 24 |
| Actual skeleton wake: background and actors | 2 |
| Loader scope | 1 |
| **Props total** | **8,681** |

Retained checks: 1,050 floor, 5,503 architecture, 1,523 palace/pickup. Combined
total: **16,757 checks, zero failures**, plus the retained climb/fall checks.
Rendering checks preserve relevant raw tile/modifier state, gameplay RNG and
Kid/Guard state. All **288** named captures shared with v13.1 are pixel-identical.
Native pack checksums pass. The build has no new warnings.

Evidence in `screenshots/v14.0/`:

- `audit-results.txt` and `build-results.txt`
- `props-comparison.png` (visually reviewed before/after bottle, bubbles and lattice)
- `v13-regression-comparison.json`
- `prop-source-inventory.json` and individual runtime captures

Before-fix/probe captures remain in `screenshots/development-audits/`.

## Scope and live checks

The original source has skeleton scenery only in the dungeon, and balcony/
lattice scenery only in the palace. The inventory records those asset limits;
synthetic cross-environment fixtures check safe redraw behavior without claiming
missing original art. Custom teleport balcony art and whole-game/original-Mac
motion fidelity are not claimed by this pass.

Live checks: inspect the life bottle and colored potions, drink/pick up items
near ledges, watch torches at room boundaries, trigger the level-3 skeleton,
and move/jump behind palace lattice with the high-jump setting enabled. Revisit
the earlier climb/gate/exit scenes as usual. V14.0 awaits that confirmation.
The next planned milestone is v15 cutscene composition.
