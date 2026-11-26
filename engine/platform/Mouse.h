// Mouse.h - flash.ui.Mouse.cursor: forces the shape of the mouse cursor.
//
// Auto lets the display list decide (a hand over buttons, an arrow
// elsewhere); the other values force a shape until set back to Auto.
#pragma once

namespace engine::Mouse {

    enum class Cursor { Auto, Arrow, Button, Hand };

    inline Cursor cursor = Cursor::Auto;

} // namespace engine::Mouse
