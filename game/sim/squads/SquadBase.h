// SquadBase.h - a group of soldiers sharing morale, experience and a
// formation (original: _SafePkg_868/_SafeCls_971).
#pragma once

#include "game/sim/abilities/Morale.h"
#include "game/sim/base/BaseObject.h"
#include "game/sim/base/Point2D.h"
#include "game/sim/battle/ArmyStatBonus.h"
#include "game/sim/battle/Race.h"
#include "game/sim/squads/MoraleStatus.h"
#include "game/sim/units/Unit.h"

#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace takeover {

    class ArmyStat;
    class CombatantKey;
    class KeyNode;

    class SquadBase : public BaseObject {
    public:
        static constexpr double UNIT_SPACING = 14; ///< _SafeStr_1035

        // Formations: offsets of the soldiers behind the leader (x back, y side).
        static const std::vector<Point2D> FORMATION_3x3;
        static const std::vector<Point2D> FORMATION_3x4;
        static const std::vector<Point2D> FORMATION_3x2;
        static const std::vector<Point2D> FORMATION_5;
        static const std::vector<Point2D> FORMATION_7;
        static const std::vector<Point2D> FORMATION_3;
        static const std::vector<Point2D> FORMATION_1;

        static const std::vector<Point2D>& formationFor(int unitCount); ///< _SafeStr_1031
        static int levelForXp(double xp);  ///< _SafeStr_1026
        static double xpForLevel(int level); ///< _SafeStr_1027

        SquadBase() = default;

        // /----- IDENTITY AND STATS -----/

        [[nodiscard]] Race getRace() const;
        [[nodiscard]] const ArmyStat* getArmyStat() const { return stat_; } ///< _SafeStr_1044
        [[nodiscard]] UnitType getUnitType() const;                       ///< _SafeStr_1048
        [[nodiscard]] const std::shared_ptr<CombatantKey>& getOwner() const { return owner_; } ///< _SafeStr_1030
        [[nodiscard]] KeyNode* getKeyNode() const { return keyNode_; }   ///< _SafeStr_1046
        [[nodiscard]] SquadRole getRole() const;                         ///< _SafeStr_1080
        [[nodiscard]] bool isMechanism() const { return getMainUnit()->isMechanism(); } ///< _SafeStr_863

        // /----- UNITS -----/

        [[nodiscard]] const std::map<int, std::shared_ptr<Unit>>& getUnits() const { return units_; } ///< _SafeStr_1004
        [[nodiscard]] int getUnitCount() const { return static_cast<int>(formation_.size()); } ///< _SafeStr_995
        /// The first living soldier, or the first one (original: getMainUnit).
        [[nodiscard]] Unit* getMainUnit() const;
        [[nodiscard]] int getMaxHP() const { return maxHP_; }  ///< _SafeStr_1037
        [[nodiscard]] int getHP() const { return currentHP_; } ///< _SafeStr_1051
        [[nodiscard]] double hpFactor() const { return static_cast<double>(currentHP_) / maxHP_; } ///< _SafeStr_1069
        [[nodiscard]] bool hpChanged() const { return previousHP_ != currentHP_; } ///< _SafeStr_1078
        void stopAll();                        ///< _SafeStr_1064
        void moveTo(double x, double y);       ///< Teleports every soldier.
        void increaseSpeed(double factor, double seconds);

        /// Puts the soldiers' destinations in formation around `point` (original: _SafeStr_1050).
        void setFormationTarget(const Point2D& point);

        /// Pulls the soldiers back into formation around the leader (original: _SafeStr_1082).
        void keepFormation();
        [[nodiscard]] bool isNear(const Point2D& point, double radius) const; ///< _SafeStr_866

        // /----- MORALE -----/

        [[nodiscard]] Morale& getMorale() const { return *morale_; } ///< _SafeStr_1060
        [[nodiscard]] MoraleStatus getMoraleStatus() const { return morale_->getStatus(); } ///< _SafeStr_1076
        [[nodiscard]] double moraleRatio() const { return morale_->ratio(); } ///< _SafeStr_1033
        [[nodiscard]] const std::shared_ptr<CombatantKey>& getLastAttackerOwner() const { return lastAttackerOwner_; } ///< _SafeStr_1049
        [[nodiscard]] std::optional<SquadState> getActionState() const { return actionState_; } ///< _SafeStr_1066
        virtual void onActionStatusUpdated() {}
        virtual void moraleChanged(MoraleStatus) {}

        // /----- EXPERIENCE -----/

        [[nodiscard]] int getLevel() const { return levelForXp(xp_); } ///< _SafeStr_999
        [[nodiscard]] double getXp() const { return xp_; }
        void addXP(double xp);
        virtual void makelevelUP(int level);

        // /----- SPELLS -----/

        /// Heals wounded soldiers, then brings dead ones back (original: _SafeStr_865).
        void heal(int amount);
        bool enchant(const Point2D& point, double radius, double seconds, int resistPercent); ///< _SafeStr_1065
        void killAll();                                                         ///< _SafeStr_1025
        void terrify();                                                         ///< _SafeStr_1045
        bool setSpellDefence(const Point2D& point, double radius);              ///< _SafeStr_1036
        void makeHeroic();                                                      ///< _SafeStr_870
        bool setDamageFactor(const Point2D& point, double radius, double factor); ///< _SafeStr_1042
        [[nodiscard]] bool isKilledBySpell() const { return killed_; }          ///< _SafeStr_1003

        void customUpdate(double dt) override;
        void customDispose() override;

    protected:
        SquadBase& init(std::shared_ptr<CombatantKey> owner, const ArmyStat& stat, const ArmyStatBonus& bonus); ///< _SafeStr_1017

        /// Creates soldier `index` (overridden by the presentation squad).
        virtual std::shared_ptr<Unit> createUnit(int index);
        virtual void registryUnitEvents(Unit& unit);
        virtual void beforeDispose() {} ///< _SafeStr_1013
        void updateMorale(double dt);   ///< _SafeStr_1012
        void initMorale();              ///< _SafeStr_1010
        void disposeMorale();           ///< _SafeStr_1021
        void initXp(int unitCount);     ///< _SafeStr_1008
        [[nodiscard]] Unit* findDeadUnit() const;    ///< _SafeStr_1022
        [[nodiscard]] Unit* findWoundedUnit() const; ///< _SafeStr_1011
        static void moveUnitTowards(Unit& unit, double x, double y, double step); ///< _SafeStr_1023
        static std::vector<Point2D> toPolar(const std::vector<Point2D>& formation, int count); ///< _SafeStr_1018
        [[nodiscard]] Unit* unitAt(int index) const { return units_.at(index).get(); }

        // Event handlers (original names in comments).
        void onUnitAttacks(int, BaseObject*, const std::any&); ///< _SafeStr_1016
        void onUnitHit(int, BaseObject*, const std::any& attackerId); ///< _SafeStr_1014
        void onAttackDone(int, BaseObject*, const std::any&);  ///< _SafeStr_1007
        void forwardEvent(int type, BaseObject*, const std::any& data); ///< _SafeStr_1015
        virtual void onChangeMorale(int, BaseObject*, const std::any& status);

        double xpPerAttack_ = 0;     ///< Original: _SafeStr_1002.
        double xp_ = 0;              ///< Original: _SafeStr_984.
        std::map<int, std::shared_ptr<Unit>> units_; ///< Original: _SafeStr_974.
        int maxHP_ = 0;              ///< Original: _SafeStr_976.
        double targetY_ = 0;         ///< Original: _SafeStr_986.
        double targetX_ = 0;         ///< Original: _SafeStr_989.
        double direction_ = 0;       ///< Original: _SafeStr_988.
        int previousHP_ = 0;         ///< Original: _SafeStr_992.
        std::optional<SquadState> previousActionState_; ///< Original: _SafeStr_983.
        KeyNode* keyNode_ = nullptr; ///< Its stronghold (original: _SafeStr_981).
        std::shared_ptr<CombatantKey> owner_;             ///< Original: _SafeStr_982.
        std::shared_ptr<CombatantKey> lastAttackerOwner_; ///< Original: _SafeStr_979.
        std::shared_ptr<Morale> morale_; ///< Original: _SafeStr_973.
        std::vector<Point2D> formation_; ///< Polar offsets (angle, distance) (original: _SafeStr_987).
        int currentHP_ = 0;              ///< Original: _SafeStr_975.
        std::optional<SquadState> actionState_; ///< Original: _SafeStr_980.
        ArmyStatBonus bonus_;            ///< Original: _SafeStr_985.
        const ArmyStat* stat_ = nullptr; ///< Original: _SafeStr_977.
        bool killed_ = false;            ///< Original: _SafeStr_1003.
    };

} // namespace takeover
