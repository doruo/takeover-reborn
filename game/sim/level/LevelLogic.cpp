#include "game/sim/level/LevelLogic.h"

#include "game/sim/abilities/AbilityEffects.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/sim/spells/SpellInfo.h"
#include "game/sim/units/Unit.h"

#include "game/framework/core/As3.h"

#include <algorithm>
#include <cmath>

namespace takeover {

    LevelLogic& LevelLogic::init(std::shared_ptr<MissionResult> result) {
        result_ = std::move(result);
        return *this;
    }

    // /----- FRAME -----/

    void LevelLogic::update(double dt) {
        BaseObject::update(dt);
        updateNodes(dt);
        updateSquads(dt);
        updateLevelObjects(dt);
        MissionStats& stats = *getPlayer()->missionStats;
        if (stats.hasChanged()) {
            invalidateStatistics();
            stats.markUnchanged();
        }
    }

    void LevelLogic::updateLevelObjects(double dt) {
        forEachObject(levelObjects_, [&](const std::shared_ptr<ILevelObject>& object) {
            object->update(dt);

            if (object->isFinished()) {
                levelObjects_.removeObject(object->getKey());
                object->dispose();
            }
        });
    }

    void LevelLogic::updateSquads(double dt) {
        result_->missionTime += dt;

        for (const auto& [index, combatant] : combatants_.objects())
            combatant->setSquadsCount(0);

        forEachObject(squads_, [&](const std::shared_ptr<Squad>& squad) {
            squad->update(dt);

            if (squad->getOwner() != nullptr) {
                Combatant* owner = combatants_.getObject(squad->getOwner().get());

                if (squad->getHP() > 0)
                    owner->incrementSquadsCount();
                else if (squad->hpChanged())
                    refreshIncome(owner);
            }

            if (squad->isReadyToRemove())
                killSquad(*squad);
        });

        getPlayer()->recordSquadsOwned();
    }

    void LevelLogic::updateNodes(double dt) {
        bool changed = false;

        forEachObject(keyNodes_, [&](const std::shared_ptr<KeyNode>& node) {
            const int attacker = node->getLastAttackerSquadId();
            if (attacker >= 0)
                tryCounterStrike(*node, attacker);

            node->update(dt);
            changed = checkNodeActivState(*node) || changed;
            node->attackers.clear();
            node->defenders.clear();
        });

        if (changed)
            refreshAllIncomes();

        forEachObject(chests_, [&](const std::shared_ptr<Chest>& chest) { chest->update(dt); });
        forEachObject(buildPlaces_, [&](const std::shared_ptr<NodeContent>& place) { place->update(dt); });
    }

    void LevelLogic::refreshAllIncomes() {
        forEachObject(combatants_, [&](const std::shared_ptr<Combatant>& combatant) { refreshIncome(combatant.get()); });
    }

    void LevelLogic::refreshIncome(Combatant* combatant) {
        if (combatant == nullptr)
            return;

        int gold = 0, mana = 0, supply = 0, ultimate = 0;
        int villages = 0, mines = 0, obelisks = 0;

        for (const auto& node : keyNodesOf(combatant->description.get())) {
            if (node->isActive()) {
                gold += node->goldIncome;
                mana += node->manaIncome;
                supply += node->supplyIncome;
                ultimate += node->ultimateIncome;
            }

            const int location = graphNode(node->nodeDescription.get())->locationNum;
            for (NodeContent* place : buildPlacesAt(location)) {
                gold += place->goldIncome;
                mana += place->manaIncome;
                supply += place->supplyIncome;
                ultimate += place->ultimateIncome;

                switch (getBuildPlaceType(*place)) {
                case SupplyBuildingType::Village: ++villages; break;
                case SupplyBuildingType::Mine: ++mines; break;
                case SupplyBuildingType::Obelisk: ++obelisks; break;
                case SupplyBuildingType::Empty: break;
                }
            }
        }

        combatant->recordBuildings(villages, mines, obelisks);
        combatant->setIncome(gold, supply, armyUpkeep(combatant->description.get()), mana, ultimate);
    }

    void LevelLogic::tryCounterStrike(KeyNode& node, int attackerSquadId) {
        auto* attacker = dynamic_cast<Squad*>(BaseObject::getObjectPtr(attackerSquadId));
        if (attacker == nullptr || attacker->isDead())
            return;

        forEachObject(squads_, [&](const std::shared_ptr<Squad>& squad) {
            if (squad->isDead())
                return;

            KeyNode* home = squad->getKeyNode();
            if (home != nullptr && squad->getOwner() != nullptr && node.owner->compare(*squad->getOwner()) &&
                node.description->compare(*home->description))
                squad->commandMove(attacker->getMainUnit()->getPosition());
        });
    }

