#include "game/gui/TitleScreen.h"

#include "game/framework/Symbols.h"
#include "game/progress/Profile.h"

namespace takeover {

    TitleScreenBack::TitleScreenBack(engine::MovieClip* background)
        : branding_(std::make_unique<Branding>(symbols::child(background, "mcLogoIriySoft"),
                                               symbols::child(background, "butArmorLogo"),
                                               symbols::child(background, "mcLogoA10Com"))) {}

    TitleScreen::TitleScreen(std::shared_ptr<ScreenDef> def) {
        def->viewClass = "scrTitleClass";
        initScreen(std::move(def));
    }

    void TitleScreen::initButtonsBlock() {
        GameScreen::initButtonsBlock();
        addButton("butStart", "START", BUT_START);
        background_ = std::make_unique<TitleScreenBack>(clip("mcBack"));
    }

    void TitleScreen::disposeButtonsBlock() {
        if (background_) {
            background_->dispose();
            background_.reset();
        }

        GameScreen::disposeButtonsBlock();
    }

    GameIntro::GameIntro(std::shared_ptr<ScreenDef> def) {
        def->viewClass = "GameIntroClass";
        initScreen(std::move(def));
    }

    void GameIntro::update(double timeElapsed) {
        auto* intro = static_cast<engine::MovieClip*>(image_.get());
        if (intro->currentFrame() == intro->totalFrames()) {
            intro->stop();
            sendButton(BUT_SKIP);
        } else {
            GameScreen::update(timeElapsed);
        }
    }

    void GameIntro::initButtonsBlock() {
        GameScreen::initButtonsBlock();
        addButton("butSkip", "SKIP", BUT_SKIP);
    }

    VictoryScreen::VictoryScreen(std::shared_ptr<GameScreenDef> def) {
        def->viewClass = "scrCongratulationsClass";
        Profile* profile = def->profile;
        initScreen(std::move(def));
        static_cast<engine::MovieClip*>(image_.get())->gotoAndStop(profile->selectedCompany().info().id() + 1);
    }

    void VictoryScreen::initButtonsBlock() {
        GameScreen::initButtonsBlock();
        addButton(symbols::find(*image_, "mcMessage.butYes"), "SELECT ANOTHER", BUT_YES);
        addButton(symbols::find(*image_, "mcMessage.butNo"), "CONTINUE THIS ONE", BUT_NO);
    }

} // namespace takeover
