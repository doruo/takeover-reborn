// Stage.h - flash.display.Stage: the root of the display list, and the
// place where raw input becomes Flash mouse and keyboard events.
//
// The platform layer (engine/platform/Application) feeds it mouse positions
// in stage pixels and Flash key codes; the Stage finds the object under the
// cursor and dispatches the same event sequence the Flash Player would:
//
//   move:   rollOut / mouseOut on what the cursor left, rollOver / mouseOver
//           on what it entered, then mouseMove on the target
//   press:  mouseDown on the target
//   release: mouseUp on the target, then click if it is the object that was
//           pressed (doubleClick for a second click within 400 ms... only on
//           objects with doubleClickEnabled, which the game never sets)
#pragma once

#include "engine/display/DisplayObjectContainer.h"

#include <memory>
#include <vector>

namespace engine {

class Stage : public DisplayObjectContainer {
public:
    Stage(int width, int height, double frameRate) : width_(width), height_(height), frameRate_(frameRate) {}

    [[nodiscard]] int stageWidth() const { return width_; }
    [[nodiscard]] int stageHeight() const { return height_; }
    [[nodiscard]] double frameRate() const { return frameRate_; }

    [[nodiscard]] double stageMouseX() const { return mouse_.x; }
    [[nodiscard]] double stageMouseY() const { return mouse_.y; }
    [[nodiscard]] bool isMouseButtonDown() const { return buttonDown_; }

    // --- Input from the platform layer ---------------------------------------

    void mouseMoved(double stageX, double stageY);
    void mouseButton(bool pressed);
    void mouseWheel(int lines);
    void key(bool pressed, unsigned keyCode, unsigned charCode, bool shift, bool ctrl, bool alt);

    /// Re-evaluates what is under a still cursor (the scene below it may have
    /// changed this frame), sending rollOver/rollOut as needed.
    void refreshMouseTarget();

    /// True when the cursor should be a hand (over a buttonMode sprite).
    [[nodiscard]] bool wantsHandCursor() const;

    // --- Frame ---------------------------------------------------------------

    /// One Flash frame: timelines advance, then enterFrame is broadcast.
    void advanceFrame() override;

private:
    struct Pick {
        bool hit = false;                    ///< Something (even a shape) is under the point.
        DisplayObject* target = nullptr;     ///< Interactive object that receives the event.
    };
    Pick pick(const DisplayObjectContainer& container, Point stagePoint) const;

public:
    /// The interactive object the mouse would hit at a point.
    [[nodiscard]] DisplayObject* targetAt(Point stagePoint) const;

private:

    void dispatchMouse(DisplayObject* target, std::string_view type, DisplayObject* related = nullptr);
    void updateHover(DisplayObject* newTarget);
    static void applyButtonLook(DisplayObject* target, std::string_view state);

    int width_;
    int height_;
    double frameRate_;
    Point mouse_{-1, -1};
    bool buttonDown_ = false;
    bool shift_ = false, ctrl_ = false, alt_ = false;

    std::weak_ptr<DisplayObject> hover_;   ///< Current mouse target.
    std::weak_ptr<DisplayObject> pressed_; ///< Target of the last mouseDown.
    /// Ancestors chain of the hovered object (for rollOver/rollOut).
    std::vector<std::weak_ptr<DisplayObject>> hoverChain_;
};

} // namespace engine
