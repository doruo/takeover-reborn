// Squad.h - a squad's orders and behaviour (original: _SafePkg_868/_SafeCls_1114).
//
// State machine of the whole squad:
//   Idle      standing; fights enemies that come close
//   Moving    follows a road path to a destination
//   Fighting  every soldier picks the best target nearby
//   Dead      every soldier is dead
//   Holding   keeps a formation point (spells, rally)
//   Inside    garrisoned in a stronghold: heals, repairs it
// Orders and situations arrive as prioritized events (see the EV_ constants),
// the strongest one per frame is processed.
#pragma once

#include "game/sim/base/TypedKey.h"
#include "game/sim/squads/SquadBase.h"

#include <memory>
#include <optional>
#include <vector>

namespace takeover {

class ISquadLevel;

/// The key of a squad (original: _SafeStr_1142, kind "sqd").
class SquadKey : public TypedKey {
public:
    SquadKey() : TypedKey("sqd") {}
};

/// What a squad is created from (original: _SafeStr_1189).
struct SquadDef {
    virtual ~SquadDef() = default;
    ISquadLevel* level = nullptr; ///< Original: _SafeStr_1241.
    std::shared_ptr<CombatantKey> owner;
    ArmyStatBonus armyStatBonus;
    const ArmyStat* armyStat = nullptr;
};

class Squad : public SquadBase {
public:
    // States.
    static constexpr int STATE_IDLE = 0;     ///< _SafeStr_1202
    static constexpr int STATE_MOVING = 1;   ///< _SafeStr_1205
    static constexpr int STATE_FIGHTING = 2; ///< _SafeStr_1206
    static constexpr int STATE_DEAD = 3;     ///< _SafeStr_1204
    static constexpr int STATE_HOLDING = 4;  ///< _SafeStr_1216
    static constexpr int STATE_INSIDE = 5;   ///< _SafeStr_1220
    // Events, by priority.
    static constexpr int EV_HOLD = 60;           ///< _SafeStr_1203
    static constexpr int EV_NEW_DESTINATION = 30; ///< _SafeStr_1219
    static constexpr int EV_ARRIVED = 1000;      ///< _SafeStr_1201
    static constexpr int EV_ENTER_NODE = 1100;   ///< _SafeStr_1218
    static constexpr int EV_MOVE = 1200;         ///< _SafeStr_1209
    static constexpr int EV_MOVE_HOME = 1300;    ///< _SafeStr_1214
    static constexpr int EV_ENEMY_NEAR = 1500;   ///< _SafeStr_1208
    static constexpr int EV_FIGHT = 1700;        ///< _SafeStr_1217
    static constexpr int EV_ALL_DEAD = 2000;     ///< _SafeStr_1200

    Squad() = default;
    Squad& init(const SquadDef& def); ///< _SafeStr_1174

    [[nodiscard]] const SquadKey& getKey() const { return *key_; } ///< _SafeStr_781
    [[nodiscard]] ISquadLevel* getLevelLogic() const { return level_; } ///< _SafeStr_1161

    // --- Orders ---------------------------------------------------------------------------
    bool commandMove(const Point2D& point);     ///< _SafeStr_1228
    bool commandMoveHome(const Point2D& point); ///< _SafeStr_1138
    void commandGotoPoint(const Point2D& point) { commandMoveHome(point); }
    void setHoldPoint(std::optional<Point2D> point) { holdPoint_ = point; } ///< _SafeStr_1221
    [[nodiscard]] const std::optional<Point2D>& getHoldPoint() const { return holdPoint_; } ///< _SafeStr_1157
    /// Garrisons the squad in a stronghold (original: _SafeStr_1232).
    void enterNode(KeyNode& node);
    bool postEvent(int event);    ///< _SafeStr_1117
    bool postArrived() { return postEvent(EV_ARRIVED); } ///< _SafeStr_1132

    // --- State ------------------------------------------------------------------------------
    [[nodiscard]] int getFSMState() const { return state_; }
    [[nodiscard]] bool isIdle() const { return state_ == STATE_IDLE; }       ///< _SafeStr_1237
    [[nodiscard]] bool isHolding() const { return state_ == STATE_HOLDING; } ///< _SafeStr_1207
    /// Every soldier is dead and no corpse is shown any more: the level can
    /// remove the squad (original: _SafeStr_1229).
    [[nodiscard]] bool isReadyToRemove() const { return readyToRemove_; }
    [[nodiscard]] bool isFighting() const { return state_ == STATE_FIGHTING; } ///< _SafeStr_1224
    [[nodiscard]] bool isInside() const { return state_ == STATE_INSIDE; }   ///< _SafeStr_1146
    [[nodiscard]] bool isDead() const { return state_ == STATE_DEAD; }       ///< _SafeStr_1136
    /// Can a squad of `race` attack this one (original: _SafeStr_1222)?
    [[nodiscard]] bool isAttackableBy(Race race) const;
    /// Is `other` within engagement distance (original: _SafeStr_1244)?
    [[nodiscard]] bool canEngage(Squad& other) const;
    [[nodiscard]] double getRadius() const { return 25; } ///< _SafeStr_1215
    [[nodiscard]] double engageDistance() const;          ///< _SafeStr_1187
    [[nodiscard]] int lastWaypointIndex() const;          ///< _SafeStr_1231
    [[nodiscard]] const std::vector<Point2D>* getPath() const { return path_ ? &*path_ : nullptr; }
    [[nodiscard]] const std::optional<Point2D>& getDestination() const { return destination_; }

    void customUpdate(double dt) override;
    void customDispose() override;

    // --- Presentation hooks ---------------------------------------------------------------------
    virtual void notifyStartBattle() {}
    virtual void notifyAllUnitsDead() {}
    virtual void notifySquadAtPosition();
    void moraleChanged(MoraleStatus status) override;

