#include "game/sim/battle/ArmyStat.h"

#include "game/sim/battle/Combatant.h"

#include <map>
#include <sstream>

namespace takeover {

    namespace {

        /// race table index -> unit type -> tier -> stat (original: _SafeStr_998._SafeStr_2487,
        /// built by Combatant._SafeStr_2497).
        std::map<int, std::map<int, std::map<int, ArmyStat*>>>& table() {
            static std::map<int, std::map<int, std::map<int, ArmyStat*>>> stats;
            return stats;
        }

        /// Splits "Charge 2;Undead" into its abilities (original: _SafeStr_978._SafeStr_1304).
        std::vector<std::string> splitAbilities(const std::string& abilities) {

            std::vector<std::string> parts;
            std::stringstream stream(abilities);

            for (std::string part; std::getline(stream, part, ';');) 
                parts.push_back(part);
            
            if (abilities.empty()) 
                parts.push_back("");
            return parts;
        }

    } // namespace

    ArmyStat::ArmyStat(
        std::string name_, 
        int hp_, Race side_, 
        int unitsCount_, 
        int dmgMin_, 
        int dmgMax_, 
        int attackSpeed_,
        int range_, 
        int meleePenalty_, 
        int morale_, 
        int speed_, 
        int cost_, 
        int upkeep_, 
        std::string abilities_,
        int techLevel_, 
        UnitType unitType_, 
        double attackAnimFactor_, 
        int distAttackStartHeight_,
        int radAfterShowBullet_, 
        double baseSpeedAnimWalk_, 
        double missileTime_
    )
        : name(std::move(name_)), 
        hp(hp_), 
        side(side_), 
        unitsCount(unitsCount_), 
        dmgMin(dmgMin_), 
        dmgMax(dmgMax_),
        attackSpeed(attackSpeed_), 
        range(range_), 
        meleePenalty(meleePenalty_), 
        morale(morale_), 
        speed(speed_),
        cost(cost_), 
        upkeep(upkeep_), 
        abilities(std::move(abilities_)), 
        techLevel(techLevel_), 
        unitType(unitType_),
        attackAnimFactor(attackAnimFactor_), 
        distAttackStartHeight(distAttackStartHeight_),
        radAfterShowBullet(radAfterShowBullet_), 
        baseSpeedAnimWalk(baseSpeedAnimWalk_), 
        missileTime(missileTime_) {

        // Like the original constructor: register in the lookup table. A later
        // stat with the same (race, type, tier) replaces an earlier one.
        table()[Combatant::raceTableIndex(side)][unit_types::toInt(unitType)][techLevel] = this;
    }

    ArmyStat* ArmyStat::find(Race race, UnitType type, int tier) {
        auto& byType = 
            table()[Combatant::raceTableIndex(race)][unit_types::toInt(type)];
        
            const auto it = byType.find(tier);
        return it == byType.end() ? nullptr : it->second;
    }

    bool ArmyStat::hasAbility(
        const std::string& ability, 
        const std::string& abilities
    ) {    
        for (const std::string& entry : splitAbilities(abilities)) {
            
            const std::string name = entry.substr(0, entry.find(' '));
            if (name == ability)
                return true;
        }
        
        return false;
    }

