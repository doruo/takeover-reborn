// FSquad.h - a squad on the battlefield (original: _SafePkg_868/FSquad and
// its definition _SafeStr_1281).
//
// The simulation's Squad with a face: its soldiers are FUnits drawn in the
// battlefield layer, it has a clickable icon, fills the unit tooltip, plays
// the battle cries and death sounds, and hides soldiers that walk under a
// building (red pixels of the level's hit mask).
#pragma once

#include "game/framework/core/Signal.h"
#include "game/sim/squads/Squad.h"
#include "game/units/FUnit.h"

#include <memory>
#include <string>

namespace engine {
class Event;
class MouseEvent;
class Sprite;
}

namespace takeover {

class HintWindow2;
class LevelVisual;
class SquadIcon;
class TOResManager;

/// What an FSquad is made from (original: _SafeStr_1281).
struct FSquadDef : SquadDef {
    TOResManager* resources = nullptr; ///< Original: _SafeStr_1252.
    LevelVisual* levelVisual = nullptr; ///< Original: _SafeStr_859.
};

class FSquad : public Squad {
public:
    static constexpr int ICON_EVENT_DOWN = 0;
    static constexpr int ICON_EVENT_UP = 1;
    static constexpr int ICON_EVENT_CLICK = 2;
    static constexpr int ICON_EVENT_OVER = 3;
    static constexpr int ICON_EVENT_OUT = 4;

    FSquad();
    ~FSquad() override;
    FSquad& init(const FSquadDef& def); ///< _SafeStr_1280

    // Tooltip words (original: _SafeStr_1290, _SafeStr_1282, _SafeStr_1283, _SafeStr_1288).
    static std::string attackSpeedText(double attackSpeed);
    static std::string rangeText(double range);
    static std::string speedText(double speed);
    static std::string moraleText(double morale, bool moraleNone);
    /// The definition of a soldier of this kind (original: _SafeStr_1263).
    static FUnitDef makeUnitDef(TOResManager& resources, const ArmyStat& stat, const ArmyStatBonus& bonus);

    Signal<const SquadKey*, int> signalIcon; ///< ICON_EVENT_ on the icon (original: _SafeStr_1251).
    Signal<FSquad*> signalLeftNode;          ///< Left its stronghold (original: _SafeStr_1255).
    Signal<FSquad*> signalAtPosition;        ///< Original: _SafeStr_1256.

    [[nodiscard]] engine::Sprite* icon() const;
    [[nodiscard]] std::shared_ptr<engine::Sprite> iconPtr() const;
    /// Shows this squad's stats in the unit tooltip, or stops (nullptr)
    /// (original: _SafeStr_1274).
    void fillHint(HintWindow2* hint, engine::MovieClip* hintWindow);
    [[nodiscard]] bool isIconHighlighted() const { return iconHighlighted_; } ///< _SafeStr_1287
    void setIconHighlight(bool enable, std::vector<swf::Filter> filters); ///< _SafeStr_1300
    [[nodiscard]] int aliveCount() const { return aliveCount_; }

    void update(double timeElapsed) override;
    void notifySquadAtPosition() override;
    void notifyAllUnitsDead() override;
    void notifyStartBattle() override;
    void checkDeath() override;
    void onActionStatusUpdated() override;
    void onChangeMorale(int eventType, BaseObject* sender, const std::any& status) override;
    void makelevelUP(int level) override;
    void changeFSMState(int newState) override;

protected:
    std::shared_ptr<Unit> createUnit(int index) override;
    void customDispose() override;

private:
    void createIcon();  ///< _SafeStr_1278
    void disposeIcon(); ///< _SafeStr_1271
    void updateIconAndHint(double timeElapsed); ///< _SafeStr_1276
    void listenIcon(engine::Sprite& icon);   ///< _SafeStr_1272
    void unlistenIcon(engine::Sprite& icon); ///< _SafeStr_1269
    void onIconClick(engine::MouseEvent&);
    void onIconUp(engine::MouseEvent&);
    void onIconDown(engine::MouseEvent&);
    void onIconOut(engine::MouseEvent&);
    void onIconOver(engine::MouseEvent&);
    /// Hides the soldiers under buildings (original: _SafeStr_1270).
    void hideUnitsUnderBuildings();
    void onUnitAnim(FUnit* unit, int animId); ///< _SafeStr_1279

    TOResManager* resources_ = nullptr;   ///< Original: _SafeStr_1252.
    LevelVisual* level_ = nullptr;        ///< Original: _SafeStr_859.
    int aliveCount_ = 0;                  ///< Original: _SafeStr_1250.
    std::unique_ptr<SquadIcon> icon_;     ///< Original: _SafeStr_1249.
    engine::MovieClip* hintWindow_ = nullptr; ///< HintUnit2Class (original: _SafeStr_1247).
    bool iconHighlighted_ = false;        ///< Original: _SafeStr_1262.
};

} // namespace takeover
