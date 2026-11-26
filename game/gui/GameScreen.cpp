#include "game/gui/GameScreen.h"

#include "game/framework/Symbols.h"
#include "game/gui/MediaSwitchers.h"

namespace takeover {

    std::shared_ptr<engine::MovieClip> sharedClip(engine::DisplayObject* clip) {
        return clip ? std::dynamic_pointer_cast<engine::MovieClip>(clip->shared_from_this()) : nullptr;
    }

    GameScreen::~GameScreen() = default;

    GameScreen& GameScreen::initScreen(std::shared_ptr<ScreenDef> def) {
        def_ = std::move(def);
        initVisualBlock();
        initDevices();
        initButtonsBlock();

        return *this;
    }

    void GameScreen::customDispose() {
        disposeButtonsBlock();
        disposeDevices();
        disposeVisualBlock();
        def_.reset();
        GameState::customDispose();
    }

    void GameScreen::update(double timeElapsed) {
        updateDevices(timeElapsed);
        updateScreen(timeElapsed);
        GameState::update(timeElapsed);
    }

    void GameScreen::initDevices() {
        keyboard_ = std::make_shared<KeyboardDevice>(image_.get());
        mouse_ = std::make_shared<MouseDevice>(image_.get());
    }

    void GameScreen::disposeDevices() {
        if (def_)
            def_->dispose();

        if (keyboard_)
            keyboard_->dispose();

        if (mouse_)
            mouse_->dispose();

        keyboard_.reset();
        mouse_.reset();
    }

    void GameScreen::updateDevices(double timeElapsed) {
        if (keyboard_)
            keyboard_->update(timeElapsed);

        if (mouse_)
            mouse_->update(timeElapsed);
    }

    void GameScreen::initVisualBlock() {
        if (def_ && !def_->viewClass.empty()) {
            image_ = symbols::create(def_->viewClass);

            // Not every view has the music/sound switches.
            if (auto* soundBlock = symbols::child<engine::MovieClip>(*image_, "mcSoundBlock"))
                mediaSwitchers_ = std::make_shared<MediaSwitchers>(soundBlock);
        } else {
            image_ = std::make_shared<engine::Sprite>();
        }
    }

    void GameScreen::disposeVisualBlock() {
        if (mediaSwitchers_) {
            mediaSwitchers_->dispose();
            mediaSwitchers_.reset();
        }

        if (image_ && image_->parent())
            image_->parent()->removeChild(image_.get());

        // The view is released with the screen (it may still be on the call
        // stack of a click handler).
    }

    void GameScreen::disposeButtonsBlock() {
        for (auto& button : buttons_)
            if (button)
                button->dispose();
        buttons_.clear();
        signalButton.dispose();
    }

    Button* GameScreen::addButton(engine::MovieClip* buttonClip, std::optional<std::string> caption, int id) {
        auto button = std::make_shared<Button>(ButtonDef::make(sharedClip(buttonClip), std::move(caption), id));
        button->signalClick.addListener(this, &GameScreen::onButtonClick);
        buttons_.push_back(button);

        return button.get();
    }

    Button* GameScreen::addButton(std::string_view childName, std::optional<std::string> caption, int id) {
        return addButton(clip(childName), std::move(caption), id);
    }

    void GameScreen::onButtonClick(Button* button) { sendButton(button->id()); }

    engine::MovieClip* GameScreen::clip(std::string_view name) const {
        return symbols::child<engine::MovieClip>(*image_, name);
    }

} // namespace takeover
