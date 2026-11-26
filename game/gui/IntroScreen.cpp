#include "game/gui/IntroScreen.h"

#include "engine/platform/Application.h"
#include "game/framework/core/Trash.h"

namespace takeover {

namespace {
/// An invisible 800x600 rectangle (the original draws one with alpha 0).
class ClickArea : public engine::Sprite {
public:
    [[nodiscard]] engine::Rectangle localBounds() const override { return {0, 0, 800, 600}; }
    [[nodiscard]] bool hitTestLocal(engine::Point p) const override {
        return p.x >= 0 && p.x <= 800 && p.y >= 0 && p.y <= 600;
    }
};
} // namespace

IntroScreen::IntroScreen(std::shared_ptr<engine::MovieClip> intro, std::string url)
    : clip_(std::move(intro)), url_(std::move(url)) {
    clip_->addEventListener(engine::Event::ENTER_FRAME, this, &IntroScreen::onEnterFrame);
    if (!url_.empty()) {
        clickArea_ = std::make_shared<ClickArea>();
        clickArea_->setButtonMode(true);
        clickArea_->addEventListener(engine::MouseEvent::CLICK, this, &IntroScreen::onClick);
        clip_->addChild(clickArea_);
    }
}

std::shared_ptr<IntroScreen> IntroScreen::play(std::shared_ptr<engine::MovieClip> intro, std::string url) {
    auto screen = std::make_shared<IntroScreen>(std::move(intro), std::move(url));
    screen->self_ = screen;
    return screen;
}

IntroScreen::~IntroScreen() { dispose(); }

void IntroScreen::onEnterFrame(engine::Event&) {
    if (clip_ && clip_->currentFrame() == clip_->totalFrames()) {
        releaseLater(std::move(self_)); // we are running: destroy at the next frame
        signalFinish.dispatch();
        dispose();
    }
}

void IntroScreen::dispose() {
    if (clickArea_) {
        clickArea_->removeEventListener(engine::MouseEvent::CLICK, this, &IntroScreen::onClick);
        if (clickArea_->parent()) clickArea_->parent()->removeChild(clickArea_.get());
        clickArea_.reset();
    }
    if (clip_) {
        clip_->removeEventListener(engine::Event::ENTER_FRAME, this, &IntroScreen::onEnterFrame);
        if (clip_->parent()) clip_->parent()->removeChild(clip_.get());
        clip_.reset();
    }
}

void IntroScreen::onClick(engine::MouseEvent&) { engine::navigateToURL(url_); }

} // namespace takeover
