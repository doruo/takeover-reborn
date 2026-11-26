#include "game/framework/ui/HintWindow.h"

namespace takeover {

using engine::Point;
using engine::Rectangle;

DockSide opposite(DockSide side) {
    switch (side) {
    case DockSide::Left: return DockSide::Right;
    case DockSide::Right: return DockSide::Left;
    case DockSide::Top: return DockSide::Bottom;
    case DockSide::Bottom: return DockSide::Top;
    }
    return side;
}

// --- HintWindow ---------------------------------------------------------------------

namespace {
constexpr double kDefaultTextMargin = 5;
}

HintWindow::HintWindow(std::shared_ptr<engine::Sprite> window, std::string_view backgroundName,
                       std::string_view textName)
    : image_(std::move(window)) {
    image_->setMouseEnabled(false);
    image_->setMouseChildren(false);
    background_ = image_->getChildByName(backgroundName);
    text_ = dynamic_cast<engine::TextField*>(image_->getChildByName(textName));
    authoredTextWidth_ = text_->width();
    text_->setAutoSize(engine::TextField::AutoSize::Left);
    setAutoWidth(true);
    text_->setX(0);
    text_->setY(0);
    hide();
    setMargins(kDefaultTextMargin, kDefaultTextMargin);
    setBounds({0, 0, 0, 0});
}

void HintWindow::setAutoWidth(bool value) {
    autoWidth_ = value;
    if (!autoWidth_) text_->setWidth(authoredTextWidth_);
    text_->setWordWrap(!autoWidth_);
}

void HintWindow::resize() {
    background_->setX(-marginX_);
    background_->setY(-marginY_);
    background_->setWidth(text_->width() + 2 * marginX_);
    background_->setHeight(text_->height() + 2 * marginY_);
}

void HintWindow::place() {
    Point position{image_->x(), image_->y()};
    dock(position, side_);
    image_->setX(position.x);
    image_->setY(position.y);
    if (isOutOfBounds(side_)) {
        dock(position, opposite(side_));
        image_->setX(position.x);
        image_->setY(position.y);
    }
    align(position, align_);
    image_->setX(position.x);
    image_->setY(position.y);

    // Keep the window inside the allowed bounds.
    const Rectangle box = image_->getBounds(image_->parent());
    bool xChanged = false, yChanged = false;
    if (box.bottom() > bounds_.bottom()) {
        position.y = bounds_.bottom() - text_->height();
        yChanged = true;
    }
    if (box.y < bounds_.y) {
        position.y = bounds_.y;
        yChanged = !yChanged;
    }
    if (box.right() > bounds_.right()) {
        position.x = bounds_.right() - text_->width();
        xChanged = true;
    }
    if (box.x < bounds_.x) {
        position.x = bounds_.x;
        xChanged = !xChanged;
    }
    if (xChanged) image_->setX(position.x);
    if (yChanged) image_->setY(position.y);
}

void HintWindow::dock(Point& position, DockSide side) const {
    switch (side) {
    case DockSide::Left: position.x = target_.x - text_->width() - targetMargin_; break;
    case DockSide::Right: position.x = target_.right() + targetMargin_; break;
    case DockSide::Top: position.y = target_.y - text_->height() - targetMargin_; break;
    case DockSide::Bottom: position.y = target_.bottom() + targetMargin_; break;
    }
}

bool HintWindow::isOutOfBounds(DockSide) const {
    // The original only ever checks the left edge, whatever the side.
    return image_->getBounds(image_->parent()).x < bounds_.x;
}

void HintWindow::align(Point& position, DockAlign alignment) const {
    if (side_ == DockSide::Left || side_ == DockSide::Right) {
        switch (alignment) {
        case DockAlign::LeftTop: position.y = target_.y; break;
        case DockAlign::RightBottom: position.y = target_.bottom() - text_->height(); break;
        case DockAlign::Center: position.y = 0.5 * (target_.y + target_.bottom() - text_->height()); break;
        }
    } else {
        switch (alignment) {
        case DockAlign::LeftTop: position.x = target_.x; break;
        case DockAlign::RightBottom: position.x = target_.right() - text_->width(); break;
        case DockAlign::Center: position.x = 0.5 * (target_.x + target_.right() - text_->width()); break;
        }
    }
}

void HintWindow::setMargins(double horizontal, double vertical) {
    marginX_ = horizontal;
    marginY_ = vertical;
    resize();
    if (image_->visible()) place();
}

void HintWindow::setText(const std::string& html) {
    text_->setHtmlText(html);
    if (!autoWidth_) text_->setWidth(authoredTextWidth_);
    resize();
    if (image_->visible()) place();
}

void HintWindow::setBounds(const Rectangle& bounds) {
    bounds_ = bounds;
    if (image_->visible()) place();
}

void HintWindow::show(const Rectangle& target, DockSide side, DockAlign align, double margin) {
    target_ = target;
    side_ = side;
    align_ = align;
    targetMargin_ = margin;
    image_->setVisible(true);
    place();
}

void HintWindow::hide() { image_->setVisible(false); }

// --- HintWindow2 ----------------------------------------------------------------------

HintWindow2::HintWindow2(std::shared_ptr<engine::MovieClip> hint) : HintWindow(std::move(hint)) {
    setMargins(7, 2);
    setBounds({0, 0, 800, 600});
}

void HintWindow2::dispose() {
    if (image_ && image_->parent()) image_->parent()->removeChild(image_.get());
}

// --- HintLayer ---------------------------------------------------------------------------

HintLayer::HintLayer() : image_(std::make_shared<engine::Sprite>()) { image_->setMouseEnabled(false); }

int HintLayer::addHintWindow(std::shared_ptr<HintWindow2> window) {
    windows_.push_back(std::move(window));
    return static_cast<int>(windows_.size()) - 1;
}

void HintLayer::showHintAtObject(engine::DisplayObject* target, int hintId) {
    showWindowAtObject(target, windows_[static_cast<std::size_t>(hintId)].get());
}

void HintLayer::showWindowAtObject(engine::DisplayObject* target, HintWindow2* window) {
    image_->addChild(window->imagePtr());
    window->showAt(target->getBounds(image_.get()));
}

void HintLayer::showHintAtRect(const Rectangle& rect, int hintId) {
    showHintWindowAtRect(rect, windows_[static_cast<std::size_t>(hintId)].get());
}

void HintLayer::showHintWindowAtRect(const Rectangle& rect, HintWindow2* window) {
    image_->addChild(window->imagePtr());
    window->showAt(rect);
}

void HintLayer::hideHint() {
    while (image_->numChildren() > 0) image_->removeChildAt(0);
}

void HintLayer::customDispose() {
    hideHint();
    for (auto& window : windows_)
        if (window) window->dispose();
    windows_.clear();
    if (image_ && image_->parent()) image_->parent()->removeChild(image_.get());
    Disposable::customDispose();
}

} // namespace takeover
