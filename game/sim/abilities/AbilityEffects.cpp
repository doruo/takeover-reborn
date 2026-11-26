#include "game/sim/abilities/AbilityEffects.h"

#include "game/framework/core/As3.h"
#include "game/sim/abilities/UnitParams.h"
#include "game/sim/battle/ArmyStatBonus.h"

#include <cstdlib>

namespace takeover {

    namespace {
        double parseFloat(const std::string& s) { return std::atof(s.c_str()); }
    } // namespace

    const int AbilityEffect::EVENT_EFFECT_ATTACK = BaseObject::newEventType();

    // /----- ABILITY EFFECT -----/

    AbilityEffect::AbilityEffect(int sourceId, std::string name, std::string paramName, int durationMs, int kind)
        : Property(std::move(name)), sourceId_(sourceId), kind_(kind), paramName_(std::move(paramName)),
          durationMs_(durationMs), duration_(durationMs < 0 ? INFINITE_TIME : durationMs / 1000.0) {}

    std::string AbilityEffect::displayName(const std::string& a, const std::string& param, const ArmyStatBonus& bonus) {
        if (a == SHIELD_WALL)
            return "SHIELD WALL";

        if (a == DISCIPLINE)
            return "DISCIPLINE";

        if (a == CHARGE)
            return "CHARGE " + as3::toString(parseFloat(param) - 1 + bonus.chargeFactor) + "x";

        if (a == SPLASH_DAMAGE)
            return "SPLASH DAMAGE";

        if (a == MECHANISM)
            return NAME_MECHANISM;

        if (a == SIEGE_WEAPON)
            return "BESIEGER";

        if (a == DIVINE_PRESENCE)
            return "DIVINE PRESENCE";

        if (a == MARTIAL_ARTISTS)
            return "MARTIAL ARTISTS";

        if (a == UNDEAD)
            return NAME_UNDEAD;

        if (a == POISONOUS)
            return "POISONOUS";

        if (a == INTIMIDIATION)
            return "INTIMIDIATION";

        if (a == HORRIFIC_DASH)
            return "DASH";

        if (a == POISON_IMMUNITY)
            return "POISON IMMUNITY";

        if (a == LONG_SPEAR)
            return "LONG SPEAR " + as3::toString(1 + parseFloat(param) / 100) + "x";

        if (a == EXPLOSIVE)
            return "EXPLOSIVE";

        if (a == FREEZE_ARROW)
            return "FREEZE ARROW";

        if (a == FREEZE_BLAST)
            return "FREEZE BLAST";

        return "";
    }

    std::string AbilityEffect::description(const std::string& a, const std::string& param, const ArmyStatBonus& bonus) {
        if (a == SHIELD_WALL)
            return "Gets less damage by the ranged attacks";

        if (a == DISCIPLINE)
            return "Can't be frightened";

        if (a == CHARGE)
            return "Speeds up and causes " + as3::toString((parseFloat(param) - 1 + bonus.chargeFactor) * 100) +
                   "% damage during the first attack";
        if (a == SPLASH_DAMAGE)
            return "Damages all the enemies at close range";

        if (a == MECHANISM)
            return "Not affected by morale. Can't be poisoned or healed";

        if (a == SIEGE_WEAPON)
            return "Causes " + as3::toString(parseFloat(param) * 10) + "% damage to strongholds";

        if (a == DIVINE_PRESENCE)
            return "Improves morale of the nearby allied units";

        if (a == MARTIAL_ARTISTS)
            return "Have no melee penalty";

        if (a == UNDEAD)
            return "Not affected by morale and poison";

        if (a == POISONOUS)
            return "Poisoned enemies get additional continuous damage";

        if (a == INTIMIDIATION)
            return "Demoralizes the enemies greatly";

        if (a == HORRIFIC_DASH)
            return "Teleports to the enemy from short distance";

        if (a == POISON_IMMUNITY)
            return "Can't be poisoned";

        if (a == LONG_SPEAR)
            return "Causes " + as3::toString(100 + parseFloat(param)) + "% damage to cavalry";

        if (a == EXPLOSIVE)
            return "Explodes after death damaging the nearby enemies";

        if (a == FREEZE_ARROW)
            return "Freezes enemies";

        if (a == FREEZE_BLAST)
            return "Freezes enemies at close range";

        return "";
    }

    std::shared_ptr<AbilityEffect> AbilityEffect::getCopy() const {
        return std::make_shared<AbilityEffect>(sourceId_, name_, paramName_, durationMs_, kind_);
    }

