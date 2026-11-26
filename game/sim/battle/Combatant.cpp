#include "game/sim/battle/Combatant.h"

#include "game/framework/core/As3.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/sim/spells/SpellInfo.h"

#include <algorithm>

namespace takeover {

    Combatant::Combatant(
        std::shared_ptr<CombatantKey> description_, 
        Race race, 
        const EdictBonuses& bonuses_
    ): 
    description(std::move(description_)), 
    bonuses(bonuses_),
    missionStats(std::make_shared<MissionStats>(race)), 
    race_(race) 
    {
        initBonuses();
        initSpells();
    }

    // --- Static tables ---------------------------------------------------------------------------

    std::uint32_t Combatant::raceColor(Race race) {
        switch (race) {
            case Race::TheEmpire: return 10027263;      // 0x98FFFF
            case Race::Westaria: return 4227072;        // 0x408000
            case Race::TheKhaganate: return 16724736;   // 0xFF3300
            case Race::TheCult: return 27349;           // 0x006AD5
            case Race::Bandits: return 0;               // 0x000000
        }
        return 0;
    }

    std::string Combatant::raceName(Race race) {
        switch (race) {
            case Race::TheEmpire: return "Rivadis Empire";
            case Race::Westaria: return "Duchy of Westaria";
            case Race::TheKhaganate: return "Crimson Horde";
            case Race::TheCult: return "Icedales Cult";
            case Race::Bandits: return "n/a";
        }
        return "";
    }

    int Combatant::raceTableIndex(Race race) {
        switch (race) {
            case Race::TheKhaganate: return 0;
            case Race::Westaria: return 1;
            case Race::TheCult: return 2;
            case Race::TheEmpire: return 3;
            case Race::Bandits: return 4;
        }
        return 0;
    }

    Race Combatant::raceFromTableIndex(int index) {
        switch (index) {
        case 0: return Race::TheKhaganate;
        case 1: return Race::Westaria;
        case 2: return Race::TheCult;
        case 3: return Race::TheEmpire;
        default: return Race::Bandits;
        }
    }

    ArmyStatBonus Combatant::makeArmyStatBonus(const ArmyStat& stat, const EdictBonuses& b) {
        
        ArmyStatBonus bonus;

        // Define default healthpoint, damage and charge
        switch (stat.unitType) {

            case UnitType::Infantry:
                bonus.hpFactor = b.factor(Bonus::INFANTRY_HP);
                bonus.infantryFreezeChance = b.value(Bonus::INFANTRY_FREEZE_CHANCE);
                break;

            case UnitType::Archers:
                bonus.damageFactor = b.factor(Bonus::RANGED_DAMAGE);
                break;

            case UnitType::Cavalry:
                bonus.chargeFactor = b.factor(Bonus::CHARGE_DAMAGE);
                bonus.hpFactor = b.factor(Bonus::CAVALRY_HP);
                break;

            case UnitType::Siege:
                bonus.damageFactor = b.factor(Bonus::RANGED_DAMAGE);
                break;

            case UnitType::Magic:
                break;
        }

        bonus.moraleFactor = b.factor(Bonus::BASE_MORALE);
        bonus.speedFactor = b.factor(Bonus::MOVING_SPEED);

        if (stat.unitType == UnitType::Siege) 
            bonus.speedFactor += b.factor(Bonus::SIEGE_SPEED) - 1;
            
        bonus.strongholdDamageBonus = b.value(Bonus::DAMAGE_VS_STRONGHOLDS);
        bonus.poisonDamageBonus = b.value(Bonus::POISON_DAMAGE);
        bonus.freezeDurationBonus = b.value(Bonus::FREEZE_DURATION);
        bonus.xpFactor = b.factor(Bonus::XP_RATE);

        if (&stat == &ArmyStat::GIANTBOTS) 
            bonus.regenFactor = b.value(Bonus::GIANTBOTS_REGENERATION) / 100.0;

        if (&stat == &ArmyStat::IMMORTALS) 
            bonus.addUnit = b.value(Bonus::IMMORTALS_EXTRA_UNITS);

        if (ArmyStat::hasAbility("Mechanism", stat.abilities)) 
            bonus.mechanismCritChance = b.value(Bonus::MECHANISM_CRIT);

        return bonus;
    }

