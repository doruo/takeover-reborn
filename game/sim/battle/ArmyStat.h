// ArmyStat.h - the statistics of a kind of squad (original: battle/_SafeStr_998).
//
// One static instance per unit kind: race, tier, hit points, damage,
// abilities... They are registered in a table indexed by race, unit type
// and tier, which is how a combatant finds "its" archers of tier 2.
#pragma once

#include "game/sim/battle/Race.h"

#include <memory>
#include <string>

namespace takeover {

    class INameModifier;

    class ArmyStat {
    public:
        ArmyStat(
            std::string name, 
            int hp, 
            Race side, 
            int unitsCount, 
            int dmgMin, 
            int dmgMax, 
            int attackSpeed, 
            int range,
            int meleePenalty, 
            int morale, 
            int speed, 
            int cost, 
            int 
            upkeep, 
            std::string abilities,
            int techLevel,
            UnitType unitType, 
            double attackAnimFactor, 
            int distAttackStartHeight, 
            int radAfterShowBullet,
            double baseSpeedAnimWalk, 
            double missileTime
        );

        std::string name;
        int hp;
        Race side;
        int unitsCount;
        int dmgMin;
        int dmgMax;
        int attackSpeed;
        int range;        ///< 0 for melee squads.
        int meleePenalty;
        int morale;
        int speed;
        int cost;
        int upkeep;       ///< Supply used.
        std::string abilities; ///< "Charge 2;Undead": name and optional parameter, ';'-separated.
        int techLevel;    ///< Tier 1..3.
        UnitType unitType;
        double attackAnimFactor;  ///< Original: _SafeStr_2189, scales the attack animation speed.
        int distAttackStartHeight;
        int radAfterShowBullet;
        double baseSpeedAnimWalk;
        double missileTime;       ///< Missile flight time in tenths of a second (original: _SafeStr_2672).
        
        /// Animation naming rules set by XMLData for special squads (original: _SafeStr_1286).
        std::shared_ptr<INameModifier> animModifier;

        // The 47 unit kinds (original static names in comments).
        static ArmyStat GUARDS;                 // _SafeStr_2686
        static ArmyStat WARRIORS;               // _SafeStr_2692
        static ArmyStat REBELS;                 // _SafeStr_2689
        static ArmyStat UNDYING_WARRIORS;       // _SafeStr_1914
        static ArmyStat SWORDSMEN;              // _SafeStr_2691
        static ArmyStat IMMORTALS;              // _SafeStr_1911
        static ArmyStat MYRMIDONS;              // _SafeStr_2682
        static ArmyStat ZOMBIE_WARRIORS;        // _SafeStr_1908
        static ArmyStat LEGIONARIES;            // _SafeStr_2687
        static ArmyStat ARCHERS;                // _SafeStr_1407
        static ArmyStat HUNTERS;                // _SafeStr_1935
        static ArmyStat GUNSMITHS;              // _SafeStr_795
        static ArmyStat UNDYING_ARCHERS;        // _SafeStr_1918
        static ArmyStat CROSSBOWMEN;            // _SafeStr_1932
        static ArmyStat SERPENT_ARCHERS;        // _SafeStr_1921
        static ArmyStat GUNBOTS;                // _SafeStr_1902
        static ArmyStat SKELETON_ARCHERS;       // _SafeStr_1916
        static ArmyStat GIANTBOTS;              // _SafeStr_1903
        static ArmyStat KNIGHTS;                // _SafeStr_2684
        static ArmyStat NOMADS;                 // _SafeStr_2694
        static ArmyStat LANCERS;                // _SafeStr_2690
        static ArmyStat UNDYING_HORSEMEN;       // _SafeStr_1919
        static ArmyStat CHAMPIONS;              // _SafeStr_2697
        static ArmyStat RAIDERS;                // _SafeStr_2696
        static ArmyStat DRAGON_LANCERS;         // _SafeStr_1917
        static ArmyStat DARK_KNIGHTS;           // _SafeStr_1915
        static ArmyStat ANCIENT_RAIDERS;        // _SafeStr_1909
        static ArmyStat TREBUCHET_T1_W;
        static ArmyStat TREBUCHET_T1_D;
        static ArmyStat TREBUCHET_T1_B;
        static ArmyStat TREBUCHET_T1_E;
        static ArmyStat POISON_CATAPULT_T2_D;
        static ArmyStat POISON_CATAPULT_T2_E;
        static ArmyStat MONKS;                  // _SafeStr_1913
        static ArmyStat ASSASSINS;              // _SafeStr_2685
        static ArmyStat ACOLYTES;               // _SafeStr_1904
        static ArmyStat UNDYING_ASSASSINS;      // _SafeStr_1910
        static ArmyStat ZEALOTS;                // _SafeStr_1906
        static ArmyStat WIZARDS;                // _SafeStr_1905
        static ArmyStat SHADES;                 // _SafeStr_2688
        static ArmyStat OGRE;                   // _SafeStr_1907
        static ArmyStat AVATAR;                 // _SafeStr_2693
        static ArmyStat WOLVES;                 // _SafeStr_2681
        static ArmyStat BRIGAND_WARRIORS;       // _SafeStr_2683
        static ArmyStat BRIGAND_ARCHERS;        // _SafeStr_2695

        /// The stat registered for (race, unit type, tier), or nullptr
        /// (original: Combatant._SafeStr_2495).
        static ArmyStat* find(Race race, UnitType type, int tier);

        /// Does the abilities string contain this ability (original:
        /// _SafePkg_909/_SafeStr_978.hasAbility)?
        static bool hasAbility(const std::string& ability, const std::string& abilities);

        [[nodiscard]] bool hasAbility(const std::string& ability) 
        const { return hasAbility(ability, abilities); }
    };

} // namespace takeover
