// LevelAi.h - the computer players (original: level/_SafeStr_2340).
//
// Once per second, every combatant gets its incomes, and every computer
// combatant:
//   - sends surplus defenders to the most interesting stronghold,
//   - builds farms, mines and obelisks when short of supply, gold or mana,
//   - recruits a squad in its most threatened stronghold,
//   - casts a spell where it hurts most (every 2 s at most),
//   - unleashes its ultimate power when the situation calls for it.
#pragma once

#include "game/sim/level/LevelLogic.h"

namespace takeover {

    class LevelAi : public LevelLogic {
    public:
        LevelAi() = default;
        explicit LevelAi(std::shared_ptr<MissionResult> result) : LevelLogic(std::move(result)) {}

        virtual bool updateCpuCombotantLogic(double dt);
        virtual bool updateBaseCombotantLogic(double dt);

        /// Sends idle scouts of computer players to their nearest stronghold
        /// (original: _SafeStr_2422).
        void sendScoutsHome();

        /// No computer player at all (original: _SafeStr_2390).
        bool aiDisabled = false;

        /// Computer players build nothing (original: _SafeStr_2391).
        bool aiBuildDisabled = false;

    public: // (all public in the original, the level and the battle use them)
        void customUpdate(double dt) override;

        // Hooks for the presentation, which creates the visible objects.
        virtual bool tryBuySupply(NodeContent& /*place*/, Combatant& /*combatant*/, SupplyBuildingType /*type*/) { return false; }
        virtual std::shared_ptr<Squad> buyUnit(const ArmyStat* /*stat*/, KeyNode& /*node*/) { return nullptr; }
        virtual bool checkEmptyPoinPos(const Point2D& /*point*/) { return true; }

        void sendSurplusDefenders(Combatant& combatant); ///< _SafeStr_2410
        void buildSupply(Combatant& combatant);          ///< _SafeStr_2411
        void buildSupplyOfType(Combatant& combatant, SupplyBuildingType type); ///< _SafeStr_2388
        void recruit(Combatant& combatant);              ///< _SafeStr_2397
        void castSpells(Combatant& combatant, double dt); ///< _SafeStr_2407
        void tryUltimate(Combatant& combatant);          ///< _SafeStr_2403
        void ultimateEmpire(Combatant& combatant);       ///< _SafeStr_2409: 3+ attackable living enemies
        void ultimateWestaria(Combatant& combatant) { castUltimateSpell(combatant); } ///< _SafeStr_2401
        void ultimateKhaganate(Combatant& combatant);    ///< _SafeStr_2399: outnumbered
        void ultimateCult(Combatant& combatant);         ///< _SafeStr_2408: 6+ enemy squads

        /// Sends `squad` from `node` to the nearest stronghold worth it (original: _SafeStr_2404).
        KeyNode* sendFrom(KeyNode& node, Squad& squad);

        /// The nearest enemy or ruined stronghold (original: _SafeStr_2396).
        KeyNode* nearestTarget(KeyNode& node);

        /// The own stronghold with most enemies around (original: _SafeStr_2423).
        KeyNode* mostThreatenedNode(Combatant& combatant);

        /// Where to cast a spell, by spell (original: _SafeStr_2413).
        std::shared_ptr<Spell> chooseSpellTarget(const Spell& spell, Combatant& combatant);
        std::shared_ptr<Spell> targetEnemyNodeOfFightingSquad(const Spell& spell, Combatant& combatant); ///< _SafeStr_2414
        std::shared_ptr<Spell> targetNowhere(const Spell& spell, Combatant& combatant);          ///< _SafeStr_2392
        std::shared_ptr<Spell> targetOwnFightingSquad(const Spell& spell, Combatant& combatant);  ///< _SafeStr_2389
        std::shared_ptr<Spell> targetEnemySquad(const Spell& spell, Combatant& combatant);        ///< _SafeStr_2395
        std::shared_ptr<Spell> targetEnemyFightingOwn(const Spell& spell, Combatant& combatant);  ///< _SafeStr_2400
        std::shared_ptr<Spell> targetBraveEnemy(const Spell& spell, Combatant& combatant);        ///< _SafeStr_2402
        std::shared_ptr<Spell> targetShakenOwn(const Spell& spell, Combatant& combatant);         ///< _SafeStr_2412
        std::shared_ptr<Spell> targetWoundedOwn(const Spell& spell, Combatant& combatant);        ///< _SafeStr_2393

        /// Centre of a squad's living soldiers, possibly moved to a free spot
        /// (original: _SafeStr_2385).
        Point2D squadCentre(Squad& squad, bool findEmptySpot = false);

        /// The way out of a stronghold (original: _SafeStr_2427).
        Point2D exitPoint(KeyNode& node);
        static double squadWeight(const Squad& squad); ///< _SafeStr_2387
        static int defendersHP(const KeyNode& node);    ///< _SafeStr_2406

        /// Unit types among `wanted` this combatant can buy now (original: _SafeStr_2415).
        std::vector<int> affordableTypes(Combatant& combatant, const std::vector<int>& wanted);

        /// Values of `a` also in `b`, in `a` order (original: _SafeStr_2398).
        static std::vector<int> intersect(const std::vector<int>& a, const std::vector<int>& b);

        /// An enemy of the combatant, not a bandit (original: _SafeStr_2405).
        static bool isRealEnemy(const Combatant& combatant, const Squad& squad);
    };

} // namespace takeover
