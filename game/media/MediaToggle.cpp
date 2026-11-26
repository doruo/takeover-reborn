#include "game/media/MediaToggle.h"

namespace takeover {

    MediaToggle::MediaToggle(std::string onButtonName, std::string offButtonName)
        : manager_(std::make_shared<SoundManager>()), onButtonName_(std::move(onButtonName)),
          offButtonName_(std::move(offButtonName)) {}

    bool MediaToggle::enabled() const { return manager_->masterVolume() > kMinVolume; }

    void MediaToggle::setEnabled(bool enable) {
        manager_->setMasterVolume(enable ? 1 : kMinVolume);

        if (auto target = std::dynamic_pointer_cast<engine::MovieClip>(switch_.lock()))
            target->gotoAndStop(enable ? "lbOn" : "lbOff");
    }

    void MediaToggle::attachSwitch(engine::MovieClip* target) {
        switch_ = target ? target->weak_from_this() : std::weak_ptr<engine::DisplayObject>{};

        if (target != nullptr) {
            target->addEventListener(engine::MouseEvent::CLICK, this, &MediaToggle::onMouseClick);
            setEnabled(enabled());
        }
    }

    void MediaToggle::detachSwitch(engine::MovieClip* target) {
        if (target == nullptr)
            return;

        target->removeEventListener(engine::MouseEvent::CLICK, this, &MediaToggle::onMouseClick);

        if (switch_.lock().get() == target)
            switch_.reset();
    }

    void MediaToggle::onMouseClick(engine::MouseEvent& event) {
        auto* clicked = dynamic_cast<engine::DisplayObject*>(event.target());
        if (clicked == nullptr)
            return;

        if (clicked->name() == onButtonName_)
            setEnabled(false);
        else if (clicked->name() == offButtonName_)
            setEnabled(true);
    }

} // namespace takeover
