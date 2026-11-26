// BadgesRoomScreen.h - the achievements, 8 per page (original: gui/BadgesRoomScreen).
#pragma once

#include "game/gui/GameScreen.h"
#include "game/gui/PlayerStatPanel.h"

namespace takeover {

class Badge;

class BadgesRoomScreen : public GameScreen {
public:
    static constexpr int BUT_OK = 0;
    static constexpr int BUT_SCROLL_LEFT = 1;
    static constexpr int BUT_SCROLL_RIGHT = 2;

    explicit BadgesRoomScreen(std::shared_ptr<GameScreenDef> def);

protected:
    void initVisualBlock() override;
    void disposeVisualBlock() override;
    void initButtonsBlock() override;
    void disposeButtonsBlock() override;
    void onButtonClick(Button* button) override;

private:
    void createPages();                                         ///< _SafeStr_3592
    static void showBadge(engine::MovieClip* slot, const Badge& badge); ///< _SafeStr_3593
    void showPage(int pageIndex);                               ///< _SafeStr_3591

    Profile* profile_ = nullptr;
    std::unique_ptr<PlayerStatPanel> statPanel_;
    Button* scrollLeft_ = nullptr;
    Button* scrollRight_ = nullptr;
    std::vector<std::shared_ptr<engine::MovieClip>> pages_; ///< AchievementsPageClass
    int currentPage_ = 0;
    std::shared_ptr<engine::Sprite> pageIndicators_;
};

} // namespace takeover
