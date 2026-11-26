#include "game/media/MusicConst.h"

namespace takeover {

    MusicConst::MusicConst() : MediaToggle("butMusicOn", "butMusicOff") { setEnabled(!MODE_MUTE); }

    MusicConst& MusicConst::instance() {
        static MusicConst music;
        return music;
    }

    void MusicConst::playGameplayMusic() {
        manager().setVolume("musicGameplayClass", 0.35);
        manager().playMusic("musicGameplayClass");
    }

    void MusicConst::playMenuMusic() {
        manager().setVolume("musicMenuClass", 0.35);
        manager().playMusic("musicMenuClass");
    }

    void MusicConst::stopMusic() { manager().stopMusic(); }

} // namespace takeover
