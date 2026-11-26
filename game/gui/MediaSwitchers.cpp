#include "game/gui/MediaSwitchers.h"

#include "game/framework/Symbols.h"
#include "game/gui/GameScreen.h"
#include "game/level/Level.h"
#include "game/media/MusicConst.h"
#include "game/media/SoundConst.h"

namespace takeover {

    using engine::MouseEvent;

    MediaSwitchers::MediaSwitchers(engine::MovieClip* block) : block_(sharedClip(block)) {
        if (butMusic() == nullptr || butSound() == nullptr) {
            block_.reset();
            return;
        }

        MusicConst::instance().attachSwitch(butMusic());
        SoundConst::instance().attachSwitch(butSound());
        butMusic()->addEventListener(MouseEvent::ROLL_OVER, this, &MediaSwitchers::onMusicRollOver);
        butMusic()->addEventListener(MouseEvent::ROLL_OUT, this, &MediaSwitchers::onRollOut);
        butSound()->addEventListener(MouseEvent::ROLL_OVER, this, &MediaSwitchers::onSoundRollOver);
        butSound()->addEventListener(MouseEvent::ROLL_OUT, this, &MediaSwitchers::onRollOut);
        attachBacklight(butSound());
        attachBacklight(butMusic());
        hint_ = std::make_unique<HintWindow>(symbols::create("Hint"));
        hint_->setMargins(2, 1);
    }

    engine::MovieClip* MediaSwitchers::butMusic() const { return symbols::child(block_.get(), "butMusic"); }
    engine::MovieClip* MediaSwitchers::butSound() const { return symbols::child(block_.get(), "butSound"); }
    engine::MovieClip* MediaSwitchers::backlight(engine::DisplayObject* icon) { return symbols::child(icon, "Backlight"); }

    void MediaSwitchers::customDispose() {
        if (block_) {
            MusicConst::instance().detachSwitch(butMusic());
            SoundConst::instance().detachSwitch(butSound());
            detachBacklight(butSound());
            detachBacklight(butMusic());
            butMusic()->removeEventListener(MouseEvent::ROLL_OVER, this, &MediaSwitchers::onMusicRollOver);
            butMusic()->removeEventListener(MouseEvent::ROLL_OUT, this, &MediaSwitchers::onRollOut);
            butSound()->removeEventListener(MouseEvent::ROLL_OVER, this, &MediaSwitchers::onSoundRollOver);
            butSound()->removeEventListener(MouseEvent::ROLL_OUT, this, &MediaSwitchers::onRollOut);
            block_.reset();
        }

        Disposable::customDispose();
    }

    void MediaSwitchers::showHint(engine::MovieClip* button, const char* text) {
        block_->addChild(hint_->imagePtr());
        hint_->setText(text);
        hint_->setBounds({0, 0, 800 - 5, 600});
        hint_->show(button->getBounds(button->parent()), DockSide::Bottom, DockAlign::LeftTop, 5);
        Level::isCursorUnderButton = true;
        SoundConst::play(SoundId::BUTTON_OVER);
    }

    void MediaSwitchers::onMusicRollOver(MouseEvent& event) {
        if (auto* icon = dynamic_cast<engine::DisplayObject*>(event.target()))
            backlight(icon)->gotoAndStop(1);

        showHint(butMusic(), "Turn music on/off");
    }

    void MediaSwitchers::onSoundRollOver(MouseEvent& event) {
        if (auto* icon = dynamic_cast<engine::DisplayObject*>(event.target()))
            backlight(icon)->gotoAndStop(1);

        showHint(butSound(), "Turn sound effects on/off");
    }

    void MediaSwitchers::onRollOut(MouseEvent& event) {
        if (auto* icon = dynamic_cast<engine::DisplayObject*>(event.target()))
            backlight(icon)->play();

        if (hint_->image()->parent() != nullptr)
            hint_->image()->parent()->removeChild(hint_->image());

        Level::isCursorUnderButton = false;
    }

    void MediaSwitchers::attachBacklight(engine::MovieClip* icon) {
        engine::MovieClip* light = backlight(icon);
        light->setMouseEnabled(false);
        light->setMouseChildren(false);
        light->gotoAndStop(light->totalFrames());
        light->addEventListener(engine::Event::ENTER_FRAME, this, &MediaSwitchers::onBacklightFrame);
    }

    void MediaSwitchers::detachBacklight(engine::MovieClip* icon) {
        if (engine::MovieClip* light = backlight(icon)) {
            light->removeEventListener(engine::Event::ENTER_FRAME, this, &MediaSwitchers::onBacklightFrame);
            icon->removeChild(light);
        }
    }

    void MediaSwitchers::onBacklightFrame(engine::Event& event) {
        // The backlight plays once when the cursor leaves, then stays dark.
        auto* light = dynamic_cast<engine::MovieClip*>(event.target());
        if (light != nullptr && light->currentFrame() == light->totalFrames())
            light->gotoAndStop(light->totalFrames());
    }

} // namespace takeover
