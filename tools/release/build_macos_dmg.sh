#!/bin/bash
# Build a clean Apple Silicon app and DMG from this source checkout.
# Requires macOS, Homebrew SDL2/SDL2_image, dylibbundler, and Xcode CLT.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
[[ "$(uname -s)" == Darwin ]] || { echo "Run this on macOS." >&2; exit 1; }
[[ "$(uname -m)" == arm64 ]] || { echo "Build on Apple Silicon (arm64)." >&2; exit 1; }
for tool in make clang pkg-config brew dylibbundler hdiutil codesign strings; do
    command -v "$tool" >/dev/null || { echo "Missing $tool. Install Xcode Command Line Tools and Homebrew dependencies first." >&2; exit 1; }
done
pkg-config --exists sdl2 SDL2_image || { echo "Install SDL2 and SDL2_image with Homebrew." >&2; exit 1; }

VERSION="$(sed -n 's/^#define REMASTERED_VERSION "\([^"]*\)"/\1/p' src/config.h)"
[[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "Invalid release version: $VERSION" >&2; exit 1; }
OUT="${1:-$ROOT/dist}"
mkdir -p "$OUT"
DMG="$OUT/prince-of-persia-remastered-v${VERSION}-macos-arm64.dmg"
[[ ! -e "$DMG" ]] || { echo "Refusing to overwrite $DMG" >&2; exit 1; }

# Compile the exact checkout with the installed macOS SDL libraries.
make -C src clean
make -C src -j"$(sysctl -n hw.ncpu)" CC=clang \
    CFLAGS="-std=gnu99 -O2 -include strings.h $(pkg-config --cflags sdl2 SDL2_image)"
[[ -x prince && -x runtime/prince-engine ]] || { echo "Build did not produce both executables." >&2; exit 1; }

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
APP="$STAGE/Prince of Persia Remastered.app"
CONTENTS="$APP/Contents"
GAME="$CONTENTS/Resources/game"
FRAMEWORKS="$GAME/Frameworks"
mkdir -p "$CONTENTS/MacOS" "$CONTENTS/Resources" "$GAME/runtime" "$GAME/data" "$GAME/mods" "$GAME/replays" "$FRAMEWORKS"
cp prince "$GAME/prince"
cp runtime/prince-engine "$GAME/runtime/prince-engine"
cp SDLPoP.ini "$GAME/SDLPoP.ini"
cp README.md COPYING CREDITS.md "$GAME/"
mkdir -p "$GAME/licenses/libxad"
cp src/third_party/libxad/COPYING.LIB src/third_party/libxad/PROVENANCE "$GAME/licenses/libxad/"
cp mods/mods.txt "$GAME/mods/"
cp replays/replays.txt "$GAME/replays/"
cp src/icon.icns "$CONTENTS/Resources/icon.icns"

# Copy only the checked-in DOS/SNES inventory. Macintosh packs and disk images
# are intentionally absent; the launcher imports graphics from the user's copy.
python3 - "$ROOT" "$GAME/data" <<'PY'
import hashlib, pathlib, sys
root, target = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
for line in (root/'tools/release/bundled_data.sha256').read_text().splitlines():
    if not line or line.startswith('#'): continue
    expected, relative = line.split(maxsplit=1)
    source = root/relative
    if not source.resolve().is_relative_to((root/'data').resolve()):
        raise SystemExit(f'Invalid bundled-data path: {relative}')
    if source.name.startswith(('res_mac_native', 'res_mac_color')) or '_macgroup' in source.name:
        raise SystemExit(f'Refusing Macintosh graphics asset: {relative}')
    if hashlib.sha256(source.read_bytes()).hexdigest() != expected:
        raise SystemExit(f'Bundled-data hash mismatch: {relative}')
    destination = target/relative.removeprefix('data/')
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(source.read_bytes())
PY

cat > "$CONTENTS/MacOS/PrinceRemastered" <<'LAUNCH'
#!/bin/bash
set -e
SRC="$(cd "$(dirname "$0")/../Resources/game" && pwd)"
DST="$HOME/Library/Application Support/PrinceRemastered"
mkdir -p "$DST"
rsync -a --ignore-existing "$SRC/" "$DST/"
rsync -a "$SRC/prince" "$DST/prince"
rsync -a "$SRC/runtime/" "$DST/runtime/"
rsync -a "$SRC/Frameworks/" "$DST/Frameworks/"
rsync -a "$SRC/data/" "$DST/data/"
cd "$DST"
exec ./prince
LAUNCH
chmod 755 "$CONTENTS/MacOS/PrinceRemastered" "$GAME/prince" "$GAME/runtime/prince-engine"

cat > "$CONTENTS/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleName</key><string>Prince of Persia Remastered</string>
<key>CFBundleDisplayName</key><string>Prince of Persia Remastered</string>
<key>CFBundleIdentifier</key><string>org.princeremastered.mac</string>
<key>CFBundleExecutable</key><string>PrinceRemastered</string>
<key>CFBundleIconFile</key><string>icon</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleShortVersionString</key><string>${VERSION}</string>
<key>CFBundleVersion</key><string>${VERSION}</string>
<key>LSMinimumSystemVersion</key><string>11.0</string>
<key>NSHighResolutionCapable</key><true/>
<key>LSApplicationCategoryType</key><string>public.app-category.games</string>
</dict></plist>
PLIST

# Bundle Homebrew SDL and its recursive dylib dependencies so players need no
# compiler or Homebrew installation. The game's data files are not touched.
# Put the libraries inside the copied game tree. The runtime symlink makes the
# same @executable_path/Frameworks install names work for both executables.
ln -s ../Frameworks "$GAME/runtime/Frameworks"
for executable in "$GAME/prince" "$GAME/runtime/prince-engine"; do
    dylibbundler -b -of -x "$executable" -d "$FRAMEWORKS" -p '@executable_path/Frameworks/'
done
# Homebrew's SDL2 compatibility build uses dlopen for SDL3, so SDL3 is not
# visible to otool -L or dylibbundler's dependency scan. Bundle it explicitly.
if strings "$FRAMEWORKS/libSDL2-2.0.0.dylib" | grep -F 'Failed loading SDL3 library.' >/dev/null; then
    SDL3_SOURCE="$(brew --prefix sdl3)/lib/libSDL3.dylib"
    [[ -f "$SDL3_SOURCE" ]] || { echo "SDL2-compat needs SDL3, but libSDL3.dylib was not found." >&2; exit 1; }
    cp -L "$SDL3_SOURCE" "$FRAMEWORKS/libSDL3.dylib"
    dylibbundler -b -of -x "$FRAMEWORKS/libSDL3.dylib" -d "$FRAMEWORKS" -p '@executable_path/Frameworks/'
fi
if [[ ! -L "$GAME/runtime/Frameworks" || "$(readlink "$GAME/runtime/Frameworks")" != ../Frameworks ]]; then
    echo "The runtime Frameworks link is missing or points to the wrong directory." >&2
    exit 1
fi
if ! otool -L "$GAME/prince" "$GAME/runtime/prince-engine" | grep -F '@executable_path/Frameworks/' >/dev/null; then
    echo "The executables are not configured to load the copied Frameworks directory." >&2
    exit 1
fi
if strings "$FRAMEWORKS/libSDL2-2.0.0.dylib" | grep -F 'Failed loading SDL3 library.' >/dev/null && [[ ! -f "$FRAMEWORKS/libSDL3.dylib" ]]; then
    echo "SDL2-compat is missing its dynamically loaded SDL3 runtime." >&2
    exit 1
fi
if otool -L "$GAME/prince" "$GAME/runtime/prince-engine" | grep -E '/opt/homebrew/|/usr/local/opt/|@executable_path/\.\./\.\./Frameworks/' >/dev/null; then
    echo "An executable still has an invalid Homebrew or app-bundle library path." >&2
    exit 1
fi
codesign --force --deep -s - "$APP"
ln -s /Applications "$STAGE/Applications"
cat > "$STAGE/Read Me.txt" <<'README'
Prince of Persia Remastered for Apple Silicon

1. Drag “Prince of Persia Remastered.app” into Applications.
2. Open it. On first launch, choose Browse for image and select your own
   Macintosh Prince of Persia game disk/archive (HFS, DSK, DC42, HQX, SIT or ISO).
   No disk image or Macintosh graphics pack is included in this download.
3. The game prepares the graphics locally and remembers the selected image.

This build is not notarized. If macOS blocks it, control-click the app in
Applications, choose Open, and confirm the prompt.
README
plutil -lint "$CONTENTS/Info.plist"

# Verify the payload does not accidentally contain private Mac imports or disks.
if find "$APP" -type f \( -iname '*.dsk' -o -iname '*.hfs' -o -iname '*.dc42' -o -iname '*.iso' -o -iname '*.hqx' -o -iname '*.sit' -o -iname 'res_mac_native*.pak' -o -iname 'res_mac_color*.pak' \) | grep .; then
    echo "Refusing to package a Macintosh disk or imported graphics pack." >&2
    exit 1
fi
hdiutil create -volname "Prince of Persia Remastered" -srcfolder "$STAGE" -ov -format UDZO "$DMG"
shasum -a 256 "$DMG" > "$DMG.sha256"
echo "Created $DMG"
