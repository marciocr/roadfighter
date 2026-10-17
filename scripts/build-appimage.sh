#!/usr/bin/env bash
# Builds dist/RoadFighter-x86_64.AppImage and
# dist/RoadFighter-linux-x86_64.tar.gz (the same files, unpacked, with a
# launcher script).
#
#   scripts/build-appimage.sh              use the SDL3 libraries found by
#                                          pkg-config on this machine
#   scripts/build-appimage.sh --container  build inside an Ubuntu 22.04
#                                          container (podman or docker); the
#                                          result runs on older distros too
#   scripts/build-appimage.sh --setup      install the build dependencies with
#                                          apt and build SDL3 from source first
#                                          (used inside the container and by CI)
#
# An AppImage only runs on systems with a glibc at least as new as the one
# it was built against, so release builds should use --container (or CI).
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD=${BUILD_DIR:-$ROOT/build-appimage}
DIST=$ROOT/dist
APP_ID=io.github.marciocr.RoadFighter
LINUXDEPLOY_URL=${LINUXDEPLOY_URL:-https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage}

case "${1:-}" in
--container)
	engine=$(command -v podman || command -v docker || true)
	if [ -z "$engine" ]; then
		echo "podman or docker is needed for --container" >&2
		exit 1
	fi
	exec "$engine" run --rm -v "$ROOT:/src:Z" -w /src \
		-e BUILD_DIR=/src/build-appimage/container \
		docker.io/library/ubuntu:22.04 scripts/build-appimage.sh --setup
	;;
--setup)
	export DEBIAN_FRONTEND=noninteractive
	apt-get update
	apt-get install -y --no-install-recommends \
		build-essential cmake ninja-build pkg-config git ca-certificates curl file \
		libasound2-dev libpulse-dev libpipewire-0.3-dev \
		libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev \
		libxss-dev libxtst-dev libxkbcommon-dev libwayland-dev libdecor-0-dev \
		libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev \
		libdbus-1-dev libudev-dev libfreetype-dev libharfbuzz-dev
	"$ROOT/scripts/build-sdl3-deps.sh" /opt/sdl3 "$BUILD/deps"
	export PKG_CONFIG_PATH=/opt/sdl3/lib/pkgconfig
	export LD_LIBRARY_PATH=/opt/sdl3/lib
	;;
"")
	;;
*)
	echo "unknown option: $1" >&2
	exit 1
	;;
esac

# Build from a copy of the sources, so the developer's own objects in src/
# aren't cleaned or mixed with this build.
rm -rf "$BUILD/tree" "$BUILD/AppDir"
mkdir -p "$BUILD/tree" "$DIST"
cp -r "$ROOT/Makefile" "$ROOT/src" "$BUILD/tree/"
make -C "$BUILD/tree" clean
make -C "$BUILD/tree" -j"$(nproc)"

# The game finds its data (and the filter shaders) in ../share/roadfighter/ relative to the binary
# (see chdir_to_game_data() in src/main.cpp).
APPDIR=$BUILD/AppDir
install -Dm755 "$BUILD/tree/roadfighter" "$APPDIR/usr/bin/roadfighter"
mkdir -p "$APPDIR/usr/share/roadfighter"
cp -r "$ROOT/graphics" "$ROOT/sound" "$ROOT/maps" "$ROOT/fonts" "$ROOT/shaders" "$APPDIR/usr/share/roadfighter/"
install -Dm644 "$ROOT/$APP_ID.desktop" "$APPDIR/usr/share/applications/$APP_ID.desktop"
install -Dm644 "$ROOT/graphics/roadfighter.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/$APP_ID.png"

LINUXDEPLOY=$BUILD/tools/linuxdeploy-x86_64.AppImage
if [ ! -x "$LINUXDEPLOY" ]; then
	mkdir -p "$BUILD/tools"
	curl -fL -o "$LINUXDEPLOY" "$LINUXDEPLOY_URL"
	chmod +x "$LINUXDEPLOY"
fi

# linuxdeploy copies the SDL3 libraries (and what they link to) into the
# AppDir; the extract-and-run mode avoids needing FUSE in containers/CI.
cd "$BUILD"
APPIMAGE_EXTRACT_AND_RUN=1 \
LDAI_OUTPUT="$DIST/RoadFighter-x86_64.AppImage" OUTPUT="$DIST/RoadFighter-x86_64.AppImage" \
	"$LINUXDEPLOY" --appdir "$APPDIR" \
		--executable "$APPDIR/usr/bin/roadfighter" \
		--desktop-file "$APPDIR/usr/share/applications/$APP_ID.desktop" \
		--icon-file "$APPDIR/usr/share/icons/hicolor/256x256/apps/$APP_ID.png" \
		--output appimage

echo "Created $DIST/RoadFighter-x86_64.AppImage"

# tar.gz: the AppDir's usr/ tree (bin/, lib/ with the SDL3 libraries
# linuxdeploy gathered, share/) plus a launcher, like Maze of Galious and
# F-1 Spirit ship.
PKG=RoadFighter-linux-x86_64
rm -rf "$BUILD/$PKG"
cp -a "$APPDIR/usr" "$BUILD/$PKG"
install -m755 "$ROOT/packaging/roadfighter.sh" "$BUILD/$PKG/roadfighter.sh"
cp "$ROOT/LICENSE" "$ROOT/README.md" "$BUILD/$PKG/"
tar -C "$BUILD" -czf "$DIST/$PKG.tar.gz" "$PKG"

echo "Created $DIST/$PKG.tar.gz"
