// RoadGraph.h - the road network of a battle map (original: level/_SafeStr_1557,
// level/_SafeCls_1460 and package _SafePkg_1543).
//
// Crossings (GraphNode) are joined by roads made of way points. Squads walk
// along the roads. LevelLogic finds the shortest road path between any two
// points of the map.
#pragma once

#include "game/sim/base/Point2D.h"
#include "game/sim/level/LevelKeys.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace takeover {

    class KeyNode;
    class Road;

    /// Scratch data of the shortest path search on a crossing (original:
    /// _SafePkg_1543/_SafeCls_2702).
    struct PathSearchData {
        PathSearchData() { reset(); }
        void reset() { distance = 999999, roads.clear(); } ///< Original: _SafeStr_2344.

        double distance = 999999;  ///< Best distance found from the start (original: _SafeStr_2297).
        std::vector<Road*> roads;  ///< Roads touching the crossing (original: _SafeStr_2334).
    };

    /// A crossing (original: level/_SafeStr_1557).
    struct GraphNode {
        explicit GraphNode(std::shared_ptr<NodeKey> key) : description(std::move(key)) {}

        Point2D pos;
        PathSearchData search;    ///< Original: _SafeStr_2289.

        /// The location (a stronghold and its build places) it belongs to, or -1.
        int locationNum = -1;
        std::shared_ptr<NodeKey> description;
    };

    /// A road between two crossings (original: level/_SafeCls_1460).
    class Road {
    public:
        explicit Road(std::shared_ptr<RoadKey> key) : description(std::move(key)) {}

        /// The crossing at the other end (original: _SafeStr_1465).
        [[nodiscard]] const std::shared_ptr<NodeKey>& otherEnd(const std::shared_ptr<NodeKey>& end) const {
            return end == nodeA ? nodeB : nodeA;
        }

        /// Length of the way points line (original: _SafeStr_1464).
        [[nodiscard]] double computeLength() const;

        std::vector<Point2D> wayPoints; ///< From nodeA to nodeB.
        double length = 0;              ///< Original: _SafeStr_1466.
        std::shared_ptr<NodeKey> nodeA; ///< Original: _SafeStr_1461.
        std::shared_ptr<NodeKey> nodeB; ///< Original: _SafeStr_1462.
        std::shared_ptr<RoadKey> description;
    };

    /// Another stronghold as seen from a stronghold: the road path and its
    /// length (original: _SafePkg_1543/_SafeStr_1510).
    struct NodeLink {
        std::optional<std::vector<Point2D>> path;
        KeyNode* keyNode = nullptr;
        double dist = 0;
    };

    /// The point of the road network closest to a point (original:
    /// _SafePkg_1543/_SafeStr_2291).
    struct RoadPoint {
        Point2D point;
        double distance2 = 9999999; ///< Squared distance (original: _SafeStr_2357).
        Road* road = nullptr;
        int roadIndex0 = -1; ///< Way points around the point.
        int roadIndex1 = -1;

        void set(const Point2D& p, double d2, Road* r, int index0 = -1, int index1 = -1) { ///< _SafeStr_2338
            point = p, distance2 = d2, road = r, roadIndex0 = index0, roadIndex1 = index1;
        }

        /// Keeps `other` if it is closer. Returns true if it was (original: _SafeStr_2320).
        bool keepCloser(const RoadPoint& other) {
            if (distance2 > other.distance2) {
                *this = other;
                return true;
            }

            return false;
        }

        [[nodiscard]] std::string toString() const;
    };

    /// Shortest routes from one crossing to the two ends of a road (original:
    /// _SafePkg_1543/_SafeCls_2201).
    struct RoutesToRoad {
        GraphNode* from = nullptr;   ///< Original: _SafeStr_2205.
        GraphNode* nodeA = nullptr;
        GraphNode* nodeB = nullptr;  ///< Like the original, set to nodeA as well.
        std::vector<Road*> routeToA; ///< Original: _SafeStr_2202.
        std::vector<Road*> routeToB; ///< Original: _SafeStr_2203.
        double lengthToA = 0;        ///< Original: _SafeStr_2204.
        double lengthToB = 0;        ///< Original: _SafeStr_2206.
    };

} // namespace takeover
