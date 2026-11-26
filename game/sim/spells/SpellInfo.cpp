#include "game/sim/spells/SpellInfo.h"

#include "game/framework/core/As3.h"
#include "game/sim/battle/Edicts.h"

namespace takeover {

namespace {
std::vector<const SpellInfo*>& registry() {
    static std::vector<const SpellInfo*> spells;
    return spells;
}
} // namespace

SpellInfo::SpellInfo(int id_, int level_, std::string name_, Race race_, int type_, int cost_, int radius_,
                     double power_, double time_, std::string hint_)
    : id(id_), level(level_), name(std::move(name_)), race(race_), type(type_), cost(cost_), radius(radius_),
      power(power_), time(time_), hint(std::move(hint_)) {
    registry().push_back(this);
}

const std::vector<const SpellInfo*>& SpellInfo::all() { return registry(); }

void SpellInfo::copyFrom(const SpellInfo& other) {
    id = other.id;
    level = other.level;
    name = other.name;
    race = other.race;
    type = other.type;
    cost = other.cost;
    radius = other.radius;
    power = other.power;
    time = other.time;
    hint = other.hint;
}

// The descriptions of the Empire's spells were never translated: the
// Empire is not playable in the shipped game.
// clang-format off
const SpellInfo SpellInfo::LORDS_JUDGEMENT       (0,  1, "LORD'S JUDGEMENT",       Race::Westaria,     0, 150, 75, 60, 30, "Damages enemy units in a selected area");
const SpellInfo SpellInfo::BANNER_OF_CONSECRATION(1,  2, "BANNER OF CONSECRATION", Race::Westaria,     1, 50,  75, 10, 12, "Heals and ressurects your units");
const SpellInfo SpellInfo::BANNER_OF_HEROISM     (2,  3, "BANNER OF HEROISM",      Race::Westaria,     1, 70,  75, 2,  12, "All your nearby squads enter into Heroic state (a squad causes 1.5x damage in Heroic state)");
const SpellInfo SpellInfo::SIGN_OF_THE_ADVENT    (3,  4, "SIGN OF THE ADVENT",     Race::Westaria,     2, 1,   0,  0,  0,  "Temporarily summons Lord's Avatar into the battlefield");
const SpellInfo SpellInfo::SUMMON_STONEWORMS     (4,  1, "SUMMON STONEWORMS",      Race::TheKhaganate, 0, 100, 75, 8,  10, "Stoneworms damage the enemy units in a selected area");
const SpellInfo SpellInfo::DISENTOMB             (5,  2, "DISENTOMB",              Race::TheKhaganate, 0, 250, 75, 3,  0,  "Summons Immortals squad");
const SpellInfo SpellInfo::BANNER_OF_BLOODLUST   (6,  3, "BANNER OF BLOODLUST",    Race::TheKhaganate, 1, 50,  75, 4,  12, "Increases the damage caused by your units");
const SpellInfo SpellInfo::CATACLYSM             (7,  4, "CATACLYSM",              Race::TheKhaganate, 2, 1,   150, 8, 0,  "Hail of meteors causes great damage to all of the enemy's squads and strongholds and average damage to yours.");
const SpellInfo SpellInfo::ICE_BLAST             (8,  1, "ICE BLAST",              Race::TheCult,      0, 100, 75, 6,  5,  "Temporarily freezes the enemy squads in a selected area");
const SpellInfo SpellInfo::FROST_ENCHANTMENT     (9,  2, "FROST ENCHANTMENT",      Race::TheCult,      0, 150, 75, 25, 10, "Enchants your squads in a selected area to temporarily cause the max damage and get 25% less damage from the enemy attacks");
const SpellInfo SpellInfo::BANNER_OF_LIBERATION  (10, 3, "BANNER OF LIBERATION",   Race::TheCult,      1, 50,  75, 3,  12, "Protects your nearby units from any negative magic effects");
const SpellInfo SpellInfo::GRAND_TRANSFORMATION  (11, 4, "GRAND TRANSFORMATION",   Race::TheCult,      2, 1,   0,  20, 0,  "Destroys a random half of the enemy units giving your mana and gold for the destroyed units");
const SpellInfo SpellInfo::TERROR                (12, 1, "TERROR",                 Race::TheEmpire,    0, 100, 75, 1,  0,  "Пугает (переводит в состояние Испуган или снимает Героизм) вражеские отряды");
const SpellInfo SpellInfo::RAISE_THE_DEAD        (13, 2, "RAISE THE DEAD",         Race::TheEmpire,    0, 250, 75, 1,  0,  "Призывает нежить-пехоту или стрелков");
const SpellInfo SpellInfo::BANNER_OF_DESECRATION (14, 3, "BANNER OF DESECRATION",  Race::TheEmpire,    1, 75,  75, 10, 12, "Лечит и возрождает ваши юниты");
const SpellInfo SpellInfo::CALL_OF_THE_GRAVE     (15, 4, "CALL OF THE GRAVE",      Race::TheEmpire,    2, 1,   0,  2,  0,  "Превращает 1-3 случайных живых вражеских отряда в соответствующие отряды нежити");
// clang-format on

Spell::Spell(const SpellInfo& info, const EdictBonuses* bonuses) {
    SpellInfo::copyFrom(info);
    if (bonuses != nullptr) applyBonuses(info, *bonuses);
}

const SpellInfo* Spell::byId(int spellId) {
    const auto& spells = all();
    return spellId >= 0 && spellId < static_cast<int>(spells.size()) ? spells[static_cast<std::size_t>(spellId)] : nullptr;
}

void Spell::applyBonuses(const SpellInfo& info, const EdictBonuses& bonuses) {
    if (type == TYPE_BANNER) time *= bonuses.factor(Bonus::BANNERS_DURATION);
    if (&info == &LORDS_JUDGEMENT) power *= bonuses.factor(Bonus::LORDS_JUDGEMENT_DAMAGE);
    else if (&info == &ICE_BLAST) time += bonuses.value(Bonus::FREEZE_DURATION);
    cost = static_cast<int>(as3::round(cost * bonuses.factor(Bonus::SPELLS_COST)));
}

void Spell::copyFrom(const Spell& other) {
    SpellInfo::copyFrom(other);
    position.copyFrom(other.position);
}

} // namespace takeover
