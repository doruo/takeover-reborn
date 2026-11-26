// Unit.h - one soldier of a squad (original: _SafePkg_805/_SafeStr_972).
//
// A small state machine: Stand, Go (walk to its place in the formation),
// Fight (a UnitCombat drives it), Dead (lies there, then fades away).
// Requests to change state arrive as event bits. The strongest one wins at
// the next update (see postEvent). Ranged soldiers also fly one missile at
// a time. The presentation subclass (FUnit) overrides the changeAnimTo_*
// hooks to play the animations.
#pragma once

#include "game/sim/abilities/AbilityEffects.h"
#include "game/sim/units/ITarget.h"
#include "game/sim/units/UnitBody.h"

#include <memory>
#include <optional>

namespace takeover {

    class UnitCombat;

    /// What a unit is created from (original: _SafeStr_1009).
    struct UnitDef {
        virtual ~UnitDef() = default;
        int squadId = -1; ///< Object id of the squad (original: _SafeStr_1294).
        ArmyStatBonus armyStatBonus;
        const ArmyStat* armyStat = nullptr;
    };

    /// Attack effects dealt around a point (original: _SafePkg_868/_SafeStr_2317).
    struct SplashArea {
        double radius = 0;
        Point2D position;
        int attackerId = 0; ///< Original: _SafeStr_2308.
        EffectList effects; ///< Original: _SafeStr_1978.

        /// Every victim gets its own copies (original: _SafeStr_2348).
        [[nodiscard]] EffectList copyEffects() const;
    };

    class Unit : public UnitBody, public ITarget {
    public:
        // States.
        static constexpr int STATE_STAND = 0; ///< _SafeStr_1202
        static constexpr int STATE_GO = 1;    ///< _SafeStr_2663
        static constexpr int STATE_DEAD = 2;  ///< _SafeStr_2662
        static constexpr int STATE_FIGHT = 3; ///< _SafeStr_2661

        // State change requests. The largest pending one is processed.
        static constexpr int EV_RESURRECT = 32768; ///< _SafeStr_2659
        static constexpr int EV_TO_GRAVE = 16384;  ///< _SafeStr_2670
        static constexpr int EV_DIE = 8192;        ///< _SafeStr_2664
        static constexpr int EV_FIGHT = 1024;      ///< _SafeStr_2668
        static constexpr int EV_STOP_FIGHT = 128;  ///< _SafeStr_2666
        static constexpr int EV_MOVE = 16;         ///< _SafeStr_2665
        static constexpr int EV_ARRIVED = 8;       ///< _SafeStr_2669

        static const int EVENT_SPLASH_ATTACK; ///< Data: SplashArea (_SafeStr_1053).
        static const int EVENT_DAMAGED;       ///< Data: attacker id (_SafeStr_1234).
        static const int EVENT_ATTACKS;       ///< Data: the attacking unit (_SafeStr_1043).

        Unit() = default;
        ~Unit() override;
        Unit& init(const UnitDef& def); ///< _SafeStr_2644

        // /----- I TARGET -----/

        void onTargetedBy(Unit&) override {}
        void onMeleeHitBy(Unit& attacker) override;
        void onMissileHitBy(Unit& attacker) override;
        [[nodiscard]] bool isDestroyed() const override { return getHP() <= 0; }
        [[nodiscard]] TargetType getTargetType() const override { return TargetType::Unit; }
        [[nodiscard]] double getHitRadius() const override;
        [[nodiscard]] Point2D getPosition() const override { return {x_, y_}; }

        // /----- STATE -----/

        [[nodiscard]] bool isStanding() const { return state_ == STATE_STAND; } ///< _SafeStr_1235
        [[nodiscard]] bool isGoing() const { return state_ == STATE_GO; }       ///< _SafeStr_1223
        [[nodiscard]] bool isDead() const { return state_ == STATE_DEAD; }      ///< _SafeStr_1019
        [[nodiscard]] bool isFighting() const { return state_ == STATE_FIGHT; } ///< _SafeStr_1151
        [[nodiscard]] bool isChasing() const;           ///< _SafeStr_2675
        [[nodiscard]] bool isAttackingBuilding() const; ///< _SafeStr_1233
        [[nodiscard]] bool hasArrived() const { return arrived_; } ///< _SafeStr_1213
        [[nodiscard]] int getState() const { return state_; }
        void postEvent(int event); ///< _SafeStr_1117
        void stopFight() { postEvent(EV_STOP_FIGHT); } ///< _SafeStr_1211
        void changeFSMState(int state);

        // /----- MOVEMENT -----/

        void stopHere();                              ///< _SafeStr_1029
        void setDestination(const Point2D& p) { destination_.copyFrom(p); } ///< _SafeStr_1061
        [[nodiscard]] const Point2D& getDestination() const { return destination_; } ///< _SafeStr_2649
        /// Moves one step towards `target`. Returns true if already there (original: _SafeStr_2657).
        bool stepTowards(double dt, const Point2D& target);
        void push(double angle, double force); ///< _SafeStr_2345
        void checkSpeedChange();               ///< _SafeStr_2656

        // /----- FIGHT -----/

