// CampaignScreens.h - choosing a campaign and its map (original:
// gui/ChooseCompaignScreen and gui/GlobalMapScreen).
#pragma once

#include "game/gui/GameScreen.h"
#include "game/gui/PlayerStatPanel.h"

namespace takeover {

class ChooseCompaignScreen : public GameScreen {
public:
    // The campaign buttons report CompanyInfo ids (0..2).
    static constexpr int BUT_QUIT = 3;
    static constexpr int BUT_DEEDS = 4;
    static constexpr int BUT_TALES = 5;

    explicit ChooseCompaignScreen(std::shared_ptr<GameScreenDef> def);

protected:
    void initButtonsBlock() override;

private:
    Button* addCampaignButton(engine::MovieClip* clip, int companyId); ///< _SafeStr_3485
    GameScreenDef* def() const { return static_cast<GameScreenDef*>(def_.get()); }
};

class GlobalMapScreen : public GameScreen {
public:
    static constexpr int BUT_QUIT = 0;
    static constexpr int BUT_EDICTS = 1;
    static constexpr int BUT_BADGES = 2;
    static constexpr int BUT_TALES = 3;

    explicit GlobalMapScreen(std::shared_ptr<GameScreenDef> def);

    /// The mission chosen with "TO BATTLE" (original: _SafeStr_754).
    Signal<int> signalLevel;

protected:
    void customDispose() override;
    void initVisualBlock() override;
    void disposeVisualBlock() override;
    void initButtonsBlock() override;

private:
    GameScreenDef* def() const { return static_cast<GameScreenDef*>(def_.get()); }
    engine::MovieClip* mapClip() const { return clip("_-Lt"); } ///< The campaign's map (_SafeStr_3433).
    void createLevelButtons();   ///< _SafeStr_3504
    void disposeLevelButtons();  ///< _SafeStr_3503
    void onLevelClick(Button* button);                         ///< _SafeStr_3499
    void showLevelInfo(double x, double y, int levelNum);      ///< _SafeStr_3500
    void onToBattle(Button* button);                           ///< _SafeStr_3498
    void onInfoBackClick(engine::MouseEvent&);                 ///< _SafeStr_3496
    void hideLevelInfo();                                      ///< _SafeStr_3497

    Profile* profile_ = nullptr;
    std::unique_ptr<PlayerStatPanel> statPanel_;
    std::vector<std::shared_ptr<Button>> levelButtons_;
    std::shared_ptr<engine::MovieClip> infoPanel_; ///< LevelInfoPanelClass
    std::shared_ptr<Button> toBattle_;
};

} // namespace takeover
