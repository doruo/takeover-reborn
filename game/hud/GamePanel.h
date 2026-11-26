// GamePanel.h - the battle HUD's bottom panel (original: hud/GamePanel and
// hud/USpellButton, with the interface _SafePkg_1406/_SafeCls_1657).
//
// Gold, supply and mana with their tooltips, the spell bar (three spells
// and the ultimate power), the menu, help and speed buttons, and the clock
// with the rank the time is worth.
#pragma once

#include "game/framework/core/Disposable.h"
#include "game/framework/core/Signal.h"
#include "game/framework/input/InputProcess.h"
#include "game/framework/ui/ButtonSet.h"

#include <memory>

namespace engine {
class Event;
class MouseEvent;
class MovieClip;
}

namespace takeover {

class HintWindow2;
class Level;
class Spell;

/// What the panel shows (original: interface _SafeCls_1657, implemented
/// by GameBattle).
class IResourceSource {
public:
    virtual ~IResourceSource() = default;
    [[nodiscard]] virtual int getGold() const = 0;
    [[nodiscard]] virtual int getGoldSpeed() const = 0;     ///< _SafeStr_1662
    [[nodiscard]] virtual int getArmyPower() const = 0;     ///< _SafeStr_1661
    [[nodiscard]] virtual int getSupplyPower() const = 0;   ///< _SafeStr_1659
    [[nodiscard]] virtual int getGoldForSupply() const = 0;
    [[nodiscard]] virtual int getMana() const = 0;
    [[nodiscard]] virtual int getManaSpeed() const = 0;     ///< _SafeStr_1660
    [[nodiscard]] virtual double getUltimateFactor() const = 0; ///< _SafeStr_1658
};

/// The ultimate power button, filling up while the power charges
/// (original: hud/USpellButton).
class USpellButton : public Button {
public:
    explicit USpellButton(ButtonDef def);
    /// Shows the charge, 0..1; clickable when full (original: _SafeStr_1427).
    void setReadyFactor(double factor);

private:
    engine::MovieClip* iconBack_ = nullptr; ///< Original: _SafeStr_2138.
};

class GamePanel : public UpdatableObject, public IInputProcess {
public:
    static constexpr double GOLD_TIME = 5 * 60 + 30;   ///< Under it: "PERFECT".
    static constexpr double SILVER_TIME = 9 * 60 + 30; ///< Under it: "GREAT".
    static constexpr int BUT_MENU = 0;
    static constexpr int BUT_HELP = 1;
    static constexpr int BUT_FAST = 2;
    static constexpr int BUT_FAST_ON = 3;
    static constexpr int BUT_COUNT = 3;
    static constexpr int BUT_USPELL = BUT_COUNT + 2;
    static constexpr int BUT_CHEAT_0 = BUT_COUNT;
    static constexpr int BUT_CHEAT_1 = BUT_COUNT + 1;

    /// Fills the spell hint for a spell bar button (original: _SafeStr_1395).
    static void showSpellHint(int spellId, Level& level);
    /// Fills a spell hint window (original: _SafeStr_3024).
    static void fillSpellHint(const Spell& spell, HintWindow2& hint);

    GamePanel& init(Level& level); ///< initGamePanel

    Signal<int> signalSpellButton; ///< Original: _SafeStr_3020.
    Signal<int> signalMenuButton;  ///< Original: _SafeStr_3019.
    Signal<int> signalUltimate;    ///< Original: _SafeStr_3014.

    [[nodiscard]] const std::shared_ptr<engine::MovieClip>& image() const { return panel_; }
    void update(double timeElapsed) override;
    /// Turns every spell button off (original: _SafeStr_3031).
    void releaseSpellButtons();
    /// Lays out the available spell buttons (original: _SafeStr_3029).
    void layoutSpellButtons();
    void setUltimateVisible(bool visible); ///< _SafeStr_3043
    /// Shows the player's resources (original: _SafeStr_3044).
    void showResources(const IResourceSource& source);

    // IInputProcess
    [[nodiscard]] bool isEnabled() const override { return enabled_; }
    void setEnabled(bool enabled) override { enabled_ = enabled; }
    bool process(double timeElapsed, bool locked) override;
    void reset() override { releaseSpellButtons(); }

protected:
    void customDispose() override;

private:
    void createPanel();  ///< _SafeStr_1389
    void destroyPanel(); ///< _SafeStr_1369
    std::shared_ptr<Button> makeButton(engine::MovieClip* movie, int id); ///< _SafeStr_3013
    std::shared_ptr<SwitchButton> makeSpellButton(int id);                 ///< _SafeStr_3016
    void toggleSpellButton(int pressedId); ///< _SafeStr_3025
    void swapSpeedButtons();               ///< _SafeStr_3037
    void onMenuClick(Button* button);      ///< _SafeStr_3036
    void onMenuOver(Button* button);       ///< _SafeStr_3034
    void onSpellOver(Button* button);      ///< onButtonsOver2
    void onButtonOut(Button* button);      ///< _SafeStr_3018
    void onSpellClick(Button* button);     ///< _SafeStr_3032
    void onUltimateClick(Button* button);  ///< _SafeStr_3028
    [[nodiscard]] Spell spellOf(int buttonId) const; ///< _SafeStr_3021
    void listenResourceHints();   ///< _SafeStr_3027
    void unlistenResourceHints(); ///< _SafeStr_3033
    void onResourceOver(engine::MouseEvent& event); ///< _SafeStr_3023
    void onResourceOut(engine::MouseEvent& event);  ///< _SafeStr_3022
    [[nodiscard]] int hintTypeOf(engine::DisplayObject* object) const; ///< _SafeStr_3026
    void fadeIn(engine::DisplayObject* hint, bool enable);  ///< _SafeStr_3010
    void fadeOut(engine::DisplayObject* hint, bool enable); ///< _SafeStr_3011
    void onFadeIn(engine::Event& event);  ///< _SafeStr_3017
    void onFadeOut(engine::Event& event); ///< _SafeStr_3015

    Level* level_ = nullptr;                    ///< Original: _SafeStr_859.
    bool enabled_ = true;                       ///< Original: _SafeStr_1335.
    std::shared_ptr<engine::MovieClip> panel_;  ///< GamePanelClass (original: _SafeStr_1413).
    std::shared_ptr<ButtonSet> menuButtons_;    ///< Original: _SafeStr_3007.
    int pendingMenuButton_ = -1;                ///< Original: _SafeStr_3008.
    std::shared_ptr<ButtonSet> spellButtons_;   ///< Original: _SafeStr_3005.
    int pendingSpellButton_ = -1;               ///< Original: _SafeStr_3009.
    std::shared_ptr<USpellButton> ultimateButton_; ///< Original: _SafeStr_3006.
    int pendingUltimate_ = -1;                  ///< Original: _SafeStr_3012.
    // The layout of the spell bar, measured on the clip (static in the original).
    int ultimateX_ = 0, centerX_ = 0, buttonSpacing_ = 0;
};

} // namespace takeover
