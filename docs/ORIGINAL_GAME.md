# How the original *Take Over* works

This is a reverse-engineering report of `takeover.swf` (v1.1.5, IriySoft,
sponsored by Armor Games / A10). It describes the game as shipped, so the C++
port can reproduce it faithfully. Each module gets a
detailed section here when it is ported.

## 1. Technology

| Aspect | Original |
|---|---|
| Container | One SWF 10 file, zlib-compressed, 16.7 MB (21.8 MB uncompressed) |
| Stage | 800 x 600 px, 30 fps, white background |
| Code | ActionScript 3, compiled into 1417 `DoABC` blocks (the obfuscator split them) |
| Art | 3796 vector shapes, 757 morph shapes, 776 bitmaps, 3117 MovieClips (36k frames) |
| Text | 18 embedded fonts (DefineFont3), 105 dynamic text fields, 101 static texts |
| Sound | 93 MP3 sounds (two music tracks, the rest effects) |
| Linkage | 1036 symbols exported to AS3 classes (`CastleRedClass`, `Level1Class`, ...) |

### Two languages in one binary

Recovering the authors' intent starts with noticing that the code came from
**two compilers**:

* **Haxe** (compiled to AVM2). Recognisable by `flash.Boot`
  (`flash/_SafeStr_791`) and its `skip_constructor` guard at the top of every
  constructor, by enum classes with `tag/index/params/__constructs__`, and by
  `haxe.ds.IntMap` / `StringMap` (`_SafeStr_872`, `_SafeCls_792`). The Haxe code
  is the **game simulation**: packages `base`, `battle`, `level` (the logic
  half), `progress`, and most of the `_SafePkg_8xx/9xx` packages (units,
  squads, abilities, AI).
* **ActionScript 3**: the presentation layer (`gui`, `hud`, `Main`,
  `GameBattle`, `LevelVisual`, `Level`) plus a reusable in-house framework
  (signals, state stack, buttons, resource cache, raster renderer, tweening).

The AS3 classes `LevelVisual` and `Level` *inherit from* the Haxe level logic:
`_SafeCls_2275 -> _SafeStr_2340 -> LevelVisual -> Level`. The simulation is
pure data plus rules, the AS3 subclasses add rendering and input.

### Obfuscation

Every non-public identifier was renamed (`_-OE` in the bytecode, shown by
ffdec as `_SafePkg_29`, `_SafeStr_80`, ...). What survives:

* public fields of Haxe classes (`hp`, `dmgMin`, `unitsCount`, `abilities`...),
* names that must exist at runtime: linkage class names, instance names placed
  on the timeline (`butLogo`, `txtCaption`, `mcArea0`), frame labels,
* overrides of Flash API methods (`update`, `dispose`, `customDispose`),
* string constants (event names, badge keys, ability names).

Real names are recovered from these clues. `docs/NAME_MAP.md` records every
decision (obfuscated name -> port name -> reason), so a renaming can always
be traced back to the decompiled source.

## 2. Boot sequence

```
Preloader (root class, frame 1)
 ├─ domain lock: decides ArmorGames / A10 / Kongregate / Newgrounds / AGI
 │  branding from the URL the SWF is served from (obfuscated byte strings)
 ├─ Mochi / CPMStar pre-roll advertisement
 └─ frame 2: instantiates Main
Main.init()
 ├─ fixed timestep: 30 fps, dt = 1/30 s per ENTER_FRAME (not wall-clock time)
 ├─ TOResManager: pre-renders every unit animation MovieClip into BitmapData
 │  frames (AnimationMaker), the "Loading N%" bar, stage.frameRate is raised
 │  to 120 during this step to finish faster
 ├─ sponsor splash (ArmorGames or A10) -> IriySoft splash
 └─ start(): GameStorage (save slots), mouse/keyboard devices, StateManager,
    GameIntro screen, menu music
```

## 3. Main loop and screen stack

`Main.onEnterFrame` runs once per Flash frame and always passes **dt = 1/30**:

```
mouse.update(dt), keyboard.update(dt), stateManager.update(dt),
achievement popup queue
```

Simulation speed therefore depends on frame count, never on real time. The
battle multiplies dt by a speed factor (`GameBattle._SafeStr_3352`, the
fast-forward button).

