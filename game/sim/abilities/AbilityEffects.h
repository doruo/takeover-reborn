// AbilityEffects.h - the abilities and attack effects of the units
// (original: package _SafePkg_909).
//
// Everything that changes a unit's numbers is an AbilityEffect: permanent
// abilities read from the unit's stats ("Charge 2", "Undead"), attacks
// (damage arrives as an effect that subtracts HP), poison, freezing, banner
// bonuses... Each frame the unit's parameters are reset and every effect
// applies itself again, until its duration runs out.
#pragma once

#include "game/sim/abilities/Property.h"

#include <memory>
#include <string>
#include <vector>

namespace takeover {

struct ArmyStatBonus;
class UnitParams;

/// Original: _SafeCls_908.
class AbilityEffect : public Property {
public:
    // Ability names, as written in ArmyStat::abilities.
    static inline const std::string SHIELD_WALL = "ShieldWall";         ///< _SafeStr_918
    static inline const std::string DISCIPLINE = "Discipline";
    static inline const std::string CHARGE = "Charge";
    static inline const std::string SPLASH_DAMAGE = "SplashDamage";     ///< _SafeStr_938
    static inline const std::string MECHANISM = "Mechanism";
    static inline const std::string SIEGE_WEAPON = "SiegeWeapon";       ///< _SafeStr_930
    static inline const std::string DIVINE_PRESENCE = "DivinePresence"; ///< _SafeStr_929
    static inline const std::string MARTIAL_ARTISTS = "MartialArtists"; ///< _SafeStr_939
    static inline const std::string UNDEAD = "Undead";
    static inline const std::string POISONOUS = "Poisonous";
    static inline const std::string INTIMIDIATION = "Intimidiation";
    static inline const std::string HORRIFIC_DASH = "HorrificDash";     ///< _SafeStr_931
    static inline const std::string POISON_IMMUNITY = "PoisonImmunity"; ///< _SafeStr_933
    static inline const std::string LONG_SPEAR = "LongSpear";           ///< _SafeStr_925
    static inline const std::string EXPLOSIVE = "Explosive";
    static inline const std::string FREEZE_ARROW = "FreezeArrow";       ///< _SafeStr_946
    static inline const std::string FREEZE_BLAST = "FreezeBlast";       ///< _SafeStr_924
    static inline const std::string DAMAGE_RESIST = "DamageResist";     ///< _SafeStr_928
    static inline const std::string REGENERATION = "Regeneration";      ///< _SafeStr_919
    // Attack kinds.
    static inline const std::string ATK_MELEE = "ATK_MELEE";
    static inline const std::string ATK_DISTANCE = "ATK_DISTANCE";
    static inline const std::string ATK_CHARGE = "ATK_CHARGE";
    static inline const std::string ATK_POISON = "ATK_POISON";
    static inline const std::string ATK_FREEZ = "ATK_FREEZ";
    static inline const std::string ATK_MAGIC = "ATK_MAGIC";
    static inline const std::string ATK_SPEAR = "ATK_SPEAR";
    // Display names (the hints show them).
    static inline const std::string NAME_MECHANISM = "MECHANISM"; ///< _SafeStr_944
    static inline const std::string NAME_UNDEAD = "UNDEAD";       ///< _SafeStr_948

    static constexpr double INFINITE_TIME = 2000000; ///< _SafeStr_949
    static constexpr int KIND_ABILITY = 1;           ///< _SafeStr_940
    static constexpr int KIND_DAMAGE = 2;            ///< _SafeStr_941
    /// Dispatched by attack effects with the attacker's id (original: EVENT_EFFECT_ATTACK).
    static const int EVENT_EFFECT_ATTACK;

    AbilityEffect(int sourceId = 0, std::string name = "", std::string paramName = "", int durationMs = -1, int kind = 1);

    /// "CHARGE 2x" (original: static _SafeStr_900).
    static std::string displayName(const std::string& ability, const std::string& param, const ArmyStatBonus& bonus);
    /// The hint text of an ability (original: static _SafeStr_781).
    static std::string description(const std::string& ability, const std::string& param, const ArmyStatBonus& bonus);

    virtual void setUnitParams(UnitParams* params) { params_ = params; }
    /// Poison or freeze: blocked by the banner of liberation (original: _SafeStr_952).
    [[nodiscard]] bool isNegativeMagic() const { return name_ == ATK_POISON || name_ == ATK_FREEZ; }
    [[nodiscard]] int getKind() const { return kind_; } ///< _SafeStr_951
    [[nodiscard]] virtual std::shared_ptr<AbilityEffect> getCopy() const;
    void customUpdate(double dt) override;
    void customDispose() override;

