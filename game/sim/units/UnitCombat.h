// UnitCombat.h - how a soldier fights its target (original:
// _SafePkg_805/_SafeStr_2642).
//
// A table-driven state machine. Each frame the distance to the target
// becomes an event ("in melee range", "in shooting range", "out of
// range"...), and the table says which state follows and what to do:
//
//   Idle -> Approach (walk / charge / dash to the target)
//        -> Melee (strike every attack period) or Ranged (shoot)
//        -> Wait (back to approach when the target moves away)
//
// Events have priorities (their numbers): the highest posted in a frame
// is processed.
#pragma once

#include "game/sim/base/BaseObject.h"
#include "game/sim/units/ITarget.h"

#include <functional>
#include <optional>
#include <vector>

namespace takeover {

class Unit;

class UnitCombat : public BaseObject {
public:
    // Events, by priority.
    static constexpr int EV_IN_MELEE_RANGE = 10;     ///< _SafeStr_2898
    static constexpr int EV_OUT_OF_MELEE_RANGE = 11; ///< _SafeStr_2901
    static constexpr int EV_IN_SHOOTING_RANGE = 50;  ///< _SafeStr_2900
    static constexpr int EV_OUT_OF_RANGE = 51;       ///< _SafeStr_2899
    static constexpr int EV_SHOOT = 90;              ///< _SafeStr_2906
    static constexpr int EV_MELEE_DONE = 100;        ///< _SafeStr_2909
    static constexpr int EV_SHOT_DONE = 101;         ///< _SafeStr_2910
    static constexpr int EV_STRIKE = 150;            ///< _SafeStr_2907
    // States.
    static constexpr int IDLE = 0;     ///< _SafeStr_2911
    static constexpr int APPROACH = 1; ///< _SafeStr_2884
    static constexpr int MELEE = 2;    ///< _SafeStr_2887
    static constexpr int RANGED = 3;   ///< _SafeStr_2885
    static constexpr int WAIT = 4;     ///< _SafeStr_2890

    explicit UnitCombat(Unit* unit);

    /// The unit was hit by `attackerId`: maybe fight back (original: _SafeStr_2678).
    void onAttackedBy(int attackerId);
    void startApproach();                  ///< _SafeStr_2673
    void setTarget(int targetId) { targetId_ = targetId; } ///< _SafeStr_2677
    void attackTarget(int targetId);       ///< _SafeStr_2680
    void reset();
    [[nodiscard]] bool isApproaching() const { return state_ == APPROACH; } ///< _SafeStr_2674
    /// The target if it is still alive (original: _SafeStr_2667).
    [[nodiscard]] static ITarget* getLiveTarget(int id);
    [[nodiscard]] int getTargetId() const { return targetId_; }    ///< _SafeStr_2645
    [[nodiscard]] std::optional<TargetType> getTargetType() const { return targetType_; } ///< _SafeStr_2679

    void customUpdate(double dt) override;
    void customDispose() override;

private:
    void updateApproach(double dt, ITarget& target); ///< _SafeStr_2913
    void processPendingEvent();                      ///< _SafeStr_2916
    void updateRanged(double dt, int targetId);      ///< _SafeStr_2925
    void updateMelee(double dt, ITarget& target);    ///< _SafeStr_2924
    void checkShotDone();   ///< _SafeStr_2895
    void checkMeleeDone();  ///< _SafeStr_2905
    void tryStrike();       ///< _SafeStr_2902
    void tryShoot();        ///< _SafeStr_2903
    void enterApproach();   ///< _SafeStr_2922
    void enterRanged();     ///< _SafeStr_2915
    void startShotAnimation();   ///< _SafeStr_2926
    void enterMelee();      ///< _SafeStr_2914
    void startStrikeAnimation(); ///< _SafeStr_2923
    [[nodiscard]] int rangeEvent(const ITarget& target) const; ///< _SafeStr_2908
    void resetPursuit();    ///< _SafeStr_2919
    void handleEvent(int event); ///< _SafeStr_1540
    void onEnterState(int state); ///< _SafeStr_2927
    void onExitState(int state);  ///< _SafeStr_2921
    void stopFight();       ///< _SafeStr_2904
    void buildTransitions();      ///< _SafeStr_2917
    [[nodiscard]] bool isTargetFightingMe(int targetId) const; ///< _SafeStr_2920
    void setState(int state);     ///< _SafeStr_743
    void addTransition(int from, int event, int to, void (UnitCombat::*action)()); ///< _SafeStr_822
    void addPursuit(int amount);  ///< _SafeStr_2918
    void postEvent(int event);    ///< _SafeStr_1506

    struct Transition {
        int from, event, to;
        void (UnitCombat::*action)();
    };

    Unit* unit_;                    ///< Original: _SafeStr_2881.
    double previousAttackTime_ = 0; ///< Original: _SafeStr_2883.
    int targetId_ = -1;             ///< Original: _SafeStr_2888.
    std::vector<Transition> transitions_;
    std::optional<TargetType> targetType_; ///< Original: _SafeStr_2897.
    int state_ = IDLE;              ///< Original: _SafeStr_1116.
    int pendingEvent_ = 0;          ///< Original: _SafeStr_1491.
    double hitTime_ = 0;            ///< When the blow lands in the animation (original: m_animAttackSpeed2).
    double animationLength_ = 0;    ///< Original: _SafeStr_2889.
    double timeOutOfAttack_ = 0;    ///< Original: _SafeStr_2886.
    double attackTime_ = 0;         ///< Original: _SafeStr_2882.
    double approachTime_ = 0;       ///< Original: _SafeStr_2896 (never reset, as the original).
};

} // namespace takeover
