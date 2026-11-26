// LevelLogic.h - the rules of a battle, without any display (original:
// level/_SafeCls_2275).
//
// LevelLogic owns everything of a battle: combatants, squads, strongholds,
// build places, chests, the road graph and the banners. Each frame it
// updates them, recomputes incomes when ownership changes, removes dead
// squads, and decides victory or defeat. Squads ask it about the world
// through the ISquadLevel interface.
//
// The class chain of a battle is
//     LevelLogic -> LevelAi (computer players) -> LevelVisual (display) -> Level (input)
#pragma once

#include "game/sim/base/BaseObject.h"
#include "game/sim/base/TypedKey.h"
#include "game/sim/battle/Combatant.h"
#include "game/sim/battle/MissionStats.h"
#include "game/sim/level/KeyNode.h"
#include "game/sim/level/NodeContent.h"
#include "game/sim/level/RoadGraph.h"
#include "game/sim/squads/Squad.h"

#include <memory>
#include <optional>
#include <vector>

namespace takeover {

    class Spell;
    struct SplashArea;

    /// Something living on the battlefield for a while, such as a banner
    /// (original: interface level/_SafeCls_779).
    class ILevelObject {
    public:
        virtual ~ILevelObject() = default;
        virtual void update(double dt) = 0;
        [[nodiscard]] virtual bool isFinished() const = 0;   ///< _SafeStr_782
        [[nodiscard]] virtual const TypedKey& getKey() const = 0; ///< _SafeStr_781
        virtual void dispose() = 0;
    };

    /// Calls `f` on every object of a storage. The keys are taken first, so
    /// `f` may remove objects (like the original Haxe map iterators).
    /// Each object is held by a copy of its pointer while `f` runs: when `f`
    /// removes it from the storage, it stays alive until `f` returns.
    template <typename T, typename F>
    void forEachObject(const KeyedStorage<T>& storage, F&& f) {
        std::vector<int> keys;
        keys.reserve(storage.objects().size());

        for (const auto& entry : storage.objects())
            keys.push_back(entry.first);

        for (const int key : keys) {
            const auto it = storage.objects().find(key);

            if (it == storage.objects().end() || !it->second)
                continue;

            const std::shared_ptr<T> object = it->second;
            f(object);
        }
    }

    class LevelLogic : public BaseObject, public ISquadLevel {
    public:
        LevelLogic() = default;
        explicit LevelLogic(std::shared_ptr<MissionResult> result) { init(std::move(result)); }
        LevelLogic& init(std::shared_ptr<MissionResult> result); ///< _SafeStr_2335

        /// One frame: strongholds, squads, banners (original: update).
        void update(double dt) override;

        // /----- I SQUAD LEVEL -----/

        bool tryEnterNode(Squad& squad) override;                       ///< _SafeStr_1238
        void onSquadStopped(Squad& squad) override;                     ///< _SafeStr_1236
        void onSquadMoraleChanged(Squad& squad, MoraleStatus status) override; ///< _SafeStr_1242
        Point2D positionOfNode(const TypedKey& node) override;          ///< _SafeStr_2274
        KeyNode* nodeAt(const Point2D& point) override;                 ///< _SafeStr_1212
        KeyNode* enemyNodeInRange(Squad& squad) override;               ///< _SafeStr_1225
        std::vector<Squad*> enemySquadsInRange(Squad& squad) override;  ///< _SafeStr_1226
        std::optional<std::vector<Point2D>> findPath(const Point2D& from, const Point2D& to) override; ///< _SafeStr_1230

        // /----- SETUP -----/

        void addCombatant(std::shared_ptr<Combatant> combatant); ///< _SafeStr_2364
        void addSquad(std::shared_ptr<Squad> squad);            ///< _SafeStr_2322
        void setLocationOwner(int location, Race race);         ///< _SafeStr_2327

        /// Computes the road links between all the strongholds (original: _SafeStr_2366).
        void linkKeyNodes();

        /// Recomputes the stronghold stats of a combatant with its edicts (original: _SafeStr_2370).
        void reinitNodes(Combatant* combatant);
        void clearNodeIncomes(Combatant* combatant);            ///< _SafeStr_2380

        // /----- QUERIES -----/