    // /----- I SQUAD LEVEL -----/

    bool LevelLogic::tryEnterNode(Squad& squad) {
        KeyNode* node = squad.getKeyNode();
        if (node->isRuined()) {
            captureNode(squad.getOwner(), *node);
            return true;
        }

        return false;
    }

    void LevelLogic::onSquadStopped(Squad& squad) {
        Unit* unit = squad.getMainUnit();
        if (unit == nullptr)
            return;

        GraphNode* crossing = nearestGraphNode(unit->getPosition());
        if (crossing == nullptr)
            return;

        for (const auto& [index, chest] : chests_.objects()) {
            if (crossing->description->compare(*chest->nodeDescription)) {
                const std::shared_ptr<CombatantKey>& owner = squad.getOwner();
                if (owner != nullptr && !chest->isCollected()) {
                    chest->collect(squad, *combatants_.getObject(owner.get()));
                    onChestCollected(squad, *chest);
                }

                return;
            }
        }
    }

    void LevelLogic::onSquadMoraleChanged(Squad& squad, MoraleStatus status) {
        if (status == MoraleStatus::Heroism) {
            Combatant* combatant = combatants_.getObject(squad.getOwner().get());
            if (combatant != nullptr)
                combatant->onHeroicMode(squad);
        }
    }

    Point2D LevelLogic::positionOfNode(const TypedKey& node) { return graphNodes_.getObject(&node)->pos.getCopy(); }

    KeyNode* LevelLogic::nodeAt(const Point2D& point) {
        GraphNode* crossing = nearestGraphNode(point);
        return crossing != nullptr ? findKeyNode(*crossing->description) : nullptr;
    }

    KeyNode* LevelLogic::enemyNodeInRange(Squad& squad) {
        if (squad.isDead())
            return nullptr;

        Unit* unit = squad.getMainUnit();
        if (unit == nullptr)
            return nullptr;

        const Point2D position = unit->getPosition();
        const double range2 = std::pow(squad.engageDistance() + 40, 2);
        const std::shared_ptr<CombatantKey>& owner = squad.getOwner();
        if (owner == nullptr)
            return nullptr;

        for (const auto& [index, node] : keyNodes_.objects()) {
            if (node->isDestroyed() || owner->compare(*node->owner))
                continue;

            const double d2 = position.getDistanceTo(node->getPosition(), true) + node->getHitRadius() * node->getHitRadius();
            if (d2 <= range2)
                return node.get();
        }

        return nullptr;
    }

    std::vector<Squad*> LevelLogic::enemySquadsInRange(Squad& squad) {
        std::vector<Squad*> found;

        for (const auto& [index, other] : squads_.objects())
            if (squad.canEngage(*other))
                found.push_back(other.get());
        return found;
    }

    std::optional<std::vector<Point2D>> LevelLogic::findPath(const Point2D& from, const Point2D& to) {
        return findRoadPath(from, to, 10000);
    }

    // /----- SETUP -----/

    void LevelLogic::addCombatant(std::shared_ptr<Combatant> combatant) {
        combatants_.addObject(combatant, *combatant->description);

        if (combatant->description == playerKey_)
            result_->playerStats = combatant->missionStats;
        else
            result_->allStats.push_back(combatant->missionStats);
    }

    void LevelLogic::addSquad(std::shared_ptr<Squad> squad) {
        squads_.addObject(squad, squad->getKey());
        squad->addEventListener(Unit::EVENT_SPLASH_ATTACK, getId(),
                                [this](int t, BaseObject* s, const std::any& d) { onSplashAttack(t, s, d); });
        refreshAllIncomes();
    }

    void LevelLogic::setLocationOwner(int location, Race race) {
        while (location >= static_cast<int>(locationOwners_.size()))
            locationOwners_.emplace_back();

        locationOwners_[static_cast<std::size_t>(location)] = race;
    }

    void LevelLogic::linkKeyNodes() {
        for (const auto& [i, from] : keyNodes_.objects())
            for (const auto& [j, to] : keyNodes_.objects())
                if (from != to)
                    from->getLinks().addObject(makeLink(*from, *to), *to->description);
        for (const auto& [index, combatant] : combatants_.objects())
            combatant->setKeyNodes(keyNodesOf(combatant->description.get()));
    }

