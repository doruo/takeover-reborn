// Profile.h - one save slot: the three campaigns and the badges
// (original: progress/Profile).
#pragma once

#include "game/framework/storage/ByteArray.h"
#include "game/progress/Badges.h"
#include "game/progress/CompanyProgress.h"

#include <memory>
#include <vector>

namespace takeover {

    class LevelInfo;

    class Profile {
    public:
        Profile();

        [[nodiscard]] bool enabled() const { return enabled_; }
        void setEnabled(bool enabled) { enabled_ = enabled; }
        [[nodiscard]] Badges& badges() { return *badges_; }

        [[nodiscard]] CompanyProgress& company(int id) { return companies_[static_cast<std::size_t>(id)]; } ///< _SafeStr_3487
        [[nodiscard]] CompanyProgress& selectedCompany() { return *selected_; }
        void selectCompany(int id); ///< _SafeStr_763

        /// Updates the per-campaign badges' metrics.
        void checkLevelBadges();

        /// Records a mission result; returns the XP earned (original: _SafeStr_764).
        double addMissionResult(const LevelProgress& result, const LevelInfo& level);
        [[nodiscard]] int missionXp(const LevelInfo& level) const; ///< _SafeStr_3506

        void reset();
        void serialize(ByteArray& data) const;
        void deserialize(ByteArray& data);

    private:
        bool enabled_ = false;
        std::unique_ptr<Badges> badges_;
        std::vector<CompanyProgress> companies_;
        CompanyProgress* selected_ = nullptr;
    };

} // namespace takeover
