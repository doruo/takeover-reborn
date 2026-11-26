// Combatant methods that need the squads and strongholds of a battle.
#include "game/sim/battle/Combatant.h"

#include "game/sim/battle/ArmyStat.h"
#include "game/sim/level/KeyNode.h"
#include "game/sim/squads/Squad.h"

namespace takeover {

bool Combatant::isOwnSquad(const Squad& squad) const { return race_ == squad.getRace(); }

bool Combatant::ownsKeyNode(const KeyNode& node) const { return node.owner == description; }

void Combatant::buySquad(Squad& squad) {
    spendGold(getUnitCost(*squad.getArmyStat()));
    missionStats->add(MissionStats::SQUADS_PURCHASED, 1);
}

bool Combatant::upgradeKeyNode(KeyNode& node) {
    const int level = node.getLevel();
    const int cost = node.getUpgradeCost(level);
    if (getGold() >= cost && !node.isMaxLevel()) {
        spendGold(cost);
        node.setLevel(level + 1);
        return true;
    }
    return false;
}

} // namespace takeover