    std::shared_ptr<NodeLink> LevelLogic::makeLink(KeyNode& from, KeyNode& to) {
        auto link = std::make_shared<NodeLink>();
        link->path = findRoadPath(from.getPosition(), to.getPosition(), 30);
        link->dist = roadDistance(from, to);
        link->keyNode = &to;

        return link;
    }

    void LevelLogic::reinitNodes(Combatant* combatant) {
        if (combatant == nullptr)
            return;

        for (const auto& node : keyNodesOf(combatant->description.get()))
            node->initByType(node->getType(), *combatant);
    }

    void LevelLogic::clearNodeIncomes(Combatant* combatant) {
        if (combatant == nullptr)
            return;

        for (const auto& node : keyNodesOf(combatant->description.get())) {
            node->goldIncome = 0;
            node->manaIncome = 0;
            node->supplyIncome = 0;
            node->ultimateIncome = 0;
        }
    }

    // /----- QUERIES -----/

    std::optional<Race> LevelLogic::getLocationOwnerRace(int location) const {
        if (location < 0 || location >= static_cast<int>(locationOwners_.size()))
            return std::nullopt;

        return locationOwners_[static_cast<std::size_t>(location)];
    }

    Combatant* LevelLogic::getLocationOwner(const NodeKey& node) {
        const std::optional<Race> race = getLocationOwnerRace(graphNode(&node)->locationNum);
        return race ? getCombatantByRace(*race) : nullptr;
    }

    Combatant* LevelLogic::getCombatantByRace(Race race) {
        for (const auto& [index, combatant] : combatants_.objects())
            if (combatant->getRace() == race)
                return combatant.get();
        return nullptr;
    }

    KeyNode* LevelLogic::nearestOwnNode(const Combatant& combatant, const Point2D& point) {
        double best = 9999999999;
        KeyNode* nearest = nullptr;

        for (const auto& [index, node] : keyNodes_.objects()) {
            if (node->owner != combatant.description)
                continue;

            const double d = point.getDistanceTo(node->getPosition());
            if (d < best)
                best = d, nearest = node.get();
        }

        return nearest;
    }

    KeyNode* LevelLogic::findKeyNode(const NodeKey& key) {
        for (const auto& [index, node] : keyNodes_.objects())
            if (key.compare(*node->nodeDescription))
                return node.get();
        return nullptr;
    }

    std::vector<std::shared_ptr<KeyNode>> LevelLogic::keyNodesOf(const CombatantKey* owner) {
        std::vector<std::shared_ptr<KeyNode>> found;

        for (const auto& [index, node] : keyNodes_.objects())
            if (node->owner.get() == owner)
                found.push_back(node);
        return found;
    }

    std::vector<std::shared_ptr<KeyNode>> LevelLogic::keyNodesNotOf(const CombatantKey* owner) {
        std::vector<std::shared_ptr<KeyNode>> found;

        for (const auto& [index, node] : keyNodes_.objects())
            if (node->owner.get() != owner)
                found.push_back(node);
        return found;
    }

    std::vector<NodeContent*> LevelLogic::buildPlacesAt(int location) {
        std::vector<NodeContent*> found;

        for (const auto& [index, place] : buildPlaces_.objects())
            if (graphNode(place->nodeDescription.get())->locationNum == location)
                found.push_back(place.get());
        return found;
    }

    std::vector<Squad*> LevelLogic::defendersOf(const NodeKey& node) {
        std::vector<Squad*> found;

        for (const auto& [index, squad] : squads_.objects())
            if (squad->getRole() == SquadRole::Defender && node.compare(*squad->getKeyNode()->nodeDescription))
                found.push_back(squad.get());
        return found;
    }

    std::vector<Squad*> LevelLogic::squadsOfNode(const NodeKey& node) {
        std::vector<Squad*> found;

        for (const auto& [index, squad] : squads_.objects())
            if (squad->getKeyNode() != nullptr && squad->getKeyNode()->nodeDescription.get() == &node)
                found.push_back(squad.get());
        return found;
    }

    std::vector<Road*> LevelLogic::roadsOf(const NodeKey& node) {
        std::vector<Road*> found;

        for (const auto& [index, road] : roads_.objects())
            if (node.compare(*road->nodeA) || node.compare(*road->nodeB))
                found.push_back(road.get());
        return found;
    }

