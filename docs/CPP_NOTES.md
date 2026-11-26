# C++ notes: the language features this project relies on

This file explains the C++ techniques used in the code base, each with a
pointer to where it appears. It assumes you know C++ basics (classes,
references, templates) and focuses on *why* each feature was chosen.

## 1. Ownership instead of garbage collection

AS3 and Haxe have a garbage collector: objects live as long as something
references them. C++ has none, so every object has exactly one owner that
decides when it dies.

* **Values first.** Most data is stored by value (`std::vector<Frame>`,
  `Rect`, `Matrix`). Destruction is automatic when the owner goes away.
* **`std::unique_ptr<T>`** when an object must live on the heap and has one
  owner (render caches, GL wrappers).
* **`std::shared_ptr<T>`** only where the original relied on the garbage
  collector to share an object: display objects. In Flash, game code keeps
  references to clips (`panel.butLogo`) that a timeline may remove at any
  time, and the clip stays alive as long as someone refers to it. Containers and
  game code therefore share ownership of display objects. The parent link is
  a plain pointer (a child never owns its parent, which would create a cycle
  and leak). See `engine/display/DisplayObject.h`.
* **Raw pointers and references are non-owning.** A `const T*` returned by a
  lookup (`Movie::get<ShapeDef>(id)`) means "here is the object, someone else
  keeps it alive". Never `delete` it.
* **RAII** (Resource Acquisition Is Initialization): a resource (file, GL
  texture, audio device) is acquired in a constructor and released in the
  destructor. Wrapping GL handles in small RAII classes makes leaks
  impossible even when an exception is thrown.

A concrete example is in `libs/vector/src/Rasterizer.cpp`: cairo is a C
library with `cairo_create` / `cairo_destroy` pairs. Wrapping its pointers in
`std::unique_ptr` with a custom deleter makes the destroy call automatic:

```cpp
struct CairoDeleter {
    void operator()(cairo_t* p) const { cairo_destroy(p); }
};
using CairoContext = std::unique_ptr<cairo_t, CairoDeleter>;

CairoContext cr(cairo_create(surface.get())); // destroyed at end of scope
```

The original's `dispose()` pattern (explicit cleanup + "disposed" signal)
maps onto this: the port keeps `dispose()` where game logic depends on the
signal, and lets destructors free memory.

## 2. Move-only types

`swf::Movie` owns the decompressed SWF bytes, and hundreds of `std::span`s
point into that buffer. Copying a Movie would copy the bytes but leave the
spans pointing into the *old* buffer, a dangling-pointer bug. So:

```cpp
Movie(const Movie&) = delete;            // no copies
Movie(Movie&&) noexcept = default;       // moves are fine
```

Moving a `std::vector` transfers its heap block without reallocating, so the
spans stay valid after a move. `= delete` turns the copy mistake into a
compile error instead of a runtime crash.

## 3. `std::span`: a view, not a container

`std::span<const std::uint8_t>` (aliased `swf::Bytes`) is a pointer + a
length. It lets the parser hand out pieces of the file without copying
megabytes of shape and image data. Rule: a span never outlives what it
views. See `libs/swf/include/swf/Characters.h`.

## 4. `std::optional`: "maybe a value"

A PlaceObject tag *may* carry a matrix, a name, a ratio... `std::optional<T>`
encodes "absent" without magic values like `-1` or empty strings:

```cpp
if (place.name) child->setName(*place.name);
```

## 5. `std::variant` + `std::visit`: closed sets of types

A SWF character is exactly one of: shape, morph shape, bitmap, sprite,
button, edit text, static text, font, sound. `std::variant` stores one of
them, with no inheritance and no heap allocation:

```cpp
using Character = std::variant<ShapeDef, MorphShapeDef, BitmapDef, ...>;
```

* `std::get_if<SpriteDef>(&c)` returns a pointer or `nullptr`.
* `std::visit(lambda, c)` calls the lambda with the active alternative. With
  `if constexpr`, the compiler checks every case at compile time
  (`tools/swfdump/main.cpp`, `kindName`).

Rule of thumb used in this project: a **closed** set of data types that
the code switches over (file records, commands) is a `variant`, an **open**
family of behaviours that game code extends (display objects, game states,
units) is a class hierarchy with virtual functions, as in the original.

## 6. `enum class`

Scoped, strongly typed enums (`swf::TagCode`, `swf::BlendMode`). They don't
convert to `int` silently, so mixing up a blend mode and a tag code is a
compile error. Converting on purpose is explicit: `static_cast<int>(mode)`.

