# Architecture of the C++ port

## Goals, in priority order

1. **Same behaviour as the Flash original**: same rules, timings (30 fps
   fixed step), layouts, animations and sounds.
2. **Same organisation**: the game code mirrors the original packages and
   classes (with real names recovered), so any line of the port can be compared
   with the decompiled ActionScript it came from.
3. **Maintainable by a human without tooling magic**: plain C++20, CMake,
   a few mainstream libraries, no code generation in the game itself.
4. **Fast on modest hardware**: the reference machine is a ThinkPad X220
   (Intel HD 3000: OpenGL 3.3, no Vulkan).

## The big picture

```
              offline (once)                         runtime (every launch)
 ┌──────────────┐   ┌──────────────┐          ┌────────────────────────────────┐
 │ takeover.swf │──>│ assetpack    │──assets/─>│ game (port of the AS3/Haxe)    │
 └──────────────┘   │ (libs/swf)   │          │   uses                          │
                    └──────────────┘          │ engine (a small Flash runtime)  │
                                              │   SDL3 · OpenGL 3.3 · cairo ·   │
                                              │   miniaudio                     │
                                              └────────────────────────────────┘
```

The original relies on the Flash Player for three things: the display list
(nested MovieClips, filters, text), the vector rasterizer, and the AVM2
language runtime. The port splits them:

