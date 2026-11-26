// MovieClip.h - a Sprite that plays a timeline (flash.display.MovieClip).
//
// The timeline is a list of edits per frame (swf::Timeline). Entering a frame
// applies its edits; jumping (gotoAndStop) replays the edits between the
// current and the target frame, from frame 1 when going backwards, and then
// reconciles the result with the existing children. That reconciliation is
// what makes Flash keep an object alive across frames (same character at the
// same depth) or recreate it, and the game's animations depend on it.
#pragma once

#include "engine/display/DisplayObjectContainer.h"
#include "swf/Timeline.h"

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace engine {

    class Library;

    struct FrameLabel {
        std::string name;
        int frame = 1; ///< 1-based, like flash.display.FrameLabel.
    };

    class MovieClip : public Sprite {
    public:
        /// An empty clip, as `new MovieClip()` in AS3 (one empty frame).
        MovieClip() = default;

        /// A clip playing a library sprite's timeline; frame 1 is entered
        /// immediately, so its children exist when the constructor returns.
        MovieClip(Library& library, const swf::Timeline& timeline, std::uint16_t spriteId = 0);

        /// Frame scripts that belong to a library symbol rather than to code:
        /// the `stop()` calls the Flash IDE compiled into some clips. Registered
        /// once at startup, they run for every instance of the symbol.
        /// `frameIndex` is 0-based like AS3 addFrameScript.
        static void registerSymbolScript(std::uint16_t spriteId, int frameIndex, std::function<void(MovieClip&)> script);

        /// While true, frames entered don't play their sounds (used when clips
        /// are rendered to bitmaps frame by frame).
        static inline bool timelineSoundsMuted = false;

        [[nodiscard]] std::uint16_t spriteId() const { return spriteId_; }

        [[nodiscard]] int currentFrame() const { return currentFrame_; }
        [[nodiscard]] int totalFrames() const;
        [[nodiscard]] bool isPlaying() const { return playing_; }
        [[nodiscard]] const std::vector<FrameLabel>& currentLabels() const { return labels_; }
        /// Label of the current frame or the closest one before it ("" if none).
        [[nodiscard]] std::string currentLabel() const;
        [[nodiscard]] bool hasLabel(std::string_view label) const { return frameForLabel(label) != 0; }

        void play() { playing_ = true; }
        void stop() { playing_ = false; }
        void gotoAndStop(int frame);
        void gotoAndPlay(int frame);
        void gotoAndStop(std::string_view label);
        void gotoAndPlay(std::string_view label);
        void nextFrame();
        void prevFrame();

        /// AS3 addFrameScript: `frameIndex` is 0-based, as in the original code.
        void addFrameScript(int frameIndex, std::function<void()> script);

        void advanceFrame() override;

    private:
        void gotoFrame(int target);
        [[nodiscard]] int frameForLabel(std::string_view label) const;
        DisplayObjectPtr instantiate(std::uint16_t characterId) const;

        void runFrameActions();

        Library* library_ = nullptr;
        const swf::Timeline* timeline_ = nullptr;
        std::uint16_t spriteId_ = 0;
        int currentFrame_ = 1;
        bool playing_ = true;
        std::vector<FrameLabel> labels_;
        std::map<int, std::function<void()>> frameScripts_; ///< Keyed by 1-based frame.
    };

    /// Creates the display object for a library character (shape, sprite,
    /// button, text...). Returns nullptr for characters that are not displayable.
    DisplayObjectPtr createDisplayObject(Library& library, std::uint16_t characterId);

} // namespace engine
