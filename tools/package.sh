#!/usr/bin/env bash
# package.sh - builds a release archive of the game in dist/.
#
#   tools/package.sh            -> dist/takeover-1.1.5-linux-x86_64.tar.gz
#
# The archive holds bin/takeover, the asset pack (share/takeover/assets),
# SDL3 when it had to be built from source, and takeover.sh to start the
# game. It needs the asset pack (assets/, see run.sh or README.md).
set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")/.."

BUILD_DIR=build-release
if [ ! -f assets/manifest.json ]; then
    echo "package.sh: no asset pack in assets/ (run ./run.sh once, or tools/assetpack)." >&2
    exit 1
fi
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DTAKEOVER_BUILD_TESTS=OFF -DTAKEOVER_BUILD_TOOLS=OFF
cmake --build "$BUILD_DIR" -j"$(nproc)" --target takeover
mkdir -p dist
(cd "$BUILD_DIR" && cpack -G TGZ -B "$PWD/../dist" >/dev/null)
rm -rf dist/_CPack_Packages
ls -lh dist/*.tar.gz
