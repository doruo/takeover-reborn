// MissionStats.h - counters kept for each combatant during a mission
// (original: _SafePkg_777/_SafeStr_789), and the result handed to the
// mission-over screen (original: _SafePkg_777/_SafeStr_720).
#pragma once

#include "game/sim/battle/Race.h"

#include <array>
#include <memory>
#include <vector>

namespace takeover {

class MissionStats {
public:
    // Counter ids, named after what the mission-over screen and the badges
    // read from them.
    static constexpr int SQUADS_PURCHASED = 0;       ///< _SafeStr_2223
    static constexpr int SQUADS_LOST = 1;            ///< _SafeStr_2230
    static constexpr int STRONGHOLDS_SEIZED = 2;     ///< _SafeStr_2216
    static constexpr int STRONGHOLDS_LOST = 3;       ///< _SafeStr_2236
    static constexpr int SPELLS_CAST = 4;            ///< _SafeStr_2235
    static constexpr int GOLD_COLLECTED = 5;         ///< _SafeStr_2233
    static constexpr int MANA_COLLECTED = 6;         ///< _SafeStr_2217
    static constexpr int CHESTS_OPENED = 7;          ///< _SafeStr_2228
    static constexpr int BANNERS_SET = 8;            ///< _SafeStr_2218
    static constexpr int ULTIMATES_USED = 9;         ///< _SafeStr_2234
    static constexpr int ENEMY_SQUADS_DESTROYED = 10; ///< _SafeStr_2226
    static constexpr int HEROIC_MODE_ENGAGED = 11;   ///< _SafeStr_2232
    static constexpr int VILLAGES_OWNED = 12;        ///< _SafeStr_2222
    static constexpr int MINES_OWNED = 13;           ///< _SafeStr_2221
    static constexpr int OBELISKS_OWNED = 14;        ///< _SafeStr_2231
    static constexpr int SQUADS_OWNED = 15;          ///< _SafeStr_2225
    static constexpr int STRONGHOLDS_OWNED = 16;     ///< _SafeStr_2219
    static constexpr int FARMS_BUILT = 17;           ///< _SafeStr_2215
    static constexpr int MINES_BUILT = 18;           ///< _SafeStr_2229
    static constexpr int OBELISKS_BUILT = 19;        ///< _SafeStr_2220
    static constexpr int GOLD_SPENT = 20;            ///< _SafeStr_2224
    static constexpr int MANA_SPENT = 21;            ///< _SafeStr_2227
    static constexpr int COUNT = 22;                 ///< _SafeStr_2213

    explicit MissionStats(Race race) : race_(race) {}

    /// Sets a counter; marks the stats as changed if the value differs
    /// (original: _SafeStr_2238).
    void set(int counter, int value);
    void add(int counter, int amount); ///< Original: _SafeStr_2239.
    [[nodiscard]] int get(int counter) const { return values_[static_cast<std::size_t>(counter)]; } ///< _SafeStr_2237
    [[nodiscard]] Race getRace() const { return race_; }

    /// Changed since markUnchanged()? (original: _SafeStr_2240).
    [[nodiscard]] bool hasChanged() const { return !unchanged_; }
    void markUnchanged() { unchanged_ = true; } ///< Original: _SafeStr_2214.
    void markChanged() { unchanged_ = false; }  ///< Original: _SafeStr_2212.

private:
    std::array<int, COUNT> values_{};
    Race race_;
    bool unchanged_ = true;
};

/// Original: _SafeStr_720.
struct MissionResult {
    static constexpr int IN_PROGRESS = 0; ///< _SafeStr_785
    static constexpr int VICTORY = 1;     ///< _SafeStr_787
    static constexpr int DEFEAT = -1;     ///< _SafeStr_788

    std::shared_ptr<MissionStats> playerStats; ///< Original: _SafeStr_790.
    double missionTime = 0;                   ///< Seconds, original: _SafeStr_783.
    int missionOverStatus = IN_PROGRESS;
    int levelNum = 0;
    std::vector<std::shared_ptr<MissionStats>> allStats; ///< Original: _SafeStr_786.
};

} // namespace takeover
