#!/usr/bin/env bash
# run.sh - builds game when needed, then starts it.
#
#   ./run.sh                    build (incrementally) and play
#   ./run.sh --fullscreen       any option is passed to the game (--scale N, --cheat...)
#   TAKEOVER_SWF=path/to/takeover.swf ./run.sh
#                               where to find the original game for the asset pack
#
# The first run configures and compiles the project (a few minutes) and
# converts the original takeover.swf into the asset pack (assets/, a few
# seconds). Later runs only rebuild what changed.

set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"

BUILD_DIR=${TAKEOVER_BUILD_DIR:-build}
OS=$(cat /etc/*release | grep ^NAME | tr -d 'NAME="' | awk '{print $1}')
PACKAGE_MANAGER=""

fetch_package_manager() {
  case $OS in
    Debian | Ubuntu)
      PACKAGE_MANAGER="apt"
      ;;

    Fedora)
      PACKAGE_MANAGER="dnf"
      ;;

    Arch)
      PACKAGE_MANAGER="pacman"
      ;;

    *)
      PACKAGE_MANAGER="unknown"
      ;;
  esac
}

fetch_package_manager

need() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "run.sh: '$1' is missing. On $OS:" >&2
        echo "  sudo $PACKAGE_MANAGER install build-essential cmake pkg-config zlib1g-dev libcairo2-dev libsdl3-dev" >&2
        exit 1
    fi
}

need cmake
need c++

# 1. Configure once, then build what changed.
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
fi
cmake --build "$BUILD_DIR" -j"$(nproc)" --target takeover assetpack

# 2. The asset pack, converted from the original SWF the first time.
if [ ! -f assets/manifest.json ]; then

    swf=${TAKEOVER_SWF:-}

    if [ -z "$swf" ]; then
        for candidate in ./takeover.swf takeover.swf; do
            if [ -f "$candidate" ]; then swf=$candidate; break; fi
        done
    fi

    if [ -z "$swf" ] || [ ! -f "$swf" ]; then
        echo "run.sh: the asset pack needs the original game file takeover.swf." >&2
        echo "  Put it in this folder, or run: TAKEOVER_SWF=/path/to/takeover.swf ./run.sh" >&2
        exit 1
    fi
    
    echo "Converting $swf into the asset pack..."
    "$BUILD_DIR/tools/assetpack/assetpack" "$swf" assets
fi

# 3. Play.
exec "$BUILD_DIR/game/takeover" --assets assets "$@"
