#include "game/framework/raster/ResourceManager.h"

#include "game/framework/Symbols.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace takeover {

    // /----- IMAGE VIEW -----/

    ImageView::ImageView(std::shared_ptr<engine::Sprite> sprite, double baseRadius) : sprite_(std::move(sprite)) {
        if (baseRadius <= 0)
            baseRadius = std::max(sprite_->width(), sprite_->height());

        baseRadius_ = baseRadius;
        radius_ = baseRadius_;
    }

    void ImageView::setRadius(double radius) {
        radius_ = radius;
        const double scale = radius_ / baseRadius_;
        sprite_->setScaleX(scale);
        sprite_->setScaleY(scale);
    }

    void ImageView::addImage(ImageView& image) {
        sprite_->addChild(image.sprite_);
        image.parent_ = this;
    }

    void ImageView::addImage(SpriteAnimation& animation) { addImage(animation.image()); }

    void ImageView::removeImage(ImageView& image) {
        sprite_->removeChild(image.sprite_.get());
        image.parent_ = nullptr;
    }

    void ImageView::removeImage(SpriteAnimation& animation) { removeImage(animation.image()); }

    void ImageView::customDispose() {
        if (sprite_ && sprite_->parent() != nullptr)
            sprite_->parent()->removeChild(sprite_.get());
    }

    // /----- CACHED CLIP TARGET -----/

    CachedClipTarget::CachedClipTarget(std::vector<std::shared_ptr<engine::BitmapData>> frames, double dx, double dy,
                                       double baseRadius)
        : ImageView(std::make_shared<engine::Sprite>(),
                    baseRadius > 0 ? baseRadius : std::max(frames[0]->width(), frames[0]->height())),
          frames_(std::move(frames)) {
        bitmap_ = std::make_shared<engine::Bitmap>(nullptr);
        bitmap_->setX(dx);
        bitmap_->setY(dy);
        sprite_->addChild(bitmap_);
    }

    void CachedClipTarget::setTweenValue(double frame) {
        const int newFrame = static_cast<int>(std::floor(frame + 0.5));
        if (newFrame != shownFrame_) {
            frame_ = newFrame;
            shownFrame_ = frame_;

            if (frame_ >= 1 && frame_ <= static_cast<int>(frames_.size()))
                bitmap_->setBitmapData(frames_[static_cast<std::size_t>(frame_ - 1)]);
        }
    }

    // /----- MOVIE CLIP TARGET -----/

    MovieClipTarget::MovieClipTarget(std::shared_ptr<engine::MovieClip> movie, double baseRadius)
        : ImageView(movie, baseRadius), movie_(std::move(movie)) {}

    void MovieClipTarget::setTweenValue(double frame) { movie_->gotoAndStop(static_cast<int>(std::floor(frame + 0.5))); }

    int MovieClipTarget::tweenEnd() const { return movie_->totalFrames(); }

    // /----- SPRITE ANIMATION -----/

    void SpriteAnimation::customDispose() {
        // Like the original, the image stays where it is (frozen on its frame):
        // whoever shows it removes it.
        TargetTween::customDispose();
    }

    // /----- RESOURCE MANAGER -----/

    ResourceManager::ResourceManager(int fps) : maker_(std::make_unique<AnimationMaker>()), fps_(fps) {
        // Like the original, the frame rate given is ignored for the clips: 30.
        fps_ = 30;
        (void)fps;
    }

    std::vector<Transition> ResourceManager::transitionsOf(engine::MovieClip& movie, int fps) {
        std::vector<Transition> transitions;
        const auto& labels = movie.currentLabels();
        int frameA = 0, frameB = 1;

        for (std::size_t i = 0; i + 1 < labels.size(); ++i) {
            frameA = labels[i].frame;
            frameB = labels[i + 1].frame - 1;
            transitions.emplace_back(easing::linear, frameA, frameB, static_cast<double>(frameB - frameA) / fps);
        }

        // Original quirk: the last section starts at the end of the previous one.
        frameA = frameB;
        frameB = movie.totalFrames() - 1;
        transitions.emplace_back(easing::linear, frameA, frameB, static_cast<double>(frameB - frameA) / fps);

        return transitions;
    }

    const std::vector<Transition>& ResourceManager::animTransitions(const std::string& key) {
        auto it = transitions_.find(key);
        if (it == transitions_.end()) {
            // Not registered: compute them now rather than fail.
            registerTransitions({ClipDef{key, "", 1}});
            it = transitions_.find(key);
        }

        return it->second;
    }

    void ResourceManager::prepareClips(const std::vector<ClipDef>& clips) {
        for (const ClipDef& clip : clips) {
            std::shared_ptr<engine::MovieClip> movie = symbols::create(clip.libName);
            const engine::Rectangle bounds = movie->getBounds(movie.get());
            movie->setScaleX(clip.scale);
            movie->setScaleY(clip.scale);
            maker_->prepare(clip.key(), movie, bounds, 0);
        }
    }

    void ResourceManager::registerTransitions(const std::vector<ClipDef>& clips) {
        for (const ClipDef& clip : clips) {
            // The original would stop with an error on a missing class.
            if (!symbols::exists(clip.libName)) {
                std::fprintf(stderr, "ResourceManager: no clip %s\n", clip.libName.c_str());
                continue;
            }

            std::shared_ptr<engine::MovieClip> movie = symbols::create(clip.libName);
            transitions_[clip.key()] = transitionsOf(*movie, fps_);
        }
    }

    void ResourceManager::registerClip(const std::string& key, std::shared_ptr<engine::MovieClip> movie) {
        if (!clips_.contains(key)) {
            clips_[key] = transitionsOf(*movie, fps_);
            maker_->prepareIfNew(key, movie);
        }
    }

    std::shared_ptr<RasterObject> ResourceManager::createRasterObject(const std::string& imageId, double x, double y,
                                                                      int animFrameNum) {
        // The original looks in a cache that is never filled: every still
        // picture is rendered from the clip.
        (void)animFrameNum;
        std::shared_ptr<engine::MovieClip> clip = symbols::create(imageId);
        return RasterObject::fromDisplayObject(*clip, x, y);
    }

    std::shared_ptr<SpriteAnimation> ResourceManager::createAnimation(const std::string& key) {
        if (!clips_.contains(key))
            registerClip(key, symbols::create(key));

        const std::shared_ptr<CachedData> cached = maker_->getCachedData(key);
        auto target = std::make_shared<CachedClipTarget>(cached->bitmapData, cached->offsetX, cached->offsetY, cached->radius);
        return std::make_shared<SpriteAnimation>(target, static_cast<double>(cached->bitmapData.size()) / fps_);
    }

    void ResourceManager::update(double timeElapsed) { UpdatableObject::update(timeElapsed); }

    void ResourceManager::customDispose() {
        maker_->dispose();
        transitions_.clear();
        clips_.clear();
        UpdatableObject::customDispose();
    }

} // namespace takeover
