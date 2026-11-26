// Edicts.h - the edicts tree of the throne room and the bonuses it gives
// (original: package _SafePkg_2271).
//
// A campaign owns an EdictTree: rows of edicts, each with up to 3 points to
// spend. Every point bought applies one EdictEffect: "bonus #k = value".
// Before a battle, the tree is flattened into EdictBonuses, a table of 44
// values the simulation reads (unit tiers, damage percentages, ...).
#pragma once

#include "game/sim/battle/Race.h"

#include <array>
#include <string>
#include <vector>

namespace takeover {

/// Indexes of EdictBonuses, named after the edict descriptions. Percentages
/// are stored as "+15" and read either as a value (15) or a factor (1.15).
namespace Bonus {
inline constexpr int INFANTRY_TIER = 0;  ///< Unit tier upgrades (0 = tier 1).
inline constexpr int CAVALRY_TIER = 1;
inline constexpr int ARCHERS_TIER = 2;
inline constexpr int SIEGE_TIER = 3;
inline constexpr int MAGIC_TIER = 4;
inline constexpr int BANNER_OF_CONSECRATION = 5;
inline constexpr int BANNERS_DURATION = 6;
inline constexpr int CHARGE_DAMAGE = 7;
inline constexpr int START_GOLD = 8;
inline constexpr int LORDS_JUDGEMENT = 9;
inline constexpr int LORDS_JUDGEMENT_DAMAGE = 10;
inline constexpr int STRONGHOLDS_HP = 11;
inline constexpr int ULTIMATE_COOLDOWN = 12;
inline constexpr int RANGED_DAMAGE = 13;
inline constexpr int BANNER_OF_HEROISM = 14;
inline constexpr int BASE_MORALE = 15;
inline constexpr int INFANTRY_HP = 16;
inline constexpr int BANNER_OF_BLOODLUST = 17;
inline constexpr int DAMAGE_VS_STRONGHOLDS = 18;
inline constexpr int MOVING_SPEED = 19;
inline constexpr int SUMMON_STONEWORMS = 20;
inline constexpr int STONEWORMS_COUNT = 21;
inline constexpr int CAPTURE_GOLD = 22;
inline constexpr int IMMORTALS_EXTRA_UNITS = 23;
inline constexpr int POISON_DAMAGE = 24;
inline constexpr int DISENTOMB = 25;
inline constexpr int ULTIMATE_CHARGE_ON_SQUAD_DEATH = 26;
inline constexpr int SIEGE_SPEED = 27;
inline constexpr int UNDEAD_COST = 28;
inline constexpr int ICE_BLAST = 29;
inline constexpr int FREEZE_DURATION = 30;
inline constexpr int SUPPLY_BUILDINGS_COST = 31;
inline constexpr int MECHANISM_CRIT = 32;
inline constexpr int INFANTRY_FREEZE_CHANCE = 33;
inline constexpr int FROST_ENCHANTMENT = 34;
inline constexpr int SPELLS_COST = 35;
inline constexpr int BANNER_OF_LIBERATION = 36;
inline constexpr int XP_RATE = 37;
inline constexpr int MAGE_TOWERS_BONUS = 38;
inline constexpr int CAVALRY_HP = 39;
inline constexpr int GIANTBOTS_REGENERATION = 40;
inline constexpr int TERROR = 41;
inline constexpr int RAISE_THE_DEAD = 42;
inline constexpr int BANNER_OF_DESECRATION = 43;
inline constexpr int COUNT = 44;
} // namespace Bonus

/// Original: _SafeStr_2484.
class EdictBonuses {
public:
    void set(int bonus, int value) { values_[static_cast<std::size_t>(bonus)] = value; } ///< _SafeStr_3004
    void clear() { values_.fill(0); }                                                    ///< _SafeStr_3429
    /// (100 + value) / 100 (original: _SafeStr_1538).
    [[nodiscard]] double factor(int bonus) const { return (100 + values_[static_cast<std::size_t>(bonus)]) / 100.0; }
    /// The raw value (original: _SafeStr_1530).
    [[nodiscard]] int value(int bonus) const { return values_[static_cast<std::size_t>(bonus)]; }

private:
    std::array<int, Bonus::COUNT> values_{};
};

/// One level of an edict (original: _SafeCls_2270).
struct EdictEffect {
    int bonus = 0; ///< Original: _SafeStr_2272.
    int value = 0; ///< Original: _SafeStr_2273.
    std::string hint;
};

/// Original: _SafeCls_2968.
class Edict {
public:
    Edict(std::string name, std::vector<EdictEffect> effects, int requiredPoints, int startPoints = 0);

    std::string name;
    int curPoints = 0;

    void setStartPoints(int points) { startPoints_ = points; }            ///< _SafeStr_3003
    void reset() { curPoints = startPoints_; }
    [[nodiscard]] int getStartPoints() const { return startPoints_; }     ///< _SafeStr_3002
    [[nodiscard]] const std::vector<EdictEffect>& getEffects() const { return effects_; } ///< _SafeStr_3000
    [[nodiscard]] int getPoints() const { return curPoints; }             ///< _SafeStr_3001
    /// Points the campaign must have spent before this edict's row opens.
    [[nodiscard]] int getRequiredPoints() const { return requiredPoints_; } ///< Original field _SafeStr_2971.
    void applyTo(EdictBonuses& bonuses) const;                            ///< _SafeStr_2492
    void addPoint();                                                      ///< _SafeStr_2999

private:
    std::vector<EdictEffect> effects_;
    int startPoints_ = 0;
    int requiredPoints_ = 0;
};

/// Original: _SafeCls_3326.
class EdictTree {
public:
    EdictTree(Race treeRace, std::vector<std::vector<Edict>> rows) : race(treeRace), rows_(std::move(rows)) {}

    static EdictTree westaria();   ///< _SafeStr_3334
    static EdictTree khaganate();  ///< _SafeStr_3335
    static EdictTree cult();       ///< _SafeStr_3328
    static EdictTree forRace(Race race); ///< _SafeStr_3333

    Race race;

    void reset();
    /// Number of edicts with at least one point (original: _SafeStr_3331).
    [[nodiscard]] int countIssued() const;
    /// Points spent in total (original: _SafeStr_3332).
    [[nodiscard]] int totalPoints() const;
    [[nodiscard]] std::vector<std::vector<Edict>>& getRows() { return rows_; } ///< _SafeStr_3330
    [[nodiscard]] const std::vector<std::vector<Edict>>& getRows() const { return rows_; }
    [[nodiscard]] EdictBonuses buildBonuses() const; ///< _SafeStr_3329

private:
    std::vector<std::vector<Edict>> rows_;
};

} // namespace takeover