`StateManager` (`_SafePkg_774`) is a stack of `GameState`s (`_SafeStr_742`).
Pushing a *modal* state pauses the ones below, states update top to bottom,
a state removes itself by calling `dispose()`, which fires its "disposed"
signal. Screens (`gui/*Screen`) extend `GameScreen`, which owns a root
`Sprite` built from a linkage clip (`scrTitleClass`, `scrThroneRoomClass`, ...)
and wires the clip's named children to `Button` objects.

Screen flow (`Main`):

```
GameIntro ─skip─> Title ─> ChooseProfile ─> ChooseCampaign ─> GlobalMap
                                                    │             │
                                    ThroneRoom (edicts) <─────────┤
                                    BadgesRoom (achievements) <───┤
                                                                  v
                                          Battle (GameBattle + BattleScreen)
                                             │  Pause (modal)
                                             v
                                         MissionOver ─> GlobalMap / Victory
```

## 4. Module map

Obfuscated packages, grouped by the role their content gives away. The port
uses the names in the right-hand column.

### In-house AS3 framework

| Package | Content | Port module |
|---|---|---|
| `_SafePkg_29`, `_SafePkg_33` | `Signal`, `ObjectPool`, `ObjectUpdater`, base classes Disposable (`_SafeCls_32`), Updatable (`_SafeStr_36`), Poolable | `core` |
| `_SafePkg_774` | `StateManager`, `GameState` | `core/states` |
| `_SafePkg_775`, `_SafePkg_776` | `KeyboardDevice`, mouse device (`_SafeStr_704`) | `input` |
| `_SafePkg_773`, `_SafePkg_46` | `Button`, `ButtonSet`, `HintLayer`, hint windows, `RadioGroup` | `ui` |
| `_SafePkg_1092` | `SoundManager`, `SoundObject` | `audio` |
| `_SafePkg_1408`, `_SafePkg_1641`, `_SafePkg_2104` | `ResourceManager`, `AnimationMaker` (MovieClip -> BitmapData cache), views | `resources` |
| `_SafePkg_1583`, `_SafePkg_1310` | Animation interfaces, `AnimationSet`, `RasterAnimation`, `UnitAnimation` | `animation` |
| `_SafePkg_851` | `RasterGraphicLayer` (one big BitmapData the battlefield is blitted into), depth-sorted renderables | `raster` |
| `_SafePkg_1737` | `TileMap` + layers (passability grid) | `tilemap` |
| `_SafePkg_1640`, `_SafePkg_1951` | Tweens with BACK / ELASTIC easing | `tween` |
| `_SafePkg_1409` | Matrix / vector helpers, seeded random | `math` |
| `_SafePkg_1650` | `LocalDataStorage`, `SerializableObject` (SharedObject saves) | `storage` |
| `_SafePkg_1601` | `LogicalCondition`, `Metric` (badge conditions) | `progress/conditions` |
| `_SafePkg_803`, `_SafePkg_798`, `_SafePkg_2031` | Actor / Effect / InteractionManager framework, `VisualActor`, `Object2dActor` | `actors` |
| `_SafePkg_3473`, `_SafePkg_1406` | Profile / edict icons, stat panels, centre panel | `gui/widgets` |

### Haxe simulation

