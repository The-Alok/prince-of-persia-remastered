# V17.0 — combat reference and whole-game visual regression

Implementation and automated sweep complete, 2026-10-26. New combat/redraw
fixes await live playtesting. V16 damage/healing, pause/resume and intro fades
were explicitly confirmed by the user. V18 cleanup/release is next.

## Scope and findings

Before moving into v17, the requested sword audit recovered the original Mac
attachment table and replaced the old native guesses. See
`MAC_SWORD_REFERENCE_AUDIT.md` for original code locations, source-data checks,
coverage and comparison images.

V17 adds a repeatable sweep of the actual bundled Levels 1–14. It captures
all 332 populated room records, including unused/disconnected map records,
four animation steps per room (1,328 total), and 14 entry frames initialized
through the real entry routine. Every room/animation frame is compared with a
clean render; static composition must preserve level data and gameplay RNG.
Entry actor layers must match a clean composition without moving the actors.
The entry capture for a falling start may show the actor outside the viewport;
the fixture does not move him into view just for a screenshot.

The sweep exposed two remaining native redraw dependencies:

1. Palace exit-left right-piece res237 has SHPD origin X=1. It reaches one
   physical column backwards into the preceding cell. Refreshing that cell
   during torch animation could repaint the door border. Native background
   dependency expansion now restores the later-drawn next-cell piece when
   its real origin confirms the overlap. Art/origins are unchanged.
2. Dungeon big-pillar-top foreground res291 has origin Y=125, putting its cap
   one physical row above its logical cell. A refresh of the upper floor could
   erase that row. The native partial draw restores only the protruding strip
   in FORE, clipped at the cell boundary. The rest of the lower pillar and its
   actor ordering are preserved. The existing v11 wall-corner clip remains.

A temporary renderer object with just these two dependencies removed fails
**48 world checks** using the corrected room-entry/animation fixture. The
current renderer passes all 3,362 world checks. No production debugging hooks
or changes to gameplay/animation state are introduced.

## Repeatable suite

From the active game directory:

```bash
make -C src -j2 CC=/usr/bin/gcc
python3 tools/mac_native_renderer/test_climb_overlay.py \
  --floors --architecture --palace --props --combat --world --ui --cutscenes \
  --output screenshots/v17.0
sha256sum -c MAC_NATIVE_SHA256SUMS.txt
```

| Suite | Passing checks |
| --- | ---: |
| Floors | 1,050 |
| Animated architecture, XOR and room-linked cases | 5,503 |
| Palace walls/pickup redraw | 1,523 |
| Props/pickups | 8,681 |
| Original-data combat placement | 4,368 |
| Real world maps/entries | 3,362 |
| HUD/UI | 687 |
| Cutscenes/story cards | 1,101 |
| **Total counted** | **26,275** |

Retained climb/fall checks also pass. All 489 PNGs shared with v16.0 retain
identical decoded RGB pixels and dimensions. Earlier captures did not expose
the weapon/room-edge defects; the new combat/world fixtures cover those paths.
Both native PAK hashes still match. Source disk/PNGs are preserved.

The normal executable builds successfully and separately stays running during
a three-second SDL dummy-driver startup check, ending only at the deliberate
timeout (exit124). The haptic-device warning is expected in that check. Build
and automated image checks are distinct from live playtest confirmation.

Evidence in `screenshots/v17.0/`:

- `audit-results.txt`, build/startup logs and negative-probe results.
- `world-scenes.csv`: every real room/entry capture and its four room links.
- `world-tile-coverage.json`: counts of all 32 tile types across the captured maps.
- `canonical-scene-index.json`: 27 verified captures for the roadmap's title,
  Princess room, floor, plates, gate phases, loose floor, spikes, pickup/combat,
  skeleton, mirror, chomper, potions, exits, palace architecture, lattice,
  HUD/menu and room-boundary categories. These combine real maps with the
  retained subsystem fixtures; transient states are not claimed as map defaults.
- `combat-comparison.png`, `level-entry-overview.png`, individual native
  combat composites, 332 real room backgrounds and 14 full-screen entry images.
- `v16-regression-comparison.json`: exact comparison of shared older captures.

## Live checks and fidelity limits

Restart `./prince` to load **Mac Native FULL v17.0 (Combat and world audit)**.
Check sword pickup, draw/sheath, attack/parry and steps from both sides of a
guard. Revisit a palace exit beside a torch and the Level9 room5 upper floor
over the large pillar; look for thin border/cap pixels changing as torches move.

The automated sweep is not a complete played run of all 14 levels. It captures
loaded room states, controlled animation steps and real entry poses; the
retained tests supply isolated transient/combat cases. Original-data sword,
palette and pixel fixtures provide specific reference evidence, not blanket
whole-Mac fidelity. Complete original-Mac motion comparisons, a live full-game
run and previously unconfirmed edge cases remain release-validation work.
