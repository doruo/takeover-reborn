// RasterAnimation.h - an animation made of pre-rendered frames (original:
// _SafePkg_1310/RasterAnimation, UnitAnimation and the interface _SafeStr_1258).
//
// A RasterAnimation is a tween over frame numbers: its Transition says which
// frames to play (x0 .. x0+dx) and in how long. Each update moves to the
// current frame, and drawTo copies that frame into the battlefield.
#pragma once

#include "game/framework/raster/AnimationMaker.h"
#include "game/framework/raster/RasterObject.h"
#include "game/framework/tween/Tween.h"

#include <memory>
#include <string>
#include <vector>

namespace takeover {

class ResourceManager;

class RasterAnimation : public TargetTween, public ITweenTarget, public IRasterDrawable {
public:
    RasterAnimation(std::shared_ptr<CachedData> frames, const Transition& transition, double x = 0, double y = 0,
                    double delayBeforeStart = 0, int repeatCounter = -1);

    [[nodiscard]] int currentFrame() const { return frame_; }
    /// Switches to another frame range (original: _SafeStr_1813).
    void setTransition(const Transition& transition, int repeatCounter = -1, double startTime = 0);
    [[nodiscard]] RasterObject& rasterObject() { return *raster_; }
    [[nodiscard]] Transition& transition() { return transition_; } ///< _SafeStr_1679, writable

    double getX() const { return raster_->x; }
    double getY() const { return raster_->y; }
    void setX(double x) { raster_->x = x; }
    void setY(double y) { raster_->y = y; }

    void drawTo(engine::BitmapData& place, double x = 0, double y = 0, double yScale = 1,
                const engine::ColorTransform* color = nullptr) override;

    // ITweenTarget
    void setTweenValue(double frame) override { frame_ = static_cast<int>(frame); } ///< _SafeStr_1804
    [[nodiscard]] int tweenStart() const override { return startFrame_; }           ///< _SafeStr_1805
    [[nodiscard]] int tweenEnd() const override { return endFrame_; }               ///< _SafeStr_1806

protected:
    void customDispose() override;

    int startFrame_ = 0;                 ///< Original: _SafeStr_1810.
    int endFrame_ = 0;                   ///< Original: _SafeStr_1811.
    int frame_ = 0;                      ///< Original: _SafeStr_1809.
    std::shared_ptr<CachedData> frames_; ///< Original: _SafeStr_1808.
    std::shared_ptr<RasterObject> raster_; ///< Original: _SafeStr_1807.
};

/// Builds animation names from a base name and a number (original:
/// interface _SafeStr_1258).
class INameModifier {
public:
    virtual ~INameModifier() = default;
    [[nodiscard]] virtual int count() const = 0;                                    ///< _SafeStr_2113
    [[nodiscard]] virtual std::string name(const std::string& base, int index) const = 0; ///< _SafeStr_2117
};

/// The animations of one unit facing one direction: walk, die, attacks...
/// (original: _SafePkg_1310/UnitAnimation).
class UnitAnimation : public Disposable {
public:
    UnitAnimation(ResourceManager& resources, const std::string& animId, const INameModifier& names);

    [[nodiscard]] RasterAnimation& current() { return *animations_[static_cast<std::size_t>(current_)]; } ///< _SafeStr_2116
    [[nodiscard]] int count() const { return static_cast<int>(animations_.size()); } ///< _SafeStr_2114
    [[nodiscard]] int currentIndex() const { return current_; }   ///< _SafeStr_2109
    void setCurrentIndex(int index) { current_ = index; }
    [[nodiscard]] RasterAnimation& animation(int index) { return *animations_[static_cast<std::size_t>(index)]; } ///< _SafeStr_2115
    [[nodiscard]] double speedFactor() const { return speed_; }  ///< _SafeStr_2111
    /// Plays every animation faster or slower (original: _SafeStr_2118).
    void setSpeedFactor(double factor);

protected:
    void customDispose() override;

private:
    std::vector<std::shared_ptr<RasterAnimation>> animations_; ///< Original: _SafeStr_2106.
    std::vector<Transition> baseTransitions_;                  ///< Original: _SafeStr_2108.
    int current_ = 0;                                          ///< Original: _SafeStr_1674.
    double speed_ = 1;                                         ///< Original: _SafeStr_2107.
};

} // namespace takeover
