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
ORIGINAL_GAMEFILE=${TAKEOVER_SWF:-$(zenity --file-selection)}

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
    printf "\nBuild completed\n"
}

# Extract the asset pack, converted from the original SWF the first time.
extract_assets(){

    printf "\n/---- Assets Extraction ----/\n"

    if [ ! -f assets/manifest.json ]; then

        printf "Fetching original .swf game file..."
        ORIGINAL_GAMEFILE=${TAKEOVER_SWF:-$(zenity --file-selection)}

        if [ -z "$ORIGINAL_GAMEFILE" ]; then
            for candidate in ./$ORIGINAL_GAMEFILE $ORIGINAL_GAMEFILE; do
                if [ -f "$candidate" ]; then ORIGINAL_GAMEFILE=$candidate; break; fi
            done
        fi

        if [ -z "$ORIGINAL_GAMEFILE" ] || [ ! -f "$ORIGINAL_GAMEFILE" ]; then
            echo "$FILENAME: the asset pack needs the original game file $ORIGINAL_GAMEFILE." >&2
            echo "  Put it in this folder, or run: ORIGINAL_GAMEFILE=/path/to/$ORIGINAL_GAMEFILE ./$FILENAME" >&2
            exit 1
        fi
        
        printf "\nConverting $ORIGINAL_GAMEFILE into the asset pack...\n"
        "$BUILD_DIR/tools/assetpack/assetpack" "$ORIGINAL_GAMEFILE" assets

        printf "\nAssets extracted\n"

    else
        printf "\nAssets already found\n"
    fi
}

main(){
    build
    extract_assets
}

main
