#include "game/sim/squads/Squad.h"

#include "game/sim/battle/ArmyStat.h"
#include "game/sim/battle/Combatant.h"
#include "game/sim/level/KeyNode.h"
#include "game/sim/units/UnitCombat.h"

#include <cmath>
#include <limits>
#include <map>

namespace takeover {

Squad& Squad::init(const SquadDef& def) {
    key_ = std::make_shared<SquadKey>();
    level_ = def.level;
    SquadBase::init(def.owner, *def.armyStat, def.armyStatBonus);
    initFSM();
    clearCaches();
    return *this;
}

void Squad::initFSM() {
    state_ = STATE_IDLE;
    pendingEvent_ = 0;
    readyToRemove_ = false;
    // Movement (original: _SafeStr_1180).
    pendingDestination_.reset();
    destination_ = Point2D();
    retargetNodeOnArrival_ = false;
    holdPoint_.reset();
    // Healing inside a stronghold (original: _SafeStr_1167).
    nextHealTime_ = 0;
    healAmount_ = getMainUnit()->getHP() / 4 + 1;
}

// --- Orders --------------------------------------------------------------------------------

bool Squad::postEvent(int event) {
    if (event > pendingEvent_) {
        pendingEvent_ = event;
        return true;
    }
    return false;
}

bool Squad::commandMove(const Point2D& point) {
    if (postEvent(EV_MOVE)) {
        pendingDestination_ = point;
        checkEnemies();
        return true;
    }
    return false;
}

bool Squad::commandMoveHome(const Point2D& point) {
    if (postEvent(EV_MOVE_HOME)) {
        pendingDestination_ = point;
        return true;
    }
    return false;
}

void Squad::enterNode(KeyNode& node) {
    keyNode_ = &node;
    const Point2D position = node.getPosition();
    moveTo(position.x, position.y);
    changeFSMState(STATE_INSIDE);
}

// --- State machine ------------------------------------------------------------------------------

void Squad::updateState(int state, double dt) {
    switch (state) {
    case STATE_IDLE: updateIdle(dt); break;
    case STATE_MOVING: updateMoving(dt); break;
    case STATE_FIGHTING: updateFighting(dt); break;
    case STATE_DEAD: break;
    case STATE_HOLDING: updateHolding(dt); break;
    case STATE_INSIDE: updateInside(dt); break;
    }
}

void Squad::updateFSM(double dt) {
    updateState(state_, dt);
    if (pendingDestination_) postEvent(EV_NEW_DESTINATION);
    checkDeath();
    if (pendingEvent_ != 0) {
        const int event = pendingEvent_;
        pendingEvent_ = 0;
        handleEvent(state_, event);
    }
    actionState_ = actionStatus(state_);
    switch (getRole()) {
    case SquadRole::Defender: keyNode_->defenders.push_back(this); break;
    case SquadRole::Attacker: keyNode_->attackers.push_back(this); break;
    default: break;
    }
}

void Squad::updateIdle(double) {
    if (holdPoint_) postHold();
    checkEnemies();
}

void Squad::updateHolding(double) { allArrived_ = allArrived(); }

void Squad::updateFighting(double dt) {
    const int count = getUnitCount();
    for (int i = 0; i < count; ++i) {
        Unit* unit = unitAt(count - 1 - i);
        if (unit->isStanding() || unit->isGoing() || unit->isAttackingBuilding())
            if (!chooseTarget(*unit)) postArrived();
    }
    separateUnits(dt);
}

void Squad::updateMoving(double) {
    if (!destination_) {
        stopMoving();
        return;
    }
    if (getMainUnit()->hasArrived()) nextWaypoint();
    keepFormation();
    checkEnemies();
}

void Squad::updateInside(double) {
    if (keyNode_ == nullptr) {
        postArrived();
        return;
    }
    if (getActiveTime() >= nextHealTime_) {
        nextHealTime_ = getActiveTime() + 0.5;
        if (getHP() < getMaxHP()) heal(healAmount_);
        if (actionState_ == SquadState::Repairing) keyNode_->activate(5);
    }
}

bool Squad::allArrived() {
    bool all = true;
    for (auto& [index, unit] : units_) {
        if (!unit->hasArrived()) {
            all = false;
            break;
        }
    }
    keepFormation();
    return all;
}

void Squad::separateUnits(double) {
    Unit* leader = getMainUnit();
    const int count = getUnitCount();
    for (int i = 0; i < count; ++i) {
        Unit* unit = unitAt(i);
        const double mass = unit->getAbilities().mass;
        const double radius = unit->getAbilities().meleeRadius();
        if (unit == leader) {
            // The leader pulls the others when they are too far.
            for (int j = 0; j < count; ++j) {
                Unit* other = unitAt(j);
                if (other->isDead() || other == leader) continue;
                const double distance = (radius + other->getAbilities().meleeRadius()) * 4;
                unit->pullCloser(*other, distance, 0.3 * mass / other->getAbilities().mass);
            }
        } else if (!unit->isDead()) {
            for (int j = 0; j < count; ++j) {
                Unit* other = unitAt(j);
                if (other->isDead() || other == leader || other == unit) continue;
                const double distance = (radius + other->getAbilities().meleeRadius()) * 7 / 9;
                unit->pushAway(*other, distance, 1.2 * mass / other->getAbilities().mass);
            }
            KeyNode* node = nearestEnemyNode();
            if (node != nullptr && !node->isDestroyed()) unit->pushAwayFrom(node->getPosition(), 50, mass);
        }
    }
}

void Squad::enterFighting() {
    pendingDestination_ = destination_;
    if (!pendingDestination_) pendingDestination_ = getMainUnit()->getPosition();
    notifyStartBattle();
}

void Squad::enterMoving() {
    if (!pendingDestination_) {
        stopMoving();
        return;
    }
    if (destination_ && destination_->compare(*pendingDestination_)) return;
    destination_ = pendingDestination_->getCopy();
    pendingDestination_.reset();
    pathIndex_ = 0;
    path_ = level_->findPath(getMainUnit()->getPosition(), *destination_);
    if (!path_) {
        stopMoving();
        return;
    }
    nextWaypoint();
    if (retargetNodeOnArrival_) keyNode_ = level_->nodeAt(*destination_);
    retargetNodeOnArrival_ = false;
}

void Squad::enterInside() {
    const bool own = keyNode_->owner && owner_ && keyNode_->owner->compare(*owner_);
    if (!own && !level_->tryEnterNode(*this)) {
        keyNode_ = nullptr;
        return;
    }
    const Point2D position = keyNode_->getPosition();
    moveTo(position.x, position.y);
}

void Squad::nextWaypoint() {
    if (!path_ || pathIndex_ >= static_cast<int>(path_->size())) {
        stopMoving();
        return;
    }
    setFormationTarget((*path_)[static_cast<std::size_t>(pathIndex_)]);
    for (auto& [index, unit] : units_) unit->stopFight();
    ++pathIndex_;
}

int Squad::chooseTargetFor(Unit& unit) {
    std::map<int, double> cost;
    const Point2D position = unit.getPosition();
    const std::vector<Squad*>& enemies = nearbyEnemySquads();
    for (Squad* enemy : enemies) {
        if (enemy->isDead()) continue;
        for (auto& [index, other] : enemy->units_) {
            if (other->isDestroyed()) continue;
            double distance = position.getDistanceTo(other->getPosition(), true);
            // Prefer the enemies attacking this soldier.
            if (other->getTargetId() == unit.getId()) distance /= 3;
            cost[other->getId()] = distance;
        }
    }
    // Spread the squad's attacks: targets already taken cost more.
    for (auto& [index, mine] : units_) {
        if (!mine->isFighting()) continue;
        const int target = mine->getTargetId();
        const auto it = cost.find(target);
        cost[target] = (it == cost.end() ? std::numeric_limits<double>::quiet_NaN() : it->second) * 1.6;
    }
    double best = 9999999;
    int bestId = -1;
    for (Squad* enemy : enemies) {
        if (enemy->isDead()) continue;
        for (auto& [index, other] : enemy->units_) {
            if (other->isDestroyed()) continue;
            const double c = cost[other->getId()];
            if (c < best) {
                bestId = other->getId();
                best = c;
            }
        }
    }
    if (bestId < 0) {
        KeyNode* node = nearestEnemyNode();
        if (node != nullptr && !node->isDestroyed()) bestId = node->getId();
    }
    return bestId;
}

void Squad::stopUnitsFighting() {
    for (auto& [index, unit] : units_) unit->stopFight();
}

void Squad::registryUnitEvents(Unit& unit) {
    SquadBase::registryUnitEvents(unit);
    unit.addEventListener(Unit::EVENT_DAMAGED, getId(),
                          [this](int t, BaseObject* s, const std::any& d) { onUnitDamaged(t, s, d); });
}

void Squad::handleEvent(int state, int event) {
    int next = state;
    switch (state) {
    case STATE_IDLE:
        if (event == EV_ALL_DEAD) next = STATE_DEAD, notifyAllUnitsDead();
        else if (event == EV_MOVE_HOME) next = STATE_MOVING, markRetargetNode();
        else if (event == EV_MOVE) next = STATE_MOVING;
        else if (event == EV_ENEMY_NEAR) next = STATE_IDLE, postFight();
        else if (event == EV_FIGHT) next = STATE_FIGHTING;
        else if (event == EV_HOLD) next = STATE_HOLDING;
        else if (event == EV_NEW_DESTINATION) next = destinationForRole();
        else if (event == EV_ENTER_NODE) next = STATE_INSIDE;
        break;
    case STATE_MOVING:
        if (event == EV_ALL_DEAD) next = STATE_DEAD, notifyAllUnitsDead();
        else if (event == EV_ARRIVED) next = STATE_IDLE, notifySquadAtPosition();
        else if (event == EV_MOVE_HOME) next = STATE_MOVING, markRetargetNode(), enterMoving();
        else if (event == EV_MOVE) next = STATE_MOVING, enterMoving();
        else if (event == EV_ENEMY_NEAR) next = STATE_MOVING, postFight();
        else if (event == EV_FIGHT) next = STATE_FIGHTING;
        break;
    case STATE_FIGHTING:
        if (event == EV_ALL_DEAD) next = STATE_DEAD, notifyAllUnitsDead();
        else if (event == EV_MOVE) next = STATE_MOVING;
        else if (event == EV_ARRIVED) next = STATE_IDLE, notifySquadAtPosition();
        else if (event == EV_FIGHT) next = STATE_FIGHTING, stopUnitsFighting();
        break;
    case STATE_HOLDING:
        if (event == EV_ALL_DEAD) next = STATE_DEAD, notifyAllUnitsDead();
        else if (event == EV_MOVE_HOME) next = STATE_MOVING, markRetargetNode();
        else if (event == EV_MOVE) next = STATE_MOVING;
        else if (event == EV_ENEMY_NEAR) next = STATE_IDLE, postFight();
        else if (event == EV_FIGHT) next = STATE_FIGHTING;
        else if (event == EV_NEW_DESTINATION) next = STATE_MOVING;
        break;
    case STATE_INSIDE:
        if (event == EV_ALL_DEAD) next = STATE_DEAD, notifyAllUnitsDead();
        else if (event == EV_MOVE_HOME) next = STATE_MOVING, markRetargetNode();
        else if (event == EV_MOVE) next = STATE_MOVING;
        else if (event == EV_NEW_DESTINATION) next = STATE_MOVING;
        else if (event == EV_ARRIVED) next = STATE_IDLE;
        break;
    default: break;
    }
    changeFSMState(next);
}

void Squad::onUnitDamaged(int, BaseObject* sender, const std::any& attackerId) {
    auto* unit = static_cast<Unit*>(sender);
    if (unit->isFighting()) {
        auto* target = dynamic_cast<ITarget*>(BaseObject::getObjectPtr(unit->getTargetId()));
        // Attacked while besieging: fight the attackers instead.
        if (target != nullptr && target->getTargetType() == TargetType::Building) postEvent(EV_FIGHT);
    } else {
        auto* attacker = dynamic_cast<ITarget*>(BaseObject::getObjectPtr(std::any_cast<int>(attackerId)));
        if (attacker != nullptr) commandMoveHome(attacker->getPosition());
    }
}

void Squad::notifySquadAtPosition() {
    const Point2D position = getMainUnit()->getPosition();
    KeyNode* node = level_->nodeAt(position);
    if (node != nullptr) {
        if (node == keyNode_ && position.getDistanceTo(node->getPosition()) < 20) postEvent(EV_ENTER_NODE);
    } else {
        if (keyNode_ != nullptr) {
            pendingDestination_ = keyNode_->getPosition();
            postEvent(EV_MOVE_HOME);
        }
        level_->onSquadStopped(*this);
    }
}

void Squad::moraleChanged(MoraleStatus status) {
    SquadBase::moraleChanged(status);
    level_->onSquadMoraleChanged(*this, status);
}

bool Squad::isAttackableBy(Race race) const {
    return !(getRace() == race || getRace() == Race::Bandits || isInside() || isDead() || killed_);
}

int Squad::destinationForRole() {
    switch (getRole()) {
    case SquadRole::Defender:
    case SquadRole::Attacker: pendingDestination_ = keyNode_->getPosition(); break;
    default: break;
    }
    return STATE_MOVING;
}

double Squad::engageDistance() const { return std::max<double>(getArmyStat()->range, 90); }

int Squad::lastWaypointIndex() const { return pathIndex_ - 1 >= 0 ? pathIndex_ - 1 : 0; }

KeyNode* Squad::nearestEnemyNode() {
    if (!enemyNodeCached_) {
        enemyNode_ = level_->enemyNodeInRange(*this);
        enemyNodeCached_ = true;
    }
    return enemyNode_;
}

const std::vector<Squad*>& Squad::nearbyEnemySquads() {
    if (!enemySquads_) enemySquads_ = level_->enemySquadsInRange(*this);
    return *enemySquads_;
}

std::optional<SquadState> Squad::actionStatus(int state) const {
    switch (state) {
    case STATE_MOVING: return SquadState::Moving;
    case STATE_FIGHTING: return getMainUnit()->getRange() > 0 ? SquadState::RangedFighting : SquadState::MeleeFighting;
    case STATE_HOLDING: return allArrived_ ? SquadState::Sleeping : SquadState::Moving;
    case STATE_INSIDE:
        return keyNode_ != nullptr && keyNode_->getHPFactor() < 1 ? SquadState::Repairing : SquadState::Defending;
    default: return std::nullopt;
    }
}

void Squad::clearMovement() {
    pendingDestination_.reset();
    destination_.reset();
    path_.reset();
}

void Squad::customUpdate(double dt) {
    SquadBase::customUpdate(dt);
    updateFSM(dt);
    clearCaches();
}

void Squad::customDispose() {
    level_ = nullptr;
    clearCaches();
    clearMovement();
    holdPoint_.reset();
    key_.reset();
    SquadBase::customDispose();
}

bool Squad::chooseTarget(Unit& unit) {
    const int targetId = chooseTargetFor(unit);
    if (targetId < 0) return false;
    auto* target = dynamic_cast<ITarget*>(BaseObject::getObjectPtr(targetId));
    if (unit.isFighting() && unit.getTargetId() == targetId) return true;
    if (target->getTargetType() == TargetType::Building) {
        keyNode_ = static_cast<KeyNode*>(target);
        pendingDestination_ = target->getPosition();
    }
    unit.setTarget(targetId);
    return true;
}

bool Squad::canEngage(Squad& other) const {
    if (other.isDead() || other.getRace() == getRace() || other.isInside()) return false;
    Unit* mine = getMainUnit();
    Unit* theirs = other.getMainUnit();
    if (mine == nullptr || theirs == nullptr) return false;
    double distance = mine->getPosition().getDistanceTo(theirs->getPosition());
    if (other.isHolding()) distance *= 1.75;
    return distance - other.getRadius() - engageDistance() <= 0;
}

void Squad::checkEnemies() {
    if (!nearbyEnemySquads().empty()) postEvent(EV_ENEMY_NEAR);
    KeyNode* node = nearestEnemyNode();
    if (node != nullptr && !node->isDestroyed()) postEvent(EV_ENEMY_NEAR);
}

void Squad::checkDeath() {
    int alive = 0;
    for (auto& [index, unit] : units_)
        if (!unit->isDead()) ++alive;
    if (alive == 0) postEvent(EV_ALL_DEAD);
}

void Squad::changeFSMState(int state) {
    if (state_ == state) return;
    state_ = state;
    switch (state_) {
    case STATE_IDLE: enterIdle(); break;
    case STATE_MOVING: enterMoving(); break;
    case STATE_FIGHTING: enterFighting(); break;
    case STATE_DEAD: enterDead(); break;
    case STATE_HOLDING: enterHolding(); break;
    case STATE_INSIDE: enterInside(); break;
    }
}

void Squad::stopMoving() {
    postArrived();
    destination_.reset();
    path_.reset();
    retargetNodeOnArrival_ = false;
}

} // namespace takeover
