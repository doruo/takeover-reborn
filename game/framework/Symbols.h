// Symbols.h - library symbols by linkage class name, and named children.
//
// In AS3, `new scrTitleClass()` instantiates a symbol exported from the
// Flash library, and `screen.butStart` (or `screen["butStart"]`) reaches a
// child placed on its timeline with that instance name. Here:
//
//     auto screen = symbols::create("scrTitleClass");
//     auto* start = symbols::child<engine::MovieClip>(*screen, "butStart");
//     auto* label = symbols::find<engine::TextField>(*screen, "mcMessage.txtName");
//
// Many instance names were obfuscated in the shipped SWF ("_-dj"); the port
// uses those strings as they are, with a comment saying what the child is.
#pragma once

#include "engine/display/MovieClip.h"
#include "engine/display/TextField.h"

#include <memory>
#include <string_view>

namespace takeover::symbols {

/// Instantiates a library symbol by class name; throws if it doesn't exist.
/// Symbols whose AS3 class extends Sprite (not MovieClip) are stopped on
/// their first frame, as Flash would show them.
std::shared_ptr<engine::MovieClip> create(std::string_view className);

/// True if the library exports this class name.
bool exists(std::string_view className);

/// A direct child by instance name, of the requested type (nullptr if
/// missing or of another type).
template <typename T = engine::MovieClip>
T* child(const engine::DisplayObject* parent, std::string_view name) {
    const auto* container = dynamic_cast<const engine::DisplayObjectContainer*>(parent);
    return container ? dynamic_cast<T*>(container->getChildByName(name)) : nullptr;
}

template <typename T = engine::MovieClip>
T* child(const engine::DisplayObject& parent, std::string_view name) {
    return child<T>(&parent, name);
}

/// A descendant by dotted path ("mcMessage.icon").
template <typename T = engine::MovieClip>
T* find(const engine::DisplayObject* parent, std::string_view path) {
    const engine::DisplayObject* current = parent;
    while (current != nullptr) {
        const std::size_t dot = path.find('.');
        if (dot == std::string_view::npos) return child<T>(current, path);
        current = child<engine::DisplayObject>(current, path.substr(0, dot));
        path.remove_prefix(dot + 1);
    }
    return nullptr;
}

template <typename T = engine::MovieClip>
T* find(const engine::DisplayObject& parent, std::string_view path) {
    return find<T>(&parent, path);
}

/// AS3 `object is ClassName` for library classes: was `object` created
/// from that library symbol?
bool isInstance(const engine::DisplayObject& object, std::string_view className);

/// Sets the htmlText of a text field found by path, if it exists.
void setHtmlText(const engine::DisplayObject& parent, std::string_view path, const std::string& html);

/// Registers the frame scripts the Flash IDE compiled into some symbols
/// (the `stop()` calls of the *_fla classes). Called once at startup.
void registerSymbolScripts();

} // namespace takeover::symbols