* **Vector art stays vector; cairo rasterizes it.** The converter turns every
  SWF shape into a *draw list* (closed fill paths + stroke paths + paints) and
  stores it in the pack. At runtime the engine rasterizes a shape with cairo
  the first time it's displayed at a given scale and caches the result as a
  GPU texture. Rasterizing all 3796 shapes of the game takes under 3 seconds
  on one core, so a screen's worth costs a few milliseconds, the pack stays
  small, and art is sharp at any window size. (Baking every shape to PNG
  offline was measured first: 84 Mpx of bitmaps at 1x before any
  supersampling, too heavy for the X220's shared video memory.) JPEXS ffdec
  serves as the reference renderer in tests: our renders differ from its
  exports by less than 0.2/255 on average.
* **The display list is reimplemented** in `engine/display`, with the same
  vocabulary as Flash (`Sprite`, `MovieClip`, `gotoAndStop`, `getChildByName`,
  `TextField`, `SimpleButton`). Porting a screen is then a line-by-line
  translation instead of a redesign.
* **The language runtime is replaced by C++.** AS3/Haxe classes become C++
  classes; garbage collection is replaced by explicit ownership (see
  `CPP_NOTES.md`).

## Source tree

```
takeover-reborn/
├─ CMakeLists.txt            top-level build (+ CPack release archive)
├─ run.sh                    build when needed, then play
├─ cmake/                    dependency setup (SDL3)
├─ libs/
│  ├─ swf/                   SWF file parser: dictionary, timelines, shapes -> draw lists
│  └─ vector/                draw list rasterizer (cairo)
├─ tools/
│  ├─ swfdump/               inspect a SWF (symbols, sprite frames)
│  ├─ shaperender/           rasterize shapes to PNG (pipeline check)
│  ├─ assetpack/             takeover.swf -> assets/ (library, bitmaps, sounds, manifest)
│  ├─ clipview/              play or render any clip with the engine
│  ├─ package.sh             release archive in dist/
│  └─ namemap.py             regenerates docs/NAME_MAP.md
├─ engine/                   reusable "Flash-like" runtime, knows nothing about the game
│  ├─ platform/              Application (the "Flash Player"), window, keyboard, mouse cursor, test scripts
│  ├─ render/                GL 3.3 renderer, render targets, filters, blend modes, shape cache
│  ├─ display/               DisplayObject tree, MovieClip timelines, Stage input, TextField, BitmapData, Graphics
│  ├─ events/                EventDispatcher, Event/MouseEvent/KeyboardEvent
│  ├─ audio/                 Sound / SoundChannel on miniaudio
│  ├─ assets/                asset pack loader, symbol library
│  ├─ core/  geom/           listener identity, Matrix/ColorTransform/Rectangle
├─ game/                     the port, original package layout
│  ├─ framework/             in-house AS3 framework: signals, disposal, states, input, buttons,
│  │                         hints, tweens, saves, raster animation (bitmap caches, battlefield layer)
│  ├─ sim/                   Haxe simulation: base, battle (combatants, edicts, stats), units,
│  │                         squads, abilities, spells, fsm, level (strongholds, roads, rules, AI)
│  ├─ level/                 the battle on screen: LevelVisual, Level (player input, spells), map, buildings, effects
│  ├─ units/  squads/        soldiers and squads on screen (FUnit, FSquad, SquadIcon)
│  ├─ hud/                   battle panel, building windows
│  ├─ gui/                   every screen (menus, map, throne room, badges, battle, pause, tutorial)
│  ├─ progress/  data/       profile, campaigns, badges; campaign and level tables
│  ├─ media/  resources/     sounds and music; the battle's clip list and resource manager
│  ├─ GameBattle.*           a battle in progress (input chain, tutorials)
│  └─ Main.*                 the screen flow, as the original document class
├─ packaging/                launcher and readme of the release archive
├─ docs/
└─ assets/                   generated by tools/assetpack, not committed
```

`libs/swf` is used only by the tools. The game never parses SWF: it loads the
asset pack, which is faster and keeps the runtime small.

## Asset pack

| File | Content |
|---|---|
| `library.swf` | Every symbol the game can reference: sprites with their timelines (place/move/remove commands, names, labels, filters, blend modes, color transforms), shapes and morph shapes as draw lists, buttons, text fields, static texts, font glyph outlines and metrics, linkage class names. It is a valid SWF stripped of code, bitmaps and sounds: no custom serialization code to maintain, and swfdump/ffdec can open it. |
| `manifest.json` | Stage size and frame rate, linkage class -> id, bitmap and sound tables (sample rate, encoder delay) |
| `bitmaps/*.png` | Decoded bitmaps (standalone characters and bitmap fills) |
| `sounds/*.mp3` | Sound effects and music, bit-identical to the SWF content |

## Engine, layer by layer

**platform** opens the window (SDL3), creates an OpenGL 3.3 core context,
runs the fixed 30 Hz loop (`update(1/30)` then render), translates SDL events
into Flash-like mouse/keyboard events, and letterboxes the 800x600 stage.

**vector** (`libs/vector`) rasterizes draw lists with cairo into premultiplied
BGRA images (cairo's native layout, uploaded to GL as-is). A cache keyed by
(shape, scale bucket, morph ratio) avoids re-rasterizing.

**render** draws textured quads in batches. A quad carries an affine matrix
and a color transform (multiply + add, exactly Flash's formula). Filters and
non-trivial blend modes render the object into an offscreen texture first,
like Flash does internally:
blur = repeated box blur passes (Flash's "quality"), glow/drop shadow = tinted
blurred alpha + composite, ColorMatrix = one shader. Masks use the stencil
buffer.

**display** is the core of the "Flash runtime":

* `DisplayObject` (transform, alpha, visible, filters, blendMode, name, parent),
* `DisplayObjectContainer` (ordered children, `addChild`, `getChildByName`),
* `MovieClip` (plays a `swf` timeline: `play/stop/gotoAndStop/gotoAndPlay`,
  labels, `currentFrame/totalFrames`, morph ratio),
* `Shape`/`Bitmap` leaves, `SimpleButton` (up/over/down/hit states),
* `getBounds`, `hitTestPoint`, mouse picking and event dispatch in `Stage`
  (roll over/out, buttons' `_up/_over/_down` looks, `mouseEnabled`,
  `mouseChildren`, hit areas, the hand cursor),
* `Graphics` (`lineTo`, `curveTo`, `drawRect`...) for the lines the game
  draws itself (order arrows, selection frame),
* `BitmapData` / `Bitmap`: pixels in memory (`fillRect`, `copyPixels`,
  `colorTransform`, `getPixel`) and `draw(displayObject)`, which renders
  through the GPU and reads the pixels back. The battle uses them exactly
  like the original (see below).

**text** (`display/TextField`) rasterizes the embedded fonts' own glyph outlines (they are SWF
shapes, so the vector path above handles them) into glyph atlases and lays
out `TextField`s the way Flash does (2 px gutter, leading, alignment,
word wrap, the `<font>`, `<b>`, `<br>`, `<p align>` HTML subset).

**audio** wraps miniaudio: a `Sound` is decoded MP3 data, `play()` returns a
`SoundChannel` with a volume/pan `SoundTransform`, matching `flash.media`.

## The game

The game code is a translation of the original, class by class: the same
classes, the same methods in the same order, the same quirks. The header of
every file names the original class; `docs/NAME_MAP.md` lists every
recovered name.

**Main** (`game/Main.cpp`) is the document class of the original: it plays
the preloader and intros, then moves between screens (`gui/`) on their
buttons' signals. Screens are `GameState`s in a `StateManager`.

**A battle** is built in layers, like the original:

```
LevelLogic   rules: incomes, captures, roads and path finding, splash damage, victory
  LevelAi    computer players: recruit, build, send troops, cast spells, ultimates
    LevelVisual   display: map, battlefield layer, icons, HUD layers, effects, signals
      Level       the player's hands: selection, orders by drag and drop, spells, ultimates
GameBattle   owns the Level and the HUD; chains the input processes; scripts the tutorials
BattleScreen hosts the GameBattle, the tutorial panel and the pause menu
```

The simulation (`sim/`) is the original's Haxe code: soldiers (`Unit`) with
a small state machine, abilities as property objects with effects, squads
(`Squad`) with their orders and morale, strongholds (`KeyNode`) with their
ruin/rebuild cycle. Objects refer to each other by id through `BaseObject`,
and talk through an event bus, as in the original.

**Drawing the battlefield** follows the original's raster pipeline: every
unit animation is a library clip that `AnimationMaker` renders once into a
list of `BitmapData` frames (on first use; the preloader only renders the
first ones). Every frame, `RasterGraphicLayer` sorts the visible objects by
their y, clears an 800x600 `BitmapData` and copies each object's current
frame into it, layer by layer (ground marks, corpses, paths, units and
buildings). A `Bitmap` shows that image on the stage, under the HUD. The
copies are done by the CPU, as in Flash; the image is uploaded to the GPU
once per frame.

## Game code conventions

* One C++ class per original class, same package, recovered name. The
  header comment names the original (`// ... (original: _SafePkg_868/FSquad)`),
  members carry their original name in a `///<` comment.
* AS3 numeric semantics live in `game/framework/core/As3.h`
  (`as3::toInt`, `as3::round`, `as3::random`) so formulas read like the
  original and behave identically.
* The `Signal` class keeps the original's listener semantics; lambdas can
  listen too.
* `dispose()` works like the original's (cleanup + `signalDisposed`); the
  memory itself is released at the start of the next frame (`Trash`), so
  an object disposed while it is running stays valid until it returns.
* AS3 constructors call virtual methods; C++ ones can't, so screens call
  `initScreen()` at the end of their constructor, and simulation objects
  are built with an `init()` after construction.
* Original bugs that change behaviour are kept and commented ("original
  quirk"), so the game plays the same.

## What was left out

* Online services of the Flash release: Mochi ads, Playtomic statistics,
  Newgrounds medals and cloud saves. Saves are local files.
* Library code the game never calls (packages `_SafePkg_798`, `_SafePkg_803`,
  `_SafePkg_2031`, `_SafePkg_1737`, `_SafePkg_1951`: a generic actor/tile map
  framework shipped with the in-house library), and unused classes
  (`AnimationSet`, `RasterAnimationSet`, `BufferedImage`).

## Third-party libraries

| Library | Role | Why this one |
|---|---|---|
| SDL3 | window, GL context, input, audio device | the standard portable layer; works on Mesa / HD 3000 |
| OpenGL 3.3 core | rendering | highest API the X220 supports; Vulkan isn't available there |
| miniaudio | MP3 decoding, mixing | single file, no system dependency, decodes the SWF MP3s as-is |
| cairo | CPU rasterization of vector shapes and glyphs | mature, exact-coverage anti-aliasing, every gradient/stroke feature SWF needs, preinstalled on Linux desktops |
| stb_image, stb_image_write | JPEG/PNG decoding, PNG writing | single-header, battle-tested |
| zlib | SWF decompression (tools) | ubiquitous |
| nlohmann/json | tool configuration and debug dumps | de-facto standard |
| doctest | unit tests | header-only, fast to compile |
| JPEXS ffdec (optional) | reference renders for visual tests | most faithful SWF exporter available |

## Verification strategy

* **Parser**: round-trip checks against the counts reported by ffdec
  (characters, tags, frames).
* **Rendering**: `tools/clipview` renders any linkage clip at any frame to a
  PNG, to compare with the original.
* **Screens and battles**: the game runs test scripts (`--script`, see the
  README) that click and drag through the menus and battles and take
  screenshots; the port was checked that way screen by screen, through the
  tutorials, and with the computer players fighting for minutes.
* **Memory**: an AddressSanitizer/UBSan build (`-DCMAKE_BUILD_TYPE=Debug
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"`) runs the same scripts.

## Delivery phases

| Phase | Content | State |
|---|---|---|
| 0 | Analysis, SWF parser, shape rasterizer, docs | done |
| 1 | Asset pipeline | done |
| 2 | Engine: window, display list, timelines, shapes | done |
| 3 | Engine: filters, blend modes, masks, text, buttons, audio, input | done |
| 4 | Framework port (signals, states, input, UI, saves, raster, tween) | done |
| 5 | Menus, profiles, saves, campaign map, throne room, badges | done |
| 6 | Simulation (Haxe part) | done |
| 7 | Battle presentation, HUD, spells, tutorial | done |
| 8 | Packaging (`run.sh`, release archive) | done |
| 9 | Side-by-side comparison with the original on every mission | ongoing |
