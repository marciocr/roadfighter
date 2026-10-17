#!/bin/sh
# Launcher for the tar.gz package. The binary finds its SDL3 libraries in
# lib/ (its RPATH is $ORIGIN/../lib) and its data in share/roadfighter/.
HERE="$(dirname "$(readlink -f "$0")")"
exec "$HERE/bin/roadfighter" "$@"
