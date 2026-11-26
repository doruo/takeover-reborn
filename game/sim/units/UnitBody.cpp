#include "game/sim/units/UnitBody.h"

#include "game/sim/battle/ArmyStat.h"

#include <cmath>

namespace takeover {

bool UnitBody::canMelee() const { return getArmyStat()->meleePenalty < 100; }

bool UnitBody::isMechanism() const {
    return abilities_->getUnitParams().getParamCurValue(AbilityEffect::MECHANISM) != 0;
}

UnitBody& UnitBody::initAbilities(const ArmyStat& stat, const ArmyStatBonus& bonus) {
    abilities_ = std::make_shared<Abilities>(stat, bonus, getId());
    return *this;
}

void UnitBody::increaseSpeed(double factor, double seconds) {
    abilities_->addProperty(std::make_shared<ParamModifier>(getId(), "SPEED_BONUS", UnitParams::SPEED,
                                                            static_cast<int>(std::floor(getSpeed() * factor)),
                                                            static_cast<int>(std::round(seconds * 1000))));
}

bool UnitBody::hasDash() const {
    return abilities_->getUnitParams().getParamCurValue(AbilityEffect::HORRIFIC_DASH) != 0;
}

bool UnitBody::hasCharge() const { return abilities_->getUnitParams().getParamCurValue(AbilityEffect::CHARGE) != 0; }

double UnitBody::getSpeed() const { return abilities_->getUnitParams().getParamCurValue(UnitParams::SPEED) / 2.0; }

int UnitBody::getRange() const { return getArmyStat()->range; }

double UnitBody::attackAnimFactor() const { return getArmyStat()->attackAnimFactor; }

double UnitBody::minRangedDistance() const { return getArmyStat()->range > 0 ? 25 : 0; }

double UnitBody::rangeValue() const { return getArmyStat()->range; }

void UnitBody::customUpdate(double dt) {
    MovingBody::customUpdate(dt);
    abilities_->update(dt);
}

void UnitBody::customDispose() {
    if (abilities_) {
        abilities_->dispose();
        abilities_.reset();
    }
    MovingBody::customDispose();
}

} // namespace takeover