    [[nodiscard]] int sourceId() const { return sourceId_; }

protected:
    UnitParams* params_ = nullptr; ///< Original: _SafeStr_913.
    int sourceId_;                 ///< The unit that created the effect (original: _SafeStr_914).
    int kind_;                     ///< Original: _SafeStr_911.
    std::string paramName_;        ///< The parameter it changes (original: _SafeStr_912).
    int durationMs_;               ///< -1 for permanent (original: _SafeStr_915).
    double duration_;              ///< Seconds (original: _SafeStr_910).
};

/// Adds a value to a parameter's base every frame; damage is one with a
/// negative value on HP (original: _SafeStr_955).
class ValueEffect : public AbilityEffect {
public:
    ValueEffect(int sourceId = 0, std::string name = "", std::string paramName = "", int value = 0, int durationMs = 0);
    void setValue(int value) { value_ = value; } ///< _SafeStr_959
    [[nodiscard]] int getValue() const { return value_; } ///< _SafeStr_957
    [[nodiscard]] std::shared_ptr<AbilityEffect> getCopy() const override;
    void customUpdate(double dt) override;

protected:
    int value_; ///< Original: _SafeStr_954.
};

/// Original: _SafeCls_1317.
class DisciplineEffect : public AbilityEffect {
public:
    explicit DisciplineEffect(int sourceId = 0, int durationMs = -1);
    [[nodiscard]] std::shared_ptr<AbilityEffect> getCopy() const override;
    void customUpdate(double dt) override;
};

/// Adds a value to a parameter's current value (original: _SafeCls_1897).
class ParamModifier : public AbilityEffect {
public:
    ParamModifier(int sourceId = 0, std::string name = "", std::string paramName = "", int value = 0,
                  int durationMs = -1, int kind = 1);
    [[nodiscard]] std::shared_ptr<AbilityEffect> getCopy() const override;
    void customUpdate(double dt) override;

private:
    int value_;
};

/// Original: _SafeCls_1943.
class PoisonImmunityEffect : public AbilityEffect {
public:
    explicit PoisonImmunityEffect(int sourceId = 0, int durationMs = -1);
    bool makeAddPropertyCollision(IProperty& added) override;
    [[nodiscard]] std::shared_ptr<AbilityEffect> getCopy() const override;
};

/// Reduces incoming damage by a percentage (original: _SafeCls_953).
class DamageResistEffect : public AbilityEffect {
public:
    DamageResistEffect(int sourceId = 0, int percent = 50, int durationMs = -1);
    bool makeAddPropertyCollision(IProperty& added) override;
    [[nodiscard]] std::shared_ptr<AbilityEffect> getCopy() const override;

private:
    int percent_;
};

/// Adds a percentage of a parameter's value (original: _SafeStr_956).
class PercentModifier : public AbilityEffect {
public:
    PercentModifier(int sourceId = 0, std::string name = "", std::string paramName = "", int percent = 0,
                    int durationMs = -1, int kind = 1);
    /// int(value * percent / 100 + 0.5) (original: _SafeStr_958).
    static int percentOf(int value, int percent) { return static_cast<int>(value * percent / 100.0 + 0.5); }
    [[nodiscard]] std::shared_ptr<AbilityEffect> getCopy() const override;
    void customUpdate(double dt) override;

private:
    int percent_;
};

/// One hit: damage on HP, reported to the attacker (original: _SafeStr_2346).
class AttackEffect : public ValueEffect {
public:
    AttackEffect(int sourceId = 0, std::string name = "", int damage = 0, int durationMs = 0);
    [[nodiscard]] std::shared_ptr<AbilityEffect> getCopy() const override;
    void customUpdate(double dt) override;
};

/// Applies a value effect every `period` seconds (poison, regeneration);
/// doesn't stack with an effect of the same name (original: _SafeStr_2860).
class PeriodicEffect : public ValueEffect {
public:
    PeriodicEffect(int sourceId = 0, std::string name = "", std::string paramName = "", double period = 0,
                   int value = 0, int durationMs = 0);
    bool makeAddPropertyCollision(IProperty& added) override;
    [[nodiscard]] std::shared_ptr<AbilityEffect> getCopy() const override;
    void customUpdate(double dt) override;

private:
    double period_; ///< Original: _SafeStr_2875.
    double timer_ = 0; ///< Original: _SafeStr_2874.
};

/// Reduces arrow damage by a percentage (original: _SafeStr_2869).
class ShieldWallEffect : public AbilityEffect {
public:
    ShieldWallEffect(int sourceId = 0, int percent = 50, int durationMs = -1);
    bool makeAddPropertyCollision(IProperty& added) override;
    [[nodiscard]] std::shared_ptr<AbilityEffect> getCopy() const override;

private:
    int percent_;
};

/// Not affected by morale, can't be poisoned (original: _SafeStr_2871).
class MechanismEffect : public AbilityEffect {
public:
    explicit MechanismEffect(int sourceId = 0, int durationMs = -1);
    void setUnitParams(UnitParams* params) override;
    bool makeAddPropertyCollision(IProperty& added) override;
    [[nodiscard]] std::shared_ptr<AbilityEffect> getCopy() const override;
    void customUpdate(double dt) override;
};

/// Not affected by morale and poison (original: _SafeStr_2873).
class UndeadEffect : public AbilityEffect {
public:
    explicit UndeadEffect(int sourceId = 0, int durationMs = -1);
    bool makeAddPropertyCollision(IProperty& added) override;
    [[nodiscard]] std::shared_ptr<AbilityEffect> getCopy() const override;
    void customUpdate(double dt) override;
};

/// Lowers the base HP by a percentage of the maximum (original: _SafeStr_1564).
class MaxHPPercentDamage : public AbilityEffect {
public:
    MaxHPPercentDamage(int sourceId = 0, std::string name = "", int percent = 0, int durationMs = 0);
    void setValue(int value) { value_ = value; }
    [[nodiscard]] int getValue() const { return value_; }
    [[nodiscard]] std::shared_ptr<AbilityEffect> getCopy() const override;
    void customUpdate(double dt) override;

private:
    int value_;
};

using EffectList = std::vector<std::shared_ptr<AbilityEffect>>;

} // namespace takeover
