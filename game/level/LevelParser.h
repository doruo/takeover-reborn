// LevelParser.h - builds a Level from the map clips (original: level/LevelParser).
//
// Each map comes with a "Level<N>GraphMarkersClass" clip whose children are
// markers: "l<location>nd_<id>" crossings (some of them strongholds, chests,
// build places or an ogre), and "ed_<idA>_<idB>" roads whose children are
// the way points.
#pragma once

#include "game/level/Level.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace takeover {

    class LevelParser {
    public:
        /// Builds the level of a battle (original: _SafeStr_3131).
        static std::shared_ptr<Level> parse(const BattleDef& def);

    private:
        LevelParser() = default;
        void init(const BattleDef& def);
        void parseMarkers(engine::DisplayObjectContainer& markers); ///< parse
        void addNode(engine::DisplayObject& marker);                ///< _SafeStr_3130
        void addRoad(engine::DisplayObject& marker);                ///< _SafeStr_3127

        /// Puts the map objects and the buildings in the battlefield layer (original: _SafeStr_3128).
        void fillLayer();

        std::map<std::string, std::shared_ptr<GraphNode>> nodes_; ///< By marker id (original: _SafeStr_2277).
        std::shared_ptr<Level> level_;                            ///< Original: _SafeStr_1502.
        std::vector<int> locationViews_;                          ///< Original: _SafeStr_3125.
        TOResManager* resources_ = nullptr;                       ///< Original: _SafeStr_1252.
    };

} // namespace takeover
