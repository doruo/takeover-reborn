#!/usr/bin/env bash
# run.sh - starts the game.
#
#   ./run.sh                    build (incrementally) and play
#   ./run.sh --fullscreen       any option is passed to the game (--scale N, --cheat...)
#   TAKEOVER_SWF=path/to/takeover.swf ./run.sh
#                               where to find the original game for the asset pack
#
# The first run configures and compiles the project (a few minutes) and
# converts the original takeover.swf into the asset pack (assets/, a few
# seconds). Later runs only rebuild what changed.

BUILD_DIR=${TAKEOVER_BUILD_DIR:-build}

set -euo pipefail

/bin/bash "./build.sh"

printf "\nStarting Game...\n"

exec "$BUILD_DIR/game/takeover" --assets assets "$@"

printf "\n Game ended succesfully\n"