        [[nodiscard]] int getTargetId() const;  ///< _SafeStr_1145
        [[nodiscard]] int getSquadId() const { return squadId_; } ///< _SafeStr_1073
        [[nodiscard]] UnitCombat& combat() { return *combat_; }
        void attack(int targetId) { setTarget(targetId); } ///< _SafeStr_2671
        void setTarget(int targetId);                       ///< _SafeStr_1243
        void notifyAttack();                                ///< _SafeStr_2653
        void receiveAttack(const EffectList& effects, int attackerId); ///< _SafeStr_2311
        void launchMissile(int targetId);                   ///< _SafeStr_2676
        [[nodiscard]] int getDamageForFortress() const { return abilities_->getDamageForFortress(); }

        // /----- SPELLS AND BANNERS -----/

        void kill();                                ///< _SafeStr_1025
        void setSpellDefence() { abilities_->setSpellDefence(); } ///< _SafeStr_1036
        void setMaxDamageTime(double seconds);      ///< _SafeStr_1058
        void addDamageResist(double seconds, int percent = 25); ///< _SafeStr_1072
        void setDamageFactor(double factor);        ///< _SafeStr_1063

        /// Restores HP. Returns what didn't fit (original: _SafeStr_1062).
        int heal(int amount);

        /// Back from the dead at `position` (original: _SafeStr_1057).
        int resurrect(int hp, const Point2D& position);

        // Missile in flight (read by the presentation).
        [[nodiscard]] bool hasMissile() const { return missileTimeLeft_ >= 0 && missilePos_.has_value(); }
        [[nodiscard]] const Point2D& missilePosition() const { return *missilePos_; }
        [[nodiscard]] const Point2D& missilePreviousPosition() const { return *missilePrevPos_; }
        [[nodiscard]] const Point2D& missileStart() const { return *missileFrom_; }
        [[nodiscard]] const Point2D& missileEnd() const { return *missileTo_; }
        [[nodiscard]] double missileProgress() const { return (missileFlightTime_ - missileTimeLeft_) / missileFlightTime_; }

        void customUpdate(double dt) override;
        void customDispose() override;

        // /----- PRESENTATION HOOKS (FUNIT) -----/

        virtual void updateGoAnimSpeed() {}
        virtual void updateAnimDir(double /*dx*/, double /*dy*/) {}
        virtual void missileAtTarget(ITarget* /*target*/) {}
        virtual void changeAnimTo_Stand() {}
        virtual void changeAnimTo_Special() {}
        virtual void changeAnimTo_Go() {}
        virtual void changeAnimTo_FallDown() {}
        /// Starts an attack animation lasting about `period`. Returns its length.
        virtual double changeAnimTo_DistanceAttack(double period) { return period; }
        virtual void changeAnimTo_CorpseToGrave() {}
        virtual double changeAnimTo_Attack(double period) { return period; }
        virtual void activateDistanceAttack() {}
        virtual void activateAttack() {}
        virtual void beforeDispose() {} ///< _SafeStr_1188

    protected:
        virtual void updateStand(double) {} ///< _SafeStr_1196
        void updateGo(double dt);           ///< _SafeStr_2650
        void updateDead(double dt);         ///< _SafeStr_2646
        void updateFight(double dt);        ///< _SafeStr_2641
        void updateState(int state, double dt); ///< _SafeStr_1165
        void updateFSM(double dt);          ///< _SafeStr_1160
        void updateMissile(double dt);      ///< _SafeStr_2648
        void onFightTargetChanged();        ///< _SafeStr_2651
        void enterGo() { lastSpeed_ = getSpeed(); } ///< _SafeStr_2654
        void enterDead() { deadTime_ = 0; }          ///< _SafeStr_2652
        void enterFight();                  ///< _SafeStr_2647
        void startFight();                  ///< _SafeStr_2639
        void handleEvent(int state, int event); ///< _SafeStr_1162
        void initFSM();                     ///< _SafeStr_1182
        void initMissile();                 ///< _SafeStr_2643
        void clearMissile();                ///< _SafeStr_2658
        void dealSplashOrDirect(Unit& attacker, EffectList effects);

        Point2D destination_;          ///< Original: _SafeStr_2630.
        std::shared_ptr<UnitCombat> combat_; ///< Original: _SafeStr_2626.
        int squadId_ = -1;             ///< Original: _SafeStr_2640.
        double lastSpeed_ = 0;         ///< Original: _SafeStr_2637.
        std::optional<Point2D> missileTo_, missileFrom_, missilePrevPos_, missilePos_; ///< _SafeStr_2633/2627/2634/2631
        int missileTargetId_ = -1;     ///< Original: _SafeStr_2629.
        double missileTimeLeft_ = -1;  ///< Original: _SafeStr_2628.
        double missileFlightTime_ = 0; ///< Original: _SafeStr_2636.
        int nextTargetId_ = -1;        ///< Original: _SafeStr_2638.
        double deadTime_ = 0;          ///< Original: _SafeStr_2632.
        int state_ = STATE_STAND;      ///< Original: _SafeStr_1116.
        int pendingEvent_ = 0;         ///< Original: _SafeStr_1122.
        bool arrived_ = false;         ///< Original: _SafeStr_2635.
    };

} // namespace takeover
