// MediaSwitchers.h - the music and sound buttons in the corner of the
// screens, with their glowing backlight and hint (original: MediaSwitchers).
#pragma once

#include "engine/display/MovieClip.h"
#include "game/framework/core/Disposable.h"
#include "game/framework/ui/HintWindow.h"

#include <memory>

namespace takeover {

    class MediaSwitchers : public Disposable {
    public:
        explicit MediaSwitchers(engine::MovieClip* block);

    protected:
        void customDispose() override;

    private:
        [[nodiscard]] engine::MovieClip* butMusic() const;
        [[nodiscard]] engine::MovieClip* butSound() const;
        static engine::MovieClip* backlight(engine::DisplayObject* icon); ///< _SafeStr_3615
        void attachBacklight(engine::MovieClip* icon);                     ///< _SafeStr_3619
        void detachBacklight(engine::MovieClip* icon);                     ///< _SafeStr_3618
        void onMusicRollOver(engine::MouseEvent& event);                   ///< _SafeStr_3616
        void onSoundRollOver(engine::MouseEvent& event);                   ///< _SafeStr_3620
        void showHint(engine::MovieClip* button, const char* text);
        void onRollOut(engine::MouseEvent& event);                         ///< _SafeStr_1571
        void onBacklightFrame(engine::Event& event);                       ///< _SafeStr_3617

        std::shared_ptr<engine::MovieClip> block_;
        std::unique_ptr<HintWindow> hint_;
    };

} // namespace takeover
