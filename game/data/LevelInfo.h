// LevelInfo.h - a mission of a campaign: its texts, XP reward, medal times
// and enemies (original: data/LevelInfo).
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace takeover {

    class Combatant;
    struct MissionResult;

    class LevelInfo {
    public:
        /// Time limits for the gold and silver medals, in seconds (original:
        /// hud/GamePanel.GOLD_TIME and SILVER_TIME).
        static constexpr double GOLD_TIME = 5 * 60 + 30;
        static constexpr double SILVER_TIME = 9 * 60 + 30;

        static std::optional<LevelInfo> make(int companyId, int levelNum); ///< Original: _SafeStr_701.

        [[nodiscard]] int companyId() const { return companyId_; } ///< _SafeStr_3482
        [[nodiscard]] int levelNum() const { return levelNum_; }
        /// Id of the level clip: "Level<N>Class" (original: _SafeStr_3480).
        [[nodiscard]] int globalId() const { return levelNum_ + 1 + companyId_ * 10; }
        [[nodiscard]] std::string caption() const;     ///< _SafeStr_3484
        [[nodiscard]] std::string description() const; ///< _SafeStr_3479
        [[nodiscard]] int xpReward() const;            ///< _SafeStr_3483
        [[nodiscard]] double goldTime() const { return GOLD_TIME; }     ///< _SafeStr_3478
        [[nodiscard]] double silverTime() const { return SILVER_TIME; } ///< _SafeStr_3477
        /// Medal earned by a mission result (LevelProgress::RANK_*) (original: _SafeStr_3444).
        [[nodiscard]] int rankFor(const MissionResult& result) const;

        /// The computer players of this mission, with their edicts (original: _SafeStr_3481).
        [[nodiscard]] std::vector<std::shared_ptr<Combatant>> enemyPlayers() const;

    private:
        LevelInfo(int companyId, int levelNum) : companyId_(companyId), levelNum_(levelNum) {}
        int companyId_;
        int levelNum_;
    };

} // namespace takeover
