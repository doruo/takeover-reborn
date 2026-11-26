// MediaToggle.h - what MusicConst and SoundConst share: a SoundManager that
// can be muted, and the on/off switch clip of the screens (frames "lbOn" /
// "lbOff", children "butMusicOn"/"butMusicOff" or "butSoundOn"/"butSoundOff").
#pragma once

#include "engine/display/MovieClip.h"
#include "game/framework/audio/SoundManager.h"

#include <memory>
#include <string>

namespace takeover {

    class MediaToggle {
    public:
        MediaToggle(std::string onButtonName, std::string offButtonName);
        virtual ~MediaToggle() = default;

        [[nodiscard]] SoundManager& manager() { return *manager_; }

        [[nodiscard]] bool enabled() const;
        void setEnabled(bool enable);

        /// Binds / unbinds a switch clip (original: _SafeStr_1090 / _SafeStr_1091).
        void attachSwitch(engine::MovieClip* target);
        void detachSwitch(engine::MovieClip* target);

    private:
        void onMouseClick(engine::MouseEvent& event);

        /// Muting sets the volume to a tiny value instead of 0, as the original.
        static constexpr double kMinVolume = 1e-7;
        std::shared_ptr<SoundManager> manager_;
        std::weak_ptr<engine::DisplayObject> switch_;
        std::string onButtonName_, offButtonName_;
    };

} // namespace takeover
