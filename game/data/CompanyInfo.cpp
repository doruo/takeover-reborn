#include "game/data/CompanyInfo.h"

#include <algorithm>
#include <array>

namespace takeover {

std::optional<CompanyInfo> CompanyInfo::make(int id) {
    if (id < 0 || id >= COMPANY_COUNT) return std::nullopt;
    return CompanyInfo(id);
}

int CompanyInfo::raceIconFrame(Race race) {
    switch (race) {
    case Race::TheKhaganate: return 2;
    case Race::TheCult: return 3;
    case Race::TheEmpire: return 4;
    default: return 1;
    }
}

Race CompanyInfo::getRace() const {
    switch (id_) {
    case WESTARIA: return Race::Westaria;
    case CULT: return Race::TheCult;
    case KHAGANATE: return Race::TheKhaganate;
    default: return Race::Westaria;
    }
}

namespace XPConverter {

namespace {
constexpr std::array<std::uint32_t, 14> kRankTable = {1000,  2200,  3800,  6000,  9000,  13000, 18200,
                                                      24800, 33000, 43000, 55000, 69200, 85800, 105000};
}

int rankCount() { return static_cast<int>(kRankTable.size()); }

std::uint32_t rankOf(std::uint32_t xp) {
    std::uint32_t rank = 0;
    while (rank < kRankTable.size() && xp >= kRankTable[rank]) ++rank;
    return rank + 1;
}

std::uint32_t xpForRank(std::uint32_t rank) { return rank <= 1 ? 0 : kRankTable[rank - 2]; }

std::uint32_t clampXp(std::uint32_t xp) { return std::min(xp, xpForRank(static_cast<std::uint32_t>(rankCount()))); }

std::uint32_t rankSpan(std::uint32_t totalXp) {
    const std::uint32_t rank = rankOf(totalXp);
    if (rank >= static_cast<std::uint32_t>(rankCount())) return 0;
    return xpForRank(rank + 1) - xpForRank(rank);
}

std::uint32_t xpInRank(std::uint32_t totalXp) {
    const std::uint32_t rank = rankOf(totalXp);
    if (rank >= static_cast<std::uint32_t>(rankCount())) return 0;
    return totalXp - xpForRank(rank);
}

} // namespace XPConverter

} // namespace takeover
