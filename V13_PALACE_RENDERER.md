# v13.1 native palace walls — implementation and automated audit complete

Later correction at v17.0: an adjacent torch refresh can overwrite the palace
exit-left right-piece's one-physical-column backward overhang (res237, origin
X=1). Native background dependency expansion now restores its next-cell piece
when the real SHPD origin confirms that overlap. The earlier wall stripe,
sword/skeleton dependencies, private RNG and wall palette are preserved.
See `V17_WHOLE_GAME_VISUAL_REGRESSION.md` for real-map evidence and checks.

The user confirmed v13.0 works as expected. The final boundary/fake-wall audit
found and fixed two incremental redraw defects in v13.1. The palace milestone's
implementation and automated audit are complete; v13.1's new fixes await live
playtesting. Broader original-Mac scene comparisons remain a release-validation
task. V12 regressions stay enabled; v12.3's separate edge-case playtest is not
inferred from a request to continue.

## v13.1 redraw edges — 2026-10-26

The native palace wall stripe is 32 logical pixels wide and starts 24 pixels
into the wall-side cell. Its final 24 pixels cross into the following cell.
Refreshing the side could overwrite that cell's decoration; wiping the next
cell could erase the stripe. The renderer now expands background redraws to
both cells before collecting the row's drawing tables, retaining BACK order.
Visible fake walls use the same dependency; the no-blue modifier skips it.
FORE-only and actor-overlay requests retain their separate semantics.

Real potion/sword animation checks also exposed a two-pixel sword outline
over the next floor cell. The native sword spans one physical column beyond
its cell. Sword background refreshes now restore the next cell afterward;
actual pickup requests erase/restore that edge before the old sword state is
lost. This covers dungeon and palace, skips the offscreen right boundary,
and leaves other packs and pickup effects/timing unchanged.

The expanded audit adds 180 fake/real visual-equivalence cases, 279 linked-room
checks (nine views, each full and 30 partial refreshes), and 864 real animation
and pickup checks across both environments, three rows, two positions and four
real/fake/blue/no-blue wall states. Gameplay RNG is checked after rendering.
The earlier 200 palace checks remain. All 279 named captures shared with v13.0
are pixel-identical; only the defective partial refreshes change.

Evidence: `screenshots/v13.1/audit-results.txt`,
`v13-regression-comparison.json`, and visually reviewed
`palace-redraw-comparison.png`. Before-fix captures remain under
`screenshots/development-audits/`.

## Original reference recovery — 2026-10-26

Preserve the user-supplied files at workspace root:

- `Mac-Color-Native-Graphics/`: 820 RGBA PNGs and the SHPD origin manifest.
- `PoP1_spf.dsk`: original HFS disk; SHA-256
  `455cb7bd1bc37bfd21c24fd91b34d2c0aee488d9b066d48fd7e6a49ddd3ee504`.

Read-only extraction recovered nine files and their data/resource forks.
The application contains CLUT 2000 with 201 indexed colors. A fresh decode
reproduces all 820 exported images/origins, covering **3,835,931 pixels** across
19 SHPD groups. All 174 opaque export colors occur in the original CLUT.

Both installed native packs rebuild byte-identically from the PNG folder;
the builder now accepts either an extracted folder or its original ZIP.
The installed packs and original source files were not modified. Reference
forks, hashes and inventory are under `tools/mac_reference/`. The extractor
supports this raw classic-HFS image and inline catalog extents, and refuses
unsupported overflow extents instead of silently truncating forks.

## Recovered drawing model

Offsets below are resource-relative, including the four-byte CODE header.

| Routine | Original location | Ported behavior |
| --- | --- | --- |
| Rendering RNG | CODE 2: `$223E` | Original 16-bit arithmetic and 0x6A59 multiplier, using private state |
| Room colors | CODE 3: `$0B72` | Room seed, discarded sample, 3 rows x 4 courses x 11 colors; adjacent colors differ |
| Main wall | CODE 4: `$5CB6` | Five body rectangles, bottom rectangle and five SHPD detail pieces |
| Bottom-only wall | CODE 4: `$5EE6` | Bottom rectangle and SHPD detail in BACK |

