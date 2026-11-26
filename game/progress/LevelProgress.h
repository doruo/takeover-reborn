// LevelProgress.h - the best result of one mission (original: progress/_SafeStr_702).
#pragma once

#include "game/framework/storage/ByteArray.h"

namespace takeover {

    class LevelInfo;
    struct MissionResult;

    class LevelProgress {
    public:
        static constexpr int RANK_NEW = 0; ///< Not completed.
        static constexpr int RANK_BRONZE = 1;
        static constexpr int RANK_SILVER = 2;
        static constexpr int RANK_GOLD = 3;

        int score = 0;          ///< Original: _SafeStr_3443 (saved as a 16-bit short).
        int completeStatus = RANK_NEW;

        /// Original: _SafeStr_770.
        static LevelProgress fromMission(const MissionResult& result, const LevelInfo& level);

        /// True if `other` is better than this result.
        [[nodiscard]] bool compareStatus(const LevelProgress& other) const;
        void reset() { score = 0, completeStatus = RANK_NEW; }
        void serialize(ByteArray& data) const;
        void deserialize(ByteArray& data);
    };

} // namespace takeover
