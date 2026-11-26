// MovieClipButton.h - turns a library MovieClip into a button (original:
// _SafePkg_46/_SafeStr_41).
//
// The clip's frames carry the looks: "_up", "_over", "_down" (switched by
// the Flash Player itself in button mode) and "_disabled". A child named
// "_hitArea", if any, becomes the invisible clickable area.
#pragma once

#include "engine/display/MovieClip.h"

#include <memory>

namespace takeover {

    class MovieClipButton {
    public:
        explicit MovieClipButton(std::shared_ptr<engine::MovieClip> clip);
        virtual ~MovieClipButton();
        MovieClipButton(const MovieClipButton&) = delete;
        MovieClipButton& operator=(const MovieClipButton&) = delete;

        [[nodiscard]] engine::MovieClip* image() const { return clip_.get(); }
        [[nodiscard]] engine::MovieClip* button() const { return clip_.get(); }
        [[nodiscard]] const std::shared_ptr<engine::MovieClip>& imagePtr() const { return clip_; }

        [[nodiscard]] bool isEnabled() const { return enabled_; }
        virtual void setEnabled(bool value);

    protected:
        /// A disabled button swallows the click and re-sends it to its parent,
        /// so what lies "under" the button still reacts.
        virtual void onMouseClick(engine::MouseEvent& event);

        static constexpr const char* FRAME_UP = "_up";
        static constexpr const char* FRAME_OVER = "_over";
        static constexpr const char* FRAME_DOWN = "_down";
        static constexpr const char* FRAME_DISABLED = "_disabled";
        static constexpr const char* HIT_AREA_NAME = "_hitArea";

        std::shared_ptr<engine::MovieClip> clip_;
        bool enabled_ = false;
    };

} // namespace takeover
