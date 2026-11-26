#include "game/progress/Profile.h"

#include "game/data/LevelInfo.h"
#include "game/framework/core/As3.h"
#include "game/framework/storage/SerializableObject.h"

namespace takeover {

    Profile::Profile() : badges_(std::make_unique<Badges>()) {
        companies_.reserve(CompanyInfo::COMPANY_COUNT);

        for (int id = 0; id < CompanyInfo::COMPANY_COUNT; ++id)
            companies_.emplace_back(id);

        selected_ = &companies_[0];
        reset();
    }

    void Profile::selectCompany(int id) {
        selected_ = &companies_[static_cast<std::size_t>(CompanyInfo::make(id)->number())];
    }

    void Profile::checkLevelBadges() {
        int perfect = 0;

        for (const CompanyProgress& company : companies_)
            perfect += company.checkLevelBadges(*badges_);

        badges_->metric(Badges::METRIC_PERFECT_RATINGS).setValue(perfect);
    }

    double Profile::addMissionResult(const LevelProgress& result, const LevelInfo& level) {
        const int levelNum = level.levelNum();
        double xp = result.completeStatus != LevelProgress::RANK_NEW ? missionXp(level) : as3::round(missionXp(level) / 5.0);
        selectedCompany().setXp(selectedCompany().xp() + static_cast<std::uint32_t>(xp));

        // The original records every campaign's rank in the Westaria metric.
        badges_->metric(Badges::METRIC_WESTARIA_RANK).setValue(selectedCompany().rank());

        if (selectedCompany().levels()[static_cast<std::size_t>(levelNum)].compareStatus(result))
            selectedCompany().levels()[static_cast<std::size_t>(levelNum)] = result;

        Metric& total = badges_->metric(Badges::METRIC_XP_GAINED_TOTAL);
        total.setValue(total.value() + xp);

        return xp;
    }

    int Profile::missionXp(const LevelInfo& level) const { return level.xpReward(); }

    void Profile::reset() {
        enabled_ = false;

        for (CompanyProgress& company : companies_)
            company.reset();

        badges_->reset();
    }

    void Profile::serialize(ByteArray& data) const {
        SerializableObject object;
        object.add("on", SerializableObject::BOOLEAN, enabled_);
        object.save(data);
        SerializableObject().save(data); // empty per-profile statistics (original: _SafeStr_3546)

        for (const CompanyProgress& company : companies_)
            company.serialize(data);

        badges_->serialize(data);
    }

    void Profile::deserialize(ByteArray& data) {
        SerializableObject object;
        object.load(data);

        if (object.has("on"))
            enabled_ = object.getBool("on");

        SerializableObject().load(data);

        for (CompanyProgress& company : companies_)
            company.deserialize(data);

        badges_->deserialize(data);
    }

} // namespace takeover
