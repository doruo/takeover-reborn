// LevelKeys.h - keys naming the parts of a battle map (original:
// level/_SafeStr_1403, _SafeStr_1463, _SafeStr_1484).
#pragma once

#include "game/sim/base/TypedKey.h"

namespace takeover {

    /// A crossing of the road graph (original: _SafeStr_1403, kind "node").
    class NodeKey : public TypedKey {
    public:
        NodeKey() : TypedKey("node") {}
    };

    /// A road between two crossings (original: _SafeStr_1463, kind "road").
    class RoadKey : public TypedKey {
    public:
        RoadKey() : TypedKey("road") {}
    };

    /// What stands on a crossing: stronghold, build place, chest (original:
    /// _SafeStr_1484, kind "ndcntnt").
    class NodeContentKey : public TypedKey {
    public:
        NodeContentKey() : TypedKey("ndcntnt") {}
    };

} // namespace takeover
