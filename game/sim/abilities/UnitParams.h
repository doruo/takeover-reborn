// UnitParams.h - the live numbers of one soldier: HP, speed, attack
// speed, morale bonus and the ability parameters, each a BoundedValue that
// effects modify every frame (original: _SafePkg_909/_SafeStr_916).
//
// Each frame, every value is reset to its base and the active effects add
// their modifiers again (see Abilities::customUpdate).
#pragma once

#include "game/sim/abilities/Property.h"

#include <map>
#include <string>

namespace takeover {

    class ArmyStat;
    struct ArmyStatBonus;

    class UnitParams {
    public:
        // Parameter names.
        static inline const std::string HP = "HP";                        ///< _SafeStr_2565
        static inline const std::string ATTACK_SPEED = "ATTACK_SPEED";    ///< _SafeStr_2573
        static inline const std::string SPEED = "SPEED";                  ///< _SafeStr_2191
        static inline const std::string RANGE = "RANGE";                  ///< _SafeStr_2569
        static inline const std::string MELEE_PENALTY = "MELEE_PENALTY";  ///< _SafeStr_2572
        static inline const std::string PRP_MORALE_LOCK = "PRP_MORALE_LOCK";
        static inline const std::string PRP_MORALE_BONUS = "PRP_MORALE_BONUS";
        static inline const std::string PRP_MORALE_BONUS_FOR_ATTACK = "PRP_MORALE_BONUS_FOR_ATTACK";
        static inline const std::string PRP_FEAR_LOCK = "PRP_FEAR_LOCK";
        static inline const std::string PRP_DOUBLE_DMG_CHANCE = "PRP_DOUBLE_DMG_CHANCE";
        static inline const std::string PRP_FREEZE_CHANCE = "PRP_FREEZE_CHANCE";
        static inline const std::string PRP_FREEZE_DURATION = "PRP_FREEZE_DURATION";
        static inline const std::string PRP_PURSUIT_TIME = "PRP_PURSUIT_TIME";
        static inline const std::string PRP_SPEAR_DEFENCE = "PRP_SPEAR_DEFENCE";
        static constexpr int MORALE_SCALE = 10; ///< Original: _SafeStr_1107.

        UnitParams(const ArmyStat& stat, const ArmyStatBonus& bonus) { init(stat, bonus); }

        /// HP / damage of a squad member at a squad level (original:
        /// _SafeStr_1405, _SafeStr_1292, _SafeStr_1295).
        static int levelHP(const ArmyStat& stat, const ArmyStatBonus& bonus, int level);
        static int levelDamageMin(const ArmyStat& stat, const ArmyStatBonus& bonus, int level);
        static int levelDamageMax(const ArmyStat& stat, const ArmyStatBonus& bonus, int level);

        void update(double dt);
        void setMaxDamageTime(double seconds) { maxDamageTime_ = seconds; } ///< _SafeStr_2577
        [[nodiscard]] bool isMaxDamage() const { return maxDamageTime_ > 0; } ///< _SafeStr_2576
        void setLevel(int level);
        void resetParams(); ///< _SafeStr_2570

        /// Doubles the damage with the critical hit chance (original: _SafeStr_2575).
        [[nodiscard]] int applyCritical(int damage) const;
        void init(const ArmyStat& stat, const ArmyStatBonus& bonus);

        [[nodiscard]] int getParamCurValue(const std::string& name) const { return params_.at(name).getValue(); }
        [[nodiscard]] BoundedValue& getParam(const std::string& name) { return params_.at(name); }
        /// Morale multiplier of the damage (original: _SafeStr_2574).
        [[nodiscard]] double moraleFactor() const { return getParamCurValue(PRP_MORALE_BONUS) / 10.0; }
        /// A random damage between min and max (max while enchanted) (original: _SafeStr_2579).
        [[nodiscard]] int rollDamage() const;

        /// Damage after the melee penalty of ranged units (original: _SafeStr_2578).
        [[nodiscard]] int applyMeleePenalty(int damage) const;
        [[nodiscard]] std::string debugString() const; ///< _SafeStr_1032

        int dmgMin = 0;
        int dmgMax = 0;

        /// Damage multiplier of this frame, set by banners (original: _SafeStr_2567).
        double nextDamageFactor = 1;

    private:
        double maxDamageTime_ = 0;  ///< Frost enchantment time left (original: _SafeStr_2563).
        double damageFactor_ = 1;   ///< Original: _SafeStr_2566.
        std::map<std::string, BoundedValue> params_; ///< Original: _SafeStr_1094.
        int baseHP_ = 0;            ///< Original: _SafeStr_2564.
        int baseDmgMin_ = 0;        ///< Original: _SafeStr_2568.
        int baseDmgMax_ = 0;        ///< Original: _SafeStr_2571.
    };

} // namespace takeover
