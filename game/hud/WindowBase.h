// WindowBase.h - the window that opens on a building (original: hud/WindowBase,
// hud/_SafeStr_1325, hud/WindowContentManager, hud/_SafeCls_2448 and the
// center panel of package _SafePkg_1406).
//
// A window has a center ring with the building's name and incomes, and
// buttons: recruit units, upgrade, build a farm/mine/obelisk, destroy,
// unleash the ultimate power. The content manager prices the buttons and
// enables those the player can afford.
#pragma once

#include "game/framework/core/Disposable.h"
#include "game/framework/core/Signal.h"
#include "game/framework/ui/Button.h"
#include "game/sim/base/Point2D.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace engine {
class MovieClip;
class Sprite;
}

namespace takeover {

class Combatant;
class Level;
class NodeContent;
class USpellButton;
class WindowBase;

/// The ring in the middle of a window (original: _SafePkg_1406/_SafeStr_1322).
struct CenterPanelDef {
    bool active = true;          ///< Original: _SafeStr_3167.
    std::string caption;
    int frame = 1;               ///< Owner color.
    int goldIncome = 0;          ///< Original: _SafeStr_3143.
    int supplyIncome = 0;        ///< Original: _SafeStr_3145.
    int manaIncome = 0;          ///< Original: _SafeStr_3144.
    bool statsOnRightSide = true; ///< Original: _SafeStr_3146.
};

/// Fills the center ring (original: CenterPanel._SafeStr_1431).
void showCenterPanel(engine::MovieClip& panel, const CenterPanelDef& def);

/// Prices and enables a window's buttons (original: interface _SafeCls_2448).
class IWindowContent {
public:
    virtual ~IWindowContent() = default;
    virtual void dispose() = 0;
    /// The portrait of a unit button (original: _SafeStr_1426).
    virtual std::shared_ptr<engine::DisplayObject> unitIcon(int buttonId) = 0;
    [[nodiscard]] virtual int price(int buttonId) = 0;          ///< _SafeStr_1419
    [[nodiscard]] virtual double ultimateFactor() = 0;          ///< _SafeStr_1433
    virtual void refreshButtons(WindowBase& window) = 0;        ///< _SafeStr_1430
};

/// The usual content manager (original: hud/WindowContentManager).
class WindowContentManager : public IWindowContent {
public:
    // Tutorial locks.
    static inline bool UNLOCK_VILAGE = true;
    static inline bool UNLOCK_MINE = true;
    static inline bool UNLOCK_OBELISK = true;
    static inline int LOCK_UNIT_ABOVE = 999;

    WindowContentManager(Level& level, NodeContent& node) : level_(&level), node_(&node) {}

    void dispose() override { level_ = nullptr, node_ = nullptr; }
    std::shared_ptr<engine::DisplayObject> unitIcon(int buttonId) override;
    [[nodiscard]] int price(int buttonId) override;
    [[nodiscard]] double ultimateFactor() override;
    void refreshButtons(WindowBase& window) override;

private:
    [[nodiscard]] Combatant* owner(); ///< _SafeStr_3153
    Level* level_;
    NodeContent* node_; ///< Original: _SafeStr_3152.
};

/// What a window is made from (original: hud/_SafeStr_1325).
struct WindowDef {
    CenterPanelDef center;               ///< Original: _SafeStr_1432.
    std::string windowClass;             ///< Original: _SafeStr_1338.
    int panelColorFrame = 1;             ///< Original: _SafeStr_1423.
    std::string text;
    /// Frame label by x position: the window opens to the free side
    /// (original: _SafeStr_1339).
    std::vector<std::pair<std::string, double>> positions;
    Point2D position;
    std::shared_ptr<IWindowContent> content; ///< Original: _SafeStr_1416.
};

class WindowBase : public UpdatableObject {
public:
    static constexpr int BUT_ID_UNIT_0 = 0;
    static constexpr int BUT_ID_UNIT_1 = 1;
    static constexpr int BUT_ID_UNIT_2 = 2;
    static constexpr int BUT_ID_UNIT_3 = 3;
    static constexpr int BUT_ID_UNIT_4 = 4;
    static constexpr int BUT_ID_UPGRADE = 5;
    static constexpr int BUT_ID_BUILD_FARM = 6;
    static constexpr int BUT_ID_BUILD_MINE = 7;
    static constexpr int BUT_ID_BUILD_OBELISK = 8;
    static constexpr int BUT_ID_DESTROY = 9;
    static constexpr int BUT_ID_ULTIMATE_SPELL = 10;

    explicit WindowBase(WindowDef def);

    Signal<int> signalAction; ///< A button id (original: _SafeStr_1371).

    [[nodiscard]] const std::shared_ptr<engine::MovieClip>& image() const { return window_; } ///< _SafeStr_1368
    [[nodiscard]] const std::vector<std::shared_ptr<Button>>& buttons() const { return buttons_; } ///< _SafeStr_1363
    [[nodiscard]] USpellButton* ultimateButton() const { return ultimate_.get(); } ///< butSpell
    /// Updates prices, availability and the ultimate charge (original: _SafeStr_1390).
    void refresh();
    /// Adds the button of a unit slot (original: _SafeStr_1333).
    Button& addUnitButton(int slot, int typeId);
    void addUpgradeButton();                 ///< _SafeStr_1350
    /// Shows "max level" instead of the price (original: _SafeStr_1429).
    void showLocked(Button& button, bool locked);
    USpellButton& addUltimateButton();       ///< _SafeStr_1396
    Button& addBuildButton(int slot, int typeId); ///< _SafeStr_1354
    Button& addDestroyButton();              ///< _SafeStr_1404

protected:
    void customDispose() override;

private:
    static void showGold(engine::MovieClip* clip, int gold); ///< _SafeStr_1418
    void placeWindow();   ///< _SafeStr_1421
    void decorate();      ///< _SafeStr_1422
    Button& addButton(engine::MovieClip* movie, int id); ///< _SafeStr_1417
    void onButtonClick(Button* button);

    WindowDef def_;                              ///< Original: _SafeStr_839.
    std::shared_ptr<engine::MovieClip> window_;  ///< Original: _SafeStr_1413.
    std::vector<std::shared_ptr<Button>> buttons_; ///< Original: _SafeStr_1415.
    std::shared_ptr<USpellButton> ultimate_;     ///< Original: _SafeStr_1414.
};

} // namespace takeover