        [[nodiscard]] MissionResult* getMissionResult() const { return result_.get(); } ///< _SafeStr_2362
        [[nodiscard]] const std::shared_ptr<MissionResult>& getMissionResultPtr() const { return result_; }
        [[nodiscard]] KeyedStorage<Combatant>& getCombatants() { return combatants_; } ///< _SafeStr_1352
        [[nodiscard]] KeyedStorage<Squad>& getSquads() { return squads_; }
        [[nodiscard]] KeyedStorage<KeyNode>& getKeyNodes() { return keyNodes_; }       ///< _SafeStr_2361
        [[nodiscard]] KeyedStorage<Chest>& getChests() { return chests_; }             ///< _SafeStr_2373
        [[nodiscard]] KeyedStorage<NodeContent>& getBuildPlaces() { return buildPlaces_; } ///< _SafeStr_2350
        [[nodiscard]] KeyedStorage<GraphNode>& getGraphNodes() { return graphNodes_; } ///< _SafeStr_2281
        [[nodiscard]] KeyedStorage<Road>& getRoads() { return roads_; }                ///< _SafeStr_2377
        [[nodiscard]] KeyedStorage<ILevelObject>& getLevelObjects() { return levelObjects_; }
        [[nodiscard]] const std::vector<std::optional<Race>>& getLocationOwners() const { return locationOwners_; } ///< _SafeStr_2383
        [[nodiscard]] std::optional<Race> getLocationOwnerRace(int location) const;   ///< _SafeStr_2324

        /// The owner of the location of a crossing (original: _SafeStr_1346).
        [[nodiscard]] Combatant* getLocationOwner(const NodeKey& node);
        [[nodiscard]] Combatant* getCombatantByRace(Race race);                         ///< _SafeStr_2299
        [[nodiscard]] Combatant* getCombatant(const CombatantKey* key) { return combatants_.getObject(key); }
        [[nodiscard]] Combatant* getPlayer() { return combatants_.getObject(playerKey_.get()); }
        [[nodiscard]] const std::shared_ptr<CombatantKey>& getPlayerKey() const { return playerKey_; }
        void setPlayerKey(std::shared_ptr<CombatantKey> key) { playerKey_ = std::move(key); }
        /// The combatant's stronghold closest to a point (original: _SafeStr_2360).
        [[nodiscard]] KeyNode* nearestOwnNode(const Combatant& combatant, const Point2D& point);
        [[nodiscard]] KeyNode* findKeyNode(const NodeKey& node);                         ///< _SafeStr_2330
        [[nodiscard]] std::vector<std::shared_ptr<KeyNode>> keyNodesOf(const CombatantKey* owner); ///< _SafeStr_2287
        [[nodiscard]] std::vector<std::shared_ptr<KeyNode>> keyNodesNotOf(const CombatantKey* owner); ///< _SafeStr_2372
        [[nodiscard]] std::vector<NodeContent*> buildPlacesAt(int location);            ///< _SafeStr_2302
        [[nodiscard]] std::vector<Squad*> defendersOf(const NodeKey& node);             ///< _SafeStr_2382
        [[nodiscard]] std::vector<Squad*> squadsOfNode(const NodeKey& node);            ///< _SafeStr_2354
        [[nodiscard]] std::vector<Road*> roadsOf(const NodeKey& node);                  ///< _SafeStr_2306
        [[nodiscard]] GraphNode* nearestGraphNode(const Point2D& point);                ///< _SafeStr_2305
        [[nodiscard]] RoadPoint nearestRoadPoint(const Point2D& point);                 ///< _SafeStr_2300

        /// Road path between two points, or nothing if `to` is farther than
        /// sqrt(maxDistance2)... from the roads (original: _SafeStr_2298).
        [[nodiscard]] std::optional<std::vector<Point2D>> findRoadPath(const Point2D& from, const Point2D& to,
                                                                       double maxDistance2);
        /// Road path between two crossings (original: _SafeStr_2363).
        [[nodiscard]] std::vector<Point2D> findNodePath(GraphNode& from, GraphNode& to);

        /// Road length between two strongholds (original: _SafeStr_2310).
        [[nodiscard]] double roadDistance(const KeyNode& a, const KeyNode& b);

        /// Supply used by a combatant's living squads (original: _SafeStr_2341).
        [[nodiscard]] int armyUpkeep(const CombatantKey* owner);
        virtual SupplyBuildingType getBuildPlaceType(NodeContent& /*place*/) { return SupplyBuildingType::Empty; }

        // /----- ACTIONS -----/

        bool buySquad(std::shared_ptr<Squad> squad, Combatant& combatant); ///< _SafeStr_2343
        virtual void buildSupplyBuilding(NodeContent& place, SupplyBuildingType type);
        virtual void captureNode(const std::shared_ptr<CombatantKey>& newOwner, KeyNode& node);
        virtual void castUltimateSpell(Combatant& combatant) { combatant.castUltimateSpell(); }
        virtual void castSpell(const Spell* spell, Combatant* combatant);
        virtual void killSquad(Squad& squad);

