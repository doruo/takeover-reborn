#!/usr/bin/env bash
# run.sh - starts the game.
#
#   ./run.sh                    build (incrementally) and play
#   ./run.sh --fullscreen       any option is passed to the game (--scale N, --cheat...)
#
# The first run configures and compiles the project (a few minutes) and
# converts the original takeover.swf into the asset pack (assets/, a few
# seconds). Later runs only rebuild what changed.

set -euo pipefail

# Made with this ASCII Generator: https://patorjk.com/software/taag
printf "
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

 ███████████           █████                                                      
▒█▒▒▒███▒▒▒█          ▒▒███                                                       
▒   ▒███  ▒   ██████   ▒███ █████  ██████   ██████  █████ █████  ██████  ████████ 
    ▒███     ▒▒▒▒▒███  ▒███▒▒███  ███▒▒███ ███▒▒███▒▒███ ▒▒███  ███▒▒███▒▒███▒▒███
    ▒███      ███████  ▒██████▒  ▒███████ ▒███ ▒███ ▒███  ▒███ ▒███████  ▒███ ▒▒▒ 
    ▒███     ███▒▒███  ▒███▒▒███ ▒███▒▒▒  ▒███ ▒███ ▒▒███ ███  ▒███▒▒▒   ▒███     
    █████   ▒▒████████ ████ █████▒▒██████ ▒▒██████   ▒▒█████   ▒▒██████  █████    
   ▒▒▒▒▒     ▒▒▒▒▒▒▒▒ ▒▒▒▒ ▒▒▒▒▒  ▒▒▒▒▒▒   ▒▒▒▒▒▒     ▒▒▒▒▒     ▒▒▒▒▒▒  ▒▒▒▒▒     
                                                                                  
                                                                                  
                                                                                  
          ███████████            █████                                            
         ▒▒███▒▒▒▒▒███          ▒▒███                                             
          ▒███    ▒███   ██████  ▒███████   ██████  ████████  ████████            
          ▒██████████   ███▒▒███ ▒███▒▒███ ███▒▒███▒▒███▒▒███▒▒███▒▒███           
          ▒███▒▒▒▒▒███ ▒███████  ▒███ ▒███▒███ ▒███ ▒███ ▒▒▒  ▒███ ▒███           
          ▒███    ▒███ ▒███▒▒▒   ▒███ ▒███▒███ ▒███ ▒███      ▒███ ▒███           
          █████   █████▒▒██████  ████████ ▒▒██████  █████     ████ █████          
         ▒▒▒▒▒   ▒▒▒▒▒  ▒▒▒▒▒▒  ▒▒▒▒▒▒▒▒   ▒▒▒▒▒▒  ▒▒▒▒▒     ▒▒▒▒ ▒▒▒▒▒           
                                     
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"

BUILD_DIR=${TAKEOVER_BUILD_DIR:-build}
GAME_PATH="$BUILD_DIR/game/takeover"
PARAMS="--assets assets "$@""

# Build
/bin/bash "./build.sh"

printf "\nStarting Game...\n"

# Run
exec "$GAME_PATH" PARAMS
