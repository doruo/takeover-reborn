// CompanyInfo.h - the three campaigns ("companies" in the original code),
// and the XP -> rank table (original: data/CompanyInfo, data/XPConverter).
#pragma once

#include "game/sim/battle/Race.h"

#include <cstdint>
#include <optional>
#include <string>

namespace takeover {

class CompanyInfo {
public:
    static constexpr int WESTARIA = 0;
    static constexpr int KHAGANATE = 1;
    static constexpr int CULT = 2;
    static constexpr int COMPANY_COUNT = 3;

    /// Original: _SafeStr_701 (null for an unknown id).
    static std::optional<CompanyInfo> make(int id);
    /// Icon frame of a race in the map/HUD (original: _SafeStr_3495).
    static int raceIconFrame(Race race);

    [[nodiscard]] int id() const { return id_; }
    [[nodiscard]] int number() const { return id_; }
    [[nodiscard]] Race getRace() const;
    [[nodiscard]] std::string description() const { return "WESTARIA description"; } ///< _SafeStr_781, as shipped

private:
    explicit CompanyInfo(int id) : id_(id) {}
    int id_;
};

namespace XPConverter {

/// XP needed for ranks 2, 3, ... (the last rank is 15).
[[nodiscard]] int rankCount();                                 ///< _SafeStr_3691
[[nodiscard]] std::uint32_t rankOf(std::uint32_t xp);          ///< _SafeStr_3646
[[nodiscard]] std::uint32_t xpForRank(std::uint32_t rank);     ///< _SafeStr_3690
[[nodiscard]] std::uint32_t clampXp(std::uint32_t xp);         ///< _SafeStr_3645
/// XP between the current rank and the next one (original: _SafeStr_3627).
[[nodiscard]] std::uint32_t rankSpan(std::uint32_t totalXp);
/// XP earned inside the current rank (original: _SafeStr_3626).
[[nodiscard]] std::uint32_t xpInRank(std::uint32_t totalXp);

} // namespace XPConverter

} // namespace takeover
