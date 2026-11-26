// ResourceManager.h - the battle's picture library (original:
// _SafePkg_1408/ResourceManager with the image classes of _SafePkg_1408 and
// _SafePkg_1583).
//
// It hands out the pre-rendered frames of library clips (getCachedData,
// through the AnimationMaker), their animation ranges (animTransitions: one
// Transition per labelled part of the clip), still pictures for the
// battlefield (createRasterObject) and small animated display objects for
// the HUD (createAnimation).
#pragma once

#include "game/framework/raster/AnimationMaker.h"
#include "game/framework/raster/RasterObject.h"
#include "game/framework/tween/Tween.h"

#include "engine/display/DisplayObjectContainer.h"
#include "engine/display/Shape.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace takeover {

    class SpriteAnimation;

    /// A clip to load: its library class, the name it is known by (the class
    /// name if empty) and its scale (original: the <movie_clip> XML items of XMLData).
    struct ClipDef {
        std::string libName;
        std::string id;
        double scale = 1;
        [[nodiscard]] const std::string& key() const { return id.empty() ? libName : id; }
    };

    /// A display object wrapped for the animation classes (original:
    /// _SafeStr_1620, only the parts the game uses are ported).
    class ImageView : public Disposable {
    public:
        /// Wraps `sprite`. The base radius defaults to its larger side.
        explicit ImageView(std::shared_ptr<engine::Sprite> sprite = std::make_shared<engine::Sprite>(), double baseRadius = 0);

        [[nodiscard]] const std::shared_ptr<engine::Sprite>& sprite() const { return sprite_; }
        [[nodiscard]] ImageView* parent() const { return parent_; }
        [[nodiscard]] double getX() const { return sprite_->x(); }
        [[nodiscard]] double getY() const { return sprite_->y(); }
        void setX(double x) { sprite_->setX(x); }
        void setY(double y) { sprite_->setY(y); }
        [[nodiscard]] bool isVisible() const { return sprite_->visible(); }
        void setVisible(bool visible) { sprite_->setVisible(visible); }
        [[nodiscard]] double baseRadius() const { return baseRadius_; } ///< _SafeStr_1665
        [[nodiscard]] double radius() const { return radius_; }         ///< _SafeStr_1666
        /// Scales the sprite so that its base radius becomes `radius`.
        void setRadius(double radius);

        void addImage(ImageView& image);
        void addImage(SpriteAnimation& animation);
        void removeImage(ImageView& image);
        void removeImage(SpriteAnimation& animation);

    protected:
        void customDispose() override;

        std::shared_ptr<engine::Sprite> sprite_; ///< Original: _SafeStr_1771.
        ImageView* parent_ = nullptr;            ///< Original: _SafeStr_1772.
        double baseRadius_ = 0;                  ///< Original: _SafeStr_1774.
        double radius_ = 0;                      ///< Original: _SafeStr_1775.
    };

    /// An empty sprite to put images in (original: _SafePkg_1408/View).
    class View : public ImageView {};

    /// Shows the cached frames of a clip in a Bitmap (original: CachedMCAnimationTarget).
    class CachedClipTarget : public ImageView, public ITweenTarget {
    public:
        CachedClipTarget(std::vector<std::shared_ptr<engine::BitmapData>> frames, double dx, double dy, double baseRadius = 0);

        [[nodiscard]] engine::Bitmap& bitmap() { return *bitmap_; }
        void setTweenValue(double frame) override; ///< _SafeStr_1804: 1-based frame
        [[nodiscard]] int tweenStart() const override { return 1; }
        [[nodiscard]] int tweenEnd() const override { return static_cast<int>(frames_.size()); }

    private:
        std::shared_ptr<engine::Bitmap> bitmap_;                  ///< Original: _SafeStr_2098.
        std::vector<std::shared_ptr<engine::BitmapData>> frames_; ///< Original: _SafeStr_1808.
        int frame_ = 1;                                          ///< Original: _SafeStr_2099.
        int shownFrame_ = -1;                                    ///< Original: _SafeStr_2100.
    };

    /// Plays a library clip directly (original: _SafeStr_1628).
    class MovieClipTarget : public ImageView, public ITweenTarget {
    public:
        MovieClipTarget(std::shared_ptr<engine::MovieClip> movie, double baseRadius = 0);
        void setTweenValue(double frame) override;
        [[nodiscard]] int tweenStart() const override { return 1; }
        [[nodiscard]] int tweenEnd() const override;

    private:
        std::shared_ptr<engine::MovieClip> movie_; ///< Original: _SafeStr_3434.
    };

    /// An animated display object: a frame tween on an ImageView (original:
    /// _SafePkg_1583/_SafeStr_1619).
    class SpriteAnimation : public TargetTween {
    public:
        template <typename Target>
        SpriteAnimation(std::shared_ptr<Target> target, double duration, int repeatCounter = -1, double delayBeforeStart = 0,
                        bool disposeAfterFinish = false)
            : image_(target) {
            TargetTween::init(target.get(), duration, repeatCounter, delayBeforeStart, disposeAfterFinish);
        }

        [[nodiscard]] ImageView& image() { return *image_; }               ///< image, _SafeStr_1672
        void restartAt(int repeatCount = -1, double timePosition = 0) { restart(repeatCount, timePosition); } ///< _SafeStr_1668
        void play() { setRunning(true); }
        void stop() { setRunning(false); }
        [[nodiscard]] double duration() const { return transition_.mscDuration / 1000; }
        void setDuration(double seconds) { transition_.mscDuration = seconds * 1000; }

    protected:
        void customDispose() override;

    private:
        std::shared_ptr<ImageView> image_; ///< Original: _SafeStr_2625.
    };

    class ResourceManager : public UpdatableObject {
    public:
        explicit ResourceManager(int fps = 30);

        /// One Transition per labelled section of a clip. The last one runs to
        /// the end (original: _SafeStr_1621).
        static std::vector<Transition> transitionsOf(engine::MovieClip& movie, int fps);

        [[nodiscard]] AnimationMaker& maker() { return *maker_; } ///< _SafeStr_765
        std::shared_ptr<CachedData> getCachedData(const std::string& key) { return maker_->getCachedData(key); }
        /// The animation ranges of a clip registered with registerTransitions.
        [[nodiscard]] const std::vector<Transition>& animTransitions(const std::string& key);
        [[nodiscard]] bool hasTransitions(const std::string& key) const { return transitions_.contains(key); }

        /// Registers clips to render, with their scale and radius (original: _SafeStr_1637).
        void prepareClips(const std::vector<ClipDef>& clips);

        /// Computes the animation ranges of clips (original: _SafeStr_1635).
        void registerTransitions(const std::vector<ClipDef>& clips);

        /// Starts rendering. The signal fires when done (original: startLoadRes).
        virtual Signal<>& startLoadRes() { return maker_->signalFinish; }

        /// A still picture of a clip, from its cached frames if any (original: createRasterObject).
        std::shared_ptr<RasterObject> createRasterObject(const std::string& imageId, double x = 0, double y = 0,
                                                         int animFrameNum = 0);
        /// An animated display object playing a clip (original: createAnimation).
        std::shared_ptr<SpriteAnimation> createAnimation(const std::string& key);

        void update(double timeElapsed) override;

    protected:
        void customDispose() override;

        /// Registers a clip and its ranges (original: _SafeStr_1626, _SafeStr_1622).
        void registerClip(const std::string& key, std::shared_ptr<engine::MovieClip> movie);

        std::unique_ptr<AnimationMaker> maker_;                     ///< Original: _SafeStr_1613.
        int fps_;                                                    ///< Original: _SafeStr_684.
        std::map<std::string, std::vector<Transition>> transitions_; ///< Original: _SafeStr_1617.
        std::map<std::string, std::vector<Transition>> clips_;       ///< Original: _SafeStr_1614.
    };

} // namespace takeover