    GraphNode* LevelLogic::nearestGraphNode(const Point2D& point) {
        double best = 99999;
        GraphNode* nearest = nullptr;

        for (const auto& [index, crossing] : graphNodes_.objects()) {
            const double d = crossing->pos.getDistanceTo(point);
            if (d < best && !roadsOf(*crossing->description).empty())
                best = d, nearest = crossing.get();
        }

        return nearest;
    }

    RoadPoint LevelLogic::nearestRoadPoint(const Point2D& point) {
        RoadPoint best, candidate;

        for (const auto& [index, roadPtr] : roads_.objects()) {
            Road* road = roadPtr.get();
            if (road->wayPoints.empty())
                continue;

            Point2D current = road->wayPoints[0];
            candidate.set(current, point.getDistanceTo(current, true), road, 0, 0);
            best.keepCloser(candidate);

            for (int i = 1; i < static_cast<int>(road->wayPoints.size()); ++i) {
                const Point2D previous = current;
                current = road->wayPoints[static_cast<std::size_t>(i)];
                const Point2D foot = geometry::projectOnLine(point, previous, current);
                if (geometry::isInsideBox(foot, previous, current)) {
                    candidate.set(foot, point.getDistanceTo(foot, true), road, i - 1, i);
                    best.keepCloser(candidate);
                }

                candidate.set(current, point.getDistanceTo(current, true), road, i, i);
                best.keepCloser(candidate);
            }
        }

        return best;
    }

    std::optional<std::vector<Point2D>> LevelLogic::findRoadPath(const Point2D& from, const Point2D& to,
                                                                 double maxDistance2) {
        std::vector<Point2D> points;
        const RoadPoint start = nearestRoadPoint(from);
        const RoadPoint end = nearestRoadPoint(to);
        if (end.distance2 > maxDistance2)
            return std::nullopt;

        if (start.road == end.road) {
            // Along one road.
            points.push_back(start.point.getCopy());

            if (start.roadIndex0 != end.roadIndex0) {
                if (start.roadIndex0 < end.roadIndex0) {
                    for (int i = start.roadIndex1; i < end.roadIndex1; ++i)
                        points.push_back(start.road->wayPoints[static_cast<std::size_t>(i)].getCopy());
                } else {
                    // Original quirk: walking backwards along the road adds no
                    // way point at all (this loop is empty).
                    for (int i = start.roadIndex0; i < end.roadIndex0; ++i)
                        points.push_back(start.road->wayPoints[static_cast<std::size_t>(i)].getCopy());
                }
            }

            points.push_back(end.point.getCopy());
            return points;
        }

        // From an end of the start road to an end of the destination road: the
        // shortest of the four combinations.
        GraphNode& startA = *graphNode(start.road->nodeA.get());
        GraphNode& startB = *graphNode(start.road->nodeB.get());
        GraphNode& endA = *graphNode(end.road->nodeA.get());
        GraphNode& endB = *graphNode(end.road->nodeB.get());
        RoutesToRoad fromA = routesToRoad(startA, endA, endB);
        RoutesToRoad fromB = routesToRoad(startB, endA, endB);
        const double aa = fromA.lengthToA, ab = fromA.lengthToB, ba = fromB.lengthToA, bb = fromB.lengthToB;
        GraphNode* first;
        GraphNode* last;
        const std::vector<Road*>* route;

        if (aa <= ab && aa <= ba && aa <= bb)
            first = &startA, last = &endA, route = &fromA.routeToA;
        else if (ab <= aa && ab <= ba && ab <= bb)
            first = &startA, last = &endB, route = &fromA.routeToB;
        else if (ba <= aa && ba <= ab && ba <= bb)
            first = &startB, last = &endA, route = &fromB.routeToA;
        else
            first = &startB, last = &endB, route = &fromB.routeToB;

        appendRoadPoints(points, *start.road, start.roadIndex0, start.roadIndex1, first == &startA);
        points.push_back(start.point.getCopy());
        std::reverse(points.begin(), points.end());
        points = appendRoutePoints(*route, *first, std::move(points));
        appendRoadPoints(points, *end.road, end.roadIndex0, end.roadIndex1, last == &endA);
        points.push_back(end.point.getCopy());

        return points;
    }

    std::vector<Point2D> LevelLogic::findNodePath(GraphNode& from, GraphNode& to) {
        resetPathSearch();
        propagateDistances(from, to, 0);

        return appendRoutePoints(routeTo(to), from);
    }

