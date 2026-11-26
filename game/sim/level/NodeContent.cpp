#include "game/sim/level/NodeContent.h"

#include "game/sim/battle/Combatant.h"

namespace takeover {

NodeContent::NodeContent(std::shared_ptr<NodeContentKey> contentKey, std::shared_ptr<NodeKey> nodeKey)
    : description(std::move(contentKey)), nodeDescription(std::move(nodeKey)) {}

void NodeContent::initBySupplyBuilding(SupplyBuildingType type, const Combatant* combatant) {
    switch (type) {
    case SupplyBuildingType::Empty:
        goldIncome = 0;
        manaIncome = 0;
        supplyIncome = 0;
        name = "";
        break;
    case SupplyBuildingType::Village:
        goldIncome = 0;
        supplyIncome = 1;
        name = combatant->getRace() == Race::TheEmpire ? "Graveyard" : "Farm";
        break;
    case SupplyBuildingType::Mine:
        goldIncome = 2;
        supplyIncome = 0;
        name = "Mine";
        break;
    case SupplyBuildingType::Obelisk:
        goldIncome = 0;
        manaIncome = 1;
        supplyIncome = 0;
        name = "Obelisk";
        break;
    }
    if (combatant != nullptr) cost = combatant->getSupplyCost(type);
}

bool Chest::collect(Squad& /*squad*/, Combatant& combatant) {
    if (!collected_) {
        combatant.addChestGold(GOLD);
        collected_ = true;
        return true;
    }
    return false;
}

} // namespace takeover
