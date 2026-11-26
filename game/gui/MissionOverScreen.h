// MissionOverScreen.h - victory or defeat, the medal and the statistics of
// every side (original: gui/MissionOverScreen and _SafePkg_3473/StripeIndicator).
#pragma once

#include "game/framework/tween/Tween.h"
#include "game/gui/GameScreen.h"
#include "game/gui/PlayerStatPanel.h"

#include <map>

namespace takeover {

    class LevelInfo;

    /// A bar that fills up to a statistic (original: StripeIndicator). Bars
    /// with the same key share their maximum, so the sides compare visually.
    class StripeIndicator : public UpdatableObject {
    public:
        StripeIndicator(std::string key, int value, engine::MovieClip* stripe);
        static void resetMaxValues(); ///< _SafeStr_3567
        void update(double timeElapsed) override;

    protected:
        void customDispose() override;

    private:
        static std::map<std::string, int>& maxValues();
        std::shared_ptr<engine::MovieClip> stripe_;
        std::string key_;
        int value_;
        std::shared_ptr<Tween> tween_;
    };

    class MissionOverScreen : public GameScreen {
    public:
        static constexpr int BUT_OK = 0;

        explicit MissionOverScreen(std::shared_ptr<MissionOverDef> def);
        void update(double timeElapsed) override;

    protected:
        void customDispose() override;
        void initVisualBlock() override;
        void disposeVisualBlock() override;
        void initButtonsBlock() override;

    private:
        MissionOverDef* def() const { return static_cast<MissionOverDef*>(def_.get()); }
        void createStripes();                                  ///< _SafeStr_3566
        void disposeStripes();                                 ///< _SafeStr_3561
        void updateStripes(double timeElapsed);                ///< _SafeStr_3564
        void applyStatusScale();                               ///< _SafeStr_3559
        void addStripe(int id, const MissionStats& stats);     ///< _SafeStr_3560
        static std::string formatTime(double seconds);         ///< _SafeStr_3558
        static std::string nextMedalText(int completeStatus, const LevelInfo& level); ///< _SafeStr_3563

        Profile* profile_ = nullptr;
        std::shared_ptr<MissionResult> result_;
        std::unique_ptr<PlayerStatPanel> statPanel_;
        std::shared_ptr<engine::MovieClip> stripes_; ///< Original: _SafeStr_3557.
        std::vector<std::shared_ptr<StripeIndicator>> indicators_;
        std::shared_ptr<Tween> statusTween_;
    };

} // namespace takeover
