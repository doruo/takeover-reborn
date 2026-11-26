#include "game/gui/MissionOverScreen.h"

#include "game/data/LevelInfo.h"
#include "game/framework/Symbols.h"
#include "game/framework/core/Format.h"
#include "game/progress/Profile.h"

#include <algorithm>
#include <cmath>

namespace takeover {

    // /----- STRIPE INDICATOR -----/

    std::map<std::string, int>& StripeIndicator::maxValues() {
        static std::map<std::string, int> values;
        return values;
    }

    StripeIndicator::StripeIndicator(std::string key, int value, engine::MovieClip* stripe)
        : stripe_(sharedClip(stripe)), key_(std::move(key)), value_(value) {
        int& max = maxValues()[key_];
        max = std::max(max, value);
        tween_ = std::make_shared<Tween>(Transition(easing::cubicOut, 0, value_, 0.5));
        stripe_->gotoAndStop(1);
    }

    void StripeIndicator::resetMaxValues() {
        for (auto& [key, value] : maxValues())
            value = 0;
    }

    void StripeIndicator::customDispose() {
        if (tween_)
            tween_->dispose();

        tween_.reset();
        stripe_.reset();
        UpdatableObject::customDispose();
    }

    void StripeIndicator::update(double timeElapsed) {
        tween_->update(timeElapsed);
        const int max = maxValues()[key_];
        stripe_->gotoAndStop(max == 0 ? 1 : static_cast<int>(std::round(stripe_->totalFrames() * tween_->value() / max)));
        symbols::setHtmlText(*stripe_, "txtValue", std::to_string(static_cast<int>(std::round(tween_->value()))));
        UpdatableObject::update(timeElapsed);
    }

    // /----- MISSION OVER SCREEN -----/

    MissionOverScreen::MissionOverScreen(std::shared_ptr<MissionOverDef> def)
        : profile_(def->profile), result_(def->missionResult) {
        def->viewClass = "scrMissionOverClass";
        initScreen(std::move(def));
        createStripes();
    }

    void MissionOverScreen::customDispose() {
        stripes_.reset();
        GameScreen::customDispose();
    }

    void MissionOverScreen::update(double timeElapsed) {
        updateStripes(timeElapsed);
        GameScreen::update(timeElapsed);
    }

    std::string MissionOverScreen::formatTime(double seconds) { return format::time(seconds, false, true, true, false); }

    void MissionOverScreen::initVisualBlock() {
        GameScreen::initVisualBlock();

        // Obfuscated children: _-UJ messages block, _-9 victory/defeat banner,
        // _-8C statistics stripes, _-uC XP block, _-dj player panel.
        clip("mcBack")->gotoAndStop(profile_->selectedCompany().info().number() + 1);
        symbols::setHtmlText(*image_, "_-UJ.txtResultMessage",
                             "PLAYTIME <FONT COLOR='#00CBFF'>" + formatTime(result_->missionTime) + "</FONT>");
        symbols::setHtmlText(*image_, "_-UJ.txtTaskMessage", "");

        if (result_->missionOverStatus == MissionResult::VICTORY) {
            clip("_-9")->gotoAndStop("_victory");
            clip("mcStatus")->gotoAndStop(def()->completeStatus);
            const LevelInfo level = *LevelInfo::make(profile_->selectedCompany().info().id(), result_->levelNum);
            symbols::setHtmlText(*image_, "_-UJ.txtTaskMessage", nextMedalText(def()->completeStatus, level));
        } else {
            clip("_-9")->gotoAndStop("_defeat");
            clip("mcStatus")->setVisible(false);
        }

        // Without the CPMStar advertisement, the first stripes layout is used.
        stripes_ = sharedClip(clip("_-8C"));
        clip("mcStatStripes2")->setVisible(false);
        clip("mcCPMPlace")->setVisible(false);
        symbols::setHtmlText(*image_, "_-uC.txtField", "+" + std::to_string(static_cast<long long>(def()->xpEarned)));
        statPanel_ = std::make_unique<PlayerStatPanel>(clip("_-dj"), *profile_);
    }

    void MissionOverScreen::disposeVisualBlock() {
        disposeStripes();
        statPanel_.reset();
        GameScreen::disposeVisualBlock();
    }

    void MissionOverScreen::initButtonsBlock() {
        GameScreen::initButtonsBlock();
        addButton("butOk", "OK", BUT_OK);
    }

    void MissionOverScreen::createStripes() {
        const int enemyCount = static_cast<int>(result_->allStats.size());
        stripes_->gotoAndStop(4 - enemyCount);
        StripeIndicator::resetMaxValues();
        addStripe(0, *result_->playerStats);

        for (int i = 0; i < enemyCount; ++i)
            addStripe(i + 1, *result_->allStats[static_cast<std::size_t>(i)]);

        const double scale = clip("mcStatus")->scaleX();
        statusTween_ = std::make_shared<Tween>(Transition(easing::elasticInOut, 0, scale, 0.5), 0.5);
        applyStatusScale();
    }

    void MissionOverScreen::disposeStripes() {
        if (statusTween_)
            statusTween_->dispose();

        statusTween_.reset();

        for (auto& indicator : indicators_)
            indicator->dispose();

        indicators_.clear();
    }

    void MissionOverScreen::updateStripes(double timeElapsed) {
        for (auto& indicator : indicators_)
            indicator->update(timeElapsed);

        statusTween_->update(timeElapsed);
        applyStatusScale();
    }

    void MissionOverScreen::applyStatusScale() {
        engine::MovieClip* status = clip("mcStatus");
        status->setScaleX(statusTween_->value());
        status->setScaleY(statusTween_->value());
    }

    void MissionOverScreen::addStripe(int id, const MissionStats& stats) {
        engine::MovieClip* stripe = symbols::child(*stripes_, "mcStripe" + std::to_string(id));
        stripe->gotoAndStop(CompanyInfo::raceIconFrame(stats.getRace()));
        symbols::child(*stripe, "mcDefeatedRibon")->setVisible(stats.get(MissionStats::STRONGHOLDS_OWNED) == 0);

        const auto add = [&](const char* key, int counter, const char* clipName) {
            indicators_.push_back(std::make_shared<StripeIndicator>(key, stats.get(counter), symbols::child(*stripe, clipName)));
        };

        add("spellCast", MissionStats::SPELLS_CAST, "mcSpellCast");
        add("resGold", MissionStats::GOLD_COLLECTED, "mcResGold");
        add("resMana", MissionStats::MANA_COLLECTED, "mcResMana");
        add("unitPurchased", MissionStats::SQUADS_PURCHASED, "mcUnitPurchased");
        add("unitLost", MissionStats::SQUADS_LOST, "mcUnitLost");
        add("strongholdsLost", MissionStats::STRONGHOLDS_LOST, "mcKnLost");
        add("strongholdsSeized", MissionStats::STRONGHOLDS_SEIZED, "mcKnSeized");
    }

    std::string MissionOverScreen::nextMedalText(int completeStatus, const LevelInfo& level) {
        const int rank = completeStatus - 1;
        if (rank == LevelProgress::RANK_SILVER)
            return "Complete the mission<BR>in <FONT COLOR='#00CBFF'>" + formatTime(level.goldTime()) +
                   "</FONT><BR>to achieve<BR>the PERFECT rating";
        if (rank == LevelProgress::RANK_BRONZE)
            return "Complete the mission<BR>in <FONT COLOR='#00CBFF'>" + formatTime(level.silverTime()) +
                   "</FONT><BR>to achieve<BR>the GREAT rating";
        return "";
    }

} // namespace takeover
