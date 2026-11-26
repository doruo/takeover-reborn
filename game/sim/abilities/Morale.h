// Morale.h - the morale of a squad (original: _SafePkg_909/_SafeStr_993).
//
// Morale goes down when the squad is hit (more if the attacker is
// intimidating) and up when it strikes. At the top it turns into Heroism,
// at the bottom into Fear, each lasting 10 seconds. Otherwise it drifts
// back towards the base value. Undead and mechanisms ignore morale
// (locked); disciplined squads can't be frightened.
#pragma once

#include "game/sim/abilities/Property.h"

#include <map>
#include <string>

namespace takeover {

class ArmyStat;
struct ArmyStatBonus;
enum class MoraleStatus;

class Morale : public BaseObject {
public:
    static constexpr int MODE_FEARED = -1;      ///< _SafeStr_1109
    static constexpr int MODE_NORMAL = 0;       ///< _SafeStr_1112
    static constexpr int MODE_HEROIC = 1;       ///< _SafeStr_1111
    static constexpr double MODE_DURATION = 10; ///< _SafeStr_1106
    static constexpr double RECOVERY_PERIOD = 0.5; ///< _SafeStr_1108
    /// Dispatched with the MoraleStatus, every frame in normal mode (original: _SafeStr_1071).
    static const int EVENT_MORALE_CHANGED;

    Morale(const ArmyStat& stat, const ArmyStatBonus& bonus) { init(stat, bonus); }

    /// Locks from the squad's abilities, refreshed every frame (original: _SafeStr_1077).
    void setLocks(bool moraleLocked, bool fearLocked, int attackBonus);
    void setNormalMode();  ///< _SafeStr_1067
    void setHeroicMode();  ///< _SafeStr_1059
    void setFearedMode();  ///< _SafeStr_1068
    void setJustAboveHalf(); ///< _SafeStr_1047
    void resetParams();    ///< _SafeStr_1103
    /// Hit by a squad with that morale (original: _SafeStr_1079).
    void onHitBy(Morale& attacker);
    /// Drifts towards the base morale (original: _SafeStr_1104).
    void recover();
    [[nodiscard]] bool isNormal() const { return getParamCurValue("PRP_MORALE_MODE") == MODE_NORMAL; } ///< _SafeStr_1081
    [[nodiscard]] bool isHeroic() const { return getParamCurValue("PRP_MORALE_MODE") == MODE_HEROIC; } ///< _SafeStr_1034
    [[nodiscard]] bool isFeared() const { return getParamCurValue("PRP_MORALE_MODE") == MODE_FEARED; } ///< _SafeStr_1105
    void init(const ArmyStat& stat, const ArmyStatBonus& bonus);
    [[nodiscard]] int getParamCurValue(const std::string& name) const { return params_.at(name).getValue(); }
    [[nodiscard]] BoundedValue& getParam(const std::string& name) { return params_.at(name); }
    [[nodiscard]] MoraleStatus getStatus() const; ///< _SafeStr_1083
    /// Current / base (original: _SafeStr_1033).
    [[nodiscard]] double ratio() const;
    [[nodiscard]] double modeTimeLeft() const { return modeTimeLeft_; } ///< _SafeStr_1056
    [[nodiscard]] bool isLocked() const { return moraleLocked_; } ///< _SafeStr_1028
    void customUpdate(double dt) override;
    void setStatus(MoraleStatus status); ///< _SafeStr_1095
    /// The squad struck an enemy (original: _SafeStr_1041).
    void onAttack();
    [[nodiscard]] std::string toString() const;

private:
    int attackBonus_ = 0;        ///< Original: _SafeStr_1100.
    double modeTimeLeft_ = 0;    ///< Original: _SafeStr_1096.
    bool moraleLocked_ = false;  ///< Original: _SafeStr_1097.
    bool fearLocked_ = false;    ///< Original: _SafeStr_1101.
    int status_ = 2;             ///< MoraleStatus, unset (Neutral) at first (original: _SafeStr_1099).
    bool hasStatus_ = false;
    double recoveryTimer_ = 0;   ///< Original: _SafeStr_1098.
    std::map<std::string, BoundedValue> params_;
};

} // namespace takeover