        /// Recomputes every combatant's incomes (original: _SafeStr_2292).
        void refreshAllIncomes();
        void refreshIncome(Combatant* combatant); ///< _SafeStr_2296

        /// Stone worms: damages the enemy strongholds in a circle (original: _SafeStr_2351).
        void damageNodesInRadius(const Point2D& point, double radius, int percent, const CombatantKey* owner);

        /// Damages the enemy soldiers in a circle by min..max percent of their
        /// HP (original: _SafeStr_2369).
        void damageUnitsInRadius(int minPercent, int maxPercent, int radius, const Point2D& point, Race race);

        /// Hits every soldier of each enemy squad touched by the area (original: _SafeStr_2356).
        void applySplashToSquads(const SplashArea& area, std::optional<Race> race);

        /// Hits the soldiers in the area, pushing them back unless `noPush`;
        /// friends too with `friendlyFire` (original: _SafeStr_2319).
        void applySplash(const SplashArea& area, std::optional<Race> race, bool noPush = false, bool friendlyFire = false);

    protected:
        void customDispose() override;

        // Hooks for the presentation.
        virtual void resetBuildPlace(NodeContent& /*place*/) {}
        virtual void onChestCollected(Squad& /*squad*/, Chest& /*chest*/) {} ///< _SafeStr_2312
        virtual void invalidateStatistics() {}
        virtual void createSplashDamadgeEffectFrom(const SplashArea& /*area*/, const ArmyStat* /*stat*/) {}
        virtual bool checkNodeActivState(KeyNode& node) { return node.hasStateChanged(); }

        void updateLevelObjects(double dt); ///< _SafeStr_2315
        void updateSquads(double dt);       ///< _SafeStr_2329
        void updateNodes(double dt);        ///< _SafeStr_2309

        /// A stronghold was hit: its owner's squads that guard it strike back (original: tryCounterStrike).
        void tryCounterStrike(KeyNode& node, int attackerSquadId);
        void onSplashAttack(int, BaseObject*, const std::any& data); ///< _SafeStr_1015
        void resetBuildPlacesOf(KeyNode& node); ///< _SafeStr_2321

        /// Victory when the player owns every stronghold, defeat when none
        /// (original: _SafeStr_2337).
        void checkMissionOver();

        // Shortest paths on the road graph (a plain recursive relaxation).
        void resetPathSearch();             ///< _SafeStr_2293
        void resetDistances();              ///< _SafeStr_2355
        void resetSearch();                 ///< _SafeStr_2378
        void propagateDistances(GraphNode& node, GraphNode& target, double distance); ///< _SafeStr_2290
        Road* bestRoadBack(GraphNode& node);               ///< _SafeStr_2316
        std::vector<Road*> routeTo(GraphNode& node);       ///< _SafeStr_2294
        static double routeLength(const std::vector<Road*>& route); ///< _SafeStr_2295
        RoutesToRoad routesToRoad(GraphNode& from, GraphNode& a, GraphNode& b); ///< _SafeStr_2307
        std::vector<Point2D> appendRoutePoints(const std::vector<Road*>& route, GraphNode& from,
                                               std::vector<Point2D> points = {}); ///< _SafeStr_2303
        static void appendRoadPoints(std::vector<Point2D>& points, const Road& road, int index0, int index1,
                                     bool towardA); ///< _SafeStr_2301
        std::shared_ptr<NodeLink> makeLink(KeyNode& from, KeyNode& to); ///< _SafeStr_2323

        GraphNode* graphNode(const NodeKey* key) { return graphNodes_.getObject(key); }

        std::shared_ptr<MissionResult> result_;   ///< Original: _SafeStr_2280.
        KeyedStorage<ILevelObject> levelObjects_; ///< Banners (original: _SafeStr_2286).
        KeyedStorage<Squad> squads_;              ///< Original: _SafeStr_2278.
        KeyedStorage<Road> roads_;                ///< Original: _SafeStr_2284.
        std::shared_ptr<CombatantKey> playerKey_; ///< Original: _SafeStr_2288.
        KeyedStorage<GraphNode> graphNodes_;      ///< Original: _SafeStr_2277.

        /// Owner of each location (original: _SafeStr_2282).
        std::vector<std::optional<Race>> locationOwners_;
        KeyedStorage<KeyNode> keyNodes_;          ///< Original: _SafeStr_2279.
        KeyedStorage<Combatant> combatants_;      ///< Original: _SafeStr_2276.
        KeyedStorage<Chest> chests_;              ///< Original: _SafeStr_2285.
        KeyedStorage<NodeContent> buildPlaces_;   ///< Original: _SafeStr_2283.
    };

} // namespace takeover
