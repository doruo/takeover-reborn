// KeyNode.h - a stronghold: town, castle, camp or magic tower (original:
// level/_SafeStr_997).
//
// A stronghold has hit points that grow with its level (upgrades), gives
// incomes to its owner and recruits squads. Its life is a small state
// machine:
//
//     Active --(HP reach 0)--> Ruined --(repair)--> Rebuilding --(full HP)--> Active
//
// Damage and repairs are accumulated during the frame and applied by the
// strongest event of the frame (see postEvent).
#pragma once

#include "game/sim/base/TypedKey.h"
#include "game/sim/fsm/StateMachine.h"
#include "game/sim/level/KeyNodeType.h"
#include "game/sim/level/NodeContent.h"
#include "game/sim/level/RoadGraph.h"
#include "game/sim/units/ITarget.h"

#include <memory>
#include <vector>

namespace takeover {

    class CombatantKey;
    class KeyNode;
    class Squad;

    /// A state of a stronghold (original: _SafePkg_1542/_SafeStr_1493).
    class KeyNodeState : public FsmState {
    public:
        explicit KeyNodeState(KeyNode* node) : node_(node) {}
        void customDispose() override { node_ = nullptr; }

    private:
        KeyNode* node_; ///< Original: _SafeStr_2929.
    };

    class KeyNode : public NodeContent, public ITarget {
    public:
        // Events, shared by every stronghold.
        static const PriorityEvent EV_REPAIR;    ///< Priority 1000 (original: _SafeStr_1509).
        static const PriorityEvent EV_DAMAGE;    ///< Priority 1000 (original: _SafeStr_1512).
        static const PriorityEvent EV_FULL_HP;   ///< Priority 9999 (original: _SafeStr_1517).
        static const PriorityEvent EV_DESTROYED; ///< Priority 9999 (original: _SafeStr_1511).

        KeyNode(std::shared_ptr<NodeContentKey> description, std::shared_ptr<NodeKey> nodeDescription,
                const Combatant& owner, KeyNodeType type, const Point2D& position);

        // /----- I TARGET -----/

        void onTargetedBy(Unit& attacker) override;  ///< _SafeStr_1482
        void onMeleeHitBy(Unit& attacker) override;  ///< _SafeStr_1480
        void onMissileHitBy(Unit& attacker) override; ///< _SafeStr_1481
        [[nodiscard]] bool isDestroyed() const override { return isRuined(); } ///< _SafeStr_991
        [[nodiscard]] TargetType getTargetType() const override { return TargetType::Building; }
        [[nodiscard]] double getHitRadius() const override { return 50; }
        [[nodiscard]] Point2D getPosition() const override { return position_; }

        // /----- HIT POINTS AND LEVEL -----/

        virtual void setLevel(int level);
        virtual void setHP(int hp);
        [[nodiscard]] int getHP() const { return hp_; }
        [[nodiscard]] int getMaxHP() const;  ///< _SafeStr_1496
        [[nodiscard]] double getHPFactor() const { return static_cast<double>(getHP()) / getMaxHP(); }
        [[nodiscard]] int getLevel() const { return level_; } ///< _SafeStr_834
        [[nodiscard]] bool isMaxLevel() const { return level_ >= 2; } ///< _SafeStr_1534
        /// Gold to reach the next level, -1 at the top (original: _SafeStr_1532).
        [[nodiscard]] int getUpgradeCost(int level) const;

        /// Damage of a percentage of the maximum HP (Stone worms, original: _SafeStr_1536).
        void damagePercent(int percent);

        /// Adds damage for this frame (original: _SafeStr_1507).
        void addDamage(int damage);

        /// Adds repairs for this frame (original: activate).
        void activate(int repair);

        // /----- STATE -----/

