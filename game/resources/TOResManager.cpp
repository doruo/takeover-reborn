#include "game/resources/TOResManager.h"

#include "game/resources/XMLData.h"

namespace takeover {

    TOResManager::TOResManager(int gameFps, bool addXMLData) : ResourceManager(gameFps) {
        XMLData& data = XMLData::instance();
        prepareClips(addXMLData ? data.battleClips : data.preloadClips);
        registerTransitions(data.battleClips);
    }

} // namespace takeover
