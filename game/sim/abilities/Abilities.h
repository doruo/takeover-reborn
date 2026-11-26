// Abilities.h - the abilities, effects and parameters of one soldier
// (original: _SafePkg_909/_SafeStr_978).
//
// Built from an ArmyStat: its ability string becomes permanent effects,
// and attacks the soldier makes are produced here as effect lists that the
// victim's Abilities receive.
#pragma once

#include "game/sim/abilities/AbilityEffects.h"
#include "game/sim/abilities/UnitParams.h"
#include "game/sim/battle/ArmyStatBonus.h"
#include "game/sim/battle/Race.h"

#include <memory>
#include <string>
#include <vector>

namespace takeover {

class ArmyStat;
enum class MoraleStatus;

/// Body size of a unit (original: Haxe enum _SafePkg_805/_SafeCls_2162).
enum class UnitSize { Normal = 0, Middle = 1, Gaint = 2 };

class Abilities : public PropertyContainer {
public:
    static const int EVENT_PROPERTY_ADDED;   ///< _SafeStr_2785
    static const int EVENT_PROPERTY_REMOVED; ///< _SafeStr_2782
    static const int EVENT_ATTACK_DONE;      ///< On the attacker's abilities (_SafeStr_1055).
    static const int EVENT_HIT;              ///< On the victim's abilities (_SafeStr_1040).

    Abilities(const ArmyStat& stat, const ArmyStatBonus& bonus, int ownerId = -1);

    static std::vector<std::string> splitAbilities(const std::string& abilities) ; ///< _SafeStr_1304
    static std::vector<std::string> splitAbility(const std::string& ability);      ///< _SafeStr_1302
    static bool hasAbility(const std::string& ability, const std::string& abilities);
    /// Freezing slows movement and attacks (original: _SafeStr_2861).
    static void addFreezeEffects(EffectList& effects, int durationMs, int sourceId = -1, int percent = -110);
    static UnitSize sizeOf(const ArmyStat& stat); ///< _SafeStr_2870
    static int massOf(UnitSize size);             ///< _SafeStr_2872

    double x = 0;
    double y = 0;
    UnitType type = UnitType::Infantry;
    double speed = 30;
    UnitSize size = UnitSize::Normal;
    double power = 0;
    int mass = 1;
    double scale = 1; ///< Original: _SafeStr_2867.

    /// Frames during which negative magic can't touch the unit (banner of
    /// liberation) (original: setSpellDefence / _SafeStr_2857).
    void setSpellDefence(int frames = 2) { spellDefence_ = frames; }
    [[nodiscard]] bool hasSpellDefence() const { return spellDefence_ > 0; } ///< _SafeStr_1054
    void setHealth(int hp);
    void setType(const ArmyStat& stat); ///< _SafeStr_2866

    void removeProperty(std::shared_ptr<IProperty> property) override;
    bool addProperty(std::shared_ptr<IProperty> property) override;
    /// Adds attack effects, unless magic is blocked (original: addEffects).
    void addEffects(const EffectList& effects);
    /// Heroism / fear modify the morale bonus for a while (original: _SafeStr_1074).
    void addMoraleEffect(MoraleStatus status, double seconds);

    [[nodiscard]] UnitParams& getUnitParams() { return *params_; }
    [[nodiscard]] int getSplashDamageRadius() const { return params_->getParamCurValue("SplashDamage"); }
    [[nodiscard]] int getMaxHP() const { return params_->getParam(UnitParams::HP).getMax(); } ///< _SafeStr_996
    [[nodiscard]] int getHP() const { return params_->getParamCurValue(UnitParams::HP); }     ///< _SafeStr_1006
    [[nodiscard]] int freezeDurationMs() const;   ///< _SafeStr_2859
    [[nodiscard]] int getDamageForFortress() const;
    /// Seconds between attacks (original: _SafeStr_2190).
    [[nodiscard]] double attackPeriod() const { return params_->getParamCurValue(UnitParams::ATTACK_SPEED) / 10.0; }
    /// Melee contact distance (original: _SafeStr_1190).
    [[nodiscard]] double meleeRadius() const;
    [[nodiscard]] const ArmyStat* getArmyStat() const { return stat_; } ///< _SafeStr_1044
    [[nodiscard]] const ArmyStatBonus& getBonus() const { return bonus_; }

    EffectList createMeleeAttack();
    EffectList createDistanceAttack();

    void customUpdate(double dt) override;
    void customDispose() override;

private:
    void addAbilities(const std::string& abilities, const ArmyStatBonus& bonus); ///< _SafeStr_2862
    void onEffectAttack(int eventType, BaseObject* sender, const std::any& attackerId); ///< _SafeStr_2868
    /// Poison, freeze, charge and spear effects that come with a hit (original: _SafeStr_2858).
    void addSecondaryEffects(EffectList& effects, int damage);
    static int chargeBonus(int charge, const ArmyStatBonus& bonus); ///< _SafeStr_2864

    const ArmyStat* stat_ = nullptr; ///< Original: _SafeStr_977.
    ArmyStatBonus bonus_;            ///< Original: _SafeStr_985.
    std::unique_ptr<UnitParams> params_; ///< Original: _SafeStr_2856.
    int ownerId_ = -1;               ///< The unit (original: _SafeStr_2855).
    int spellDefence_ = 0;
};

} // namespace takeover