## 7. `[[nodiscard]]`, `const`, `constexpr`

* `[[nodiscard]]` on getters: ignoring the result of `movie.find(id)` is
  almost always a bug, so the compiler warns.
* Member functions that don't modify the object are `const`. This documents
  intent and lets them be called on `const` references.
* `constexpr` constants (`kTwipsPerPixel`) are evaluated at compile time and
  have no runtime cost.

## 8. Exceptions for broken input, not for control flow

The parser throws `swf::FormatError` on truncated or invalid data, with the
tag and offset in the message. A malformed asset is a bug to fix, not a
situation to recover from. The game itself avoids exceptions in per-frame
code.

## 9. Build system: CMake targets

Every component is a *target* (`add_library`, `add_executable`) that
declares its own include directories and dependencies:

```cmake
target_include_directories(swf PUBLIC include)
target_link_libraries(swf PUBLIC ZLIB::ZLIB PRIVATE takeover::warnings)
```

`PUBLIC` propagates to whoever links `swf`, `PRIVATE` stays internal. The
`takeover::warnings` interface target applies one warning policy everywhere.
`CMAKE_EXPORT_COMPILE_COMMANDS` produces `compile_commands.json` so clangd
gives exact code navigation in any editor.

## 10. X-macros: one list, several expansions

`engine/render/Gl.h` lists the OpenGL functions once:

```cpp
#define TAKEOVER_GL_FUNCTIONS(X) X(glClear) X(glViewport) ...
```

The same list is expanded twice with different macros: once to declare a
function pointer per entry, once (in `Gl.cpp`) to load each pointer by name.
Adding a function is one line, and the declaration and the loading can't
get out of sync. It's an old preprocessor technique, use it sparingly,
for tables like this one.

## 11. Designated initializers

C++20 lets you name the fields you set in an aggregate:

```cpp
engine::Window window({.title = "clipview", .width = 64, .height = 64, .hidden = true});
```

Fields must appear in declaration order, the others keep their default
values. It reads better than a long positional argument list.

## 12. Disposal that waits for the next frame

The original calls `dispose()` on objects that are still running: a button
disposes itself in its own click handler, a squad is disposed while the
level iterates over its squads. With a garbage collector that is harmless.
In C++, freeing the memory right there would leave the running code with a
dangling `this`.

So `dispose()` (in `game/framework/core/Disposable.cpp` and
`game/sim/base/BaseObject.cpp`) does the original's work - cleanup, signal -
and then hands a `shared_ptr` to itself to the `Trash` (`Trash.h`), which
drops it at the start of the next frame. Objects that dispose themselves
derive from `std::enable_shared_from_this` so they can produce that pointer.

Containers that iterate while objects may be removed (squads killed during
the squads' update...) iterate over a copy of the keys, like the Haxe maps
of the original did (`forEachObject` in `game/sim/level/LevelLogic.h`).

## 13. Lambdas and `std::function`

AS3 passes methods as values (`signal.addListener(this.onClick)`). C++ has
two ways, both used:

```cpp
button->signalClick.addListener(this, &WindowBase::onButtonClick); // a method
animation->signalFinish.addListener([this, caster](Tween*) {        // a lambda
    releaseUltimate(*caster);
});
```

A lambda is an anonymous function object. `[this, caster]` lists what it
captures from the surrounding code. `std::function<void(int)>` stores any
callable with that signature, which is how `FunctionProcess`
(`game/framework/input/InputProcess.h`) keeps the functions of the battle's
input chain.

## 14. Interfaces and multiple inheritance

AS3 classes implement interfaces, C++ classes inherit from abstract classes
(only pure virtual methods). A stronghold on screen is both a simulation
object and something drawn on the battlefield:

```cpp
class FKeyNodeContent : public KeyNode, public ILayerObject { ... };
```

The battlefield layer keeps `std::shared_ptr<ILayerObject>`: converting a
`shared_ptr<FKeyNodeContent>` to it shares ownership of the same object.

## 15. `is` and `as`: `dynamic_cast`

`node is BuildPlace` / `node as BuildPlace` become
`dynamic_cast<BuildPlace*>(node)`, which returns `nullptr` when the object
is of another type - the same as `as` returning `null`. When the type is
certain (the original casts without checking), `static_cast` says so.

## 16. AS3 `null` and `undefined`

A value that may be missing is a `std::optional<T>` (a path that may not
exist, the owner of a location). An object that may be missing is a
pointer that may be `nullptr`. Where the original reads past the end of an
array and gets `undefined` (treated as 0 or `false`), the port checks the
index and returns that value explicitly, with a comment.
