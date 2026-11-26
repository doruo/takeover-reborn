#include "game/sim/battle/Edicts.h"

namespace takeover {

Edict::Edict(std::string name_, std::vector<EdictEffect> effects, int requiredPoints, int startPoints)
    : name(std::move(name_)), effects_(std::move(effects)), startPoints_(startPoints), requiredPoints_(requiredPoints) {
    reset();
}

void Edict::applyTo(EdictBonuses& bonuses) const {
    for (int i = 0; i < curPoints; ++i) bonuses.set(effects_[static_cast<std::size_t>(i)].bonus, effects_[static_cast<std::size_t>(i)].value);
}

void Edict::addPoint() {
    if (curPoints < static_cast<int>(effects_.size())) ++curPoints;
}

EdictTree EdictTree::westaria() {
    return EdictTree(Race::Westaria, {
        {Edict("THE LORD'S CHURCH", {{5, 1, "Unlocks Banner of Consecration"}, {6, 25, "+25% banners` duration"}, {6, 50, "+50% banners` duration"}}, 0, 0),
         Edict("DUCAL SOVEREIGNITY", {{0, 0, "Unlocks basic troops"}, {8, 75, "+75 gold in the beginning of the mission"}, {8, 150, "+150 gold in the beginning of the mission"}}, 0, 0)},
        {Edict("CALL FOR THE CRUSADE", {{9, 1, "Unlocks Lord`s Judgement"}, {10, 15, "+15% Lord`s Judgement damage"}, {10, 30, "+30% Lord`s Judgement damage"}}, 4),
         Edict("TOWN GUILDS", {{0, 1, "Upgrades Guards to Swordsmen"}, {11, 15, "+15% strongholds' HP"}, {11, 30, "+30% strongholds' HP"}}, 4)},
        {Edict("THE ZEALOTS ORDER", {{4, 1, "Upgrades Monks to Zealots"}, {12, -10, "-10% ultimate power cooldown"}, {12, -20, "-20% ultimate power cooldown"}}, 7),
         Edict("MECHANICS RESEARCH", {{2, 1, "Upgrades Archers to Crossbowmen"}, {13, 10, "+10% bows / seige weapons damage"}, {13, 20, "+20% bows / seige weapons damage"}}, 7)},
        {Edict("HEROIC EPOS", {{14, 1, "Unlocks Banner of Heroism"}, {15, 7, "+7% troops base morale"}, {15, 15, "15% troops base morale"}}, 11),
         Edict("REGULAR TOURNAMENTS", {{1, 1, "Upgrades Knights to Champions"}, {7, 50, "Cavalry charge causes 2.5x damage"}, {7, 100, "Cavalry charge causes 3x damage"}}, 11)},
        {Edict("LEGION", {{0, 2, "Upgrades infantry to Legionaries"}, {16, 15, "+15% infantry HP"}, {16, 30, "+30% infantry HP"}}, 15)},
    });
}

EdictTree EdictTree::khaganate() {
    return EdictTree(Race::TheKhaganate, {
        {Edict("BLOODLUST", {{17, 1, "Unlocks Banner of Bloodlust"}, {18, 25, "+25% damage vs strongholds"}, {18, 50, "+50% damage vs strongholds"}}, 0, 0),
         Edict("DECLARATION OF WAR", {{0, 0, "Unlocks basic troops"}, {19, 10, "+10% troops moving speed"}, {19, 20, "+20% troops moving speed"}}, 0, 0)},
        {Edict("SPEAKING TO ANIMALS", {{20, 1, "Unlock Lure Stoneworms spell"}, {21, 1, "+1 worm (longer Lure Stoneworms)"}, {21, 2, "+2 worm (longer Lure Stoneworms)"}}, 4),
         Edict("3 DAYS TO PLUNDER", {{1, 1, "Upgrades Nomads to Riders"}, {22, 75, "+75 gold when capture a stronghold"}, {22, 150, "+150 gold when capture a stronghold"}}, 4)},
        {Edict("RAID THE TOMBS", {{0, 1, "Upgrades Warriors to Immortals"}, {23, 1, "+1 unit in an Immortals squad"}, {23, 2, "+2 unit in an Immortals squad"}}, 7),
         Edict("POISONS RESEARCH", {{2, 1, "Upgrades Hunters to Serpent Archers"}, {24, 1, "+1 poison damage"}, {24, 2, "+2 poison damage"}}, 7)},
        {Edict("DARK TEACHING", {{25, 1, "Unlocks Disentomb"}, {26, 3, "+3% ultimate charge if your squad dies"}, {26, 6, "+6% ultimate charge if your squad dies"}}, 11),
         Edict("DREADFUL MACHINERY", {{3, 1, "Upgrades Trebouchet to Poison Catapult"}, {27, 50, "+50% siege weapons moving speed"}, {27, 100, "+100% siege weapons moving speed"}}, 11)},
        {Edict("NECROPOLIS EXCAVATIONS", {{1, 2, "Upgrades cavalry to Ancient Riders"}, {28, -7, "-7% cost of your undead troops"}, {28, -15, "-15% cost of your undead troops"}}, 15)},
    });
}

EdictTree EdictTree::cult() {
    return EdictTree(Race::TheCult, {
        {Edict("ARCANE FROST", {{29, 1, "Unlocks Ice Blast"}, {30, 1, "+1 second to freeze duration"}, {30, 2, "+2 seconds to freeze duration"}}, 0, 0),
         Edict("DECLARATION OF INDEPENDENCE", {{0, 0, "Unlocks basic troops"}, {31, -15, "-15% farms, mines and obelisks cost"}, {31, -30, "-30% farms, mines and obelisks cost"}}, 0, 0)},
        {Edict("ARTISIANS GUILD", {{2, 1, "Upgrades Gunsmiths to Gunbots"}, {32, 5, "+5% crit chance for mechanisms"}, {32, 10, "+10% crit chance for mechanisms"}}, 4),
         Edict("REBEL ARMY", {{0, 1, "Upgrades Rebels to Myrmidons"}, {33, 7, "+7% freeze chance to infantry"}, {33, 15, "+15% freeze chance to infantry"}}, 4)},
        {Edict("MAGIC LIBRARY", {{34, 1, "Unlocks Frost Enchantmnet"}, {35, -10, "-10% spells cost"}, {35, -20, "-20% spells cost"}}, 7),
         Edict("LIBERATION ARMY", {{36, 1, "Unlocks Banner of Liberation"}, {37, 15, "Your squads gain xp 15% faster"}, {37, 30, "Your squads gain xp 30% faster"}}, 7)},
        {Edict("MAGIC ACADEMY", {{4, 1, "Upgrades Acolytes to Wizards"}, {38, 1, "Magic Towers give +1 gold/supply/mana"}, {38, 2, "Magic Towers give +2 gold/supply/mana"}}, 11),
         Edict("TAME THE REPTILES", {{1, 1, "Upgrades Lancers to Dragon Lancers"}, {39, 10, "+10% cavalry HP"}, {39, 20, "+20% cavalry HP"}}, 11)},
        {Edict("GIANT CONSTRUCTION", {{2, 2, "Upgrades marskmen to Giant Bots"}, {40, 2, "Giant Bots restore 2% HP per second"}, {40, 4, "Giant Bots restore 4% HP per second"}}, 15)},
    });
}

EdictTree EdictTree::forRace(Race race) {
    switch (race) {
    case Race::Westaria: return westaria();
    case Race::TheKhaganate: return khaganate();
    case Race::TheCult: return cult();
    default: return EdictTree(race, {});
    }
}

void EdictTree::reset() {
    for (auto& row : rows_)
        for (auto& edict : row) edict.reset();
}

int EdictTree::countIssued() const {
    int count = 0;
    for (const auto& row : rows_)
        for (const auto& edict : row) count += edict.getPoints() > 0 ? 1 : 0;
    return count;
}

int EdictTree::totalPoints() const {
    int points = 0;
    for (const auto& row : rows_)
        for (const auto& edict : row) points += edict.getPoints();
    return points;
}

EdictBonuses EdictTree::buildBonuses() const {
    EdictBonuses bonuses;
    for (const auto& row : rows_)
        for (const auto& edict : row) edict.applyTo(bonuses);
    return bonuses;
}

} // namespace takeover
