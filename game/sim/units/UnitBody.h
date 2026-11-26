// UnitBody.h - a moving body with abilities: the stats-related part of a
// soldier (original: _SafePkg_805/_SafeCls_2186).
#pragma once

#include "game/sim/abilities/Abilities.h"
#include "game/sim/units/MovingBody.h"

#include <memory>

namespace takeover {

    class UnitBody : public MovingBody {
    public:
        [[nodiscard]] bool canMelee() const;        ///< _SafeStr_2195
        [[nodiscard]] bool isMechanism() const;     ///< _SafeStr_863
        [[nodiscard]] bool isRanged() const { return rangeValue() > 0; } ///< _SafeStr_2193
        UnitBody& initAbilities(const ArmyStat& stat, const ArmyStatBonus& bonus); ///< _SafeStr_2199

        /// Speeds up by `factor` of the current speed for `seconds`.
        void increaseSpeed(double factor, double seconds);
        [[nodiscard]] bool hasDash() const;         ///< _SafeStr_2200
        [[nodiscard]] bool hasCharge() const;       ///< _SafeStr_2192
        [[nodiscard]] Abilities& getAbilities() { return *abilities_; } ///< _SafeStr_990
        [[nodiscard]] const Abilities& getAbilities() const { return *abilities_; }
        [[nodiscard]] double distanceIn(double seconds) const { return getSpeed() * seconds; } ///< _SafeStr_2196
        [[nodiscard]] double getSpeed() const;
        [[nodiscard]] int getRange() const;         ///< _SafeStr_1245
        [[nodiscard]] int getMaxHP() const { return abilities_->getMaxHP(); } ///< _SafeStr_996
        [[nodiscard]] int getHP() const { return abilities_->getHP(); }       ///< _SafeStr_1006
        [[nodiscard]] const ArmyStat* getArmyStat() const { return abilities_->getArmyStat(); } ///< _SafeStr_1044
        [[nodiscard]] double attackAnimFactor() const; ///< _SafeStr_2189

        /// Closest distance a ranged unit shoots from (original: _SafeStr_2197).
        [[nodiscard]] double minRangedDistance() const;
        [[nodiscard]] double meleeRadius() const { return abilities_->meleeRadius(); } ///< _SafeStr_2198
        [[nodiscard]] double rangeValue() const;    ///< _SafeStr_2188
        [[nodiscard]] double rangedAttackPeriod() const { return abilities_->attackPeriod(); } ///< _SafeStr_2194
        [[nodiscard]] double meleeAttackPeriod() const { return abilities_->attackPeriod(); }  ///< _SafeStr_1038

        void customUpdate(double dt) override;
        void customDispose() override;

    protected:
        std::shared_ptr<Abilities> abilities_; ///< Original: _SafeStr_2187.
    };

} // namespace takeover