    std::vector<const SpellInfo*> Combatant::raceSpells(Race race) {

        switch (race) {

            case Race::TheEmpire: 
                return {
                    &SpellInfo::TERROR, 
                    &SpellInfo::RAISE_THE_DEAD, 
                    &SpellInfo::BANNER_OF_DESECRATION
                };

            case Race::Westaria:
                return {
                    &SpellInfo::LORDS_JUDGEMENT, 
                    &SpellInfo::BANNER_OF_CONSECRATION, 
                    &SpellInfo::BANNER_OF_HEROISM
                };
            
            case Race::TheKhaganate:
                return {
                    &SpellInfo::SUMMON_STONEWORMS, 
                    &SpellInfo::DISENTOMB, 
                    &SpellInfo::BANNER_OF_BLOODLUST
                };
            
            case Race::TheCult: 
                return {
                    &SpellInfo::ICE_BLAST, 
                    &SpellInfo::FROST_ENCHANTMENT, 
                    &SpellInfo::BANNER_OF_LIBERATION
                };
            
            case Race::Bandits: 
                return {
                    nullptr, 
                    nullptr, 
                    nullptr
                };
        }
        return {};
    }

int Combatant::spellUnlockValue(const SpellInfo* spell, const EdictBonuses& b) {
    if (spell == &SpellInfo::LORDS_JUDGEMENT) return b.value(Bonus::LORDS_JUDGEMENT);

    if (spell == &SpellInfo::BANNER_OF_CONSECRATION) return b.value(Bonus::BANNER_OF_CONSECRATION);
    
    if (spell == &SpellInfo::BANNER_OF_HEROISM) return b.value(Bonus::BANNER_OF_HEROISM);
    
    if (spell == &SpellInfo::SUMMON_STONEWORMS) return b.value(Bonus::SUMMON_STONEWORMS);
    
    if (spell == &SpellInfo::DISENTOMB) return b.value(Bonus::DISENTOMB);
    
    if (spell == &SpellInfo::BANNER_OF_BLOODLUST) return b.value(Bonus::BANNER_OF_BLOODLUST);
    
    if (spell == &SpellInfo::ICE_BLAST) return b.value(Bonus::ICE_BLAST);
    
    if (spell == &SpellInfo::FROST_ENCHANTMENT) return b.value(Bonus::FROST_ENCHANTMENT);
    
    if (spell == &SpellInfo::BANNER_OF_LIBERATION) return b.value(Bonus::BANNER_OF_LIBERATION);
    
    if (spell == &SpellInfo::TERROR) return b.value(Bonus::TERROR);
    
    if (spell == &SpellInfo::RAISE_THE_DEAD) return b.value(Bonus::RAISE_THE_DEAD);
    
    if (spell == &SpellInfo::BANNER_OF_DESECRATION) return b.value(Bonus::BANNER_OF_DESECRATION);
    return 0;
}

    // --- Identity --------------------------------------------------------------------------------

    // --- Economy ------------------------------------------------------------------------------------

    int Combatant::getGoldSpeed() const {
        if (squadsCount_ == 0 || goldSpeed_ == 0) return goldSpeed_;
        const double extraSquads = std::min(squadsCount_, 10) - 1;
        const double factor = (100 - (30 + extraSquads * 5)) / 100;
        return static_cast<int>(as3::round(std::max(goldSpeed_ * factor, 1.0)));
    }

    int Combatant::getSupplyModifierPercent() const {
        if (squadsCount_ == 0) return 0;
        if (squadsCount_ >= 10) return 75;
        return 25 + squadsCount_ * 5;
    }

    int Combatant::getSupplyCost(SupplyBuildingType type) const {
        
        const int base = type == SupplyBuildingType::Empty ? 0 : 150;

        return static_cast<int>(
            as3::round(base * bonuses.factor(Bonus::SUPPLY_BUILDINGS_COST))
        );
    }

    void Combatant::addGold(int amount) {
        gold_ += amount;
        missionStats->add(MissionStats::GOLD_COLLECTED, amount);
    }

    void Combatant::addMana(int amount) {
        const int before = mana_;
        mana_ = std::min(mana_ + amount, MAX_MANA);
        missionStats->add(MissionStats::MANA_COLLECTED, mana_ - before);
    }

    void Combatant::spendGold(int amount) {
        gold_ -= amount;
        missionStats->add(MissionStats::GOLD_SPENT, amount);
    }

    void Combatant::tick() {
        addGold(getGoldSpeed());
        addMana(getManaSpeed());
        ultimateCharge_ = std::min(ultimateCharge_ + ultimateChargeSpeed_, ultimateChargeMax_);
    }

    void Combatant::setIncome(int goldSpeed, int supplyPower, int armyPower, int manaSpeed, int ultimateChargeSpeed) {
        goldSpeed_ = goldSpeed;
        manaSpeed_ = manaSpeed;
        supplyPower_ = supplyPower;
        armyPower_ = armyPower;
        ultimateChargeSpeed_ = ultimateChargeSpeed;
        if (ultimateChargeSpeed_ <= 0 && forcedUltimateFactor_ == 0) ultimateCharge_ = 0;
    }

    // --- Army ----------------------------------------------------------------------------------------

    int Combatant::getTier(UnitType type) const {
        int upgrade = 0;
        switch (type) {
        case UnitType::Infantry: upgrade = bonuses.value(Bonus::INFANTRY_TIER); break;
        case UnitType::Archers: upgrade = bonuses.value(Bonus::ARCHERS_TIER); break;
        case UnitType::Cavalry: upgrade = bonuses.value(Bonus::CAVALRY_TIER); break;
        case UnitType::Siege: upgrade = bonuses.value(Bonus::SIEGE_TIER); break;
        case UnitType::Magic: upgrade = bonuses.value(Bonus::MAGIC_TIER); break;
        }
        return upgrade + 1;
    }