    /// The enemy stronghold in reach, cached for the frame (original: _SafeStr_1133).
    KeyNode* nearestEnemyNode();
    /// Enemy squads in reach, cached for the frame (original: _SafeStr_1137).
    const std::vector<Squad*>& nearbyEnemySquads();

protected:
    void registryUnitEvents(Unit& unit) override;
    virtual void changeFSMState(int state);
    void handleEvent(int state, int event); ///< _SafeStr_1162
    void updateState(int state, double dt); ///< _SafeStr_1165
    void updateFSM(double dt);              ///< _SafeStr_1160
    void updateIdle(double dt);             ///< _SafeStr_1196
    void updateHolding(double dt);          ///< _SafeStr_1179
    void updateFighting(double dt);         ///< _SafeStr_1175
    void updateMoving(double dt);           ///< _SafeStr_1163
    void updateInside(double dt);           ///< _SafeStr_1176
    [[nodiscard]] bool allArrived();        ///< include
    void clearCaches() { enemySquads_.reset(), enemyNode_ = nullptr, enemyNodeCached_ = false; } ///< _SafeStr_1191
    /// Keeps the soldiers apart and away from the enemy stronghold (original: _SafeStr_1186).
    void separateUnits(double dt);
    bool postHold() { return postEvent(EV_HOLD); } ///< _SafeStr_1195
    void enterIdle() { stopAll(); }                ///< _SafeStr_1198
    void enterHolding() { setFormationTarget(*holdPoint_); } ///< _SafeStr_1156
    void enterFighting();      ///< _SafeStr_1154
    void enterMoving();        ///< _SafeStr_1143
    void enterInside();        ///< _SafeStr_1159
    virtual void enterDead() {} ///< _SafeStr_1166
    void nextWaypoint();       ///< _SafeStr_1140
    /// The best target for a soldier: close enemies, preferring those that
    /// attack it, spreading the squad's attacks (original: _SafeStr_1155).
    int chooseTargetFor(Unit& unit);
    void stopUnitsFighting();  ///< _SafeStr_1152
    void markRetargetNode() { retargetNodeOnArrival_ = true; } ///< _SafeStr_1129
    void onUnitDamaged(int, BaseObject* sender, const std::any& attackerId); ///< _SafeStr_1153
    void postFight() { postEvent(EV_FIGHT); } ///< _SafeStr_1135
    void initFSM();            ///< _SafeStr_1182
    int destinationForRole();  ///< _SafeStr_1164
    [[nodiscard]] std::optional<SquadState> actionStatus(int state) const; ///< _SafeStr_1183
    void clearMovement();      ///< _SafeStr_1193
    bool chooseTarget(Unit& unit); ///< _SafeStr_1168
    void checkEnemies();       ///< _SafeStr_1134
    virtual void checkDeath();
    void stopMoving();         ///< _SafeStr_1130

    std::optional<std::vector<Squad*>> enemySquads_; ///< Cache (original: _SafeStr_1121).
    KeyNode* enemyNode_ = nullptr;          ///< Cache (original: _SafeStr_1124).
    bool enemyNodeCached_ = false;
    std::optional<std::vector<Point2D>> path_; ///< Original: _SafeStr_1120.
    std::optional<Point2D> destination_;       ///< Original: _SafeStr_1119.
    std::optional<Point2D> holdPoint_;         ///< Original: _SafeStr_1126.
    int pathIndex_ = 0;                        ///< Original: _SafeStr_1123.
    bool readyToRemove_ = false;               ///< Original: _SafeStr_842.
    bool retargetNodeOnArrival_ = false;       ///< Original: _SafeStr_1125.
    std::optional<Point2D> pendingDestination_; ///< Original: _SafeStr_1115.
    std::shared_ptr<SquadKey> key_;            ///< Original: _SafeStr_843.
    int state_ = STATE_IDLE;                   ///< Original: _SafeStr_1116.
    int pendingEvent_ = 0;                     ///< Original: _SafeStr_1122.
    ISquadLevel* level_ = nullptr;             ///< Original: _SafeStr_1118.
    bool allArrived_ = false;                  ///< Original: _SafeStr_1127.
    double nextHealTime_ = 0;                  ///< Original: _SafeStr_1131.
    int healAmount_ = 0;                       ///< Original: _SafeStr_1141.
};

/// What a squad asks the level (original: interface _SafeStr_1147,
/// implemented by LevelLogic).
class ISquadLevel {
public:
    virtual ~ISquadLevel() = default;
    /// The squad reached an enemy or neutral stronghold: may it go in? (original: _SafeStr_1238)
    virtual bool tryEnterNode(Squad& squad) = 0;
    /// The squad stopped outside any stronghold (original: _SafeStr_1236).
    virtual void onSquadStopped(Squad& squad) = 0;
    virtual void onSquadMoraleChanged(Squad& squad, MoraleStatus status) = 0; ///< _SafeStr_1242
    virtual Point2D positionOfNode(const TypedKey& node) = 0;                 ///< _SafeStr_2274
    virtual KeyNode* nodeAt(const Point2D& point) = 0;                        ///< _SafeStr_1212
    /// The enemy stronghold the squad can attack from where it is (original: _SafeStr_1225).
    virtual KeyNode* enemyNodeInRange(Squad& squad) = 0;
    /// Enemy squads close enough to fight (original: _SafeStr_1226).
    virtual std::vector<Squad*> enemySquadsInRange(Squad& squad) = 0;
    /// Road path between two points (original: _SafeStr_1230).
    virtual std::optional<std::vector<Point2D>> findPath(const Point2D& from, const Point2D& to) = 0;
};

} // namespace takeover
