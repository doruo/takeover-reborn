// PreloaderScreen.h - the loading screen and its "start" button (original:
// PreloaderScreen). The port has nothing to load: Main shows it at 100%,
// waiting for the click that starts the splash screens, as the original
// did once its resources were cached.
#pragma once

#include "engine/display/MovieClip.h"
#include "engine/display/TextField.h"
#include "game/framework/core/Disposable.h"
#include "game/framework/ui/MovieClipButton.h"
#include "game/gui/Branding.h"

#include <memory>

namespace takeover {

class PreloaderScreen : public Disposable {
public:
    PreloaderScreen(int statusTextNum = 0, bool showButtonAfterComplete = false);

    /// Loading finished and the player clicked start (original: _SafeStr_18).
    Signal<> signalStart;

    [[nodiscard]] const std::shared_ptr<engine::MovieClip>& image() const { return clip_; }
    void setProgress(double fraction); ///< _SafeStr_21
    void complete();                   ///< _SafeStr_24

protected:
    void customDispose() override;

private:
    void onStartClick(engine::MouseEvent&); ///< _SafeStr_40
    [[nodiscard]] engine::TextField* text(std::string_view name) const;
    [[nodiscard]] engine::MovieClip* clip(std::string_view name) const;

    static constexpr int FRAME_LOADING = 1;  ///< _SafeStr_44
    static constexpr int FRAME_COMPLETE = 10; ///< _SafeStr_43

    std::shared_ptr<engine::MovieClip> clip_;
    std::unique_ptr<MovieClipButton> startButton_;
    std::unique_ptr<Branding> branding_;
    bool showButton_ = false;
};

/// The "ver. 1.1.5" label (original: Preloader._SafeStr_6).
std::shared_ptr<engine::MovieClip> createVersionLabel();

} // namespace takeover
