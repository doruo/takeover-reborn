#include "game/sim/abilities/UnitParams.h"

#include "game/framework/core/As3.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/sim/battle/ArmyStatBonus.h"

#include <algorithm>
#include <cmath>

namespace takeover {

namespace {
/// round(max(ceil(value * (1 + level / 10)), value + level)): +10% per
/// level, at least +1 per level.
int scaleByLevel(int value, int level) {
    const double factor = 1 + level / 10.0;
    return static_cast<int>(as3::round(std::max<double>(ArmyStatBonus::apply(value, factor), value + level)));
}
} // namespace

int UnitParams::levelHP(const ArmyStat& stat, const ArmyStatBonus& bonus, int level) {
    return scaleByLevel(bonus.getHP(stat.hp), level);
}

int UnitParams::levelDamageMin(const ArmyStat& stat, const ArmyStatBonus& bonus, int level) {
    return scaleByLevel(bonus.getDamage(stat.dmgMin), level);
}

int UnitParams::levelDamageMax(const ArmyStat& stat, const ArmyStatBonus& bonus, int level) {
    return scaleByLevel(bonus.getDamage(stat.dmgMax), level);
}

void UnitParams::update(double dt) {
    maxDamageTime_ -= dt;
    resetParams();
}

void UnitParams::setLevel(int level) {
    const double factor = 1 + level / 10.0;
    dmgMin = static_cast<int>(as3::round(std::max<double>(ArmyStatBonus::apply(baseDmgMin_, factor), dmgMin + level)));
    // As the original: the floor of the max damage uses the min damage.
    dmgMax = static_cast<int>(as3::round(std::max<double>(ArmyStatBonus::apply(baseDmgMax_, factor), dmgMin + level)));
    getParam(HP).setMax(static_cast<int>(as3::round(std::max<double>(ArmyStatBonus::apply(baseHP_, factor), baseHP_ + level))));
}

void UnitParams::resetParams() {
    for (auto& [name, value] : params_) value.reset();
    damageFactor_ = nextDamageFactor;
    nextDamageFactor = 1;
}

int UnitParams::applyCritical(int damage) const {
    const double chance = getParamCurValue(PRP_DOUBLE_DMG_CHANCE) / 100.0;
    return as3::random() < chance ? damage * 2 : damage;
}

void UnitParams::init(const ArmyStat& stat, const ArmyStatBonus& bonus) {
    params_.clear();
    baseHP_ = bonus.getHP(stat.hp);
    const auto add = [&](const std::string& name, int base, int min, int max) {
        params_.emplace(name, BoundedValue(name, base, min, max));
    };
    add(HP, baseHP_, 0, baseHP_);
    add(ATTACK_SPEED, stat.attackSpeed, 0, 9999);
    add(SPEED, bonus.getSpeed(stat.speed), 0, 9999);
    add(RANGE, stat.range, 0, 9999);
    add(MELEE_PENALTY, stat.meleePenalty, 0, 100);
    add(PRP_MORALE_BONUS, 10, 1, 100);
    add(PRP_MORALE_LOCK, 0, 0, 1);
    add(PRP_FEAR_LOCK, 0, 0, 1);
    add(PRP_MORALE_BONUS_FOR_ATTACK, 0, 0, 9999);
    add(PRP_DOUBLE_DMG_CHANCE, bonus.mechanismCritChance, 0, 100);
    add(PRP_FREEZE_CHANCE, bonus.infantryFreezeChance, 0, 100);
    add(PRP_FREEZE_DURATION, 3 + bonus.freezeDurationBonus, 0, 999999);
    add(PRP_PURSUIT_TIME, 0, 0, 7500);
    add("ShieldWall", 0, 0, 9999);
    add("Charge", 0, 0, 9999);
    add("SplashDamage", 0, 0, 9999);
    add("SiegeWeapon", bonus.strongholdDamageBonus, 0, 9999);
    add("DivinePresence", 0, 0, 9999);
    add("Poisonous", 0, 0, 9999);
    add("Intimidiation", 0, 0, 9999);
    add("HorrificDash", 0, 0, 9999);
    add("LongSpear", 0, 0, 9999);
    add("Explosive", 0, 0, 9999);
    add("FreezeArrow", 0, 0, 9999);
    add("FreezeBlast", 0, 0, 9999);
    add("Mechanism", 0, 0, 1);
    add("MartialArtists", 0, 0, 1);
    add("Undead", 0, 0, 1);
    add(PRP_SPEAR_DEFENCE, stat.unitType == UnitType::Cavalry ? 0 : 1, 0, 1);
    nextDamageFactor = 1;
    resetParams();
    baseDmgMin_ = bonus.getDamage(stat.dmgMin);
    baseDmgMax_ = bonus.getDamage(stat.dmgMax);
    setLevel(0);
    maxDamageTime_ = 0;
}

int UnitParams::rollDamage() const {
    int damage = 0;
    if (isMaxDamage()) {
        damage = static_cast<int>(dmgMax * moraleFactor() * damageFactor_ + 0.5);
    } else {
        const int spread = dmgMax - dmgMin + 1;
        damage = static_cast<int>((dmgMin + as3::random() * spread) * moraleFactor() * damageFactor_);
    }
    return applyCritical(damage);
}

int UnitParams::applyMeleePenalty(int damage) const {
    const double factor = (100 - getParamCurValue(MELEE_PENALTY)) / 100.0;
    return static_cast<int>(damage * factor + 0.5);
}

std::string UnitParams::debugString() const {
    return "HP: " + std::to_string(getParamCurValue(HP)) + "\tMAX_HP: " + std::to_string(params_.at(HP).getMax()) +
           "\tDMG(min): " + std::to_string(dmgMin) + "\tDMG(max): " + std::to_string(dmgMax) +
           "\tATK(speed): " + std::to_string(getParamCurValue(ATTACK_SPEED)) +
           "\tRNG: " + std::to_string(getParamCurValue(RANGE)) + "\tSPEED: " + std::to_string(getParamCurValue(SPEED)) + "\t";
}

} // namespace takeover