        [[nodiscard]] bool isActive() const { return fsm_->getState() == stateActive_.get(); } ///< _SafeStr_1531, _SafeStr_1341
        [[nodiscard]] bool isRuined() const { return hp_ <= 0; } ///< _SafeStr_1520
        [[nodiscard]] FsmState* getFSMState() const { return fsm_->getState(); } ///< _SafeStr_833
        /// Became ruined or active again during this frame (original: _SafeStr_1541).
        [[nodiscard]] bool hasStateChanged() const { return stateChanged_; }
        [[nodiscard]] KeyNodeType getType() const { return type_; } ///< _SafeStr_1397
        virtual void changeType(KeyNodeType type) { type_ = type; }
        /// The other strongholds, by their content key (original: _SafeStr_1533).
        [[nodiscard]] KeyedStorage<NodeLink>& getLinks() { return links_; }
        /// How much the AI wants to send troops here (original: _SafeStr_1539).
        [[nodiscard]] double getThreat() const { return threat_; }
        /// The squad that hit the stronghold this frame, or -1 (original: _SafeStr_1535).
        [[nodiscard]] int getLastAttackerSquadId() const { return lastAttackerSquadId_; }
        /// Sets incomes, base HP, name and recruitable units of a stronghold
        /// kind, with the owner's edicts (original: _SafeStr_1525).
        void initByType(KeyNodeType type, const Combatant& owner);

        void customUpdate(double dt) override;
        void customDispose() override;

        // /----- PRESENTATION HOOKS -----/

        virtual void changeViewToRize() {}
        virtual void changeViewToActive() {}

        std::shared_ptr<CombatantKey> owner;

        /// Unit types this stronghold recruits (original: _SafeStr_1501).
        std::vector<int> recruitableTypes;

        /// The AI sends defenders away beyond this count (original: _SafeStr_1529).
        int aiDefendersWanted = 1;

        /// Squads inside, refreshed every frame (original: _defenders, _attackers).
        std::vector<Squad*> defenders;
        std::vector<Squad*> attackers;

    protected:
        void updateFSM(double dt);    ///< _SafeStr_1528
        void applyDamage() { setHP(hp_ - pendingDamage_); } ///< _SafeStr_1514
        void applyRepair() { setHP(hp_ + pendingRepair_); } ///< _SafeStr_865
        void initLinks();             ///< _SafeStr_1524
        void initFSM();               ///< _SafeStr_1527
        void disposeLinks();          ///< _SafeStr_1521
        void disposeFSM();            ///< _SafeStr_1522
        void onRuined();              ///< _SafeStr_1519
        void onRebuilt();             ///< _SafeStr_1526

        /// Keeps the strongest event of the frame. Returns true if `event` is it
        /// (original: _SafeStr_1506).
        bool postEvent(const PriorityEvent& event);

        KeyNodeType type_;                         ///< Original: _SafeStr_1504.
        std::shared_ptr<KeyNodeState> stateRuined_;     ///< Original: _SafeStr_1494.
        std::shared_ptr<KeyNodeState> stateRebuilding_; ///< Original: _SafeStr_1487.
        std::shared_ptr<KeyNodeState> stateActive_;     ///< Original: _SafeStr_1486.
        int pendingRepair_ = 0;                    ///< Original: _SafeStr_1499.
        Point2D position_;                         ///< Original: _SafeStr_1513.
        KeyedStorage<NodeLink> links_;             ///< Original: _SafeStr_1497.
        int baseHP_ = 0;                           ///< Original: _SafeStr_1490.
        int level_ = 0;                            ///< Original: _SafeStr_1502.
        bool stateChanged_ = false;                ///< Original: _SafeStr_1505.
        int hp_ = 0;                               ///< Original: _SafeStr_1489.
        std::unique_ptr<StateMachine> fsm_;        ///< Original: _SafeStr_1485.
        double threat_ = 0;                        ///< Original: _SafeStr_1503.
        int pendingDamage_ = 0;                    ///< Original: _SafeStr_1500.
        const PriorityEvent* pendingEvent_ = nullptr; ///< Original: _SafeStr_1491.
        int lastAttackerSquadId_ = -1;             ///< Original: _SafeStr_1495.
    };

} // namespace takeover
