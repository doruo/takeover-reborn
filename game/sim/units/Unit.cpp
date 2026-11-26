#include "game/sim/units/Unit.h"

#include "game/framework/core/As3.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/sim/units/UnitCombat.h"

#include <cmath>

namespace takeover {

const int Unit::EVENT_SPLASH_ATTACK = BaseObject::newEventType();
const int Unit::EVENT_DAMAGED = BaseObject::newEventType();
const int Unit::EVENT_ATTACKS = BaseObject::newEventType();

EffectList SplashArea::copyEffects() const {
    EffectList copies;
    for (const auto& effect : effects) copies.push_back(effect->getCopy());
    return copies;
}

Unit::~Unit() = default;

Unit& Unit::init(const UnitDef& def) {
    squadId_ = def.squadId;
    initAbilities(*def.armyStat, def.armyStatBonus);
    initFSM();
    initMissile();
    moveTo(abilities_->x, abilities_->y);
    stopHere();
    resetVelocity();
    changeAnimTo_Stand();
    return *this;
}

void Unit::initFSM() {
    destination_ = {x_, y_};
    arrived_ = false;
    state_ = STATE_STAND;
    pendingEvent_ = 0;
    combat_ = std::make_shared<UnitCombat>(this);
}

void Unit::initMissile() {
    missileTargetId_ = -1;
    missileFrom_ = Point2D();
    missileTo_ = Point2D();
    missilePos_ = Point2D();
    missilePrevPos_ = Point2D();
}

void Unit::clearMissile() {
    missileTargetId_ = -1;
    missileTimeLeft_ = -1;
    missileFrom_.reset();
    missileTo_.reset();
    missilePos_.reset();
    missilePrevPos_.reset();
}

// --- State machine ---------------------------------------------------------------------------

void Unit::updateGo(double dt) {
    const Point2D target = getDestination();
    if (stepTowards(dt, target)) {
        resetVelocity();
        moveTo(target.x, target.y);
    }
}

void Unit::updateDead(double dt) {
    deadTime_ += dt;
    if (deadTime_ > 3) {
        postEvent(EV_TO_GRAVE);
        deadTime_ = -4000000; // never again
    }
}

void Unit::updateFight(double dt) { combat_->update(dt); }

void Unit::updateState(int state, double dt) {
    switch (state) {
    case STATE_STAND: updateStand(dt); break;
    case STATE_GO: updateGo(dt); break;
    case STATE_DEAD: updateDead(dt); break;
    case STATE_FIGHT: updateFight(dt); break;
    }
}

void Unit::updateFSM(double dt) {
    updateState(state_, dt);
    if (pendingEvent_ != 0) {
        handleEvent(state_, pendingEvent_);
        pendingEvent_ = 0;
    }
    if (getHP() <= 0) {
        postEvent(EV_DIE);
    } else if (isAt(destination_)) {
        postEvent(EV_ARRIVED);
        arrived_ = true;
    } else {
        postEvent(EV_MOVE);
        arrived_ = false;
    }
}

bool Unit::stepTowards(double dt, const Point2D& target) {
    const double dx = target.x - getX(), dy = target.y - getY();
    const double d2 = dx * dx + dy * dy;
    const double step = distanceIn(dt);
    if (d2 < step * step) return true;
    updateAnimDir(dx, dy);
    const double angle = std::atan2(dy, dx);
    addVelocity(std::cos(angle) * step, std::sin(angle) * step);
    checkSpeedChange();
    return false;
}

void Unit::updateMissile(double dt) {
    if (missileTimeLeft_ < 0) return;
    ITarget* target = dynamic_cast<ITarget*>(BaseObject::getObjectPtr(missileTargetId_));
    if (target == nullptr) missileTargetId_ = -1;
    missileTimeLeft_ -= dt;
    if (missileTimeLeft_ < 0) {
        missileAtTarget(target);
        if (target != nullptr) target->onMissileHitBy(*this);
        return;
    }
    missilePrevPos_->copyFrom(*missilePos_);
    const double progress = (missileFlightTime_ - missileTimeLeft_) / missileFlightTime_;
    missilePos_->x = missileFrom_->x + (missileTo_->x - missileFrom_->x) * progress;
    missilePos_->y = missileFrom_->y + (missileTo_->y - missileFrom_->y) * progress;
}

void Unit::onFightTargetChanged() {
    // A new target while fighting: switch to it if attacking a building.
    auto* current = dynamic_cast<ITarget*>(BaseObject::getObjectPtr(combat_->getTargetId()));
    if (current != nullptr && current->getTargetType() == TargetType::Building) combat_->attackTarget(nextTargetId_);
}

void Unit::dealSplashOrDirect(Unit& attacker, EffectList effects) {
    const int attackerId = attacker.getId();
    const int splashRadius = attacker.abilities_->getSplashDamageRadius();
    if (splashRadius > 0) {
        SplashArea area;
        area.effects = std::move(effects);
        area.attackerId = attackerId;
        area.radius = splashRadius;
        area.position = getPosition();
        dispatchEvent(EVENT_SPLASH_ATTACK, std::make_shared<SplashArea>(std::move(area)));
    } else {
        receiveAttack(effects, attackerId);
    }
}

void Unit::onMeleeHitBy(Unit& attacker) {
    attacker.notifyAttack();
    auto* source = dynamic_cast<Unit*>(BaseObject::getObjectPtr(attacker.getId()));
    if (source == nullptr) return;
    dealSplashOrDirect(*source, source->abilities_->createMeleeAttack());
}

void Unit::onMissileHitBy(Unit& attacker) {
    attacker.notifyAttack();
    auto* source = dynamic_cast<Unit*>(BaseObject::getObjectPtr(attacker.getId()));
    if (source == nullptr) return;
    dealSplashOrDirect(*source, source->abilities_->createDistanceAttack());
}

void Unit::enterFight() {
    combat_->startApproach();
    if (ITarget* target = UnitCombat::getLiveTarget(combat_->getTargetId())) target->onTargetedBy(*this);
}

void Unit::startFight() { combat_->setTarget(nextTargetId_); }

void Unit::stopHere() {
    destination_.x = x_;
    destination_.y = y_;
}

void Unit::postEvent(int event) {
    if (event > pendingEvent_) pendingEvent_ = event;
}

void Unit::push(double angle, double force) {
    const double strength = force / abilities_->mass;
    addVelocity(std::cos(angle) * strength, std::sin(angle) * strength);
}

void Unit::checkSpeedChange() {
    if (lastSpeed_ != getSpeed()) {
        updateGoAnimSpeed();
        lastSpeed_ = getSpeed();
    }
}

void Unit::handleEvent(int state, int event) {
    int next = state;
    switch (state) {
    case STATE_STAND:
        if (event == EV_MOVE) {
            next = STATE_GO;
            changeAnimTo_Go();
        } else if (event == EV_FIGHT) {
            next = STATE_FIGHT;
            startFight();
        } else if (event == EV_DIE) {
            next = STATE_DEAD;
            changeAnimTo_FallDown();
        }
        break;
    case STATE_GO:
        if (event == EV_ARRIVED) {
            next = STATE_STAND;
            changeAnimTo_Stand();
        } else if (event == EV_FIGHT) {
            next = STATE_FIGHT;
            startFight();
        } else if (event == EV_DIE) {
            next = STATE_DEAD;
            changeAnimTo_FallDown();
        }
        break;
    case STATE_DEAD:
        if (event == EV_TO_GRAVE) {
            next = STATE_DEAD;
            changeAnimTo_CorpseToGrave();
        } else if (event == EV_RESURRECT) {
            next = STATE_STAND;
            changeAnimTo_Stand();
        }
        break;
    case STATE_FIGHT:
        if (event == EV_STOP_FIGHT) {
            next = STATE_STAND;
            changeAnimTo_Stand();
        } else if (event == EV_FIGHT) {
            next = STATE_FIGHT;
            onFightTargetChanged();
        } else if (event == EV_DIE) {
            next = STATE_DEAD;
            changeAnimTo_FallDown();
        }
        break;
    }
    changeFSMState(next);
}

void Unit::changeFSMState(int state) {
    if (state_ == state) return;
    state_ = state;
    switch (state_) {
    case STATE_GO: enterGo(); break;
    case STATE_DEAD: enterDead(); break;
    case STATE_FIGHT: enterFight(); break;
    default: break;
    }
}

void Unit::notifyAttack() { dispatchEvent(EVENT_ATTACKS, this); }

bool Unit::isChasing() const { return state_ == STATE_FIGHT && combat_->isApproaching(); }

bool Unit::isAttackingBuilding() const {
    return state_ == STATE_FIGHT && combat_->getTargetType() == TargetType::Building;
}

int Unit::getTargetId() const { return combat_->getTargetId(); }

double Unit::getHitRadius() const {
    switch (abilities_->size) {
    case UnitSize::Middle: return 10;
    case UnitSize::Gaint: return 20;
    default: return 8;
    }
}

void Unit::kill() { abilities_->getUnitParams().getParam(UnitParams::HP).setBase(0); }

void Unit::setMaxDamageTime(double seconds) { abilities_->getUnitParams().setMaxDamageTime(seconds); }

void Unit::addDamageResist(double seconds, int percent) {
    abilities_->addEffects({std::make_shared<DamageResistEffect>(-1, percent, static_cast<int>(seconds * 1000))});
}

void Unit::setDamageFactor(double factor) { abilities_->getUnitParams().nextDamageFactor = factor; }

void Unit::receiveAttack(const EffectList& effects, int attackerId) {
    abilities_->addEffects(effects);
    dispatchEvent(EVENT_DAMAGED, attackerId);
    if (isFighting()) combat_->onAttackedBy(attackerId);
}

void Unit::customUpdate(double dt) {
    UnitBody::customUpdate(dt);
    if (getHP() > 0) {
        applyVelocity();
        abilities_->x = x_;
        abilities_->y = y_;
    }
    updateFSM(dt);
    updateMissile(dt);
    vx_ *= 0.5;
    vy_ *= 0.5;
}

void Unit::customDispose() {
    clearMissile();
    beforeDispose();
    if (combat_) {
        combat_->dispose();
        combat_.reset();
    }
    UnitBody::customDispose();
}

void Unit::launchMissile(int targetId) {
    ITarget* target = dynamic_cast<ITarget*>(BaseObject::getObjectPtr(targetId));
    if (target == nullptr) {
        missileTargetId_ = -1;
        return;
    }
    missileTargetId_ = targetId;
    missileFrom_ = getPosition();
    missileTo_ = target->getPosition();
    missilePrevPos_ = *missileFrom_;
    missilePos_ = *missileFrom_;
    const double flight = getArmyStat()->missileTime / 10;
    missileFlightTime_ = flight * (0.5 + as3::random());
    missileTimeLeft_ = missileFlightTime_;
}

int Unit::heal(int amount) {
    int healed = amount;
    const int hp = getHP();
    const int missing = getMaxHP() - hp;
    int surplus = 0;
    if (missing < healed) {
        surplus = healed - missing;
        healed = missing;
    }
    abilities_->setHealth(hp + healed);
    return surplus;
}

void Unit::setTarget(int targetId) {
    if (UnitCombat::getLiveTarget(targetId) != nullptr) {
        nextTargetId_ = targetId;
        postEvent(EV_FIGHT);
    }
}

int Unit::resurrect(int hp, const Point2D& position) {
    postEvent(EV_RESURRECT);
    moveTo(position.x, position.y);
    setDestination(position);
    return heal(hp);
}

} // namespace takeover