    double LevelLogic::roadDistance(const KeyNode& a, const KeyNode& b) {
        GraphNode& from = *graphNode(a.nodeDescription.get());
        GraphNode& to = *graphNode(b.nodeDescription.get());
        resetPathSearch();
        propagateDistances(from, to, 0);

        return routeLength(routeTo(to));
    }

    int LevelLogic::armyUpkeep(const CombatantKey* owner) {
        int upkeep = 0;
        const Combatant* combatant = combatants_.getObject(owner);
        if (combatant != nullptr) {
            for (const auto& [index, squad] : squads_.objects())
                if (squad->getRace() == combatant->getRace() && squad->getHP() > 0)
                    upkeep += squad->getArmyStat()->upkeep;
        }

        return upkeep;
    }

    // /----- ROAD GRAPH -----/

    void LevelLogic::resetPathSearch() {
        for (const auto& [index, crossing] : graphNodes_.objects()) {
            crossing->search.reset();
            crossing->search.roads = roadsOf(*crossing->description);
        }
    }

    void LevelLogic::resetDistances() {
        for (const auto& [index, crossing] : graphNodes_.objects())
            crossing->search.distance = 999999;
    }

    void LevelLogic::resetSearch() {
        for (const auto& [index, crossing] : graphNodes_.objects())
            crossing->search.reset();
    }

    void LevelLogic::propagateDistances(GraphNode& node, GraphNode& target, double distance) {
        if (node.search.distance > distance) {
            node.search.distance = distance;

            if (&node != &target) {
                for (Road* road : node.search.roads)
                    propagateDistances(*graphNode(road->otherEnd(node.description).get()), target, road->length + distance);
            }
        }
    }

    Road* LevelLogic::bestRoadBack(GraphNode& node) {
        double best = 999999;
        Road* bestRoad = nullptr;

        for (Road* road : node.search.roads) {
            GraphNode& other = *graphNode(road->otherEnd(node.description).get());
            if (best > other.search.distance)
                best = other.search.distance, bestRoad = road;
        }

        return bestRoad;
    }

    std::vector<Road*> LevelLogic::routeTo(GraphNode& node) {
        std::vector<Road*> route;
        GraphNode* current = &node;
        while (current->search.distance > 0) {
            Road* road = bestRoadBack(*current);
            if (road == nullptr)
                break;

            route.push_back(road);
            current = graphNode(road->otherEnd(current->description).get());
        }

        return route;
    }

    double LevelLogic::routeLength(const std::vector<Road*>& route) {
        double total = 0;

        for (const Road* road : route)
            total += road->length;

        return total;
    }

    RoutesToRoad LevelLogic::routesToRoad(GraphNode& from, GraphNode& a, GraphNode& b) {
        RoutesToRoad routes;
        routes.from = &from;
        routes.nodeA = &a;
        routes.nodeB = &a;
        resetPathSearch();
        propagateDistances(from, a, 0);
        routes.routeToA = routeTo(a);
        routes.lengthToA = routeLength(routes.routeToA);
        resetPathSearch();
        propagateDistances(from, b, 0);
        routes.routeToB = routeTo(b);
        routes.lengthToB = routeLength(routes.routeToB);

        return routes;
    }

    std::vector<Point2D> LevelLogic::appendRoutePoints(const std::vector<Road*>& route, GraphNode& from,
                                                       std::vector<Point2D> points) {
        // routeTo() lists the roads from the destination back to the start:
        // walk them in reverse.
        points.push_back(from.pos.getCopy());
        std::shared_ptr<NodeKey> at = from.description;

        for (std::size_t i = 0; i < route.size(); ++i) {
            const Road& road = *route[route.size() - i - 1];
            at = road.otherEnd(at);
            const int count = static_cast<int>(road.wayPoints.size());

            if (at == road.nodeB) {
                for (int k = 1; k < count; ++k)
                    points.push_back(road.wayPoints[static_cast<std::size_t>(k)].getCopy());
            } else {
                for (int k = 1; k < count; ++k)
                    points.push_back(road.wayPoints[static_cast<std::size_t>(count - k - 1)].getCopy());
            }
        }

        return points;
    }

