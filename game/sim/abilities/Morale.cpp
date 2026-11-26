#include "game/sim/abilities/Morale.h"

#include "game/sim/abilities/AbilityEffects.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/sim/battle/ArmyStatBonus.h"
#include "game/sim/squads/MoraleStatus.h"

namespace takeover {

const int Morale::EVENT_MORALE_CHANGED = BaseObject::newEventType();

void Morale::setLocks(bool moraleLocked, bool fearLocked, int attackBonus) {
    moraleLocked_ = moraleLocked;
    fearLocked_ = fearLocked;
    attackBonus_ = attackBonus;
}

std::string Morale::toString() const {
    return "MORALE(BASE: " + std::to_string(getParamCurValue("PRP_MORALE_BASE")) +
           "; CUR: " + std::to_string(getParamCurValue("PRP_MORALE_CUR")) +
           "; MODE: " + std::to_string(getParamCurValue("PRP_MORALE_MODE")) +
           "; BONUS: " + std::to_string(getParamCurValue("PRP_MORALE_BONUS")) +
           "; ATK: " + std::to_string(getParamCurValue("PRP_MORALE_ATK")) +
           "; DEF: " + std::to_string(getParamCurValue("PRP_MORALE_DEF")) +
           "; CALM: " + std::to_string(getParamCurValue("PRP_MORALE_CALM")) + ") ";
}

void Morale::setNormalMode() {
    getParam("PRP_MORALE_MODE").setValue(MODE_NORMAL);
    setStatus(MoraleStatus::Neutral);
}

void Morale::setHeroicMode() {
    modeTimeLeft_ = MODE_DURATION;
    getParam("PRP_MORALE_MODE").setValue(MODE_HEROIC);
    setStatus(MoraleStatus::Heroism);
}

void Morale::setFearedMode() {
    if (fearLocked_) return;
    modeTimeLeft_ = MODE_DURATION;
    getParam("PRP_MORALE_MODE").setValue(MODE_FEARED);
    setStatus(MoraleStatus::Feared);
}

void Morale::setJustAboveHalf() { getParam("PRP_MORALE_CUR").setValue(51); }

void Morale::resetParams() {
    for (auto& [name, value] : params_) value.reset();
}

void Morale::onHitBy(Morale& attacker) {
    if (moraleLocked_) return;
    BoundedValue& defence = getParam("PRP_MORALE_DEF");
    const int attack = attacker.getParamCurValue("PRP_MORALE_ATK") + attackBonus_;
    const int loss = static_cast<int>((defence.getMax() - defence.getValue()) * attack / static_cast<double>(defence.getMax()));
    getParam("PRP_MORALE_CUR").add(-loss);
}

void Morale::recover() {
    const int base = getParamCurValue("PRP_MORALE_BASE");
    BoundedValue& current = getParam("PRP_MORALE_CUR");
    const int value = current.getValue();
    const int step = PercentModifier::percentOf(base, getParamCurValue("PRP_MORALE_CALM"));
    if (base > value) current.setValue(step < base - value ? value + step : base);
    else current.setValue(step < value - base ? value - step : base);
}

void Morale::init(const ArmyStat& stat, const ArmyStatBonus& bonus) {
    params_.clear();
    setLocks(false, false, 0);
    const int base = bonus.getBaseMorale(stat.morale) * 10;
    const auto add = [&](const std::string& name, int value, int min, int max) {
        params_.emplace(name, BoundedValue(name, value, min, max));
    };
    add("PRP_MORALE_CUR", base, 0, 1000);
    add("PRP_MORALE_BASE", base, 200, 800);
    add("PRP_MORALE_CALM", 1, 0, 1000);
    add("PRP_MORALE_BONUS", 50, 0, 1000);
    add("PRP_MORALE_ATK", 50, 0, 1000);
    add("PRP_MORALE_DEF", 0, 0, 1000);
    add("PRP_MORALE_MODE", 0, -1, 1);
    resetParams();
    recoveryTimer_ = 0;
}

MoraleStatus Morale::getStatus() const { return static_cast<MoraleStatus>(status_); }

double Morale::ratio() const {
    return static_cast<double>(getParamCurValue("PRP_MORALE_CUR")) / getParamCurValue("PRP_MORALE_BASE");
}

void Morale::customUpdate(double dt) {
    if (moraleLocked_) return;
    BoundedValue& current = getParam("PRP_MORALE_CUR");
    int value = current.getValue();
    if (isNormal()) {
        if (value >= current.getMax()) {
            setHeroicMode();
        } else if (value <= current.getMin()) {
            setFearedMode();
        } else {
            recoveryTimer_ -= dt;
            if (recoveryTimer_ < 0) {
                recover();
                recoveryTimer_ = RECOVERY_PERIOD;
            }
            if (isNormal()) {
                value = current.getValue();
                if (value < 350) setStatus(MoraleStatus::LowMorale);
                else if (value > 650) setStatus(MoraleStatus::HightMorale);
                else setStatus(MoraleStatus::Neutral);
            }
        }
    } else {
        modeTimeLeft_ -= dt;
        if (modeTimeLeft_ <= 0) {
            if (isFeared()) getParam("PRP_MORALE_CUR").setValue(getParam("PRP_MORALE_BASE").getMin());
            else getParam("PRP_MORALE_CUR").setValue(getParam("PRP_MORALE_BASE").getMax());
            setNormalMode();
        }
    }
    BaseObject::customUpdate(dt);
}

void Morale::setStatus(MoraleStatus status) {
    status_ = static_cast<int>(status);
    hasStatus_ = true;
    dispatchEvent(EVENT_MORALE_CHANGED, status);
}

void Morale::onAttack() {
    if (moraleLocked_) return;
    getParam("PRP_MORALE_CUR").add(getParamCurValue("PRP_MORALE_BONUS"));
}

} // namespace takeover
