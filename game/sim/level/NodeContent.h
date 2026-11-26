// NodeContent.h - something standing on a crossing of the map, with the
// incomes it gives its owner (original: level/_SafeStr_832).
//
// Strongholds (KeyNode), build places (BuildPlace) and chests derive from it.
#pragma once

#include "game/sim/base/BaseObject.h"
#include "game/sim/level/LevelKeys.h"
#include "game/sim/level/SupplyBuildingType.h"

#include <memory>
#include <string>

namespace takeover {

    class Combatant;
    class Squad;

    class NodeContent : public BaseObject {
    public:
        NodeContent(std::shared_ptr<NodeContentKey> description, std::shared_ptr<NodeKey> nodeDescription);

        /// Sets the incomes and the name of a supply building; the cost is the
        /// combatant's price for it (original: initBySupplyBuilding).
        virtual void initBySupplyBuilding(SupplyBuildingType type, const Combatant* combatant);

        std::shared_ptr<NodeContentKey> description;
        std::shared_ptr<NodeKey> nodeDescription; ///< The crossing it stands on.
        std::string name;
        int goldIncome = 0;     ///< Original: _SafeStr_1327.
        int manaIncome = 0;     ///< Original: _SafeStr_1328.
        int supplyIncome = 0;   ///< Original: _SafeStr_1329.
        int ultimateIncome = 0; ///< Charge of the ultimate power (original: _SafeStr_1483).
        int cost = 0;
    };

    /// A treasure chest: the first squad to reach it brings 500 gold to its
    /// owner (original: level/_SafeCls_1956).
    class Chest : public NodeContent {
    public:
        static constexpr int GOLD = 500; ///< Original: _SafeStr_1958.

        using NodeContent::NodeContent;
        [[nodiscard]] bool isCollected() const { return collected_; } ///< _SafeStr_1960
        /// Gives the gold once to the owner of `squad`; true the first time
        /// (original: collect).
        virtual bool collect(Squad& squad, Combatant& combatant);

    protected:
        bool collected_ = false; ///< Original: _SafeStr_1957.
    };

} // namespace takeover