    // clang-format off
    using R = Race;
    using U = UnitType;
    //                                name,                 hp, race,            n,dmg,atkSpd,range,melee,morale,speed,cost, upk,abilities                                         tier type         anim  dASH rABS walk  missile
    ArmyStat ArmyStat::GUARDS          ("Guards",           26, R::Westaria,     9, 2, 3,  15, 0,   0,  50, 40, 100, 1, "",                                               1, U::Infantry, 0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::WARRIORS        ("Warriors",         18, R::TheKhaganate, 9, 3, 4,  15, 0,   0,  50, 40, 100, 1, "",                                               1, U::Infantry, 0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::REBELS          ("Rebels",           22, R::TheCult,      9, 1, 5,  16, 0,   0,  50, 40, 100, 1, "LongSpear 50",                                   1, U::Infantry, 0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::UNDYING_WARRIORS("Undying Warriors", 18, R::TheEmpire,    9, 2, 4,  17, 0,   0,  50, 40, 100, 1, "Undead",                                         1, U::Infantry, 0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::SWORDSMEN       ("Swordsmen",        36, R::Westaria,     9, 3, 4,  15, 0,   0,  50, 40, 100, 1, "ShieldWall 15",                                  2, U::Infantry, 0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::IMMORTALS       ("Immortals",        25, R::TheKhaganate, 9, 4, 6,  15, 0,   0,  50, 40, 100, 1, "Undead",                                         2, U::Infantry, 0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::MYRMIDONS       ("Myrmidons",        28, R::TheCult,      9, 2, 7,  16, 0,   0,  50, 40, 100, 1, "LongSpear 50",                                   2, U::Infantry, 0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::ZOMBIE_WARRIORS ("Zombie Warriors",  25, R::TheEmpire,    9, 3, 5,  17, 0,   0,  50, 40, 100, 1, "Undead",                                         2, U::Infantry, 0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::LEGIONARIES     ("Legionaries",      40, R::Westaria,     9, 4, 5,  15, 0,   0,  50, 40, 100, 1, "ShieldWall 20;Discipline",                       3, U::Infantry, 0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::ARCHERS         ("Archers",          16, R::Westaria,     9, 4, 8,  30, 100, 70, 50, 40, 120, 1, "",                                               1, U::Archers,  0.75, 0,  0,  11,  10);
    ArmyStat ArmyStat::HUNTERS         ("Hunters",          15, R::TheKhaganate, 9, 4, 10, 30, 100, 70, 50, 40, 120, 1, "Poisonous 1",                                    1, U::Archers,  0.75, 0,  0,  11,  10);
    ArmyStat ArmyStat::GUNSMITHS       ("Gunsmiths",        14, R::TheCult,      9, 2, 8,  20, 115, 70, 50, 40, 120, 1, "",                                               1, U::Archers,  0.05, 25, 25, 11,   3);
    ArmyStat ArmyStat::UNDYING_ARCHERS ("Undying Archers",  12, R::TheEmpire,    9, 4, 8,  30, 100, 70, 50, 40, 120, 1, "Undead",                                         1, U::Archers,  0.75, 0,  0,  11,  10);
    ArmyStat ArmyStat::CROSSBOWMEN     ("Crossbowmen",      22, R::Westaria,     9, 8, 10, 30, 100, 70, 50, 40, 120, 1, "",                                               2, U::Archers,  0.75, 0,  0,  11,  10);
    ArmyStat ArmyStat::SERPENT_ARCHERS ("Serpent Archers",  18, R::TheKhaganate, 9, 7, 14, 30, 100, 70, 50, 40, 120, 1, "Poisonous 2",                                    2, U::Archers,  0.75, 0,  0,  11,  10);
    ArmyStat ArmyStat::GUNBOTS         ("Gunbots",          27, R::TheCult,      6, 8, 14, 20, 115, 70, 50, 40, 120, 1, "Mechanism",                                      2, U::Archers,  0.15, 25, 10, 11,   3);
    ArmyStat ArmyStat::SKELETON_ARCHERS("Skeleton Archers", 16, R::TheEmpire,    9, 8, 10, 30, 100, 70, 50, 40, 120, 1, "Undead",                                         2, U::Archers,  0.75, 0,  0,  11,  10);
    ArmyStat ArmyStat::GIANTBOTS       ("Giantbots",       200, R::TheCult,      1, 30, 50, 10, 115, 0, 50, 40, 120, 1, "Mechanism;MartialArtists",                       3, U::Archers,  0.15, 55, 15, 11,   3);
    ArmyStat ArmyStat::KNIGHTS         ("Knights",          58, R::Westaria,     5, 5, 8,  15, 0,   0,  50, 60, 180, 1, "Charge 2",                                       1, U::Cavalry,  0.75, 0,  0,  15.5, 5);
    ArmyStat ArmyStat::NOMADS          ("Nomads",           40, R::TheKhaganate, 7, 4, 7,  15, 0,   0,  50, 60, 180, 1, "Charge 2",                                       1, U::Cavalry,  0.75, 0,  0,  15.5, 5);
    ArmyStat ArmyStat::LANCERS         ("Lancers",          54, R::TheCult,      5, 4, 10, 17, 0,   0,  50, 60, 180, 1, "Charge 2",                                       1, U::Cavalry,  0.75, 0,  0,  15.5, 5);
    ArmyStat ArmyStat::UNDYING_HORSEMEN("Undying Horsemen", 54, R::TheEmpire,    5, 5, 8,  17, 0,   0,  50, 60, 180, 1, "Charge 2;Undead",                                1, U::Cavalry,  0.75, 0,  0,  15.5, 5);
    ArmyStat ArmyStat::CHAMPIONS       ("Champions",        72, R::Westaria,     5, 6, 9,  15, 0,   0,  50, 60, 180, 1, "Charge 2",                                       2, U::Cavalry,  0.75, 0,  0,  15.5, 5);
    ArmyStat ArmyStat::RAIDERS         ("Raiders",          48, R::TheKhaganate, 7, 5, 8,  15, 0,   0,  50, 60, 180, 1, "Charge 2;Intimidiation 20",                      2, U::Cavalry,  0.75, 0,  0,  15.5, 5);
    ArmyStat ArmyStat::DRAGON_LANCERS  ("Dragon Lancers",   67, R::TheCult,      5, 6, 11, 17, 0,   0,  50, 60, 180, 1, "Charge 2",                                       2, U::Cavalry,  0.75, 0,  0,  15.5, 5);
    ArmyStat ArmyStat::DARK_KNIGHTS    ("Dark Knights",     67, R::TheEmpire,    5, 6, 10, 17, 0,   0,  50, 60, 180, 1, "Charge 2;Undead",                                2, U::Cavalry,  0.75, 0,  0,  15.5, 5);
    ArmyStat ArmyStat::ANCIENT_RAIDERS ("Ancient Raiders",  75, R::TheKhaganate, 5, 7, 12, 15, 0,   0,  50, 60, 180, 1, "Charge 2;Intimidiation 20;Undead",               3, U::Cavalry,  0.75, 0,  0,  15.5, 5);
    ArmyStat ArmyStat::TREBUCHET_T1_W  ("Trebuchet",       400, R::Westaria,     1, 15, 25, 50, 200, 100, 50, 20, 200, 2, "Mechanism;SplashDamage 25;SiegeWeapon 50",            1, U::Siege, 0.2, 100, 0, 11, 10);
    ArmyStat ArmyStat::TREBUCHET_T1_D  ("Trebuchet",       400, R::TheKhaganate, 1, 15, 25, 50, 200, 100, 50, 20, 200, 2, "Mechanism;SplashDamage 25;SiegeWeapon 50",            1, U::Siege, 0.2, 100, 0, 11, 10);
    ArmyStat ArmyStat::TREBUCHET_T1_B  ("Trebuchet",       400, R::TheCult,      1, 15, 25, 50, 200, 100, 50, 20, 200, 2, "Mechanism;SplashDamage 25;SiegeWeapon 50",            1, U::Siege, 0.2, 100, 0, 11, 10);
    ArmyStat ArmyStat::TREBUCHET_T1_E  ("Trebuchet",       400, R::TheEmpire,    1, 15, 25, 50, 200, 100, 50, 20, 200, 2, "Mechanism;SplashDamage 25;SiegeWeapon 50",            1, U::Siege, 0.2, 100, 0, 11, 10);
    ArmyStat ArmyStat::POISON_CATAPULT_T2_D("Poison Catapult", 600, R::TheKhaganate, 1, 20, 35, 50, 200, 100, 50, 20, 200, 2, "Mechanism;SplashDamage 25;SiegeWeapon 50;Poisonous 2", 2, U::Siege, 0.3, 50, 0, 11, 10);
    ArmyStat ArmyStat::POISON_CATAPULT_T2_E("Poison Catapult", 600, R::TheEmpire,    1, 20, 35, 50, 200, 100, 50, 20, 200, 1, "Mechanism;SplashDamage 25;SiegeWeapon 50;Poisonous 2", 2, U::Siege, 0.3, 50, 0, 11, 10);
    ArmyStat ArmyStat::MONKS           ("Monks",            65, R::Westaria,     6, 5, 6,  15, 0,   0,  50, 40, 150, 1, "DivinePresence 10",                              1, U::Magic,    0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::ASSASSINS       ("Assassins",       120, R::TheKhaganate, 3, 8, 16, 15, 0,   0,  50, 40, 150, 1, "HorrificDash 5;PoisonImmunity;Discipline",       1, U::Magic,    0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::ACOLYTES        ("Acolytes",         55, R::TheCult,      6, 8, 12, 25, 100, 50, 50, 40, 150, 1, "FreezeArrow 50",                                 1, U::Magic,    0.5,  44, 0,  11,   5);
    ArmyStat ArmyStat::UNDYING_ASSASSINS("Undying Assassins", 120, R::TheEmpire, 3, 6, 12, 15, 0,   0,  50, 40, 150, 1, "Undead;HorrificDash 3;Intimidiation 10",         1, U::Magic,    0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::ZEALOTS         ("Zealots",          75, R::Westaria,     6, 6, 9,  15, 50,  0,  50, 40, 150, 1, "DivinePresence 10;MartialArtists",               2, U::Magic,    0.33, 50, 0,  11,   5);
    ArmyStat ArmyStat::WIZARDS         ("Wizards",          65, R::TheCult,      6, 12, 18, 25, 100, 50, 50, 40, 150, 1, "FreezeBlast 40",                                2, U::Magic,    0.75, 55, 0,  11,   5);
    ArmyStat ArmyStat::SHADES          ("Shades",          135, R::TheEmpire,    3, 12, 20, 15, 0,  0,  50, 40, 150, 1, "Undead;HorrificDash 4;Intimidiation 15",         2, U::Magic,    0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::OGRE            ("Ogre",            400, R::Bandits,      1, 8, 12, 30, 0,   0,  50, 30, 0,   0, "SplashDamage 25",                                1, U::Magic,    0.75, 0,  0,  20,   5);
    ArmyStat ArmyStat::AVATAR          ("Avatar",         1000, R::Westaria,     1, 10, 15, 30, 0,  0,  50, 30, 0,   0, "SplashDamage 25;Undead;SiegeWeapon 50",          3, U::Magic,    0.75, 0,  0,  20,   5);
    ArmyStat ArmyStat::WOLVES          ("Wolves",           40, R::Bandits,      6, 4, 10, 17, 0,   0,  50, 60, 0,   0, "Charge 2",                                       1, U::Cavalry,  0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::BRIGAND_WARRIORS("Brigand Warriors", 20, R::Bandits,      9, 1, 5,  16, 0,   0,  50, 40, 0,   0, "",                                               1, U::Infantry, 0.75, 0,  0,  11,   5);
    ArmyStat ArmyStat::BRIGAND_ARCHERS ("Brigand Archers",  16, R::Bandits,      9, 4, 8,  30, 100, 50, 50, 40, 0,   0, "",                                               1, U::Archers,  0.75, 0,  0,  11,   5);
    // clang-format on

} // namespace takeover
