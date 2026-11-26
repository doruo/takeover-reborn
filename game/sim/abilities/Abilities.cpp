#include "game/sim/abilities/Abilities.h"

#include "game/framework/core/As3.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/sim/squads/MoraleStatus.h"
#include "game/sim/units/Unit.h"

#include <cmath>
#include <cstdlib>
#include <sstream>

namespace takeover {

const int Abilities::EVENT_PROPERTY_ADDED = BaseObject::newEventType();
const int Abilities::EVENT_PROPERTY_REMOVED = BaseObject::newEventType();
const int Abilities::EVENT_ATTACK_DONE = BaseObject::newEventType();
const int Abilities::EVENT_HIT = BaseObject::newEventType();

namespace {
int parseInt(const std::vector<std::string>& parts, std::size_t index) {
    return index < parts.size() ? std::atoi(parts[index].c_str()) : 0;
}
} // namespace

Abilities::Abilities(const ArmyStat& stat, const ArmyStatBonus& bonus, int ownerId) {
    // Original: _SafeStr_2865 then _SafeStr_2863.
    x = 0;
    y = 0;
    scale = 1;
    speed = 30;
    setType(stat);
    stat_ = &stat;
    bonus_ = bonus;
    params_ = std::make_unique<UnitParams>(stat, bonus);
    ownerId_ = ownerId;
    addAbilities(stat.abilities, bonus);
    spellDefence_ = 0;
}

std::vector<std::string> Abilities::splitAbilities(const std::string& abilities) {
    std::vector<std::string> parts;
    std::stringstream stream(abilities);
    for (std::string part; std::getline(stream, part, ';');) parts.push_back(part);
    if (abilities.empty() || abilities.back() == ';') parts.push_back("");
    return parts;
}

std::vector<std::string> Abilities::splitAbility(const std::string& ability) {
    std::vector<std::string> parts;
    std::stringstream stream(ability);
    for (std::string part; std::getline(stream, part, ' ');) parts.push_back(part);
    if (ability.empty()) parts.push_back("");
    return parts;
}

bool Abilities::hasAbility(const std::string& ability, const std::string& abilities) {
    for (const std::string& entry : splitAbilities(abilities))
        if (splitAbility(entry)[0] == ability) return true;
    return false;
}

void Abilities::addFreezeEffects(EffectList& effects, int durationMs, int sourceId, int percent) {
    effects.push_back(std::make_shared<PercentModifier>(sourceId, AbilityEffect::ATK_FREEZ, UnitParams::SPEED, percent,
                                                        durationMs, AbilityEffect::KIND_DAMAGE));
    effects.push_back(std::make_shared<PercentModifier>(sourceId, AbilityEffect::ATK_FREEZ, UnitParams::ATTACK_SPEED,
                                                        percent, durationMs, AbilityEffect::KIND_DAMAGE));
}

UnitSize Abilities::sizeOf(const ArmyStat& stat) {
    switch (stat.unitType) {
    case UnitType::Cavalry: return UnitSize::Middle;
    case UnitType::Siege: return UnitSize::Gaint;
    default:
        if (&stat == &ArmyStat::AVATAR || &stat == &ArmyStat::GIANTBOTS || &stat == &ArmyStat::OGRE) return UnitSize::Gaint;
        return UnitSize::Normal;
    }
}

int Abilities::massOf(UnitSize size) {
    switch (size) {
    case UnitSize::Normal: return 1;
    case UnitSize::Middle: return 4;
    case UnitSize::Gaint: return 15;
    }
    return 1;
}

void Abilities::setHealth(int hp) { params_->getParam(UnitParams::HP).setBase(hp); }

void Abilities::setType(const ArmyStat& stat) {
    type = stat.unitType;
    size = sizeOf(stat);
    mass = massOf(size);
}

void Abilities::removeProperty(std::shared_ptr<IProperty> property) {
    dispatchEvent(EVENT_PROPERTY_REMOVED, property);
    PropertyContainer::removeProperty(std::move(property));
}

void Abilities::addAbilities(const std::string& abilities, const ArmyStatBonus& bonus) {
    using E = AbilityEffect;
    for (const std::string& entry : splitAbilities(abilities)) {
        const std::vector<std::string> parts = splitAbility(entry);
        const std::string& name = parts[0];
        const auto modifier = [&](const std::string& param, int value) {
            addProperty(std::make_shared<ParamModifier>(ownerId_, name, param, value));
        };
        if (name == E::SHIELD_WALL) addProperty(std::make_shared<ShieldWallEffect>(ownerId_, parseInt(parts, 1)));
        else if (name == E::CHARGE) modifier(name, chargeBonus(parseInt(parts, 1), bonus));
        else if (name == E::SPLASH_DAMAGE) modifier(name, parseInt(parts, 1));
        else if (name == E::SIEGE_WEAPON) modifier(name, parseInt(parts, 1) * 10);
        else if (name == E::DIVINE_PRESENCE) modifier(name, parseInt(parts, 1));
        else if (name == E::POISONOUS) modifier(name, parseInt(parts, 1) + bonus_.poisonDamageBonus);
        else if (name == E::HORRIFIC_DASH) modifier(name, parseInt(parts, 1));
        else if (name == E::LONG_SPEAR) modifier(name, parseInt(parts, 1));
        else if (name == E::EXPLOSIVE) modifier(name, parseInt(parts, 1));
        else if (name == E::FREEZE_ARROW) modifier(name, parseInt(parts, 1));
        else if (name == E::FREEZE_BLAST) modifier(name, parseInt(parts, 1));
        else if (name == E::INTIMIDIATION) modifier(UnitParams::PRP_MORALE_BONUS_FOR_ATTACK, parseInt(parts, 1));
        else if (name == E::DISCIPLINE) addProperty(std::make_shared<DisciplineEffect>(ownerId_));
        else if (name == E::MECHANISM) addProperty(std::make_shared<MechanismEffect>(ownerId_));
        else if (name == E::MARTIAL_ARTISTS) modifier(name, 1);
        else if (name == E::UNDEAD) addProperty(std::make_shared<UndeadEffect>(ownerId_));
        else if (name == E::POISON_IMMUNITY) addProperty(std::make_shared<PoisonImmunityEffect>(ownerId_));
    }
    // Giantbots regenerate a percentage of their HP every second.
    if (bonus.regenFactor != 1) {
        const int regen = bonus.getRegen(params_->getParam(UnitParams::HP).getMax());
        addProperty(std::make_shared<PeriodicEffect>(ownerId_, E::REGENERATION, UnitParams::HP, 1, -regen, -1));
    }
}

void Abilities::onEffectAttack(int, BaseObject*, const std::any& attackerId) {
    const int attacker = std::any_cast<int>(attackerId);
    dispatchEvent(EVENT_HIT, attacker);
    if (auto* unit = dynamic_cast<Unit*>(BaseObject::getObjectPtr(attacker)))
        unit->getAbilities().dispatchEvent(EVENT_ATTACK_DONE, attacker);
}

bool Abilities::addProperty(std::shared_ptr<IProperty> property) {
    auto effect = std::dynamic_pointer_cast<AbilityEffect>(property);
    effect->setUnitParams(params_.get());
    if (PropertyContainer::addProperty(property)) {
        dispatchEvent(EVENT_PROPERTY_ADDED, property);
        effect->addEventListener(AbilityEffect::EVENT_EFFECT_ATTACK, getId(),
                                 [this](int eventType, BaseObject* sender, const std::any& data) { onEffectAttack(eventType, sender, data); });
        return true;
    }
    return false;
}

void Abilities::addMoraleEffect(MoraleStatus status, double seconds) {
    const int durationMs = static_cast<int>(seconds * 1000);
    if (status == MoraleStatus::Heroism)
        addProperty(std::make_shared<ParamModifier>(ownerId_, UnitParams::PRP_MORALE_BONUS, UnitParams::PRP_MORALE_BONUS, 5, durationMs));
    else if (status == MoraleStatus::Feared)
        addProperty(std::make_shared<ParamModifier>(ownerId_, UnitParams::PRP_MORALE_BONUS, UnitParams::PRP_MORALE_BONUS, -5, durationMs));
}

void Abilities::addEffects(const EffectList& effects) {
    for (const auto& effect : effects)
        if (!(hasSpellDefence() && effect->isNegativeMagic())) addProperty(effect);
}

int Abilities::freezeDurationMs() const {
    return static_cast<int>(as3::round(params_->getParamCurValue(UnitParams::PRP_FREEZE_DURATION) * 1000.0));
}

int Abilities::getDamageForFortress() const {
    const int damage = params_->rollDamage();
    return ArmyStatBonus::apply(damage, (100 + params_->getParamCurValue("SiegeWeapon")) / 100.0);
}

double Abilities::meleeRadius() const {
    if (stat_->unitType == UnitType::Cavalry) return 16;
    if (stat_ == &ArmyStat::AVATAR || stat_ == &ArmyStat::OGRE || stat_ == &ArmyStat::GIANTBOTS) return 30;
    return 12;
}

void Abilities::customUpdate(double dt) {
    params_->update(dt);
    --spellDefence_;
    PropertyContainer::customUpdate(dt);
}

void Abilities::customDispose() {
    PropertyContainer::customDispose();
    // The parameters outlive the effects' last access (the original nulls them first).
    params_.reset();
}

EffectList Abilities::createMeleeAttack() {
    EffectList effects;
    int damage = params_->rollDamage();
    damage = params_->applyMeleePenalty(damage);
    if (damage > 0) {
        effects.push_back(std::make_shared<AttackEffect>(ownerId_, AbilityEffect::ATK_MELEE, damage));
        addSecondaryEffects(effects, damage);
    }
    return effects;
}

EffectList Abilities::createDistanceAttack() {
    EffectList effects;
    if (params_->getParamCurValue(UnitParams::RANGE) > 0) {
        const int damage = params_->rollDamage();
        effects.push_back(std::make_shared<AttackEffect>(ownerId_, AbilityEffect::ATK_DISTANCE, damage));
        addSecondaryEffects(effects, damage);
    }
    return effects;
}

int Abilities::chargeBonus(int charge, const ArmyStatBonus& bonus) {
    return charge > 0 ? bonus.getCharge((charge - 1) * 10) : 0;
}

void Abilities::addSecondaryEffects(EffectList& effects, int damage) {
    int value = params_->getParamCurValue("Poisonous");
    if (value > 0)
        effects.push_back(std::make_shared<PeriodicEffect>(ownerId_, AbilityEffect::ATK_POISON, UnitParams::HP, 3, value, 3000));
    value = std::max(params_->getParamCurValue("FreezeArrow"), params_->getParamCurValue("FreezeBlast"));
    if (value > 0) addFreezeEffects(effects, freezeDurationMs(), ownerId_, -value);
    const double freezeChance = params_->getParamCurValue(UnitParams::PRP_FREEZE_CHANCE) / 100.0;
    if (freezeChance > as3::random()) addFreezeEffects(effects, freezeDurationMs(), ownerId_);
    value = params_->getParamCurValue("Charge");
    if (value > 0) {
        // The charge bonus only counts on the first hit of a full pursuit.
        BoundedValue& pursuit = params_->getParam(UnitParams::PRP_PURSUIT_TIME);
        if (pursuit.getValue() == pursuit.getMax())
            effects.push_back(std::make_shared<AttackEffect>(ownerId_, AbilityEffect::ATK_CHARGE,
                                                             static_cast<int>(std::floor(damage * value / 10.0))));
    }
    value = params_->getParamCurValue("LongSpear");
    if (value > 0)
        effects.push_back(std::make_shared<AttackEffect>(ownerId_, AbilityEffect::ATK_SPEAR,
                                                         static_cast<int>(std::floor(damage * value / 100.0))));
}

} // namespace takeover
