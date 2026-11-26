// Race.h - the factions and unit categories (original: Haxe enums
// battle/_SafeStr_861 and _SafePkg_805/_SafeStr_806 with the helpers of
// _SafePkg_805/_SafeCls_804).
#pragma once

#include <string>

namespace takeover {

/// Values are the Haxe enum indexes, which the original switches on.
enum class Race { TheEmpire = 0, Westaria = 1, TheKhaganate = 2, TheCult = 3, Bandits = 4 };

enum class UnitType { Infantry = 0, Archers = 1, Cavalry = 2, Siege = 3, Magic = 4 };

[[nodiscard]] inline int index(Race race) { return static_cast<int>(race); }
[[nodiscard]] inline int index(UnitType type) { return static_cast<int>(type); }
[[nodiscard]] std::string raceTag(Race race); ///< Haxe enum constructor name ("Westaria"...).

namespace unit_types {

inline constexpr int COUNT = 5;                         ///< Original: _SafeStr_811.
[[nodiscard]] UnitType fromString(const std::string& name, bool* ok = nullptr); ///< _SafeStr_807
[[nodiscard]] std::string toString(UnitType type);      ///< Original: _SafeStr_810.
[[nodiscard]] UnitType fromInt(int value);              ///< Original: _SafeStr_808 (>= 4 is Magic).
[[nodiscard]] inline int toInt(UnitType type) { return static_cast<int>(type); } ///< _SafeStr_809

} // namespace unit_types

} // namespace takeover
