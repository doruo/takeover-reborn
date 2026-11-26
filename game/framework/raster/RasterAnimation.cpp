#include "game/framework/raster/RasterAnimation.h"

#include "game/framework/raster/ResourceManager.h"

namespace takeover {

    RasterAnimation::RasterAnimation(std::shared_ptr<CachedData> frames, const Transition& transition, double x, double y,
                                     double delayBeforeStart, int repeatCounter)
        : frames_(std::move(frames)) {
        TargetTween::init(this, 0, repeatCounter, delayBeforeStart, false);
        setTransition(transition);
        raster_ = std::make_shared<RasterObject>(frames_->bitmapData[0], x, y, -frames_->offsetX, -frames_->offsetY);
    }

    void RasterAnimation::setTransition(const Transition& transition, int repeatCounter, double startTime) {
        transition_ = transition;
        startFrame_ = static_cast<int>(transition_.x0);
        endFrame_ = static_cast<int>(transition_.x0 + transition_.dx);
        frame_ = 0;
        restart(repeatCounter, startTime);
    }

    void RasterAnimation::drawTo(engine::BitmapData& place, double x, double y, double yScale,
                                 const engine::ColorTransform* color) {
        // The original would throw on a frame out of range; skip it instead.
        if (frame_ < 0 || frame_ >= static_cast<int>(frames_->bitmapData.size()))
            return;

        raster_->setBitmapData(frames_->bitmapData[static_cast<std::size_t>(frame_)]);
        raster_->drawTo(place, x, y, yScale, color);
    }

    void RasterAnimation::customDispose() {
        raster_.reset();
        TargetTween::customDispose();
    }

    UnitAnimation::UnitAnimation(ResourceManager& resources, const std::string& animId, const INameModifier& names) {
        for (int i = 0; i < names.count(); ++i) {
            const std::string key = names.name(animId, i);
            const Transition transition = resources.animTransitions(key)[0];
            animations_.push_back(std::make_shared<RasterAnimation>(resources.getCachedData(key), transition));
            baseTransitions_.push_back(transition);
        }
    }

    void UnitAnimation::setSpeedFactor(double factor) {
        if (speed_ == factor)
            return;

        speed_ = factor;

        for (std::size_t i = 0; i < animations_.size(); ++i)
            animations_[i]->transition().mscDuration = baseTransitions_[i].mscDuration / speed_;
    }

    void UnitAnimation::customDispose() {
        animations_.clear();
        baseTransitions_.clear();
    }

} // namespace takeover
