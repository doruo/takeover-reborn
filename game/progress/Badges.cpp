#include "game/progress/Badges.h"

#include "game/framework/storage/SerializableObject.h"

namespace takeover {

    const std::vector<std::string>& Badges::metricIds() {
        static const std::vector<std::string> ids = {
            METRIC_WESTARIA_MISSIONS_COMPLETED, METRIC_HORDE_MISSIONS_COMPLETED,  METRIC_CULT_MISSIONS_COMPLETED,
            METRIC_WESTARIA_MISSIONS_PERFECT,   METRIC_HORDE_MISSIONS_PERFECT,    METRIC_CULT_MISSIONS_PERFECT,
            METRIC_PERFECT_RATINGS,             METRIC_WESTARIA_MAIN_EDICTS_ISSUED, METRIC_HORDE_MAIN_EDICTS_ISSUED,
            METRIC_CULT_MAIN_EDICTS_ISSUED,     METRIC_VILLAGES_OWNED,            METRIC_MINES_OWNED,
            METRIC_OBELISKS_OWNED,              METRIC_SQUADS_OWNED,              METRIC_BANNERS_SET,
            METRIC_ULITIMATE_USED,              METRIC_CHESTS_OPENED,             METRIC_ENEMY_SQUADS_DESTROYED,
            METRIC_HEROIC_MODE_ENGAGED,         METRIC_GOLD_COLLECTED,            METRIC_SQUADS_PURCHASED,
            METRIC_STRONGHOLDS_CAPTURED,        METRIC_WESTARIA_RANK,             METRIC_HORDE_RANK,
            METRIC_ICEDALES_RANK,               METRIC_XP_GAINED_TOTAL,           METRIC_STRONGHOLDS_CAPTURED_TOTAL,
            METRIC_ENEMY_SQUADS_DESTROYED_TOTAL,
        };

        return ids;
    }

    const std::vector<std::string>& Badges::badgeIds() {
        static const std::vector<std::string> ids = {
            BADGE_WESTARIA_CAMPAIGN_COMPLETE, BADGE_HORDE_CAMPAIGN_COMPLETE, BADGE_CULT_CAMPAIGN_COMPLETE,
            BADGE_WESTARIA_ALL_PERFECT,       BADGE_HORDE_ALL_PERFECT,       BADGE_CULT_ALL_PERFECT,
            BADGE_WESTARIA_ALL_BASE_EDICTS,   BADGE_HORDE_ALL_BASE_EDICTS,   BADGE_CULT_ALL_BASE_EDICTS,
            BADGE_ANY_MISSION_PERFECT,        BADGE_VILLAGES_OWNED,          BADGE_MINES_OWNED,
            BADGE_OBELISKS_OWNED,             BADGE_SQUADS_PURCHASED,        BADGE_BANNERS_SET,
            BADGE_ULTIMATES_USED,             BADGE_CHESTS_OPENED,           BADGE_ENEMY_SQUADS_DESTROYED,
            BADGE_HEROIC_MODE_ENGAGED,        BADGE_GOLD_COLLECTED,          BADGE_SQUADS_OWNED,
            BADGE_STRONGHOLDS_SIEZED,
        };

        return ids;
    }

    Badge* Badges::badge(const std::string& id) const {
        const auto it = badges_.find(id);
        return it == badges_.end() ? nullptr : it->second.get();
    }

    void Badges::createMetrics() {
        metrics_.clear();

        // Every metric keeps its maximum.
        for (const std::string& id : metricIds())
            metrics_[id] = std::make_shared<Metric>(id, MetricMode::Max);
    }

    void Badges::addBadge(const std::string& id, const std::string& metricId, double threshold, const std::string& name,
                          unsigned icon, const std::string& description) {
        auto condition = std::make_shared<MetricCondition>(metrics_.at(metricId), Comparison::GreaterOrEqual, threshold);
        auto achievement = std::make_shared<Achievement>(condition);
        achievement->signalUnlocked.addListener(this, &Badges::onAchievementUnlocked);
        badges_[id] = std::make_unique<Badge>(name, icon, achievement, description);
    }

