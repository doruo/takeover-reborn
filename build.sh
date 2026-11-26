#!/usr/bin/env bash
# build.sh - builds game when needed
#
#   ./build.sh                    build (incrementally) and play
#
# The first run configures and compiles the project (a few minutes) and
# converts the original takeover.swf into the asset pack (assets/, a few
# seconds). Later runs only rebuild what changed.

set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"

FILENAME=$(basename $0)
BUILD_DIR=${TAKEOVER_BUILD_DIR:-build}

TOOLS="build-essential cmake pkg-config zlib1g-dev libcairo2-dev libsdl3-dev"

OS=$(cat /etc/*release | grep ^NAME | tr -d 'NAME="' | awk '{print $1}')
PACKAGE_MANAGER=""

fetch_package_manager() {
  printf "\nFetching package manager\n"

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

  printf "\nOS: $OS, package manager: $PACKAGE_MANAGER\n"
}

need() {
    printf "\nChecking needed dependency: $1\n"
    
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "$FILENAME: '$1' is missing. On $OS:" >&2
        echo "  sudo $PACKAGE_MANAGER install $TOOLS" >&2
        exit 1
    fi
}

delete_existing_build() {
  printf "\nDeleting existing build at: $BUILD_DIR\n"
  rm -rf "$BUILD_DIR"
}

build(){
  printf "\n/---- Build ----/\n"

  # 1. Configure once, then build what changed.
  if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
      cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
  fi

  cmake --build "$BUILD_DIR" -j"$(nproc)" --target takeover assetpack
}

extract_assets(){

  printf "\n/---- Assets Extraction ----/\n"

  # 2. The asset pack, converted from the original SWF the first time.
  if [ ! -f assets/manifest.json ]; then

      swf=${TAKEOVER_SWF:-}

      if [ -z "$swf" ]; then
          for candidate in ./$swf $swf; do
              if [ -f "$candidate" ]; then swf=$candidate; break; fi
          done
      fi

      if [ -z "$swf" ] || [ ! -f "$swf" ]; then
          echo "$FILENAME: the asset pack needs the original game file $swf." >&2
          echo "  Put it in this folder, or run: TAKEOVER_SWF=/path/to/$swf ./$FILENAME" >&2
          exit 1
      fi
      
      printf "\nConverting $swf into the asset pack...\n"
      "$BUILD_DIR/tools/assetpack/assetpack" "$swf" assets
  fi
}

fetch_package_manager

need cmake
need c++

build
extract_assets

printf "\n/---- Build Finished ----/\n"
