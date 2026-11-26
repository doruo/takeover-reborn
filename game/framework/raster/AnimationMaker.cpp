#include "game/framework/raster/AnimationMaker.h"

#include "game/framework/Symbols.h"

#include "engine/display/DisplayObjectContainer.h"
#include "engine/events/Event.h"

#include <SDL3/SDL_timer.h>

#include <algorithm>

namespace takeover {

    namespace {
        double nowMs() { return static_cast<double>(SDL_GetTicks()); }
    } // namespace

    AnimationMaker::AnimationMaker(double maxCacheTime)
        : maxCacheTime_(maxCacheTime), ticker_(std::make_shared<engine::Sprite>()) {}

    double AnimationMaker::radiusOf(engine::MovieClip& movie) {
        engine::DisplayObject* border = movie.getChildByName(BORDER_NAME);
        if (border != nullptr)
            border->setVisible(false);

        if (border == nullptr)
            border = &movie;

        return border->width() > border->height() ? border->width() / 2 : border->height() / 2;
    }

    void AnimationMaker::prepareClass(const std::string& key) {
        if (!entries_.contains(key))
            prepare(key, symbols::create(key));
    }

    void AnimationMaker::stepClip(engine::MovieClip& movie) {
        auto& [owner, frame] = clipFrames_[&movie];

        // A counter left by a dead clip at the same address doesn't count.
        if (owner.expired() || owner.lock().get() != &movie)
            frame = 0;

        owner = movie.shared_from_this();
        frame = frame != 0 ? frame + 1 : 1;
        if (frame > movie.totalFrames())
            frame = 1;

        movie.gotoAndStop(frame);

        for (int i = 0; i < movie.numChildren(); ++i)
            if (auto* part = dynamic_cast<engine::MovieClip*>(movie.getChildAt(i)))
                stepClip(*part);
    }

    void AnimationMaker::gotoClipFrame(engine::MovieClip& movie, int frame) {
        auto& [owner, counter] = clipFrames_[&movie];
        owner = movie.shared_from_this();
        counter = frame - 1;
        stepClip(movie);
    }

    void AnimationMaker::cache(const std::string& key, double maxCacheTime) {
        if (!entries_.contains(key))
            prepareClass(key);

        Entry& entry = entries_[key];
        if (entry.cached)
            return;

        Pending& item = *entry.pending;
        if (!item.done) {
            engine::MovieClip::timelineSoundsMuted = true;
            engine::MovieClip& movie = *item.movie;
            const double scaleX = movie.scaleX(), scaleY = movie.scaleY();

            for (int i = movie.currentFrame(); i <= movie.totalFrames(); ++i) {
                auto bitmap = std::make_shared<engine::BitmapData>(static_cast<int>(item.bounds.width * scaleX),
                                                                   static_cast<int>(item.bounds.height * scaleY), true, 0);
                const engine::Matrix matrix = engine::Matrix::translation(-item.bounds.x, -item.bounds.y)
                                                  .then(engine::Matrix::scale(scaleX, scaleY));
                bitmap->draw(movie, matrix);
                item.frames[static_cast<std::size_t>(movie.currentFrame() - 1)] = bitmap;

                if (movie.currentFrame() == movie.totalFrames())
                    item.done = true;
                else
                    gotoClipFrame(movie, i + 1);

                ++framesDone_;

                if (maxCacheTime >= 0 && nowMs() - batchStart_ >= maxCacheTime)
                    break;
            }

            engine::MovieClip::timelineSoundsMuted = false;
        }

        if (item.done) {
            auto cached = std::make_shared<CachedData>();
            cached->bitmapData = std::move(item.frames);
            cached->offsetX = item.bounds.x * item.movie->scaleX();
            cached->offsetY = item.bounds.y * item.movie->scaleY();
            cached->radius = item.radius;
            entry.cached = cached;
            entry.pending.reset();
        }
    }

    std::shared_ptr<CachedData> AnimationMaker::getCachedData(const std::string& key) {
        cache(key);
        return entries_[key].cached;
    }

    void AnimationMaker::prepare(const std::string& key, std::shared_ptr<engine::MovieClip> movie,
                                 std::optional<engine::Rectangle> bounds, double radius) {
        auto item = std::make_unique<Pending>();
        item->movie = movie;
        engine::MovieClip::timelineSoundsMuted = true;
        movie->gotoAndStop(1);

        if (bounds) {
            item->bounds = *bounds;
        } else {
            engine::Rectangle all = movie->getBounds(movie.get());

            for (int i = 2; i <= movie->totalFrames(); ++i) {
                movie->gotoAndStop(i);
                all = all.united(movie->getBounds(movie.get()));
            }

            movie->gotoAndStop(1);
            item->bounds = all;
        }

        engine::MovieClip::timelineSoundsMuted = false;
        item->frames.resize(static_cast<std::size_t>(movie->totalFrames()));
        const double ownRadius = radiusOf(*movie);
        item->radius = radius <= 0 ? ownRadius : radius;
        framesTotal_ += movie->totalFrames();
        entries_[key] = Entry{std::move(item), nullptr};
        ticker_->addEventListener(engine::Event::ENTER_FRAME, this, &AnimationMaker::onEnterFrame);
    }

    void AnimationMaker::prepareIfNew(const std::string& key, std::shared_ptr<engine::MovieClip> movie) {
        if (!entries_.contains(key))
            prepare(key, std::move(movie));
    }

    void AnimationMaker::onEnterFrame(engine::Event&) {
        bool allCached = true;
        batchStart_ = nowMs();
        std::vector<std::string> keys;

        for (const auto& [key, entry] : entries_)
            keys.push_back(key);

        for (const std::string& key : keys) {
            cache(key, maxCacheTime_);

            if (!entries_[key].cached)
                allCached = false;

            if (maxCacheTime_ >= 0 && nowMs() - batchStart_ >= maxCacheTime_) {
                allCached = false;
                break;
            }
        }

        if (allCached) {
            signalFinish.dispatch();

            for (const auto& [key, entry] : entries_)
                if (!entry.cached)
                    allCached = false;
            if (allCached)
                ticker_->removeEventListener(engine::Event::ENTER_FRAME, this, &AnimationMaker::onEnterFrame);
        }
    }

    void AnimationMaker::customDispose() {
        ticker_->removeEventListener(engine::Event::ENTER_FRAME, this, &AnimationMaker::onEnterFrame);
        entries_.clear();
        clipFrames_.clear();
        signalFinish.dispose();
        UpdatableObject::customDispose();
    }

} // namespace takeover
