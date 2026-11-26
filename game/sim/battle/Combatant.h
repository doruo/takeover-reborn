// Combatant.h - one side of a battle: its treasury, mana, supply, spells,
// ultimate power and mission statistics (original: battle/_SafeStr_1324).
//
// The level logic recomputes the incomes every second from the strongholds
// the combatant owns (setIncome) and then calls tick(), which adds them.
#pragma once

#include "game/sim/base/TypedKey.h"
#include "game/sim/battle/ArmyStatBonus.h"
#include "game/sim/battle/Edicts.h"
#include "game/sim/battle/MissionStats.h"
#include "game/sim/battle/Race.h"
#include "game/sim/level/SupplyBuildingType.h"

#include <memory>
#include <string>
#include <vector>

namespace takeover {

class ArmyStat;
class KeyNode;
class Spell;
class SpellInfo;
class Squad;

/// The key identifying a combatant (original: battle/_SafeStr_835, kind "cmbtnt").
class CombatantKey : public TypedKey {
public:
    CombatantKey() : TypedKey("cmbtnt") {}
};

class Combatant {
public:
    static constexpr int MAX_MANA = 500; ///< Original: _SafeStr_2486.

    Combatant(std::shared_ptr<CombatantKey> description, Race race, const EdictBonuses& bonuses);

    // --- Static tables ------------------------------------------------------------------

    static std::uint32_t raceColor(Race race);   ///< Original: _SafeStr_2499.
    static std::string raceName(Race race);      ///< Original: _SafeStr_1377.
    /// Index of the race in the army table (original: _SafeStr_2491).
    static int raceTableIndex(Race race);
    static Race raceFromTableIndex(int index);   ///< Original: _SafeStr_2509.
    static constexpr int RACE_COUNT = 5;         ///< Original: _SafeStr_2494.
    static constexpr int TIER_COUNT = 3;         ///< Original: _SafeStr_2501.
    /// The edict-dependent modifiers of a unit kind (original: _SafeStr_2505).
    static ArmyStatBonus makeArmyStatBonus(const ArmyStat& stat, const EdictBonuses& bonuses);
    /// The four spells of a race, by level (original: _SafeStr_2504).
    static std::vector<const SpellInfo*> raceSpells(Race race);
    /// > 0 when the edicts unlocked the spell (original: _SafeStr_2503).
    static int spellUnlockValue(const SpellInfo* spell, const EdictBonuses& bonuses);

    // --- Identity ---------------------------------------------------------------------------

    std::shared_ptr<CombatantKey> description;
    EdictBonuses bonuses;
    std::shared_ptr<MissionStats> missionStats;

    [[nodiscard]] Race getRace() const { return race_; }
    [[nodiscard]] std::uint32_t getColor() const { return raceColor(race_); }
    [[nodiscard]] bool isOwnSquad(const Squad& squad) const;      ///< _SafeStr_2386
    [[nodiscard]] bool ownsKeyNode(const KeyNode& node) const;    ///< _SafeStr_2426

    // --- Economy --------------------------------------------------------------------------

    [[nodiscard]] int getGold() const { return gold_; }
    [[nodiscard]] int getMana() const { return mana_; }
    [[nodiscard]] int getManaSpeed() const { return manaSpeed_; }
    [[nodiscard]] int getSupplyPower() const { return supplyPower_; }
    [[nodiscard]] int getArmyPower() const { return armyPower_; }
    [[nodiscard]] int getFreeSupply() const { return getSupplyPower() - getArmyPower(); } ///< _SafeStr_2417
    [[nodiscard]] int getBaseGoldSpeed() const { return goldSpeed_; }                     ///< _SafeStr_2488
    /// Gold per second after the "troops supply" penalty, which grows with
    /// the number of squads.
    [[nodiscard]] int getGoldSpeed() const;
    [[nodiscard]] int getGoldForSupply() const { return getBaseGoldSpeed() - getGoldSpeed(); }
    /// The penalty in percent, for the HUD (original: _SafeStr_2507).
    [[nodiscard]] int getSupplyModifierPercent() const;
    [[nodiscard]] int getSupplyCost(SupplyBuildingType type) const;

    void addGold(int amount);
    void addMana(int amount);
    void spendGold(int amount); ///< Original: _SafeStr_2485.
    /// Called every second: adds the incomes (original: _SafeStr_2425).
    void tick();
    /// Incomes computed by the level from the owned strongholds
    /// (original: _SafeStr_2238).
    void setIncome(int goldSpeed, int supplyPower, int armyPower, int manaSpeed, int ultimateChargeSpeed);

