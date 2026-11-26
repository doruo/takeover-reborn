// DisplayObjectContainer.h - an ordered list of children (flash.display
// DisplayObjectContainer and Sprite).
//
// Children are kept in paint order (first = bottom). Timeline-placed children
// also carry a depth; the timeline inserts them in depth order.
#pragma once

#include "engine/display/Graphics.h"

#include "engine/display/DisplayObject.h"

#include <memory>
#include <string_view>
#include <vector>

namespace engine {

class DisplayObjectContainer : public DisplayObject {
public:
    ~DisplayObjectContainer() override;

    [[nodiscard]] int numChildren() const { return static_cast<int>(children_.size()); }
    [[nodiscard]] DisplayObject* getChildAt(int index) const;
    [[nodiscard]] DisplayObject* getChildByName(std::string_view name) const;
    [[nodiscard]] int getChildIndex(const DisplayObject* child) const;
    [[nodiscard]] bool contains(const DisplayObject* object) const;

    /// Typed lookup of a named child: `clip.child<MovieClip>("butLogo")`.
    template <typename T>
    [[nodiscard]] T* child(std::string_view name) const {
        return dynamic_cast<T*>(getChildByName(name));
    }

    /// Adds on top (or at `index`), removing the child from its previous
    /// parent first, as Flash does.
    DisplayObject* addChild(DisplayObjectPtr child);
    DisplayObject* addChildAt(DisplayObjectPtr child, int index);
    /// Removes and returns the child, so the caller may keep it alive.
    DisplayObjectPtr removeChild(DisplayObject* child);
    DisplayObjectPtr removeChildAt(int index);
    void swapChildren(DisplayObject* a, DisplayObject* b);
    void setChildIndex(DisplayObject* child, int index);

    [[nodiscard]] Rectangle localBounds() const override;
    void advanceFrame() override;

    // --- Interaction ---------------------------------------------------------

    [[nodiscard]] bool isInteractive() const override { return true; }
    /// When false, the children never become mouse targets: the container
    /// receives the events for all of them.
    [[nodiscard]] bool mouseChildren() const { return mouseChildren_; }
    void setMouseChildren(bool enabled) { mouseChildren_ = enabled; }

    /// True if a visible child covers the point (own space), masks applied.
    [[nodiscard]] bool hitTestLocal(Point local) const override;

    /// Calls `visit(child, maskedOut)` for the children in paint order;
    /// `isMask` children are reported too. `maskTest(child)` tells whether
    /// the masks active for that child let a given point through: used by
    /// hit testing.
    /// Iterates children like the renderer: returns, for each non-mask child,
    /// the list of masks that clip it.
    [[nodiscard]] std::vector<const DisplayObject*> masksOf(const DisplayObject* child) const;

protected:
    void draw(RenderContext& context, const Matrix& world, const ColorTransform& color, bool asMask) override;

    /// Timeline helpers (used by MovieClip).
    [[nodiscard]] DisplayObject* timelineChildAtDepth(int depth) const;
    void insertAtDepth(DisplayObjectPtr child, int depth);

    std::vector<DisplayObjectPtr> children_;
    bool mouseChildren_ = true;
};

/// flash.display.Sprite: a container that can be created from code.
class Sprite : public DisplayObjectContainer {
public:
    /// With buttonMode, the hand cursor shows over the sprite and a clip
    /// with "_up"/"_over"/"_down" frame labels switches between them with the
    /// mouse, like a button.
    [[nodiscard]] bool buttonMode() const { return buttonMode_; }
    void setButtonMode(bool enabled) { buttonMode_ = enabled; }
    [[nodiscard]] bool useHandCursor() const { return useHandCursor_; }
    void setUseHandCursor(bool enabled) { useHandCursor_ = enabled; }

    /// Another object whose shape replaces this sprite's for mouse picking.
    [[nodiscard]] DisplayObject* hitArea() const { return hitArea_.lock().get(); }
    void setHitArea(DisplayObject* area) { hitArea_ = area ? area->weak_from_this() : std::weak_ptr<DisplayObject>{}; }

    /// The sprite's own drawing, under its children.
    [[nodiscard]] Graphics& graphics();

    [[nodiscard]] Rectangle localBounds() const override;

protected:
    void draw(RenderContext& context, const Matrix& world, const ColorTransform& color, bool asMask) override;

private:
    std::unique_ptr<Graphics> graphics_;
    bool buttonMode_ = false;
    bool useHandCursor_ = true;
    std::weak_ptr<DisplayObject> hitArea_;
};

} // namespace engine