| Package | Content | Port module |
|---|---|---|
| `base` | Object base with an int-keyed event bus (`_SafeCls_871`), flags, 2D point, bit set, keyed storage | `sim/base` |
| `battle` | `Race` (Westaria, TheKhaganate, TheCult, TheEmpire, Bandits), unit stat table (`_SafeStr_998`, 47 unit kinds), combatants/players, stat modifiers | `sim/battle` |
| `_SafePkg_805` | `UnitType` (Infantry, Archers, Cavalry, Siege, Magic), individual soldiers, animation name builders | `sim/units` |
| `_SafePkg_868` | Squads (`FSquad` <- `_SafeCls_1114` <- `_SafeCls_971`), squad states (Moving, MeleeFighting, RangedFighting, Charging, Defending, Repairing, Sleeping...), morale, XP | `sim/squads` |
| `_SafePkg_909`, `_SafePkg_897` | Unit abilities (Charge, Discipline, Undead, Poisonous, Mechanism, SplashDamage, Intimidation...) on a generic property system | `sim/abilities` |
| `_SafePkg_838` | Banners and their auras (Heal, Heroism, Bloodlust, Liberation, Desecration) | `sim/banners` |
| `_SafePkg_1566` | Spells (Ice Blast, Disentomb, Terror, Lord's Judgement, Cataclysm, ...) | `sim/spells` |
| `_SafePkg_1543` | Road graph: nodes, roads, paths | `sim/graph` |
| `_SafePkg_836` | AI action selection | `sim/ai` |
| `level` (logic half) | Level rules (`_SafeCls_2275`, 1600 lines), AI-side level logic (`_SafeStr_2340`), key nodes (castles, towns, villages, mage towers), build places, chests | `sim/level` |
| `progress` | `Profile`, campaign progress, `Badges` (achievements), edicts | `progress` |
| `data` | `LevelInfo` (mission settings and texts), `CompanyInfo` (campaigns), `XPConverter` | `data` |

### Presentation (AS3)

`level/LevelVisual`, `level/Level` (spells UI, tutorial, squad selection and
orders), `level/LevelParser`, `GameBattle` (glue between the battle state,
HUD and windows), `hud/*` (bottom game panel, building windows),
`WindowController`, `gui/*` screens, `media/SoundConst`, `media/MusicConst`.

### Third party (dropped by the port)

Mochi ads and services (`_SafePkg_26`), Newgrounds API (`_SafePkg_63`,
`com.newgrounds`), CPMStar ads (`_SafePkg_27`), Playtomic analytics,
`MochiBot`, `GlobalStatistics` (Kongregate/Newgrounds stats), domain lock in
`Preloader`. The sponsor splash animations are kept.

## 5. How a level is authored

Levels are not data files. Each level is two linkage clips drawn in the Flash
IDE:

* `Level<N>Class`: the terrain, as layered children `mcArea0..2`,
  `mcRoads0..2`, `mcObjects0..2`.
* `Level<N>GraphMarkersClass`: invisible markers whose **instance names
  encode the road graph**. `l02nd_057` is node 57 on layer 2 and its class
  says what it is (`NodeCastleGreenClass`, `NodeTownPurpleClass`,
  `NodeBuildHereClass`, `EmptyNodeClass`...), `ed_070_080` is the road between
  nodes 70 and 80.

`LevelParser` walks these children, reads names and positions, builds the
graph and the key nodes, then renders the terrain once into the battlefield
bitmap. The C++ port keeps this: the asset pack preserves instance names and
transforms, so the same parser logic runs unchanged.

## 6. Rendering

* **Menus and HUD** are live MovieClips on the Flash display list: nested
  timelines, filters (ColorMatrix 16 591 uses, Glow 5 560, Blur 2 518,
  DropShadow 440), blend modes (Multiply, Overlay, Add, Screen, Layer...), 169
  masks, 13 265 color transforms, morph-shape tweens. Code drives them by
  instance name (`panel.butLogo`, `icon.gotoAndStop(n)`,
  `message.txtName.htmlText = ...`).
* **The battlefield** is software-rendered: unit animations are cached as
  BitmapData frames at load time (`AnimationMaker`, scale given per clip in
  `XMLData`), then blitted each frame into a `RasterGraphicLayer` with
  depth sorting (`RenderableObjectsContainer`). `LevelVisual.Y_KOEFF`
  squashes the ground plane for the pseudo-isometric look.

## 7. Persistence

`GameStorage` keeps 3 profile slots in a local SharedObject named `"TOVER"`
(an online save path exists for the AGI portal). `Profile`, campaign
progress, badges and edicts serialize themselves through
`SerializableObject` into a ByteArray. The port keeps the same fields in a
local save file.

## 8. Behaviours that need care to reproduce exactly

* **Fixed timestep**: everything advances by 1/30 s per frame, the port does
  the same and never uses wall-clock deltas for simulation.
* **Randomness**: the game calls `Math.random()` (unseeded). Individual
  battles can't be replayed bit for bit even in Flash, the port reproduces
  distributions and formulas, with an optional seed for debugging.
* **Iteration order**: Haxe `IntMap` and AS3 `Dictionary` iterate in
  hash-table order, and the event bus in `base/_SafeCls_871` dispatches in
  that order. Where order changes the outcome, the port documents the order it
  chose next to the code.
* **Integer semantics**: `int(x)` truncates toward zero, `Math.round` rounds
  .5 up (toward +infinity), `uint` wraps, the port uses helpers that match AS3
  instead of `std::round`.
* **Display list timing**: the order in which nested timelines advance,
  when `gotoAndStop` creates or reuses children, and when instance names
  become reachable all follow Flash Player 10 rules. The engine's display
  module documents each rule where it implements it.
