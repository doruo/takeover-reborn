// ITarget.h - what a soldier can attack: another soldier or a stronghold
// (original: interface _SafePkg_805/_SafeStr_1128 and enum _SafeStr_1148).
#pragma once

#include "game/sim/base/Point2D.h"

namespace takeover {

    class Unit;

    enum class TargetType { Unit = 0, Building = 1 };

    class ITarget {
    public:
        virtual ~ITarget() = default;

        /// A soldier starts fighting this target (original: _SafeStr_1482).
        virtual void onTargetedBy(Unit& attacker) = 0;

        /// A melee blow lands (original: _SafeStr_1480).
        virtual void onMeleeHitBy(Unit& attacker) = 0;

        /// A missile lands (original: _SafeStr_1481).
        virtual void onMissileHitBy(Unit& attacker) = 0;

        /// Destroyed or dead (original: _SafeStr_991).
        [[nodiscard]] virtual bool isDestroyed() const = 0;
        [[nodiscard]] virtual TargetType getTargetType() const = 0; ///< _SafeStr_1210
        [[nodiscard]] virtual double getHitRadius() const = 0;     ///< _SafeStr_1479
        [[nodiscard]] virtual Point2D getPosition() const = 0;
    };

} // namespace takeover
