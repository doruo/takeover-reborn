# Takeover Reborn

A faithful C++20 port of the Flash strategy game *Take Over* (IriySoft, 2011,
v1.1.5). The goal is identical behaviour to the original SWF, with source code
organised like the original and readable without the decompiler at hand.

> Status: **playable**. Every screen of the original is ported: preloader and
> intros, profiles, the three campaigns and their maps, edicts, badges, the
> battles with their HUD, building windows, spells, banners, ultimate powers,
> computer players and the two tutorial missions, mission results and saves.
> Online services of the Flash release (Mochi ads, Playtomic statistics,
> Newgrounds medals and cloud saves) are left out. See
> [docs/PORT_ARCHITECTURE.md](docs/PORT_ARCHITECTURE.md).

## Quick start (Linux)

```bash
sudo apt install build-essential cmake pkg-config zlib1g-dev libcairo2-dev libsdl3-dev
./run.sh
```

`run.sh` configures and builds the project the first time (a few minutes), converts the original `takeover.swf` into the asset pack (a few seconds), then starts the game. 

Later runs only rebuild what changed.
The original game is not distributed with the source: `run.sh` looks for it
in `takeover-original/swf/takeover.swf` or `./takeover.swf`, or wherever `TAKEOVER_SWF` points.

```bash
./run.sh --fullscreen      # F11 or Alt+Enter toggles it in game
./run.sh --scale 2         # a 1600x1200 window
./run.sh --cheat           # the original developer mode (see below)
```

Saves go to `~/.local/share/takeover/` (`TAKEOVER_SAVE_DIR` overrides it).

If `libsdl3-dev` isn't available (older distributions), CMake downloads and
builds SDL 3.2.10 automatically (needs network once).

## A release archive to copy elsewhere

```bash
tools/package.sh           # -> dist/takeover-1.1.5-linux-x86_64.tar.gz
```

The archive contains the game, the asset pack and `takeover.sh`: unpack it
anywhere and run `./takeover.sh`. The target machine needs OpenGL 3.3 and
the cairo and zlib libraries (preinstalled on desktop distributions). SDL3
is bundled when the build had to compile it. `cmake --install build
--prefix ~/.local` installs the same files.

## Playing

The controls are those of the original: click a building to open its
window (recruit, upgrade, build), drag a squad's round icon onto a place or
an enemy to send it there, draw a frame around icons to select several
squads, click a spell then the battlefield to cast it. Space or Escape
cancels the current action, and the gear button pauses.

`--cheat` turns on the original's developer mode (`Preloader.CHEAT_ON`): two
extra spell buttons (more gold and mana, any spell) and "Quit" in the pause
menu wins the mission.

## Documentation

| Document | What it covers |
|---|---|
| [docs/ORIGINAL_GAME.md](docs/ORIGINAL_GAME.md) | How the original game is built: technologies, boot, main loop, modules, levels, rendering, saves |
| [docs/PORT_ARCHITECTURE.md](docs/PORT_ARCHITECTURE.md) | How the port is organised: asset pipeline, engine, game, what was left out, how it is tested |
| [docs/NAME_MAP.md](docs/NAME_MAP.md) | Obfuscated identifier -> port name, for every class and member (generated) |
| [docs/CPP_NOTES.md](docs/CPP_NOTES.md) | The C++ techniques used here and why |

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
./build/tools/assetpack/assetpack path/to/takeover.swf assets   # once
./build/game/takeover --assets assets
```

Options: `-DTAKEOVER_BUILD_GAME=OFF` builds only the tools (no SDL needed),
`-DTAKEOVER_BUILD_TOOLS=OFF` only the game.

## Tools

```bash
# Play any clip of the game in a window (Space: pause, Right: step, Esc: quit)
./build/tools/clipview/clipview assets IriySoftIntro

# Render one frame to PNG, cropped to the clip or on the 800x600 stage
./build/tools/clipview/clipview assets CastleRedClass castle.png --scale 2
./build/tools/clipview/clipview assets ArmorGamesIntro ag.png --stage --advance 120

# Inspect the SWF: summary, linkage classes, frame-by-frame display lists
./build/tools/swfdump/swfdump assets/library.swf
./build/tools/swfdump/swfdump assets/library.swf --classes
./build/tools/swfdump/swfdump assets/library.swf --class Level1GraphMarkersClass

# Rebuild docs/NAME_MAP.md from the source comments
python3 tools/namemap.py > docs/NAME_MAP.md
```

### Automated playtest

The game runs test scripts that click, drag, press keys and take
screenshots, without a visible window:

```bash
SDL_VIDEO_DRIVER=offscreen ./build/game/takeover --assets assets --script test.txt --fast
```

```
# test.txt: one command per line
wait 600                 # let 600 frames run (20 s of game time)
click 400 540            # move the mouse there, press and release
press 450 275            # drag: press here...
move 320 240
release 300 240          # ...release there
key 27                   # a key, by Flash key code (27: Escape)
pick 140 365             # print which object the mouse would hit there
shot screen.png          # screenshot of the 800x600 stage
quit
```

`--fast` runs the frames as fast as possible. This is how the port was
checked, screen by screen and battle by battle.

## Repository layout

```
libs/swf/           SWF parser: container, timelines, shapes -> draw lists
libs/vector/        draw list rasterizer (cairo)
engine/             Flash-like runtime: assets, display list, text, OpenGL renderer, audio, window
game/               the port of the game, organised like the original packages
tools/              assetpack, clipview, swfdump, shaperender, package.sh, namemap.py
packaging/          launcher and readme of the release archive
third_party/        vendored single-header libraries (stb, miniaudio, json, doctest)
docs/               design and reverse-engineering notes
run.sh              build and play
```