    void LevelLogic::appendRoadPoints(std::vector<Point2D>& points, const Road& road, int index0, int index1,
                                      bool towardA) {
        const int count = static_cast<int>(road.wayPoints.size());

        if (towardA) {
            for (int i = 0; i < index0 + 1; ++i)
                points.push_back(road.wayPoints[static_cast<std::size_t>(i)].getCopy());
        } else {
            for (int i = 0; i < count - index1; ++i)
                points.push_back(road.wayPoints[static_cast<std::size_t>(count - i - 1)].getCopy());
        }
    }

    // /----- ACTIONS -----/

    bool LevelLogic::buySquad(std::shared_ptr<Squad> squad, Combatant& combatant) {
        if (combatant.canBuy(*squad->getArmyStat())) {
            combatant.buySquad(*squad);
            addSquad(std::move(squad));

            return true;
        }

        return false;
    }

    void LevelLogic::buildSupplyBuilding(NodeContent& place, SupplyBuildingType type) {
        Combatant* owner = getLocationOwner(*place.nodeDescription);
        owner->buildSupplyBuilding(type);
        place.initBySupplyBuilding(type, owner);
        refreshAllIncomes();
    }

    void LevelLogic::captureNode(const std::shared_ptr<CombatantKey>& newOwnerKey, KeyNode& node) {
        Combatant* newOwner = combatants_.getObject(newOwnerKey.get());
        Combatant* oldOwner = combatants_.getObject(node.owner.get());
        newOwner->onStrongholdSeized();
        oldOwner->onStrongholdLost();
        const int hp = node.getHP();
        node.initByType(node.getType(), *newOwner);
        node.setHP(hp);
        node.owner = newOwner->description;
        refreshAllIncomes();
        setLocationOwner(graphNode(node.nodeDescription.get())->locationNum, newOwner->getRace());
        newOwner->setKeyNodes(keyNodesOf(newOwner->description.get()));
        oldOwner->setKeyNodes(keyNodesOf(oldOwner->description.get()));
        resetBuildPlacesOf(node);
        checkMissionOver();
    }

    void LevelLogic::resetBuildPlacesOf(KeyNode& node) {
        for (NodeContent* place : buildPlacesAt(graphNode(node.nodeDescription.get())->locationNum))
            resetBuildPlace(*place);
    }

    void LevelLogic::checkMissionOver() {
        bool allMine = true, noneMine = true;

        for (const auto& [index, node] : keyNodes_.objects()) {
            if (node->owner != playerKey_)
                allMine = false;
            else
                noneMine = false;
        }

        if (allMine)
            result_->missionOverStatus = MissionResult::VICTORY;
        else if (noneMine)
            result_->missionOverStatus = MissionResult::DEFEAT;
        else
            result_->missionOverStatus = MissionResult::IN_PROGRESS;
    }

    void LevelLogic::castSpell(const Spell* spell, Combatant* combatant) {
        if (spell == nullptr || combatant == nullptr)
            return;

        switch (spell->type) {
        case SpellInfo::TYPE_SPELL: combatant->castNormalSpell(spell->cost); break;
        case SpellInfo::TYPE_BANNER: combatant->setBanner(spell->cost); break;
        default: break;
        }
    }

    void LevelLogic::killSquad(Squad& squad) {
        // Keep the squad alive until the end of this function.
        const std::shared_ptr<Squad> keep = squads_.getShared(&squad.getKey());
        Combatant* owner = getCombatantByRace(squad.getRace());
        Combatant* killer = combatants_.getObject(squad.getLastAttackerOwner().get());
        squads_.removeObject(squad.getKey());

        if (killer != nullptr)
            killer->onEnemySquadKilled(squad);

        if (owner != nullptr) {
            owner->onOwnSquadLost(squad);
            refreshIncome(owner);
        }

        squad.dispose();
    }

    void LevelLogic::damageNodesInRadius(const Point2D& point, double radius, int percent, const CombatantKey* owner) {
        const double radius2 = radius * radius;

        for (const auto& [index, node] : keyNodes_.objects()) {
            if (node->isDestroyed() || (owner != nullptr && owner->compare(*node->owner)))
                continue;

            const double d2 = point.getDistanceTo(node->getPosition(), true) - node->getHitRadius() * node->getHitRadius();
            if (d2 <= radius2)
                node->damagePercent(percent);
        }
    }

