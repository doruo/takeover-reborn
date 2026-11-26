#include "game/sim/battle/MissionStats.h"

namespace takeover {

    void MissionStats::set(int counter, int value) {
        int& slot = values_[static_cast<std::size_t>(counter)];
        if (slot != value) {
            slot = value;
            markChanged();
        }
    }

    void MissionStats::add(int counter, int amount) {
        values_[static_cast<std::size_t>(counter)] += amount;
        markChanged();
    }

} // namespace takeover
