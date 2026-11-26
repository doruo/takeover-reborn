#!/usr/bin/env bash
# Collects the pinned third-party sources (and your "$DECOMPILER" install) into a single
# tarball so they can be copied into the cloud build space.
#
# Usage:  bash fetch_deps.sh [/path/to/"$DECOMPILER"_install_dir]
# "$DECOMPILER" is optional: it is only used to produce reference renders for tests.
set -euo pipefail

DECOMPILER="ffdec"
DECOMPILER_DIR="${1:-}"

ROOT_PATH="$(cd "$(dirname "$0")" && pwd)"
WORK_DIR="third_party"
WORK_PATH="$ROOT_PATH/$WORK_DIR"

SDL_REPO=https://github.com/libsdl-org/SDL.git
MINIAUDIO_REPO=https://github.com/mackron/miniaudio.git
STB_REPO=https://github.com/nothings/stb.git
JSON_REPO=https://github.com/nlohmann/json.git
DOCTEST_REPO=https://github.com/doctest/doctest.git

clone() { 
  git clone --quiet --depth 1 --branch "$2" "$1" "$3"; 
  echo "ok  $3 @ $2";
}

rm -rf "$WORK_PATH" && mkdir -p "$WORK_PATH" && cd "$WORK_PATH"

clone "$SDL_REPO"          release-3.2.10   SDL3
clone "$MINIAUDIO_REPO"    0.11.22          miniaudio
clone "$STB_REPO"          master           stb
clone "$JSON_REPO"         v3.11.3          json
clone "$DOCTEST_REPO"      v2.4.11          doctest

rm -rf "$WORK_PATH/.git"

# Decompilation (optionnal)
# For ffdec : jar files only (the CLI is just "java -jar ffdec.jar").
if [ -n "$DECOMPILER_DIR" ]; then
  mkdir -p "$DECOMPILER"
  cp -r "$DECOMPILER_DIR"/*.jar "$DECOMPILER"/ 2>/dev/null || true
  if [ -d "$DECOMPILER_DIR/lib" ]; then cp -r "$DECOMPILER_DIR/lib" "$DECOMPILER"/; fi
fi

# Machine facts that drive build decisions.
{
  echo "== os";     "$OS"
  echo "== cpu";    grep -m1 "model name" /proc/cpuinfo
  echo "== gpu";    (glxinfo -B 2>/dev/null || echo "glxinfo missing (install mesa-utils)")
  echo "== tools"

  tools="g++ cmake pkg-config java"

  for tool in tools; do
    if command -v "$tool" >/dev/null; 
    then 
      "$tool" --version 2>&1 | head -1; 
    else 
      echo "$tool: missing"; 
    fi
  done

  echo "== sdl pkg"; (pkg-config --modversion sdl3 2>/dev/null || echo "no system SDL3")
  echo "== cairo";  (pkg-config --modversion cairo 2>/dev/null || echo "no cairo dev package (sudo apt install libcairo2-dev)")
} > machine_info.txt 2>&1 || true

cd "$ROOT_PATH"
tar czf "$WORK_DIR".tar.gz "$WORK_DIR"
echo "Done: $ROOT_PATH/$WORK_DIR.tar.gz ($(du -h "$WORK_DIR".tar.gz | cut -f1))"