    ArmyStat* Combatant::getArmyStat(UnitType type) const { return ArmyStat::find(race_, type, getTier(type)); }

    int Combatant::getUnitCost(const ArmyStat& stat) const {
        if (&stat != &ArmyStat::ANCIENT_RAIDERS && &stat != &ArmyStat::IMMORTALS) return stat.cost;
        return static_cast<int>(as3::round(stat.cost * bonuses.factor(Bonus::UNDEAD_COST)));
    }

    bool Combatant::canBuy(const ArmyStat& stat) const {
        return getGold() >= getUnitCost(stat) && supplyPower_ - armyPower_ >= stat.upkeep;
    }

    // --- Strongholds -------------------------------------------------------------------------------------

    void Combatant::setKeyNodes(std::vector<std::shared_ptr<KeyNode>> nodes) {
        keyNodes_ = std::move(nodes);
        missionStats->set(MissionStats::STRONGHOLDS_OWNED, static_cast<int>(keyNodes_.size()));
    }

    void Combatant::buildSupplyBuilding(SupplyBuildingType type) {
        int counter = 0;
        switch (type) {
        case SupplyBuildingType::Village: counter = MissionStats::FARMS_BUILT; break;
        case SupplyBuildingType::Mine: counter = MissionStats::MINES_BUILT; break;
        case SupplyBuildingType::Obelisk: counter = MissionStats::OBELISKS_BUILT; break;
        default: return;
        }
        missionStats->add(counter, 1);
    }

    // --- Spells ---------------------------------------------------------------------------------------------

    void Combatant::castSpell(int cost) {
        mana_ -= cost;
        missionStats->add(MissionStats::SPELLS_CAST, 1);
        missionStats->add(MissionStats::MANA_SPENT, cost);
    }

    void Combatant::setBanner(int cost) {
        missionStats->add(MissionStats::BANNERS_SET, 1);
        castSpell(cost);
    }

    void Combatant::castUltimateSpell() {
        ultimateCharge_ = 0;
        forcedUltimateFactor_ = 0;
        missionStats->add(MissionStats::ULTIMATES_USED, 1);
    }

    double Combatant::getUSpeelFactor() const {
        return std::max(forcedUltimateFactor_, ultimateCharge_ / ultimateChargeMax_);
    }

    void Combatant::initSpells() {
        const std::vector<const SpellInfo*> raceList = raceSpells(race_);
        spells_.clear();
        for (int i = 0; i < 3; ++i) spells_.push_back(makeSpell(raceList[static_cast<std::size_t>(i)]));
    }

    std::shared_ptr<Spell> Combatant::makeSpell(const SpellInfo* info) const {
        if (spellUnlockValue(info, bonuses) > 0) return std::make_shared<Spell>(*info, &bonuses);
        return nullptr;
    }

    // --- Statistics events ---------------------------------------------------------------------------------

    void Combatant::onEnemySquadKilled(Squad&) { missionStats->add(MissionStats::ENEMY_SQUADS_DESTROYED, 1); }

    void Combatant::recordSquadsOwned() { missionStats->set(MissionStats::SQUADS_OWNED, squadsCount_); }

    void Combatant::addChestGold(int amount) {
        addGold(amount);
        missionStats->add(MissionStats::CHESTS_OPENED, 1);
    }

    void Combatant::onHeroicMode(Squad&) { missionStats->add(MissionStats::HEROIC_MODE_ENGAGED, 1); }

    void Combatant::recordBuildings(int villages, int mines, int obelisks) {
        missionStats->set(MissionStats::VILLAGES_OWNED, villages);
        missionStats->set(MissionStats::MINES_OWNED, mines);
        missionStats->set(MissionStats::OBELISKS_OWNED, obelisks);
    }

    void Combatant::onOwnSquadLost(Squad&) {
        if (ultimateChargeSpeed_ > 0)
            ultimateCharge_ += ultimateChargeMax_ * bonuses.value(Bonus::ULTIMATE_CHARGE_ON_SQUAD_DEATH) / 100.0;
        missionStats->add(MissionStats::SQUADS_LOST, 1);
    }

    void Combatant::onStrongholdLost() { missionStats->add(MissionStats::STRONGHOLDS_LOST, 1); }

    void Combatant::onStrongholdSeized() {
        missionStats->add(MissionStats::STRONGHOLDS_SEIZED, 1);
        addGold(bonuses.value(Bonus::CAPTURE_GOLD));
    }

    // --- Setup ------------------------------------------------------------------------------------------------

    void Combatant::initBonuses() {
        ultimateChargeMax_ = 200 * bonuses.factor(Bonus::ULTIMATE_COOLDOWN);
        addGold(bonuses.value(Bonus::START_GOLD));
        addMana(0);
    }

    void Combatant::dispose() {
        missionStats.reset();
        keyNodes_.clear();
        spells_.clear();
    }

} // namespace takeover