    // --- Army -----------------------------------------------------------------------------

    /// Tier of a unit type given by the edicts (original: _SafeStr_2493).
    [[nodiscard]] int getTier(UnitType type) const;
    /// The squad kind this combatant recruits for a unit type (original: _SafeStr_1373).
    [[nodiscard]] ArmyStat* getArmyStat(UnitType type) const;
    [[nodiscard]] ArmyStatBonus getArmyStatBonus(const ArmyStat& stat) const { return makeArmyStatBonus(stat, bonuses); } ///< _SafeStr_1398
    /// Price of a squad, with the undead discount (original: _SafeStr_2483).
    [[nodiscard]] int getUnitCost(const ArmyStat& stat) const;
    [[nodiscard]] bool canBuy(const ArmyStat& stat) const; ///< _SafeStr_2365
    void buySquad(Squad& squad);                           ///< _SafeStr_2343
    [[nodiscard]] int getSquadsCount() const { return squadsCount_; } ///< _SafeStr_2416
    void setSquadsCount(int count) { squadsCount_ = count; }           ///< _SafeStr_2376
    void incrementSquadsCount() { ++squadsCount_; }                     ///< _SafeStr_2381

    // --- Strongholds --------------------------------------------------------------------------

    [[nodiscard]] const std::vector<std::shared_ptr<KeyNode>>& getKeyNodes() const { return keyNodes_; } ///< _SafeStr_2361
    void setKeyNodes(std::vector<std::shared_ptr<KeyNode>> nodes);                                       ///< _SafeStr_2313
    bool upgradeKeyNode(KeyNode& node); ///< _SafeStr_1382
    void buildSupplyBuilding(SupplyBuildingType type);

    // --- Spells -------------------------------------------------------------------------------

    [[nodiscard]] const std::vector<std::shared_ptr<Spell>>& getSpells() const { return spells_; }
    void castSpell(int cost);
    void castNormalSpell(int cost) { castSpell(cost); } ///< _SafeStr_2353
    void setBanner(int cost);                            ///< _SafeStr_2374
    void castUltimateSpell();
    /// Charge of the ultimate power, 0..1 (original name kept: getUSpeelFactor).
    [[nodiscard]] double getUSpeelFactor() const;

    // --- Statistics events ----------------------------------------------------------------------

    void onEnemySquadKilled(Squad& squad);   ///< _SafeStr_2368
    void recordSquadsOwned();                ///< _SafeStr_2358
    void addChestGold(int amount);           ///< _SafeStr_1959
    void onHeroicMode(Squad& squad);         ///< _SafeStr_2359
    void recordBuildings(int villages, int mines, int obelisks); ///< _SafeStr_2367
    void onOwnSquadLost(Squad& squad);       ///< _SafeStr_2352
    void onStrongholdLost();                 ///< _SafeStr_2375
    void onStrongholdSeized();               ///< _SafeStr_2371

    // --- AI and cheats --------------------------------------------------------------------------

    double aiCooldown = 0; ///< Seconds before the AI acts again (original: _SafeStr_2394).

    void setSpellsForDebug(std::vector<std::shared_ptr<Spell>> spells) { spells_ = std::move(spells); } ///< __setSpells
    void setMana(int mana) { mana_ = mana; } ///< _SafeStr_2508
    void setGoldForDebug(int gold) { gold_ = gold; }
    void forceMaxUltimateTime() { ultimateCharge_ = ultimateChargeMax_; }
    void forceEnableUltimate(double factor = 1) { forcedUltimateFactor_ = factor; }

    void dispose();

private:
    void initBonuses();   ///< _SafeStr_2492
    void initSpells();    ///< _SafeStr_2490
    [[nodiscard]] std::shared_ptr<Spell> makeSpell(const SpellInfo* info) const; ///< _SafeStr_2489

    Race race_;
    int gold_ = 0;
    int mana_ = 0;
    int goldSpeed_ = 0;
    int manaSpeed_ = 0;
    int supplyPower_ = 0;
    int armyPower_ = 0;
    int squadsCount_ = 0;
    double ultimateCharge_ = 0;       ///< Original: _SafeStr_2471.
    double ultimateChargeMax_ = 200;  ///< Original: _SafeStr_2474.
    int ultimateChargeSpeed_ = 0;     ///< Original: _SafeStr_2479.
    double forcedUltimateFactor_ = 0; ///< Original: _SafeStr_2478.
    std::vector<std::shared_ptr<KeyNode>> keyNodes_;
    std::vector<std::shared_ptr<Spell>> spells_;
};

} // namespace takeover
