#include "engine/display/DisplayObjectContainer.h"

#include "engine/render/Renderer.h"

#include <algorithm>
#include <limits>

namespace engine {

DisplayObjectContainer::~DisplayObjectContainer() {
    for (auto& child : children_) child->parent_ = nullptr;
}

DisplayObject* DisplayObjectContainer::getChildAt(int index) const {
    if (index < 0 || index >= numChildren()) return nullptr;
    return children_[static_cast<std::size_t>(index)].get();
}

DisplayObject* DisplayObjectContainer::getChildByName(std::string_view name) const {
    // Flash returns the first match in paint order.
    for (const auto& child : children_)
        if (child->name() == name) return child.get();
    return nullptr;
}

int DisplayObjectContainer::getChildIndex(const DisplayObject* child) const {
    for (std::size_t i = 0; i < children_.size(); ++i)
        if (children_[i].get() == child) return static_cast<int>(i);
    return -1;
}

bool DisplayObjectContainer::contains(const DisplayObject* object) const {
    for (const DisplayObject* o = object; o != nullptr; o = o->parent())
        if (o == this) return true;
    return false;
}

DisplayObject* DisplayObjectContainer::addChild(DisplayObjectPtr child) {
    // The index is clamped after the child leaves its previous parent, so
    // "past the end" always means "on top".
    return addChildAt(std::move(child), std::numeric_limits<int>::max());
}

DisplayObject* DisplayObjectContainer::addChildAt(DisplayObjectPtr child, int index) {
    if (!child) return nullptr;
    if (child->parent_ != nullptr) child->parent_->removeChild(child.get());
    index = std::clamp(index, 0, numChildren());
    child->parent_ = this;
    // A script-added child behaves like one placed above every existing depth.
    int highest = 0;
    for (const auto& c : children_) highest = std::max(highest, c->depth_);
    child->depth_ = highest + 1;
    child->placedByTimeline_ = false;
    DisplayObject* raw = child.get();
    children_.insert(children_.begin() + index, std::move(child));
    return raw;
}

DisplayObjectPtr DisplayObjectContainer::removeChild(DisplayObject* child) {
    const int index = getChildIndex(child);
    return index < 0 ? nullptr : removeChildAt(index);
}

DisplayObjectPtr DisplayObjectContainer::removeChildAt(int index) {
    if (index < 0 || index >= numChildren()) return nullptr;
    DisplayObjectPtr child = std::move(children_[static_cast<std::size_t>(index)]);
    children_.erase(children_.begin() + index);
    child->parent_ = nullptr;
    return child;
}

void DisplayObjectContainer::swapChildren(DisplayObject* a, DisplayObject* b) {
    const int ia = getChildIndex(a), ib = getChildIndex(b);
    if (ia < 0 || ib < 0) return;
    std::swap(children_[static_cast<std::size_t>(ia)], children_[static_cast<std::size_t>(ib)]);
    std::swap(a->depth_, b->depth_);
}

void DisplayObjectContainer::setChildIndex(DisplayObject* child, int index) {
    const int current = getChildIndex(child);
    if (current < 0) return;
    DisplayObjectPtr keep = std::move(children_[static_cast<std::size_t>(current)]);
    children_.erase(children_.begin() + current);
    index = std::clamp(index, 0, numChildren());
    children_.insert(children_.begin() + index, std::move(keep));
}

Rectangle DisplayObjectContainer::localBounds() const {
    Rectangle bounds;
    for (const auto& child : children_) {
        if (child->clipDepth()) continue; // masks don't contribute
        bounds = bounds.united(child->localBounds().transformed(child->matrix()));
    }
    return bounds;
}

void DisplayObjectContainer::advanceFrame() {
    // Iterate over a snapshot: a child's frame may change our child list.
    const std::vector<DisplayObjectPtr> snapshot = children_;
    for (const auto& child : snapshot) child->advanceFrame();
}

std::vector<const DisplayObject*> DisplayObjectContainer::masksOf(const DisplayObject* child) const {
    // Same rule as draw(): a child with a clip depth masks the following
    // siblings whose depth is <= that clip depth.
    std::vector<const DisplayObject*> masks;
    for (const auto& c : children_) {
        while (!masks.empty() && c->depth_ > *masks.back()->clipDepth()) masks.pop_back();
        if (c.get() == child) return masks;
        if (c->clipDepth()) masks.push_back(c.get());
    }
    return {};
}

bool DisplayObjectContainer::hitTestLocal(Point local) const {
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        const DisplayObject& child = **it;
        if (!child.visible() || child.clipDepth()) continue;
        if (!child.hitTestLocal(child.matrix().inverted().apply(local))) continue;
        bool insideMasks = true;
        for (const DisplayObject* mask : masksOf(&child))
            insideMasks = insideMasks && mask->hitTestLocal(mask->matrix().inverted().apply(local));
        if (insideMasks) return true;
    }
    return false;
}

DisplayObject* DisplayObjectContainer::timelineChildAtDepth(int depth) const {
    for (const auto& child : children_)
        if (child->placedByTimeline_ && child->depth_ == depth) return child.get();
    return nullptr;
}

void DisplayObjectContainer::insertAtDepth(DisplayObjectPtr child, int depth) {
    child->parent_ = this;
    child->depth_ = depth;
    child->placedByTimeline_ = true;
    const auto position = std::find_if(children_.begin(), children_.end(),
                                       [depth](const DisplayObjectPtr& c) { return c->depth_ > depth; });
    children_.insert(position, std::move(child));
}

void DisplayObjectContainer::draw(RenderContext& context, const Matrix& world, const ColorTransform& color,
                                  bool asMask) {
    if (asMask) {
        // A container used as a mask: the union of its children's shapes.
        for (const auto& child : children_)
            if (!child->clipDepth()) child->renderMask(context, world);
        return;
    }

    // A child with a clip depth masks the following siblings whose depth is
    // <= that clip depth. Masks nest, so keep a stack.
    struct ActiveMask {
        int clipDepth;
        DisplayObject* mask;
    };
    std::vector<ActiveMask> masks;
    auto popMask = [&] {
        context.renderer.beginUnmask();
        masks.back().mask->renderMask(context, world);
        context.renderer.endUnmask();
        masks.pop_back();
    };

    for (const auto& child : children_) {
        while (!masks.empty() && child->depth_ > masks.back().clipDepth) popMask();
        if (child->clipDepth()) {
            context.renderer.beginMask();
            child->renderMask(context, world);
            context.renderer.endMask();
            masks.push_back({*child->clipDepth(), child.get()});
        } else {
            child->render(context, world, color);
        }
    }
    while (!masks.empty()) popMask();
}

Graphics& Sprite::graphics() {
    if (!graphics_) graphics_ = std::make_unique<Graphics>();
    return *graphics_;
}

Rectangle Sprite::localBounds() const {
    Rectangle bounds = DisplayObjectContainer::localBounds();
    if (graphics_ && !graphics_->empty()) {
        const Rectangle own = graphics_->bounds();
        bounds = bounds.isEmpty() ? own : bounds.united(own);
    }
    return bounds;
}

void Sprite::draw(RenderContext& context, const Matrix& world, const ColorTransform& color, bool asMask) {
    if (graphics_ && !asMask) graphics_->draw(context, world, color);
    DisplayObjectContainer::draw(context, world, color, asMask);
}

} // namespace engine
