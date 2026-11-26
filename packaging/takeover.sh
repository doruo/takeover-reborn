#!/bin/sh
# Starts TakeoFver from an unpacked release archive.
#   ./takeover.sh [--fullscreen] [--scale N]
here=$(dirname "$(readlink -f "$0")")
exec "$here/bin/takeover" --assets "$here/share/takeover/assets" "$@"
