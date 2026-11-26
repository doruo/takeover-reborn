#include "game/framework/ui/MovieClipButton.h"

namespace takeover {

using engine::MouseEvent;

MovieClipButton::MovieClipButton(std::shared_ptr<engine::MovieClip> clip) : clip_(std::move(clip)) {
    engine::DisplayObject* hitArea = clip_->getChildByName(HIT_AREA_NAME);
    clip_->setHitArea(dynamic_cast<engine::MovieClip*>(hitArea));
    if (clip_->hitArea() != nullptr) clip_->hitArea()->setVisible(false);
    clip_->setMouseChildren(false);
    setEnabled(true);
    clip_->gotoAndStop(FRAME_UP); // clips without the label just stay where they are
    clip_->addEventListener(MouseEvent::CLICK, this, &MovieClipButton::onMouseClick);
}

MovieClipButton::~MovieClipButton() {
    clip_->removeEventListener(MouseEvent::CLICK, this, &MovieClipButton::onMouseClick);
}

void MovieClipButton::setEnabled(bool value) {
    if (enabled_ == value) return;
    enabled_ = value;
    clip_->setButtonMode(enabled_);
    clip_->gotoAndStop(enabled_ ? FRAME_UP : FRAME_DISABLED);
}

void MovieClipButton::onMouseClick(MouseEvent& event) {
    if (enabled_) return;
    event.stopImmediatePropagation();
    engine::DisplayObjectContainer* parent = clip_->parent();
    if (parent == nullptr) return;
    MouseEvent copy(event.type(), event.bubbles(), event.cancelable());
    const engine::Point local = parent->globalToLocal({event.stageX, event.stageY});
    copy.localX = local.x;
    copy.localY = local.y;
    copy.stageX = event.stageX;
    copy.stageY = event.stageY;
    copy.buttonDown = event.buttonDown;
    copy.ctrlKey = event.ctrlKey;
    copy.altKey = event.altKey;
    copy.shiftKey = event.shiftKey;
    copy.relatedObject = event.relatedObject;
    parent->dispatchEvent(copy);
}

} // namespace takeover
