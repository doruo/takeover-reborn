// IntroScreen.h - plays a splash clip once, clickable to open the
// sponsor's site (original: gui/_SafeStr_675).
#pragma once

#include "engine/display/MovieClip.h"
#include "game/framework/core/Signal.h"

#include <memory>
#include <string>

namespace takeover {

class IntroScreen : public std::enable_shared_from_this<IntroScreen> {
public:
    /// Creates an intro and keeps it alive until its clip reaches the last
    /// frame (the original lived on through its ENTER_FRAME listener).
    static std::shared_ptr<IntroScreen> play(std::shared_ptr<engine::MovieClip> intro, std::string url = "");
    IntroScreen(std::shared_ptr<engine::MovieClip> intro, std::string url = "");
    ~IntroScreen();

    Signal<> signalFinish;
    [[nodiscard]] const std::shared_ptr<engine::MovieClip>& clip() const { return clip_; } ///< _SafeStr_730

private:
    void onEnterFrame(engine::Event& event);
    void onClick(engine::MouseEvent& event); ///< _SafeStr_3442
    void dispose();

    std::shared_ptr<engine::MovieClip> clip_;
    std::shared_ptr<engine::Sprite> clickArea_; ///< Original: _SafeStr_3440.
    std::string url_;
    std::shared_ptr<IntroScreen> self_; ///< Keeps the intro alive while it plays.
};

} // namespace takeover