    void LevelLogic::damageUnitsInRadius(int minPercent, int maxPercent, int radius, const Point2D& point, Race race) {
        const double radius2 = static_cast<double>(radius) * radius;
        const int spread = maxPercent - minPercent;

        forEachObject(squads_, [&](const std::shared_ptr<Squad>& squad) {
            if (squad->getRace() == race)
                return;

            for (const auto& [i, unit] : squad->getUnits()) {
                if (point.getDistanceTo(unit->getPosition(), true) <= radius2) {
                    const int damage = static_cast<int>(std::ceil((minPercent + as3::random() * spread) * unit->getMaxHP() / 100));
                    EffectList effects{std::make_shared<AttackEffect>(-1, "ATK_DISTANCE", damage)};
                    unit->receiveAttack(effects, -1);
                }
            }
        });
    }

    void LevelLogic::applySplashToSquads(const SplashArea& area, std::optional<Race> race) {
        const auto* attacker = dynamic_cast<Unit*>(BaseObject::getObjectPtr(area.attackerId));
        int attackerSquadId = -1;

        if (attacker != nullptr) {
            attackerSquadId = attacker->getSquadId();
            if (const auto* squad = dynamic_cast<Squad*>(BaseObject::getObjectPtr(attackerSquadId)))
                race = squad->getRace();
        }

        const double radius2 = area.radius * area.radius;

        forEachObject(squads_, [&](const std::shared_ptr<Squad>& squad) {
            if (race && squad->getRace() == *race)
                return;

            bool touched = false;

            for (const auto& [i, unit] : squad->getUnits()) {
                if (unit->getSquadId() != attackerSquadId && area.position.getDistanceTo(unit->getPosition(), true) <= radius2) {
                    touched = true;
                    break;
                }
            }

            if (touched)
                for (const auto& [i, unit] : squad->getUnits())
                    unit->receiveAttack(area.copyEffects(), area.attackerId);
        });
    }

    void LevelLogic::onSplashAttack(int, BaseObject*, const std::any& data) {
        applySplash(*std::any_cast<std::shared_ptr<SplashArea>>(data), std::nullopt);
    }

    void LevelLogic::applySplash(const SplashArea& area, std::optional<Race> race, bool noPush, bool friendlyFire) {
        auto* attacker = dynamic_cast<Unit*>(BaseObject::getObjectPtr(area.attackerId));
        int attackerSquadId = -1;
        Squad* attackerSquad = nullptr;

        if (attacker != nullptr) {
            attackerSquadId = attacker->getSquadId();
            attackerSquad = dynamic_cast<Squad*>(BaseObject::getObjectPtr(attackerSquadId));
            if (attackerSquad != nullptr)
                race = attackerSquad->getRace();
        }

        const double radius2 = area.radius * area.radius;

        forEachObject(squads_, [&](const std::shared_ptr<Squad>& squad) {
            if (!friendlyFire && race && squad->getRace() == *race)
                return;

            for (const auto& [i, unit] : squad->getUnits()) {
                if (unit->getSquadId() == attackerSquadId)
                    continue;

                const Point2D position = unit->getPosition();
                if (area.position.getDistanceTo(position, true) > radius2)
                    continue;

                if (!noPush) {
                    // Giants hurl their victims away from themselves.
                    const double mass = std::sqrt(std::sqrt(unit->getAbilities().mass));

                    if (attacker != nullptr && attacker->getAbilities().size == UnitSize::Gaint)
                        unit->push(attacker->getPosition().angleTo(position), 15 / mass);
                    else
                        unit->push(area.position.angleTo(position), 13 / mass);
                }

                unit->receiveAttack(area.copyEffects(), area.attackerId);
            }
        });

        if (attacker != nullptr && attackerSquad != nullptr)
            createSplashDamadgeEffectFrom(area, attackerSquad->getArmyStat());
    }

    // /----- DISPOSE -----/

    void LevelLogic::customDispose() {
        forEachObject(levelObjects_, [](const std::shared_ptr<ILevelObject>& object) { object->dispose(); });
        levelObjects_.dispose();
        keyNodes_.dispose();
        chests_.dispose();
        buildPlaces_.dispose();
        graphNodes_.dispose();
        roads_.dispose();
        playerKey_.reset();
        forEachObject(squads_, [](const std::shared_ptr<Squad>& squad) { squad->dispose(); });
        squads_.dispose();
        forEachObject(combatants_, [](const std::shared_ptr<Combatant>& combatant) { combatant->dispose(); });
        combatants_.dispose();
        locationOwners_.clear();
        result_.reset();
        BaseObject::customDispose();
    }

} // namespace takeover
