// FUnit.h - a soldier on the battlefield (original: _SafePkg_805/_SafeStr_1246,
// with its definition _SafeStr_1253 and Unit3DAnimation of _SafePkg_1310).
//
// The simulation's Unit decides what the soldier does. FUnit shows it: it
// picks the animation (walk, attack, die) facing one of four directions,
// tints poisoned or frozen soldiers, and draws missiles in flight (arrows
// as lines, bullets and catapult stones as pictures).
#pragma once

#include "game/framework/core/Signal.h"
#include "game/framework/raster/RasterAnimation.h"
#include "game/framework/raster/RasterLayer.h"
#include "game/sim/base/Flags.h"
#include "game/sim/units/Unit.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace takeover {

    class ResourceManager;

    /// The animations of a unit in its 4 directions (original: Unit3DAnimation).
    class Unit3DAnimation : public UpdatableObject, public IRasterDrawable {
    public:
        Unit3DAnimation(ResourceManager& resources, const std::string& unitId, const INameModifier& directions,
                        const INameModifier& phases);

        [[nodiscard]] RasterAnimation& current() { return *current_; } ///< _SafeStr_2743
        [[nodiscard]] int direction() const { return direction_; }     ///< _SafeStr_2754
        [[nodiscard]] int phase() const { return phase_; }             ///< _SafeStr_2757
        [[nodiscard]] int phaseCount() const { return phaseCount_; }   ///< _SafeStr_2784
        void setSpeedFactor(double factor);                           ///< _SafeStr_2118

        /// Plays `phase` facing `direction` (original: _SafeStr_2948).
        void show(int phase, int direction, int repeatCount = -1);

        /// Turns, keeping the phase and its progress (original: _SafeStr_2777).
        void turn(int direction);
        void setPhase(int phase) { show(phase, direction_); } ///< _SafeStr_2748
        [[nodiscard]] bool isRunning() const { return current_->running(); } ///< _SafeStr_1794
        void setRunning(bool running) { current_->setRunning(running); }

        double getX() const { return current_->getX(); }
        double getY() const { return current_->getY(); }
        void setX(double x) { current_->setX(x); }
        void setY(double y) { current_->setY(y); }

        void update(double timeElapsed) override;
        void drawTo(engine::BitmapData& place, double x = 0, double y = 0, double yScale = 1,
                    const engine::ColorTransform* color = nullptr) override {
            current_->drawTo(place, x, y, yScale, color);
        }

    protected:
        void customDispose() override { current_ = nullptr; }

    private:
        void play(RasterAnimation& animation, int repeatCount = -1, double startTime = 0); ///< _SafeStr_2946

        std::vector<std::shared_ptr<UnitAnimation>> directions_; ///< Original: _SafeStr_2943.
        RasterAnimation* current_ = nullptr; ///< Original: _SafeStr_2942.
        int direction_ = 0;                  ///< Original: _SafeStr_2944.
        int phase_ = 0;                      ///< Original: _SafeStr_2945.
        int phaseCount_ = 0;                 ///< Original: _SafeStr_2947.
        double speed_ = 1;                   ///< Original: _SafeStr_2107.
    };

    /// What an FUnit is made from (original: _SafeStr_1253).
    struct FUnitDef : UnitDef {
        ResourceManager* resources = nullptr;
        std::string unitID = "Swordsman";
        std::shared_ptr<INameModifier> phaseNames; ///< Original: _SafeStr_1301.
    };

    class FUnit : public Unit, public ILayerObject {
    public:
        static constexpr int ANIM_FALL_DOWN = 0;
        static constexpr int ANIM_FALL_DOWN_FINISH = 1;
        static constexpr int ANIM_ATTACK = 2;
        static const engine::ColorTransform COLOR_POISON;
        static const engine::ColorTransform COLOR_FREEZ;

        // Directions.
        static constexpr int DIR_DOWN = 0;  ///< _SafeStr_2769
        static constexpr int DIR_LEFT = 1;  ///< _SafeStr_2762
        static constexpr int DIR_UP = 2;    ///< _SafeStr_2767
        static constexpr int DIR_RIGHT = 3; ///< _SafeStr_2768

        FUnit() = default;
        FUnit& init(const FUnitDef& def); ///< _SafeStr_2763

        Signal<FUnit*> signalDispose;   ///< Original: _SafeStr_2755.
        Signal<FUnit*, int> signalAnim; ///< ANIM_ events (original: _SafeStr_1306).

        /// Hidden by a spell effect (original: _SafeStr_1291).
        bool hidden = false;

        void update(double timeElapsed) override;
        [[nodiscard]] RenderableObjectsContainer& attachments() { return attached_; } ///< _SafeStr_2779
        void stopAnimation() { animation_->setRunning(false); } ///< _SafeStr_2783

        // /----- UNIT HOOKS -----/

        void changeAnimTo_Go() override;
        void changeAnimTo_Special() override;
        void updateGoAnimSpeed() override;
        double changeAnimTo_Attack(double attackSpeed) override;
        double changeAnimTo_DistanceAttack(double attackSpeed) override;
        void changeAnimTo_Stand() override;
        void changeAnimTo_FallDown() override;
        void changeAnimTo_CorpseToGrave() override { visible_ = false; }
        void updateAnimDir(double dx, double dy) override;
        void missileAtTarget(ITarget* target) override;
        void activateAttack() override;
        void activateDistanceAttack() override;

        // /----- I LAYER OBJECT -----/

        void drawTo(engine::BitmapData& place, double x = 0, double y = 0, double yScale = 1,
                    const engine::ColorTransform* color = nullptr) override;
        [[nodiscard]] double getY() const override { return y_; }
        void setY(double y) override { y_ = y; }
        void attach(std::shared_ptr<IRasterDrawable> drawable, bool above = true) override { attached_.attach(std::move(drawable), above); }
        void detach(const IRasterDrawable* drawable) override { attached_.detach(drawable); }
        [[nodiscard]] bool isVisible() const override { return visible_; }
        void setVisible(bool visible) override;
        [[nodiscard]] bool isHighlighted() const override { return highlighted_; }
        void setHighlighted(bool highlighted) override { highlighted_ = highlighted; }
        [[nodiscard]] bool hitTest(double, double, double = 1) const override { return false; }

    protected:
        void customDispose() override;

    private:
        void initHealthBar(); ///< _SafeStr_2771
        double playAttack(int phase, double attackSpeed); ///< _SafeStr_2756
        [[nodiscard]] bool isLyingDead() const; ///< _SafeStr_2765
        void onFallDownFinished(Tween* tween);  ///< _SafeStr_2761
        [[nodiscard]] int directionOf(double dx, double dy) const; ///< _SafeStr_2775

        /// Bresenham line (original: _SafeStr_2753).
        static void drawLine(engine::BitmapData& place, int x1, int y1, int x2, int y2, std::uint32_t color = 0xFF000000);

        /// Height of an arc missile (original: _SafeStr_2751).
        [[nodiscard]] double arcHeight(double time) const;
        void onEffectAdded(int, BaseObject*, const std::any& data);   ///< _SafeStr_2776
        void onEffectRemoved(int, BaseObject*, const std::any& data); ///< _SafeStr_2759
        [[nodiscard]] const engine::ColorTransform* tintFor(const engine::ColorTransform* color) const; ///< _SafeStr_2764
        void drawCatapultStone(engine::BitmapData& place, double x, double y, double yScale, const engine::ColorTransform* color); ///< _SafeStr_2773
        void drawBullet(engine::BitmapData& place, double x, double y, const std::string& className, double yScale,
                        const engine::ColorTransform* color); ///< _SafeStr_2747
        void drawArrow(engine::BitmapData& place, double x, double y, double yScale, const engine::ColorTransform* color); ///< _SafeStr_2758

        std::shared_ptr<Unit3DAnimation> animation_; ///< Original: _SafeStr_1549.
        bool disposedOnce_ = false;                  ///< Original: _SafeStr_2752.
        FUnitDef def_;                               ///< Original: _SafeStr_839.
        double healthBarWidth_ = 10;                 ///< Original: _SafeStr_2746.
        double healthBarHeight_ = 1;                 ///< Original: _SafeStr_2749.
        double healthBarOffset_ = -20;               ///< Original: _SafeStr_2750.
        std::optional<Point2D> prevArrowPos1_, prevArrowPos2_;
        RenderableObjectsContainer attached_;        ///< Original: _SafeStr_2152.
        bool visible_ = true;                        ///< Original: _SafeStr_2154.
        bool highlighted_ = false;                   ///< Original: _SafeStr_2155.
        BitSet magicMarks_;                          ///< Poisoned, frozen (original: _SafeStr_2744).
        std::shared_ptr<RasterObject> missilePicture_; ///< Original: _SafeStr_2745.
    };

} // namespace takeover
