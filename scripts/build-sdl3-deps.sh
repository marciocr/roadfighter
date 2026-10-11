#!/usr/bin/env bash
# Builds SDL3, SDL3_image, SDL3_ttf and SDL3_mixer from source into a
# prefix, for systems that don't package them (e.g. the old Ubuntu used to
# build the AppImage).
#
# The git tags and CMake options are read from the Flatpak manifest, so
# both packages always use the same SDL versions and build options.
#
# Usage: scripts/build-sdl3-deps.sh <prefix> [work dir]
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
MANIFEST="$ROOT/io.github.marciocr.RoadFighter.yml"
PREFIX=${1:?usage: $0 <prefix> [work dir]}
WORK=${2:-$ROOT/build-deps}

mkdir -p "$PREFIX" "$WORK"
PREFIX=$(cd "$PREFIX" && pwd)

# One line per module that has a git tag: "name|url|tag|-Dopt -Dopt..."
modules=$(awk '
	/^  - name:/          { name=$3; url=""; opts=""; next }
	/^ *- -D/             { opts=opts " " $2; next }
	/^ *url:/             { url=$2; next }
	/^ *tag:/             { print name "|" url "|" $2 "|" opts }
' "$MANIFEST")

while IFS='|' read -r name url tag opts; do
	[ -n "$name" ] || continue
	echo "==> $name $tag"
	src="$WORK/$name"
	if [ ! -d "$src" ]; then
		git clone --quiet --depth 1 --branch "$tag" --recursive --shallow-submodules "$url" "$src"
	fi
	# shellcheck disable=SC2086 # $opts is a list of separate -D options
	cmake -S "$src" -B "$src/build" -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX="$PREFIX" \
		-DCMAKE_INSTALL_LIBDIR=lib \
		-DCMAKE_PREFIX_PATH="$PREFIX" \
		$opts
	cmake --build "$src/build"
	cmake --install "$src/build"
done <<< "$modules"
