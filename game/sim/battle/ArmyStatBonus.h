// ArmyStatBonus.h - multipliers and extras applied to an ArmyStat by the
// edicts (original: battle/_SafeStr_932). Built by Combatant::getArmyStatBonus.
#pragma once

#include <cmath>

namespace takeover {

    struct ArmyStatBonus {
        double hpFactor = 1;          ///< Original: _SafeStr_2247.
        double chargeFactor = 1;      ///< Original: _SafeStr_921.
        double moraleFactor = 1;      ///< Original: _SafeStr_2251.
        double damageFactor = 1;      ///< Original: _SafeStr_2249.
        int addUnit = 0;              ///< Extra units per squad.
        double speedFactor = 1;       ///< Original: _SafeStr_2250.
        int strongholdDamageBonus = 0; ///< Percent, original: _SafeStr_2256.
        int poisonDamageBonus = 0;    ///< Original: _SafeStr_2255.
        double xpFactor = 1;          ///< Original: _SafeStr_1084.
        double regenFactor = 1;       ///< Giantbots' regeneration, original: _SafeStr_2248.
        int mechanismCritChance = 0;  ///< Percent, original: _SafeStr_2252.
        int infantryFreezeChance = 0; ///< Percent, original: _SafeStr_2253.
        int freezeDurationBonus = 0;  ///< Seconds, original: _SafeStr_2254.

        /// ceil(value * factor) (original: _SafeStr_2246).
        static int apply(int value, double factor) { return static_cast<int>(std::ceil(value * factor)); }

        [[nodiscard]] int getSpeed(int value) const { return apply(value, speedFactor); }
        [[nodiscard]] int getHP(int value) const { return apply(value, hpFactor); }
        [[nodiscard]] int getDamage(int value) const { return apply(value, damageFactor); }
        [[nodiscard]] int getCharge(int value) const { return apply(value, chargeFactor); }       ///< _SafeStr_2258
        [[nodiscard]] int getBaseMorale(int value) const { return apply(value, moraleFactor); }
        [[nodiscard]] int getRegen(int value) const { return apply(value, regenFactor); }         ///< _SafeStr_2257
    };

} // namespace takeover
