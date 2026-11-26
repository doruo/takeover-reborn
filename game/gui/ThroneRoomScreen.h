// ThroneRoomScreen.h - spending rank points on edicts (original:
// gui/ThroneRoomScreen). Shows the edicts tree, the unit tiers and spells
// it unlocks, with hints.
#pragma once

#include "game/gui/EdictIcon.h"
#include "game/gui/GameScreen.h"
#include "game/gui/PlayerStatPanel.h"

namespace takeover {

class ArmyStat;

class ThroneRoomScreen : public GameScreen {
public:
    static constexpr int BUT_OK = 0;
    static constexpr int BUT_UNDO = 1;

    explicit ThroneRoomScreen(std::shared_ptr<GameScreenDef> def);

protected:
    void customDispose() override;
    void initVisualBlock() override;
    void disposeVisualBlock() override;
    void initButtonsBlock() override;
    void disposeButtonsBlock() override;
    void onButtonClick(Button* button) override;

private:
    [[nodiscard]] EdictTree& edicts() const; ///< _SafeStr_3457
    void createIcons();   ///< _SafeStr_1278
    void disposeIcons();  ///< _SafeStr_1271
    void refresh();       ///< _SafeStr_3458
    void onEdictClick(EdictIcon* icon);                                   ///< _SafeStr_3465
    std::shared_ptr<Button> createUnitButton(engine::MovieClip* icon, const ArmyStat& stat, int id); ///< _SafeStr_3466
    static void setGrayed(bool enabled, engine::DisplayObject* clip);     ///< _SafeStr_3460
    void refreshUnlocks();                                                ///< _SafeStr_3467
    void onUnitRollOver(Button* button);                                  ///< _SafeStr_3464
    void onSpellRollOver(Button* button);                                 ///< _SafeStr_3462
    void hideHint(Button* button = nullptr);

    GameScreenDef* def() const { return static_cast<GameScreenDef*>(def_.get()); }

    Profile* profile_ = nullptr;
    HintLayer* hintLayer_ = nullptr;
    std::unique_ptr<PlayerStatPanel> statPanel_;
    Button* resetButton_ = nullptr; ///< Original: _SafeStr_3459.
    std::vector<std::shared_ptr<EdictIcon>> edictIcons_;
    std::vector<std::vector<std::shared_ptr<Button>>> unitButtons_;
    std::vector<std::shared_ptr<Button>> spellButtons_;
    int unitHintId_ = 0;  ///< Original: _SafeStr_3087.
    int spellHintId_ = 0; ///< Original: _SafeStr_3078.
};

} // namespace takeover
