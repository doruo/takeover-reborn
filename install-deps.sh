#!/usr/bin/env bash
# install-deps.sh - install all needed deps before any other steps
#
#   ./install-deps.sh                    install requiered dependenies
#

set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"

FILENAME=$(basename $0)

DEPS_PATH="./requirements.txt"
DEPS=$(sed 's/#.*//' "$DEPS_PATH")

OS=$(cat /etc/*release | grep ^NAME | tr -d 'NAME="' | awk '{print $1}')
PACKAGE_MANAGER=""

fetch_package_manager() {
  printf "\nFetching Package Manager...\n"

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
    printf "Checking needed dependency: $1"
    
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "$FILENAME: '$1' is missing. On $OS:" >&2
        echo "  sudo $PACKAGE_MANAGER install $DEPS" >&2
        exit 1
    fi

    printf "\n$1 detected.\n"
}

fetch_package_manager

need cmake
need c++

bash tools/fetch_third_parties.sh

printf "\n(Password required for dependencies installation)\n"
sudo $PACKAGE_MANAGER install -y $DEPS
