#!/usr/bin/env bash
# fetch_third_parties.sh - downloads the pinned single-header libraries into third_party/.
# Does nothing when they are already there. SDL3 is handled by CMake.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="$ROOT/third_party"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

JSON_REPO=https://github.com/nlohmann/json.git
DOCTEST_REPO=https://github.com/doctest/doctest.git
MINIAUDIO_REPO=https://github.com/mackron/miniaudio.git
STB_REPO=https://github.com/nothings/stb.git

# fetch NAME REPO TAG "SRC:DST ..."
fetch() {
  local name=$1 repo=$2 tag=$3 files=$4

  if [ -d "$DEST/$name" ]; then
    return
  fi

  git clone --quiet --depth 1 --branch "$tag" "$repo" "$TMP/$name"
  mkdir -p "$DEST/$name"

  for pair in $files; do
    cp "$TMP/$name/${pair%%:*}" "$DEST/$name/${pair##*:}"
  done

  echo "Fetched  $name @$tag"
}

main(){
  printf "\n/----- Fetching third parties -----/\n"

  fetch nlohmann  "$JSON_REPO"      v3.11.3       "single_include/nlohmann/json.hpp:json.hpp LICENSE.MIT:LICENSE.MIT"
  fetch doctest   "$DOCTEST_REPO"   v2.4.11       "doctest/doctest.h:doctest.h LICENSE.txt:LICENSE.txt"
  fetch miniaudio "$MINIAUDIO_REPO" 0.11.22       "miniaudio.h:miniaudio.h LICENSE:LICENSE"
  fetch stb       "$STB_REPO"       master        "stb_image.h:stb_image.h stb_image_write.h:stb_image_write.h LICENSE:LICENSE"

  printf "\nThird parties fetched\n"
}

main
