// MusicConst.h - the two music tracks (original: media/MusicConst, a
// singleton reached through _SafeStr_50).
#pragma once

#include "game/media/MediaToggle.h"

namespace takeover {

    class MusicConst : public MediaToggle {
    public:
        static MusicConst& instance(); ///< Original: _SafeStr_50.

        static inline bool MODE_MUTE = false;

        void playGameplayMusic(); ///< Original: _SafeStr_749 (also used for the intro).
        void playMenuMusic();     ///< Original: _SafeStr_746.
        void stopMusic();         ///< Original: _SafeStr_745.

    private:
        MusicConst();
    };

} // namespace takeover