Even courses use CLUT 76..79; odd courses 80..83. The exact RGB values are
in generated `src/mac_palace_palette.h`, whose comment records the original
CLUT resource hash. The rendering RNG/cache never touches gameplay random_seed.

Main rectangle dimensions are 32x20, 16x21, 16x21, 8x19 and 24x19 in logical
320x200 coordinates; the bottom is 32x3. Positions/detail selections follow
the recovered code while retaining the existing three-logical-pixel native
floor registration bridge. Detail groups are IDs 15..17 for the bottom, then
3..5, 6..8, 9..11 and 12..14, all using their SHPD origins.

The initial resource inventory appeared to select decorative arches or tiny
crack strips as wall faces. The recovered code clarified the missing step:
those sprites are underlayers/details; the opaque generated wall face was
absent. The new compositor supplies that step without modifying artwork.

Native RGB rectangles enter BACK/MID/FORE as native_entry_fill (mode 2), before
image lookup. They obey viewport clipping and outward-rounded logical dirty
rectangles. Bottom fills route to MID during airborne overlays; faces/details
stay FORE for actor occlusion. Sprite entries remain modes 0/1. Dense walls
exceed the old foreground capacity, so FORETABLE_MAX is 600; full/above-room
wall fixtures exercise it. seg008.o depends on the generated palette header.

## Reproduce

From the active source folder:

```bash
python3 tools/mac_native_renderer/extract_mac_reference.py ../PoP1_spf.dsk --output tools/mac_reference
python3 tools/mac_color_port/decode_mac_pop_reference.py --hfs-root tools/mac_reference/hfs-forks --output tools/mac_reference/decoded
python3 tools/mac_native_renderer/verify_mac_reference.py
python3 tools/mac_native_renderer/disassemble_mac_code.py
python3 tools/mac_native_renderer/build_palace_wall_reference.py --header src/mac_palace_palette.h
make -C src -j2 CC=/usr/bin/gcc
python3 tools/mac_native_renderer/test_climb_overlay.py --floors --architecture --palace --output screenshots/v13.1
sha256sum -c MAC_NATIVE_SHA256SUMS.txt
```

The disassembler uses the installed libcapstone.so.5 public C API. Normal
builds do not require Capstone or disk extraction. The PNG tools use Pillow.
The former web decoder is now import-safe and accepts local input/output paths.

## Verification and limits

| Check | Result |
| --- | --- |
| Palace and pickup audit | 1,523 passed, 0 failures |
| Retained architecture audit | 5,503 passed, 0 failures |
| Retained floor audit | 1,050 passed, 0 failures |
| Shared dungeon/climb/fall captures vs v12.3 | All 124 pixel-identical |
| Both installed native PAK hashes | Unchanged |
| Shared v13.0 captures | All 279 pixel-identical |

The original 200-check palace audit checks four full 640x384 wall scenes against independent
Python fixtures (room seeds 1, 4, 12, 24), totaling **983,040 pixel comparisons**;
120 individual tile redraws; 72 actor/neighbor/edge arrangements; and four
actual level-4 rooms. It asserts gameplay RNG preservation. Fixtures use
recovered palette/routine constants and original PNGs, not C output. They
validate the documented interpretation independently of C, but do not replace
an original-Mac emulator/playthrough comparison.

100 shared palace captures intentionally change because their wall faces now
exist. The change list is in screenshots/v13.0/v12-regression-comparison.json.
Original captures, visually reviewed palace-wall-comparison.png and audit-results.txt
are in screenshots/v13.0/. The user confirmed that generated-wall pass.

Next milestone: v14 props and pickups (potions/bubbles, sword, skeletons,
torches, debris, balconies and lattice). The wall-adjacent pickup checks above
provide its first regression coverage. Live v13.1 edge validation and broader
palace-level/original-Mac comparisons remain explicitly tracked, without
claiming whole-game Macintosh fidelity.
