// Badges.h - the 22 achievements and the 28 metrics they watch (original:
// progress/Badges and progress/_SafeStr_690 for one badge).
//
// Metrics are saved with the profile. Badges are not: loading the metrics
// re-unlocks them.
#pragma once

#include "game/framework/storage/ByteArray.h"
#include "game/progress/Conditions.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace takeover {

    /// Original: progress/_SafeStr_690.
    class Badge {
    public:
        Badge(std::string name, unsigned iconNum, std::shared_ptr<Achievement> condition, std::string description)
            : name_(std::move(name)), iconNum_(iconNum), condition_(std::move(condition)),
              description_(std::move(description)) {}

        [[nodiscard]] const std::string& name() const { return name_; }
        [[nodiscard]] unsigned iconNum() const { return iconNum_; }
        [[nodiscard]] Achievement* condition() const { return condition_.get(); }
        [[nodiscard]] const std::string& description() const { return description_; }

    private:
        std::string name_;
        unsigned iconNum_;
        std::shared_ptr<Achievement> condition_;
        std::string description_;
    };

    class Badges {
    public:
        // Metric ids (the strings are also the save keys).
        static inline const std::string METRIC_WESTARIA_MISSIONS_COMPLETED = "METRIC_WESTARIA_MISSIONS_COMPLETED";
        static inline const std::string METRIC_HORDE_MISSIONS_COMPLETED = "METRIC_HORDE_MISSIONS_COMPLETED";
        static inline const std::string METRIC_CULT_MISSIONS_COMPLETED = "METRIC_CULT_MISSIONS_COMPLETED";
        static inline const std::string METRIC_WESTARIA_MISSIONS_PERFECT = "METRIC_WESTARIA_MISSIONS_PERFECT";
        static inline const std::string METRIC_HORDE_MISSIONS_PERFECT = "METRIC_HORDE_MISSIONS_PERFECT";
        static inline const std::string METRIC_CULT_MISSIONS_PERFECT = "METRIC_CULT_MISSIONS_PERFECT";
        static inline const std::string METRIC_PERFECT_RATINGS = "METRIC_PERFECT_RATINGS";
        static inline const std::string METRIC_WESTARIA_MAIN_EDICTS_ISSUED = "METRIC_WESTARIA_MAIN_EDICTS_ISSUED";
        static inline const std::string METRIC_HORDE_MAIN_EDICTS_ISSUED = "METRIC_HORDE_MAIN_EDICTS_ISSUED";
        static inline const std::string METRIC_CULT_MAIN_EDICTS_ISSUED = "METRIC_CULT_MAIN_EDICTS_ISSUED";
        static inline const std::string METRIC_VILLAGES_OWNED = "METRIC_VILLAGES_OWNED";
        static inline const std::string METRIC_MINES_OWNED = "METRIC_MINES_OWNED";
        static inline const std::string METRIC_OBELISKS_OWNED = "METRIC_OBELISKS_OWNED";
        static inline const std::string METRIC_SQUADS_OWNED = "METRIC_SQUADS_OWNED";
        static inline const std::string METRIC_BANNERS_SET = "METRIC_BANNERS_SET";
        static inline const std::string METRIC_ULITIMATE_USED = "METRIC_ULITIMATE_USED";
        static inline const std::string METRIC_CHESTS_OPENED = "METRIC_CHESTS_OPENED";
        static inline const std::string METRIC_ENEMY_SQUADS_DESTROYED = "METRIC_ENEMY_SQUADS_DESTROYED";
        static inline const std::string METRIC_HEROIC_MODE_ENGAGED = "METRIC_HEROIC_MODE_ENGAGED";
        static inline const std::string METRIC_GOLD_COLLECTED = "METRIC_GOLD_COLLECTED";
        static inline const std::string METRIC_SQUADS_PURCHASED = "METRIC_SQUADS_PURCHASED";
        static inline const std::string METRIC_STRONGHOLDS_CAPTURED = "METRIC_STRONGHOLDS_CAPTURED";
        static inline const std::string METRIC_WESTARIA_RANK = "METRIC_WESTARIA_RANK";
        static inline const std::string METRIC_HORDE_RANK = "METRIC_HORDE_RANK";
        static inline const std::string METRIC_ICEDALES_RANK = "METRIC_ICEDALES_RANK";
        static inline const std::string METRIC_XP_GAINED_TOTAL = "METRIC_XP_GAINED_TOTAL";
        static inline const std::string METRIC_STRONGHOLDS_CAPTURED_TOTAL = "METRIC_STRONGHOLDS_CAPTURED_TOTAL";
        static inline const std::string METRIC_ENEMY_SQUADS_DESTROYED_TOTAL = "METRIC_ENEMY_SQUADS_DESTROYED_TOTAL";

        static inline const std::string BADGE_WESTARIA_CAMPAIGN_COMPLETE = "BADGE_WESTARIA_CAMPAIGN_COMPLETE";
        static inline const std::string BADGE_HORDE_CAMPAIGN_COMPLETE = "BADGE_HORDE_CAMPAIGN_COMPLETE";
        static inline const std::string BADGE_CULT_CAMPAIGN_COMPLETE = "BADGE_CULT_CAMPAIGN_COMPLETE";
        static inline const std::string BADGE_WESTARIA_ALL_PERFECT = "BADGE_WESTARIA_ALL_PERFECT";
        static inline const std::string BADGE_HORDE_ALL_PERFECT = "BADGE_HORDE_ALL_PERFECT";
        static inline const std::string BADGE_CULT_ALL_PERFECT = "BADGE_CULT_ALL_PERFECT";
        static inline const std::string BADGE_WESTARIA_ALL_BASE_EDICTS = "BADGE_WESTARIA_ALL_BASE_EDICTS";
        static inline const std::string BADGE_HORDE_ALL_BASE_EDICTS = "BADGE_HORDE_ALL_BASE_EDICTS";
        static inline const std::string BADGE_CULT_ALL_BASE_EDICTS = "BADGE_CULT_ALL_BASE_EDICTS";
        static inline const std::string BADGE_ANY_MISSION_PERFECT = "BADGE_ANY_MISSION_PERFECT";
        static inline const std::string BADGE_VILLAGES_OWNED = "BADGE_VILLAGES_OWNED";
        static inline const std::string BADGE_MINES_OWNED = "BADGE_MINES_OWNED";
        static inline const std::string BADGE_OBELISKS_OWNED = "BADGE_OBELISKS_OWNED";
        static inline const std::string BADGE_SQUADS_PURCHASED = "BADGE_SQUADS_PURCHASED";
        static inline const std::string BADGE_BANNERS_SET = "BADGE_BANNERS_SET";
        static inline const std::string BADGE_ULTIMATES_USED = "BADGE_ULTIMATES_USED";
        static inline const std::string BADGE_CHESTS_OPENED = "BADGE_CHESTS_OPENED";
        static inline const std::string BADGE_ENEMY_SQUADS_DESTROYED = "BADGE_ENEMY_SQUADS_DESTROYED";
        static inline const std::string BADGE_HEROIC_MODE_ENGAGED = "BADGE_HEROIC_MODE_ENGAGED";
        static inline const std::string BADGE_GOLD_COLLECTED = "BADGE_GOLD_COLLECTED";
        static inline const std::string BADGE_SQUADS_OWNED = "BADGE_SQUADS_OWNED";
        static inline const std::string BADGE_STRONGHOLDS_SIEZED = "BADGE_STRONGHOLDS_SIEZED";

        static const std::vector<std::string>& metricIds();
        static const std::vector<std::string>& badgeIds(); ///< BADGES_IDS, the order of the badges room.

        Badges() { reset(); }

        /// (badge, badge id) when a badge gets unlocked (original: _SafeStr_708).
        Signal<Badge*, const std::string&> signalBadgeUnlocked;

        [[nodiscard]] Metric& metric(const std::string& id) const { return *metrics_.at(id); } ///< _SafeStr_3543
        [[nodiscard]] Badge* badge(const std::string& id) const;                               ///< _SafeStr_3505

        void reset();
        void serialize(ByteArray& data) const;
        void deserialize(ByteArray& data); ///< Original: _SafeStr_3554.

    private:
        void createMetrics(); ///< _SafeStr_3570
        void createBadges();  ///< _SafeStr_3569
        void addBadge(const std::string& id, const std::string& metric, double threshold, const std::string& name,
                      unsigned icon, const std::string& description);
        void onAchievementUnlocked(Achievement* condition); ///< _SafeStr_696

        std::map<std::string, std::shared_ptr<Metric>> metrics_;
        std::map<std::string, std::unique_ptr<Badge>> badges_;
    };

} // namespace takeover
