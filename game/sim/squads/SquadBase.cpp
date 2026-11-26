#include "game/sim/squads/SquadBase.h"

#include "game/framework/core/As3.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/sim/battle/Combatant.h"
#include "game/sim/level/KeyNode.h"

#include <cmath>

namespace takeover {

const std::vector<Point2D> SquadBase::FORMATION_3x3 = {{0, 0}, {0, 14}, {0, -14}, {-14, 0}, {-14, 14}, {-14, -14}, {-28, 0}, {-28, 14}, {-28, -14}};
const std::vector<Point2D> SquadBase::FORMATION_3x4 = {{0, 0}, {0, 14}, {0, -14}, {-14, 0}, {-14, 14}, {-14, -14}, {-28, 0}, {-28, 14}, {-28, -14}, {-42, 0}, {-42, 14}, {-42, -14}};
const std::vector<Point2D> SquadBase::FORMATION_3x2 = {{0, 0}, {-14, 10.5}, {-14, -10.5}, {-21, 0}, {-31.5, 10.5}, {-31.5, -10.5}};
const std::vector<Point2D> SquadBase::FORMATION_5 = {{0, 0}, {-21, 14}, {-21, -14}, {-42, 14}, {-42, -14}};
const std::vector<Point2D> SquadBase::FORMATION_7 = {{0, 0}, {-21, 14}, {-21, -14}, {-42, 14}, {-42, -14}, {-63, 14}, {-63, -14}};
const std::vector<Point2D> SquadBase::FORMATION_3 = {{0, 0}, {-14, 7}, {-14, -7}};
const std::vector<Point2D> SquadBase::FORMATION_1 = {{0, 0}};

const std::vector<Point2D>& SquadBase::formationFor(int unitCount) {
    if (unitCount >= 9) return FORMATION_3x4;
    if (unitCount >= 7) return FORMATION_7;
    if (unitCount >= 6) return FORMATION_3x2;
    if (unitCount >= 5) return FORMATION_5;
    if (unitCount >= 3) return FORMATION_3;
    return FORMATION_1;
}

int SquadBase::levelForXp(double xp) {
    int level = 2;
    while (xp < xpForLevel(level))
        if (--level <= 0) return 0;
    return level;
}

double SquadBase::xpForLevel(int level) {
    switch (level) {
    case 1: return 25;
    case 2: return 100;
    default: return 0;
    }
}

// --- Setup ---------------------------------------------------------------------------------

SquadBase& SquadBase::init(std::shared_ptr<CombatantKey> owner, const ArmyStat& stat, const ArmyStatBonus& bonus) {
    stat_ = &stat;
    bonus_ = bonus;
    owner_ = std::move(owner);
    keyNode_ = nullptr;
    lastAttackerOwner_.reset();
    units_.clear();
    const int count = stat_->unitsCount + bonus_.addUnit;
    formation_ = toPolar(formationFor(count), count);
    maxHP_ = 0;
    for (int i = 0; i < getUnitCount(); ++i) {
        std::shared_ptr<Unit> unit = createUnit(i);
        registryUnitEvents(*unit);
        units_[i] = unit;
        maxHP_ += unit->getMaxHP();
    }
    currentHP_ = maxHP_;
    previousHP_ = currentHP_;
    initMorale();
    targetX_ = targetY_ = direction_ = 0;
    initXp(count);
    killed_ = false;
    return *this;
}

std::shared_ptr<Unit> SquadBase::createUnit(int) {
    auto unit = std::make_shared<Unit>();
    UnitDef def;
    def.armyStat = &ArmyStat::ARCHERS;
    unit->init(def);
    return unit;
}

void SquadBase::registryUnitEvents(Unit& unit) {
    unit.addEventListener(Unit::EVENT_ATTACKS, getId(),
                          [this](int t, BaseObject* s, const std::any& d) { onUnitAttacks(t, s, d); });
}

void SquadBase::initMorale() {
    morale_ = std::make_shared<Morale>(*stat_, bonus_);
    morale_->addEventListener(Morale::EVENT_MORALE_CHANGED, getId(),
                              [this](int t, BaseObject* s, const std::any& d) { onChangeMorale(t, s, d); });
    for (auto& [index, unit] : units_) {
        Abilities& abilities = unit->getAbilities();
        abilities.addEventListener(Abilities::EVENT_ATTACK_DONE, getId(),
                                   [this](int t, BaseObject* s, const std::any& d) { onAttackDone(t, s, d); });
        abilities.addEventListener(Abilities::EVENT_HIT, getId(),
                                   [this](int t, BaseObject* s, const std::any& d) { onUnitHit(t, s, d); });
        unit->addEventListener(Unit::EVENT_SPLASH_ATTACK, getId(),
                               [this](int t, BaseObject* s, const std::any& d) { forwardEvent(t, s, d); });
    }
    actionState_.reset();
    previousActionState_.reset();
}

void SquadBase::disposeMorale() {
    actionState_.reset();
    previousActionState_.reset();
    if (morale_) {
        morale_->dispose();
        morale_.reset();
    }
}

void SquadBase::initXp(int unitCount) {
    xp_ = 0;
    xpPerAttack_ = getMainUnit()->meleeAttackPeriod() / unitCount;
}

std::vector<Point2D> SquadBase::toPolar(const std::vector<Point2D>& formation, int count) {
    std::vector<Point2D> polar;
    for (int i = 0; i < count; ++i) {
        const Point2D& p = formation[static_cast<std::size_t>(i)];
        polar.emplace_back(std::atan2(p.y, p.x), std::sqrt(p.x * p.x + p.y * p.y));
    }
    return polar;
}

// --- Units -----------------------------------------------------------------------------------

Race SquadBase::getRace() const { return stat_->side; }

UnitType SquadBase::getUnitType() const { return stat_->unitType; }

SquadRole SquadBase::getRole() const {
    if (keyNode_ == nullptr || owner_ == nullptr) return SquadRole::Scout;
    if (keyNode_->owner && keyNode_->owner->compare(*owner_)) return SquadRole::Defender;
    return SquadRole::Attacker;
}

Unit* SquadBase::getMainUnit() const {
    for (const auto& [index, unit] : units_)
        if (!unit->isDead()) return unit.get();
    return units_.empty() ? nullptr : units_.begin()->second.get();
}

void SquadBase::stopAll() {
    for (auto& [index, unit] : units_) unit->stopHere();
}

void SquadBase::setFormationTarget(const Point2D& point) {
    double direction = direction_;
    Unit* leader = getMainUnit();
    targetX_ = point.x;
    targetY_ = point.y;
    const double dx = targetX_ - leader->getX(), dy = targetY_ - leader->getY();
    if (!(dx == 0 && dy == 0)) direction = std::atan2(dy, dx);
    direction_ = direction;
    int slot = 0;
    for (int i = 0; i < getUnitCount(); ++i) {
        Unit* unit = unitAt(i);
        if (unit->isDead()) continue;
        const Point2D& offset = formation_[static_cast<std::size_t>(slot)];
        const double angle = offset.x + direction;
        const double distance = offset.y;
        unit->setDestination({targetX_ + std::cos(angle) * distance, targetY_ + std::sin(angle) * distance});
        unit->resetVelocity();
        ++slot;
    }
}

void SquadBase::keepFormation() {
    Unit* leader = getMainUnit();
    int slot = 0;
    for (int i = 0; i < getUnitCount(); ++i) {
        Unit* unit = unitAt(i);
        if (unit->isDead()) continue;
        const Point2D& offset = formation_[static_cast<std::size_t>(slot)];
        const double angle = offset.x + direction_;
        const double distance = offset.y;
        moveUnitTowards(*unit, leader->getX() + std::cos(angle) * distance, leader->getY() + std::sin(angle) * distance,
                        unit->getSpeed() * 0.01);
        ++slot;
    }
}

void SquadBase::moveUnitTowards(Unit& unit, double x, double y, double step) {
    const double dx = x - unit.getX(), dy = y - unit.getY();
    const double distance = std::sqrt(dx * dx + dy * dy);
    if (distance < step) {
        unit.setX(x);
        unit.setY(y);
    } else {
        const double angle = std::atan2(dy, dx);
        unit.addVelocity(std::cos(angle) * step, std::sin(angle) * step);
    }
}

void SquadBase::moveTo(double x, double y) {
    for (auto& [index, unit] : units_) {
        unit->moveTo(x, y);
        unit->stopHere();
    }
}

void SquadBase::increaseSpeed(double factor, double seconds) {
    for (auto& [index, unit] : units_) unit->increaseSpeed(factor, seconds);
}

bool SquadBase::isNear(const Point2D& point, double radius) const {
    for (const auto& [index, unit] : units_)
        if (!unit->isDestroyed() && unit->getPosition().getDistanceTo(point) <= radius) return true;
    return false;
}

Unit* SquadBase::findDeadUnit() const {
    for (const auto& [index, unit] : units_)
        if (unit->isDestroyed()) return unit.get();
    return nullptr;
}

Unit* SquadBase::findWoundedUnit() const {
    for (const auto& [index, unit] : units_)
        if (!unit->isDestroyed() && unit->getHP() < unit->getMaxHP()) return unit.get();
    return nullptr;
}

// --- Morale and experience --------------------------------------------------------------------------

void SquadBase::updateMorale(double dt) {
    UnitParams& params = getMainUnit()->getAbilities().getUnitParams();
    morale_->setLocks(params.getParamCurValue(UnitParams::PRP_MORALE_LOCK) > 0,
                      params.getParamCurValue(UnitParams::PRP_FEAR_LOCK) > 0,
                      params.getParamCurValue(UnitParams::PRP_MORALE_BONUS_FOR_ATTACK));
    morale_->update(dt);
    if (previousActionState_ != actionState_) {
        onActionStatusUpdated();
        previousActionState_ = actionState_;
    }
}

void SquadBase::onUnitAttacks(int, BaseObject*, const std::any&) { addXP(xpPerAttack_); }

void SquadBase::onUnitHit(int, BaseObject*, const std::any& attackerId) {
    if (auto* attacker = dynamic_cast<Unit*>(BaseObject::getObjectPtr(std::any_cast<int>(attackerId)))) {
        if (auto* squad = dynamic_cast<SquadBase*>(BaseObject::getObjectPtr(attacker->getSquadId()))) {
            morale_->onHitBy(*squad->morale_);
            lastAttackerOwner_ = squad->getOwner();
            return;
        }
    }
    lastAttackerOwner_.reset();
}

void SquadBase::onAttackDone(int, BaseObject*, const std::any&) { morale_->onAttack(); }

void SquadBase::forwardEvent(int type, BaseObject*, const std::any& data) { dispatchEvent(type, data); }

void SquadBase::onChangeMorale(int, BaseObject*, const std::any& data) {
    const auto status = std::any_cast<MoraleStatus>(data);
    const double time = morale_->modeTimeLeft();
    bool anyAlive = false;
    for (auto& [index, unit] : units_) {
        if (!unit->isDestroyed()) {
            unit->getAbilities().addMoraleEffect(status, time);
            anyAlive = true;
        }
    }
    if (anyAlive) moraleChanged(status);
}

void SquadBase::addXP(double xp) {
    const int before = getLevel();
    xp_ += xp * bonus_.xpFactor;
    const int after = getLevel();
    if (before < after) makelevelUP(after);
}

void SquadBase::makelevelUP(int level) {
    maxHP_ = 0;
    for (auto& [index, unit] : units_) {
        const int oldMax = unit->getMaxHP();
        unit->getAbilities().getUnitParams().setLevel(level);
        if (unit->getHP() > 0) unit->getAbilities().setHealth(unit->getHP() + unit->getMaxHP() - oldMax);
        maxHP_ += unit->getMaxHP();
    }
}

// --- Spells ----------------------------------------------------------------------------------------------

void SquadBase::heal(int amount) {
    int left = amount;
    while (left > 0) {
        Unit* unit = findWoundedUnit();
        if (unit == nullptr) {
            unit = findDeadUnit();
            if (unit == nullptr) break;
            left = unit->resurrect(left, getMainUnit()->getPosition());
        } else {
            left = unit->heal(left);
        }
    }
}

bool SquadBase::enchant(const Point2D& point, double radius, double seconds, int resistPercent) {
    bool affected = false;
    for (auto& [index, unit] : units_) {
        if (!unit->isDestroyed() && unit->getPosition().getDistanceTo(point) <= radius) {
            affected = true;
            unit->setMaxDamageTime(seconds);
            unit->addDamageResist(seconds, resistPercent);
        }
    }
    return affected;
}

void SquadBase::killAll() {
    for (auto& [index, unit] : units_)
        if (!unit->isDestroyed()) unit->kill();
    killed_ = true;
}

void SquadBase::terrify() {
    if (morale_->isLocked()) return;
    Unit* leader = getMainUnit();
    if (leader != nullptr && leader->getAbilities().hasSpellDefence()) return;
    if (morale_->isHeroic()) {
        morale_->setNormalMode();
        morale_->setJustAboveHalf();
    } else if (morale_->isNormal()) {
        morale_->setFearedMode();
    }
}

bool SquadBase::setSpellDefence(const Point2D& point, double radius) {
    bool affected = false;
    for (auto& [index, unit] : units_) {
        if (!unit->isDestroyed() && unit->getPosition().getDistanceTo(point) <= radius) {
            affected = true;
            unit->setSpellDefence();
        }
    }
    return affected;
}

void SquadBase::makeHeroic() {
    if (!morale_->isLocked() && !morale_->isHeroic()) morale_->setHeroicMode();
}

bool SquadBase::setDamageFactor(const Point2D& point, double radius, double factor) {
    bool affected = false;
    for (auto& [index, unit] : units_) {
        if (!unit->isDestroyed() && unit->getPosition().getDistanceTo(point) <= radius) {
            affected = true;
            unit->setDamageFactor(factor);
        }
    }
    return affected;
}

// --- Frame -------------------------------------------------------------------------------------------------

void SquadBase::customUpdate(double dt) {
    BaseObject::customUpdate(dt);
    previousHP_ = currentHP_;
    currentHP_ = 0;
    for (auto& [index, unit] : units_) {
        unit->update(dt);
        currentHP_ += unit->getHP();
    }
    if (currentHP_ > 0) updateMorale(dt);
}

void SquadBase::customDispose() {
    beforeDispose();
    disposeMorale();
    for (auto& [index, unit] : units_) unit->dispose();
    units_.clear();
    keyNode_ = nullptr;
    owner_.reset();
    lastAttackerOwner_.reset();
    BaseObject::customDispose();
}

} // namespace takeover