    void Badges::createBadges() {
        badges_.clear();
        addBadge(BADGE_WESTARIA_CAMPAIGN_COMPLETE, METRIC_WESTARIA_MISSIONS_COMPLETED, 8, "Valor of the West", 1,
                 "Complete the Duchy of Westaria campaign");
        addBadge(BADGE_HORDE_CAMPAIGN_COMPLETE, METRIC_HORDE_MISSIONS_COMPLETED, 8, "Rampage of the South", 2,
                 "Complete the Crimson Horde campaign");
        addBadge(BADGE_CULT_CAMPAIGN_COMPLETE, METRIC_CULT_MISSIONS_COMPLETED, 8, "Freedom of the North", 3,
                 "Complete the Icedales Cult campaign");
        addBadge(BADGE_WESTARIA_ALL_PERFECT, METRIC_WESTARIA_MISSIONS_PERFECT, 8, "The Holy King", 4,
                 "Complete all the Duchy of Westaria missions with Perfect rating");
        addBadge(BADGE_HORDE_ALL_PERFECT, METRIC_HORDE_MISSIONS_PERFECT, 8, "The Great Khan", 5,
                 "Complete all the Crimson Horde missions with Perfect rating");
        addBadge(BADGE_CULT_ALL_PERFECT, METRIC_CULT_MISSIONS_PERFECT, 8, "The Prophet", 6,
                 "Complete all the Icedales Cult missions with Perfect rating");
        addBadge(BADGE_WESTARIA_ALL_BASE_EDICTS, METRIC_WESTARIA_MAIN_EDICTS_ISSUED, 9, "Justice of the Lord", 7,
                 "Issue all the basic edicts of the Duchy of Westaria");
        addBadge(BADGE_HORDE_ALL_BASE_EDICTS, METRIC_HORDE_MAIN_EDICTS_ISSUED, 9, "The Law of the Steppe", 8,
                 "Issue all the basic edicts of the Crimson Horde");
        addBadge(BADGE_CULT_ALL_BASE_EDICTS, METRIC_CULT_MAIN_EDICTS_ISSUED, 9, "Patronage of the Cult", 9,
                 "Issue all the basic edicts of the Icedales Cult");
        addBadge(BADGE_ANY_MISSION_PERFECT, METRIC_PERFECT_RATINGS, 1, "Veni, Vidi, Vici", 10,
                 "Complete a mission with the Perfect rating");
        addBadge(BADGE_VILLAGES_OWNED, METRIC_VILLAGES_OWNED, 7, "Peaceful Place", 11, "Control 7 farms at the same time");
        addBadge(BADGE_MINES_OWNED, METRIC_MINES_OWNED, 7, "Land of Artisians", 12, "Control 7 mines at the same time");
        addBadge(BADGE_OBELISKS_OWNED, METRIC_OBELISKS_OWNED, 7, "Enchanted Kingdom", 13,
                 "Control 7 obelisks at the same time");
        addBadge(BADGE_SQUADS_PURCHASED, METRIC_SQUADS_PURCHASED, 10, "General Mobilization", 14,
                 "Purchase 10 squads in a mission");
        addBadge(BADGE_BANNERS_SET, METRIC_BANNERS_SET, 5, "Banner Time", 15, "Set 5 banners in a battlefield in a mission");
        addBadge(BADGE_ULTIMATES_USED, METRIC_ULITIMATE_USED, 5, "Test of Faith", 16,
                 "Use an ultimate spell 5 times in a mission");
        addBadge(BADGE_CHESTS_OPENED, METRIC_CHESTS_OPENED, 1, "My Precious!", 17, "Open a treasure chest");
        addBadge(BADGE_ENEMY_SQUADS_DESTROYED, METRIC_ENEMY_SQUADS_DESTROYED, 30, "Onslaught", 18,
                 "Destroy 30 enemy squads in a mission");
        addBadge(BADGE_HEROIC_MODE_ENGAGED, METRIC_HEROIC_MODE_ENGAGED, 20, "Epic Battle", 19,
                 "You squads must enter in the heroic state 20 times during a mission");
        addBadge(BADGE_GOLD_COLLECTED, METRIC_GOLD_COLLECTED, 3000, "Put It Into Treasury", 20,
                 "Collect 3000 gold in a mission");
        // The original watches the squads *purchased* metric for this one too.
        addBadge(BADGE_SQUADS_OWNED, METRIC_SQUADS_PURCHASED, 20, "Great Army", 21, "Control 20 squads at the same time");
        addBadge(BADGE_STRONGHOLDS_SIEZED, METRIC_STRONGHOLDS_CAPTURED, 10, "From Hand to Hand", 22,
                 "Seize strongholds 10 times in a mission");
    }

    void Badges::onAchievementUnlocked(Achievement* condition) {
        for (auto& [id, badge] : badges_) {
            if (badge->condition() == condition) {
                signalBadgeUnlocked.dispatch(badge.get(), id);
                break;
            }
        }
    }

    void Badges::reset() {
        createMetrics();
        createBadges();

        // The original also forwards every metric change to the online
        // statistics service, which the port doesn't have.
    }

    void Badges::serialize(ByteArray& data) const {
        SerializableObject object;

        for (const std::string& id : metricIds())
            object.add(id, SerializableObject::DOUBLE, metric(id).value());

        object.save(data);
    }

    void Badges::deserialize(ByteArray& data) {
        SerializableObject object;
        object.load(data);

        for (const std::string& id : metricIds())
            if (object.has(id))
                metric(id).setValue(object.getNumber(id));
    }

} // namespace takeover
