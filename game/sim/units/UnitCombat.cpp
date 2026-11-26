#include "game/sim/units/UnitCombat.h"

#include "game/sim/abilities/UnitParams.h"
#include "game/sim/units/Unit.h"

#include <algorithm>
#include <cmath>

namespace takeover {

UnitCombat::UnitCombat(Unit* unit) : unit_(unit) { reset(); }

ITarget* UnitCombat::getLiveTarget(int id) {
    auto* target = dynamic_cast<ITarget*>(BaseObject::getObjectPtr(id));
    if (target == nullptr || target->isDestroyed()) return nullptr;
    return target;
}

void UnitCombat::updateApproach(double dt, ITarget& target) {
    unit_->stepTowards(dt, target.getPosition());
    unit_->checkSpeedChange();
    approachTime_ += dt;
    if (approachTime_ > 1) stopFight();
    addPursuit(static_cast<int>(unit_->distanceIn(dt) * 100));
}

void UnitCombat::processPendingEvent() {
    const int event = pendingEvent_;
    pendingEvent_ = 0;
    handleEvent(event);
}

void UnitCombat::updateRanged(double dt, int targetId) {
    if (unit_->rangedAttackPeriod() <= 0) return;
    if (attackTime_ >= hitTime_ && previousAttackTime_ < hitTime_) {
        unit_->launchMissile(targetId);
        unit_->activateDistanceAttack();
    }
    previousAttackTime_ = attackTime_;
    attackTime_ += dt;
    if (attackTime_ >= unit_->rangedAttackPeriod()) {
        previousAttackTime_ = 0;
        attackTime_ -= unit_->rangedAttackPeriod();
        startShotAnimation();
    }
}

void UnitCombat::updateMelee(double dt, ITarget& target) {
    if (unit_->meleeAttackPeriod() <= 0) return;
    if (attackTime_ >= hitTime_ && previousAttackTime_ < hitTime_) {
        target.onMeleeHitBy(*unit_);
        unit_->activateAttack();
    }
    previousAttackTime_ = attackTime_;
    attackTime_ += dt;
    if (attackTime_ >= unit_->meleeAttackPeriod()) {
        previousAttackTime_ = 0;
        attackTime_ -= unit_->meleeAttackPeriod();
        startStrikeAnimation();
    }
}

void UnitCombat::checkShotDone() {
    if (attackTime_ >= animationLength_ || previousAttackTime_ == 0) postEvent(EV_SHOT_DONE);
}

void UnitCombat::checkMeleeDone() {
    if (attackTime_ >= animationLength_ || previousAttackTime_ == 0) postEvent(EV_MELEE_DONE);
}

void UnitCombat::tryStrike() {
    if (unit_->canMelee()) postEvent(EV_STRIKE);
}

void UnitCombat::onAttackedBy(int attackerId) {
    ITarget* attacker = getLiveTarget(attackerId);
    if (attacker == nullptr) return;
    const int event = rangeEvent(*attacker);
    // Only fight back an attacker within melee reach.
    if (event != EV_IN_MELEE_RANGE && event != EV_OUT_OF_RANGE) return;
    switch (state_) {
    case IDLE:
    case WAIT: attackTarget(attackerId); return;
    case RANGED:
    case APPROACH:
    case MELEE:
        if (isTargetFightingMe(targetId_)) return;
        attackTarget(attackerId);
        return;
    default: return;
    }
}

void UnitCombat::tryShoot() {
    if (unit_->isRanged()) postEvent(EV_SHOOT);
}

void UnitCombat::startApproach() {
    setState(APPROACH);
    if (unit_->hasCharge()) {
        unit_->increaseSpeed(1.75, 1.5);
    } else if (unit_->hasDash()) {
        unit_->increaseSpeed(15, 0.5);
        unit_->changeAnimTo_Special();
    }
}

void UnitCombat::enterApproach() {
    unit_->changeAnimTo_Go();
    resetPursuit();
}

void UnitCombat::enterRanged() {
    previousAttackTime_ = std::min(timeOutOfAttack_, unit_->rangedAttackPeriod());
    attackTime_ = previousAttackTime_;
}

void UnitCombat::startShotAnimation() {
    animationLength_ = unit_->changeAnimTo_DistanceAttack(unit_->rangedAttackPeriod());
    hitTime_ = animationLength_ * unit_->attackAnimFactor();
}

void UnitCombat::enterMelee() {
    previousAttackTime_ = std::min(timeOutOfAttack_, unit_->meleeAttackPeriod());
    attackTime_ = previousAttackTime_;
}

void UnitCombat::startStrikeAnimation() {
    animationLength_ = unit_->changeAnimTo_Attack(unit_->meleeAttackPeriod());
    hitTime_ = animationLength_ * 3 / 4;
}

int UnitCombat::rangeEvent(const ITarget& target) const {
    const double distance = target.getPosition().distanceTo(unit_->getX(), unit_->getY()) - target.getHitRadius();
    double event = 0;
    if (unit_->minRangedDistance() > 0) {
        if (distance > unit_->rangeValue()) event = std::max(event, static_cast<double>(EV_OUT_OF_RANGE));
        else if (distance > unit_->minRangedDistance()) event = std::max(event, static_cast<double>(EV_IN_SHOOTING_RANGE));
    }
    if (distance > unit_->meleeRadius()) event = std::max(event, static_cast<double>(EV_OUT_OF_MELEE_RANGE));
    else event = std::max(event, static_cast<double>(EV_IN_MELEE_RANGE));
    return static_cast<int>(std::round(event));
}

void UnitCombat::attackTarget(int targetId) {
    setTarget(targetId);
    startApproach();
}

void UnitCombat::resetPursuit() {
    unit_->getAbilities().getUnitParams().getParam(UnitParams::PRP_PURSUIT_TIME).setBase(0);
}

void UnitCombat::reset() {
    targetId_ = -1;
    pendingEvent_ = 0;
    timeOutOfAttack_ = 999999;
    buildTransitions();
}

void UnitCombat::handleEvent(int event) {
    // As the original: the loop goes on after a transition, so a following
    // row may match the new state with the same event.
    for (std::size_t i = 0; i < transitions_.size(); ++i) {
        const Transition t = transitions_[i];
        if (t.from == state_ && event == t.event) {
            if (t.action != nullptr) (this->*t.action)();
            setState(t.to);
        }
    }
}

void UnitCombat::onEnterState(int state) {
    if (state == APPROACH) enterApproach();
    else if (state == MELEE) enterMelee();
    else if (state == RANGED) enterRanged();
    else if (state == WAIT) unit_->changeAnimTo_Stand();
}

void UnitCombat::onExitState(int state) {
    if (state == MELEE || state == RANGED) timeOutOfAttack_ = attackTime_;
}

void UnitCombat::stopFight() { unit_->stopFight(); }

void UnitCombat::buildTransitions() {
    transitions_.clear();
    state_ = IDLE;
    addTransition(APPROACH, EV_IN_MELEE_RANGE, WAIT, &UnitCombat::tryStrike);
    addTransition(APPROACH, EV_IN_SHOOTING_RANGE, APPROACH, &UnitCombat::tryShoot);
    addTransition(APPROACH, EV_SHOOT, RANGED, nullptr);
    addTransition(APPROACH, EV_STRIKE, MELEE, nullptr);
    addTransition(WAIT, EV_IN_MELEE_RANGE, WAIT, &UnitCombat::tryStrike);
    addTransition(WAIT, EV_OUT_OF_MELEE_RANGE, APPROACH, nullptr);
    addTransition(WAIT, EV_IN_SHOOTING_RANGE, APPROACH, nullptr);
    addTransition(WAIT, EV_OUT_OF_RANGE, APPROACH, nullptr);
    addTransition(WAIT, EV_SHOOT, RANGED, nullptr);
    addTransition(WAIT, EV_STRIKE, MELEE, nullptr);
    addTransition(MELEE, EV_OUT_OF_MELEE_RANGE, MELEE, &UnitCombat::checkMeleeDone);
    addTransition(MELEE, EV_OUT_OF_RANGE, MELEE, &UnitCombat::checkMeleeDone);
    addTransition(MELEE, EV_IN_SHOOTING_RANGE, APPROACH, &UnitCombat::tryShoot);
    addTransition(MELEE, EV_MELEE_DONE, APPROACH, nullptr);
    addTransition(RANGED, EV_OUT_OF_MELEE_RANGE, RANGED, &UnitCombat::checkShotDone);
    addTransition(RANGED, EV_OUT_OF_RANGE, RANGED, &UnitCombat::checkShotDone);
    addTransition(RANGED, EV_IN_MELEE_RANGE, RANGED, &UnitCombat::checkShotDone);
    addTransition(RANGED, EV_SHOT_DONE, APPROACH, nullptr);
}

void UnitCombat::customUpdate(double dt) {
    ITarget* target = getLiveTarget(targetId_);
    if (target == nullptr) {
        targetType_.reset();
        stopFight();
        return;
    }
    targetType_ = target->getTargetType();
    postEvent(rangeEvent(*target));
    processPendingEvent();
    // As the original, the target fetched above is used even if the events
    // just changed it (it stays in memory until the next frame anyway).
    if (state_ == APPROACH) updateApproach(dt, *target);
    else if (state_ == MELEE) updateMelee(dt, *target);
    else if (state_ == RANGED) updateRanged(dt, targetId_);
    timeOutOfAttack_ += dt;
}

void UnitCombat::customDispose() {
    transitions_.clear();
    unit_ = nullptr;
    BaseObject::customDispose();
}

bool UnitCombat::isTargetFightingMe(int targetId) const {
    ITarget* target = getLiveTarget(targetId);
    if (target == nullptr || target->getTargetType() != TargetType::Unit) return false;
    auto* unit = static_cast<Unit*>(target);
    return unit->isFighting() && unit->getTargetId() == unit_->getId();
}

void UnitCombat::setState(int state) {
    if (state_ != state) {
        onExitState(state_);
        state_ = state;
        onEnterState(state_);
    }
}

void UnitCombat::addTransition(int from, int event, int to, void (UnitCombat::*action)()) {
    transitions_.push_back({from, event, to, action});
}

void UnitCombat::addPursuit(int amount) {
    unit_->getAbilities().getUnitParams().getParam(UnitParams::PRP_PURSUIT_TIME).addBase(amount);
}

void UnitCombat::postEvent(int event) {
    if (pendingEvent_ < event) pendingEvent_ = event;
}

} // namespace takeover
