// AnimationMaker.h - renders library clips into bitmaps, frame by frame,
// and keeps them (original: _SafePkg_1641/AnimationMaker, CachedData and
// _SafeStr_1869).
//
// Copying pre-rendered pictures is how the original keeps hundreds of
// animated soldiers fast. A clip is registered (prepare), then rendered a
// few frames at a time on ENTER_FRAME within a time budget, or all at once
// the first time someone asks for it (getCachedData).
#pragma once

#include "game/framework/core/Disposable.h"
#include "game/framework/core/Signal.h"

#include "engine/display/BitmapData.h"
#include "engine/display/MovieClip.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace engine {
    class Sprite;
    class Event;
}

namespace takeover {

    /// The frames of a rendered clip (original: CachedData).
    class CachedData : public Disposable {
    public:
        std::vector<std::shared_ptr<engine::BitmapData>> bitmapData;
        double offsetX = 0; ///< Left of the clip's bounds, scaled (original: _SafeStr_1623).
        double offsetY = 0; ///< Top of the clip's bounds, scaled (original: _SafeStr_1625).
        double radius = 0;

    protected:
        void customDispose() override { bitmapData.clear(); }
    };

    class AnimationMaker : public UpdatableObject {
    public:
        /// Name of the invisible child giving a clip's size (original: BORDER_NAME).
        static constexpr const char* BORDER_NAME = "border";

        /// `maxCacheTime`: milliseconds of rendering per frame.
        explicit AnimationMaker(double maxCacheTime = 1000);

        /// Dispatched once everything registered is rendered.
        Signal<> signalFinish;

        /// Half the larger side of the clip's "border" child, or of the clip
        /// (original: _SafeStr_1875). Hides the border.
        static double radiusOf(engine::MovieClip& movie);

        /// The frames of a clip, rendering it now if needed (original: _SafeStr_1630).
        std::shared_ptr<CachedData> getCachedData(const std::string& key);

        /// Registers a clip to render (original: _SafeStr_1629). Without
        /// `bounds`, the union of the bounds of all its frames is used.
        void prepare(const std::string& key, std::shared_ptr<engine::MovieClip> movie,
                     std::optional<engine::Rectangle> bounds = std::nullopt, double radius = 0);
        void prepareIfNew(const std::string& key, std::shared_ptr<engine::MovieClip> movie); ///< _SafeStr_1626

        /// Frames rendered / frames registered (original: getProgress).
        [[nodiscard]] double getProgress() const { return static_cast<double>(framesDone_) / framesTotal_; }

    protected:
        void customDispose() override;

    private:
        /// A clip being rendered (original: _SafeStr_1869).
        struct Pending {
            std::shared_ptr<engine::MovieClip> movie;
            engine::Rectangle bounds;
            std::vector<std::shared_ptr<engine::BitmapData>> frames; ///< Original: _SafeStr_1808.
            double radius = 0;
            bool done = false; ///< Original: _SafeStr_1876.
        };

        struct Entry {
            std::unique_ptr<Pending> pending;
            std::shared_ptr<CachedData> cached;
        };

        void prepareClass(const std::string& key); ///< _SafeStr_1881

        /// Renders frames of a clip, at most `maxCacheTime` ms (-1: all) (original: _SafeStr_1878).
        void cache(const std::string& key, double maxCacheTime = -1);

        /// Advances a clip and its sub-clips by one frame, the way the original
        /// emulates playback while rendering (original: _SafeStr_1877).
        void stepClip(engine::MovieClip& movie);
        void gotoClipFrame(engine::MovieClip& movie, int frame); ///< _SafeStr_1880
        void onEnterFrame(engine::Event& event);

        std::map<std::string, Entry> entries_; ///< Original: _SafeStr_1868.
        double maxCacheTime_;                  ///< Original: _SafeStr_1870.
        std::shared_ptr<engine::Sprite> ticker_; ///< Receives ENTER_FRAME (original: _SafeStr_1871).
        int framesDone_ = 1;                   ///< Original: _SafeStr_1873.
        int framesTotal_ = 1;                  ///< Original: _SafeStr_1872.
        double batchStart_ = 0;                ///< Original: _SafeStr_1874.

        /// The frame counters the original stored on each clip (the `_frame`
        /// dynamic property), by clip.
        std::map<const engine::MovieClip*, std::pair<std::weak_ptr<engine::DisplayObject>, int>> clipFrames_;
    };

} // namespace takeover