    void AbilityEffect::customUpdate(double dt) {
        if (!finished_ && getActiveTime() + dt > duration_)
            finish();

        Property::customUpdate(dt);
    }

    void AbilityEffect::customDispose() {
        params_ = nullptr;
        Property::customDispose();
    }

    // /----- VALUE EFFECT -----/

    ValueEffect::ValueEffect(int sourceId, std::string name, std::string paramName, int value, int durationMs)
        : AbilityEffect(sourceId, std::move(name), std::move(paramName), durationMs, KIND_DAMAGE), value_(value) {}

    std::shared_ptr<AbilityEffect> ValueEffect::getCopy() const {
        return std::make_shared<ValueEffect>(sourceId_, name_, paramName_, value_, durationMs_);
    }

    void ValueEffect::customUpdate(double dt) {
        params_->getParam(paramName_).addBase(value_);
        AbilityEffect::customUpdate(dt);
    }

    // /----- SIMPLE ABILITIES -----/

    DisciplineEffect::DisciplineEffect(int sourceId, int durationMs)
        : AbilityEffect(sourceId, DISCIPLINE, DISCIPLINE, durationMs, KIND_ABILITY) {}

    std::shared_ptr<AbilityEffect> DisciplineEffect::getCopy() const {
        return std::make_shared<DisciplineEffect>(sourceId_, durationMs_);
    }

    void DisciplineEffect::customUpdate(double dt) {
        params_->getParam(UnitParams::PRP_FEAR_LOCK).setBase(1);
        AbilityEffect::customUpdate(dt);
    }

    ParamModifier::ParamModifier(int sourceId, std::string name, std::string paramName, int value, int durationMs, int kind)
        : AbilityEffect(sourceId, std::move(name), std::move(paramName), durationMs, kind), value_(value) {}

    std::shared_ptr<AbilityEffect> ParamModifier::getCopy() const {
        return std::make_shared<ParamModifier>(sourceId_, name_, paramName_, value_, durationMs_, kind_);
    }

    void ParamModifier::customUpdate(double dt) {
        params_->getParam(paramName_).add(value_);
        AbilityEffect::customUpdate(dt);
    }

    PoisonImmunityEffect::PoisonImmunityEffect(int sourceId, int durationMs)
        : AbilityEffect(sourceId, POISON_IMMUNITY, POISON_IMMUNITY, durationMs, KIND_ABILITY) {}

    bool PoisonImmunityEffect::makeAddPropertyCollision(IProperty& added) { return added.getName() == ATK_POISON; }

    std::shared_ptr<AbilityEffect> PoisonImmunityEffect::getCopy() const {
        return std::make_shared<PoisonImmunityEffect>(sourceId_, durationMs_);
    }

    DamageResistEffect::DamageResistEffect(int sourceId, int percent, int durationMs)
        : AbilityEffect(sourceId, DAMAGE_RESIST, DAMAGE_RESIST, durationMs, KIND_ABILITY), percent_(percent) {}

    bool DamageResistEffect::makeAddPropertyCollision(IProperty& added) {
        const std::string& kind = added.getName();
        if (kind != ATK_MELEE && kind != ATK_DISTANCE && kind != ATK_CHARGE && kind != ATK_POISON && kind != ATK_FREEZ)
            return false;

        // The original casts every one of them to a ValueEffect. Freezing ones
        // are PercentModifiers, which this cast doesn't fit: leave them alone.
        if (auto* effect = dynamic_cast<ValueEffect*>(&added)) {
            int value = effect->getValue();
            value -= PercentModifier::percentOf(value, percent_);
            effect->setValue(value);
        }

        return false;
    }

    std::shared_ptr<AbilityEffect> DamageResistEffect::getCopy() const {
        return std::make_shared<DamageResistEffect>(sourceId_, percent_, durationMs_);
    }

    PercentModifier::PercentModifier(int sourceId, std::string name, std::string paramName, int percent, int durationMs,
                                     int kind)
        : AbilityEffect(sourceId, std::move(name), std::move(paramName), durationMs, kind), percent_(percent) {}

    std::shared_ptr<AbilityEffect> PercentModifier::getCopy() const {
        return std::make_shared<PercentModifier>(sourceId_, name_, paramName_, percent_, durationMs_, kind_);
    }

    void PercentModifier::customUpdate(double dt) {
        const int value = params_->getParamCurValue(paramName_);
        params_->getParam(paramName_).add(percentOf(value, percent_));
        AbilityEffect::customUpdate(dt);
    }

    // /----- ATTACKS -----/

    AttackEffect::AttackEffect(int sourceId, std::string name, int damage, int durationMs)
        : ValueEffect(sourceId, std::move(name), UnitParams::HP, -damage, durationMs) {}

