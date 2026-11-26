// ChooseProfileScreen.h - the three save slots (original:
// gui/ChooseProfileScreen; the Armor Games online slots are not ported).
#pragma once

#include "game/gui/GameScreen.h"
#include "game/gui/ProfileIcon.h"
#include "game/gui/TitleScreen.h"

namespace takeover {

    class GameStorage;

    class ChooseProfileScreen : public GameScreen {
    public:
        static constexpr int BUT_SELECT = 100;
        static constexpr int BUT_QUIT = 101;

        ChooseProfileScreen(std::shared_ptr<ScreenDef> def, GameStorage& storage);

    protected:
        void customDispose() override;
        void initButtonsBlock() override;

    private:
        [[nodiscard]] engine::MovieClip* window() const { return static_cast<engine::MovieClip*>(image_.get()); }
        [[nodiscard]] engine::MovieClip* panel() const { return clip("mcBackPanel"); }
        void createIcons();  ///< _SafeStr_1278
        void disposeIcons(); ///< _SafeStr_1271
        void onIconClick(ProfileIcon* icon);              ///< _SafeStr_3517
        void onIconDelete(ProfileIcon* icon, bool yes);   ///< _SafeStr_3521

        GameStorage* storage_;
        std::unique_ptr<TitleScreenBack> background_;
        std::vector<std::shared_ptr<ProfileIcon>> icons_;
    };

} // namespace takeover
