#!/usr/bin/env bash
# build.sh - builds game when needed
#
#   ./build.sh                    build (incrementally) and play
#
# The first run configures and compiles the project (a few minutes) and
# converts the original takeover.swfF into the asset pack (assets/, a few
# seconds). Later runs only rebuild what changed.

set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"

FILENAME=$(basename $0)
BUILD_DIR=${TAKEOVER_BUILD_DIR:-build}

delete_existing_build() {
  printf "Deleting existing build at: $BUILD_DIR"
  rm -rf "$BUILD_DIR"
}

# Configure once, then build what changed.
build(){
  printf "\n/---- Build ----/\n"

  if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
      cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
  fi

  cmake --build "$BUILD_DIR" -j"$(nproc)" --target takeover assetpack
}

# Extract the asset pack, converted from the original SWF the first time.
extract_assets(){

  printf "\Extracting assets...\n"

  if [ ! -f assets/manifest.json ]; then

      printf "Fetching original .swf game file..."
      swf_gamefile=${TAKEOVER_SWF:-$(zenity --file-selection)}

      if [ -z "$swf_gamefile" ]; then
          for candidate in ./$swf_gamefile $swf_gamefile; do
              if [ -f "$candidate" ]; then swf_gamefile=$candidate; break; fi
          done
      fi

      if [ -z "$swf_gamefile" ] || [ ! -f "$swf_gamefile" ]; then
          echo "$FILENAME: the asset pack needs the original game file $swf_gamefile." >&2
          echo "  Put it in this folder, or run: TAKEOVER_SWF=/path/to/$swf_gamefile ./$FILENAME" >&2
          exit 1
      fi
      
      printf "\nConverting $swf_gamefile into the asset pack...\n"
      "$BUILD_DIR/tools/assetpack/assetpack" "$swf_gamefile" assets
  fi
}

build
extract_assets

printf "\n/---- Build Finished ----/\n"
