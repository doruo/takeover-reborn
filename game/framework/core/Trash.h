// Trash.h - deferred destruction.
//
// Flash code often drops the last reference to an object from inside one
// of that object's own methods: a button handler that closes its screen,
// a state that disposes itself in update(). The garbage collector made that
// safe. In C++ the object would be destroyed while its method is still on
// the call stack, so such objects go to the trash instead, and the trash
// is emptied at the start of the next frame (Main::onEnterFrame).
#pragma once

#include <memory>

namespace takeover {

/// Keeps `object` alive until the next emptyTrash().
void releaseLater(std::shared_ptr<void> object);

/// Destroys everything released since the last call.
void emptyTrash();

} // namespace takeover
