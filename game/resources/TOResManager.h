// TOResManager.h - Take Over's resource manager (original: TOResManager).
//
// At startup it renders the few clips of XMLData::preloadClips (the
// preloader shows the progress) and computes the animation ranges of every
// battle clip. The battle clips themselves are rendered the first time a
// battle needs them.
#pragma once

#include "game/framework/raster/ResourceManager.h"

namespace takeover {

    class TOResManager : public ResourceManager {
    public:
        /// With `addXMLData`, every battle clip is rendered at startup too
        /// (the game passes false).
        /// (The original also raised the stage frame rate to 120 while
        /// loading, to render more per second. The budget here is per frame.)
        explicit TOResManager(int gameFps, bool addXMLData = true);
    };

} // namespace takeover
