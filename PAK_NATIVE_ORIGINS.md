# Native Macintosh origins in POPSPAK1

The native packs extend POPSPAK1 version 1 with optional `org` records. No
container-version bump is required. The builder is
`tools/mac_native_renderer/build_mac_native_pak.py`; the reader is `src/pak.c`.

All integers are little endian. The 16-byte header is `<8sII`: magic
`POPSPAK1`, version 1, then record count. Each 36-byte record is
`<16s4sHHHBBII`:

| Field | Bytes | Meaning |
| --- | ---: | --- |
| folder | 16 | NUL-padded ASCII resource group, e.g. `KID` |
| extension | 4 | NUL-padded ASCII `png`, `org` or another resource type |
| resource ID | 2 | Unsigned ID within the group |
| width, height | 2 each | Physical image size for native images |
| palette count | 1 | Number of RGB triples, at most 255 |
| flags | 1 | Bit 0: index 0 transparent; bit 1: native resolution |
| offset, size | 4 each | Absolute payload offset and payload byte count |

Records sort by case-insensitive folder, numeric ID, then case-insensitive
extension. A `png` payload is **not an encoded PNG file**: it contains palette
RGB triples followed by width × height palette-index bytes. The dimensions
exclude SDL surface pitch/padding.

Each native image has a same-folder, same-ID `org` record. Width, height,
palette count and flags are zero. Its four-byte payload is `<hh`: signed
16-bit SHPD origin X, Y in original physical pixels. Negative origins and
origins outside the image are valid. Never clamp or scale them. Older packs
without the native flag retain DOS sizing; a native image without an origin
uses the reader's existing native-size fallback rather than an invented origin.

For a 320×200 logical anchor `(x, y)`, native top-left is
`(2*x - origin_x, 2*y - origin_y)` on the 640×400 surface. A mirrored image uses
`width - 1 - origin_x`. Actors first convert their separate 280-wide X space
to 320-wide screen X. Sword attachment offsets are applied after that conversion;
see `MAC_SWORD_REFERENCE_AUDIT.md`. Gameplay uses a 640×384 room surface with
the remaining 16 physical rows reserved for the HUD; cutscenes use all 400 rows.

The loader puts immutable origin metadata in a bounded static pool shared by
equal origin pairs. SDL temporary flips may share that metadata without owning
it. Sprite surfaces are still freed by the normal chtab/surface paths.

The stable pack contains 517 images plus origins; native-full contains all
820 images plus origins. The original input manifest and source PNGs remain
unchanged. Rebuild into a temporary output when verifying reproducibility:

```sh
python3 tools/mac_native_renderer/build_mac_native_pak.py \
  '../Mac-Color-Native-Graphics' /tmp/res_mac_native_full.pak --full
cmp data/res_mac_native_full.pak /tmp/res_mac_native_full.pak
sha256sum -c MAC_NATIVE_SHA256SUMS.txt
```
