// PlayerStatPanel.h - rank, XP bar and free edicts of the selected campaign
// (original: _SafePkg_3473/PlayerStatPanel; its definition _SafeStr_3461 is
// folded into the constructor).
#pragma once

#include "engine/display/MovieClip.h"

#include <memory>

namespace takeover {

    class Profile;

    class PlayerStatPanel {
    public:
        PlayerStatPanel(engine::MovieClip* panel, Profile& profile);
        void refresh(); ///< _SafeStr_3470

    private:
        std::shared_ptr<engine::MovieClip> panel_;
        Profile* profile_;
    };

} // namespace takeover
