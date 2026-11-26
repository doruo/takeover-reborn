#include "game/gui/ChooseProfileScreen.h"

#include "game/GameStorage.h"
#include "game/framework/Symbols.h"
#include "game/framework/core/Trash.h"

namespace takeover {

    ChooseProfileScreen::ChooseProfileScreen(std::shared_ptr<ScreenDef> def, GameStorage& storage) : storage_(&storage) {
        def->viewClass = "scrChooseProfileClass";
        initScreen(std::move(def));
        background_ = std::make_unique<TitleScreenBack>(clip("mcBack"));
        const char* mode = storage_->isOnline() ? "lbAG" : "lbMain";
        window()->gotoAndStop(mode);
        panel()->gotoAndStop(mode);
        createIcons();
    }

    void ChooseProfileScreen::customDispose() {
        if (background_) {
            background_->dispose();
            background_.reset();
        }

        disposeIcons();
        storage_ = nullptr;
        GameScreen::customDispose();
    }

    void ChooseProfileScreen::createIcons() {
        for (int i = 0; i < GameStorage::PROFILES_COUNT; ++i) {
            auto icon = std::make_shared<ProfileIcon>(clip("mcSlot" + std::to_string(i)), *storage_, i);
            icon->signalClick.addListener(this, &ChooseProfileScreen::onIconClick);
            icon->signalDelete.addListener(this, &ChooseProfileScreen::onIconDelete);
            icons_.push_back(icon);
        }
    }

    void ChooseProfileScreen::disposeIcons() {
        for (auto& icon : icons_) {
            icon->dispose();
            releaseLater(icon);
        }

        icons_.clear();
    }

    void ChooseProfileScreen::onIconClick(ProfileIcon* icon) {
        if (icon->isEmpty())
            storage_->createProfile(icon->id());

        storage_->selectProfile(icon->id());
        sendButton(BUT_SELECT);
    }

    void ChooseProfileScreen::onIconDelete(ProfileIcon* icon, bool yes) {
        if (yes) {
            storage_->eraseProfile(icon->id());
            storage_->save();
        }
    }

    void ChooseProfileScreen::initButtonsBlock() {
        GameScreen::initButtonsBlock();
        addButton(symbols::child(panel(), "mcCloseButton"), std::nullopt, BUT_QUIT);
    }

} // namespace takeover
