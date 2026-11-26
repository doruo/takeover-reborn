// MoraleStatus.h - small enums of the squads (original Haxe enums of
// package _SafePkg_868). Values are the Haxe enum indexes.
#pragma once

namespace takeover {

    /// Original: _SafeStr_1001.
    enum class MoraleStatus { Heroism = 0, HightMorale = 1, Neutral = 2, LowMorale = 3, Feared = 4 };

    /// What a computer squad is used for (original: _SafeStr_1005).
    enum class SquadRole { Defender = 0, Attacker = 1, Scout = 2 };

    /// Original: _SafeStr_994.
    enum class SquadState {
        Charging = 0,
        DefendingAndAttack = 1,
        Defending = 2,
        MeleeFighting = 3,
        Moving = 4,
        RangedFighting = 5,
        Repairing = 6,
        Sleeping = 7,
    };

} // namespace takeover