    std::shared_ptr<AbilityEffect> AttackEffect::getCopy() const {
        return std::make_shared<AttackEffect>(sourceId_, name_, -value_, durationMs_);
    }

    void AttackEffect::customUpdate(double dt) {
        // A spear only hurts cavalry (units without spear defence).
        if (name_ == ATK_SPEAR && params_->getParamCurValue(UnitParams::PRP_SPEAR_DEFENCE) != 0)
            return;

        dispatchEvent(EVENT_EFFECT_ATTACK, sourceId_);
        ValueEffect::customUpdate(dt);
    }

    PeriodicEffect::PeriodicEffect(int sourceId, std::string name, std::string paramName, double period, int value,
                                   int durationMs)
        : ValueEffect(sourceId, std::move(name), std::move(paramName), -value, durationMs), period_(period) {}

    bool PeriodicEffect::makeAddPropertyCollision(IProperty& added) { return added.getName() == getName(); }

    std::shared_ptr<AbilityEffect> PeriodicEffect::getCopy() const {
        // As the original: the period isn't copied.
        return std::make_shared<PeriodicEffect>(sourceId_, name_, paramName_, 0, -value_, durationMs_);
    }

    void PeriodicEffect::customUpdate(double dt) {
        if (timer_ <= 0 || isFinished()) {
            timer_ += period_;
            ValueEffect::customUpdate(dt);
        }

        timer_ -= dt;
    }

    ShieldWallEffect::ShieldWallEffect(int sourceId, int percent, int durationMs)
        : AbilityEffect(sourceId, SHIELD_WALL, SHIELD_WALL, durationMs, KIND_ABILITY), percent_(percent) {}

    bool ShieldWallEffect::makeAddPropertyCollision(IProperty& added) {
        if (added.getName() == ATK_DISTANCE) {
            auto& attack = dynamic_cast<AttackEffect&>(added);
            int value = attack.getValue();
            value -= PercentModifier::percentOf(value, percent_);
            attack.setValue(value);
        }

        return false;
    }

    std::shared_ptr<AbilityEffect> ShieldWallEffect::getCopy() const {
        return std::make_shared<ShieldWallEffect>(sourceId_, percent_, durationMs_);
    }

    MechanismEffect::MechanismEffect(int sourceId, int durationMs)
        : AbilityEffect(sourceId, MECHANISM, MECHANISM, durationMs, KIND_ABILITY) {}

    void MechanismEffect::setUnitParams(UnitParams* params) {
        AbilityEffect::setUnitParams(params);
        params_->getParam(MECHANISM).setBase(1);
    }

    bool MechanismEffect::makeAddPropertyCollision(IProperty& added) { return added.getName() == ATK_POISON; }

    std::shared_ptr<AbilityEffect> MechanismEffect::getCopy() const {
        return std::make_shared<MechanismEffect>(sourceId_, durationMs_);
    }

    void MechanismEffect::customUpdate(double dt) {
        params_->getParam(UnitParams::PRP_MORALE_LOCK).setBase(1);
        AbilityEffect::customUpdate(dt);
    }

    UndeadEffect::UndeadEffect(int sourceId, int durationMs) : AbilityEffect(sourceId, UNDEAD, UNDEAD, durationMs, KIND_ABILITY) {}

    bool UndeadEffect::makeAddPropertyCollision(IProperty& added) { return added.getName() == ATK_POISON; }

    std::shared_ptr<AbilityEffect> UndeadEffect::getCopy() const { return std::make_shared<UndeadEffect>(sourceId_, durationMs_); }

    void UndeadEffect::customUpdate(double dt) {
        params_->getParam(UnitParams::PRP_MORALE_LOCK).setBase(1);
        AbilityEffect::customUpdate(dt);
    }

    MaxHPPercentDamage::MaxHPPercentDamage(int sourceId, std::string name, int percent, int durationMs)
        : AbilityEffect(sourceId, std::move(name), UnitParams::HP, durationMs, KIND_DAMAGE), value_(percent) {}

    std::shared_ptr<AbilityEffect> MaxHPPercentDamage::getCopy() const {
        return std::make_shared<MaxHPPercentDamage>(sourceId_, name_, value_, durationMs_);
    }

    void MaxHPPercentDamage::customUpdate(double dt) {
        BoundedValue& hp = params_->getParam(paramName_);
        hp.addBase(-PercentModifier::percentOf(hp.getMax(), value_));
        AbilityEffect::customUpdate(dt);
    }

} // namespace takeover
