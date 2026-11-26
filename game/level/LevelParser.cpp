#include "game/level/LevelParser.h"

#include "game/framework/Symbols.h"
#include "game/sim/battle/ArmyStat.h"

#include "engine/display/MovieClip.h"

#include <cstdio>

namespace takeover {

    std::shared_ptr<Level> LevelParser::parse(const BattleDef& def) {
        LevelParser parser;
        parser.init(def);

        return parser.level_;
    }

    void LevelParser::init(const BattleDef& def) {
        const int levelNum = def.levelUID;

        resources_ = def.resources;
        level_ = std::make_shared<Level>();
        level_->init(def);

        const std::string mapClass = "Level" + std::to_string(levelNum) + "Class";
        std::shared_ptr<engine::MovieClip> markers = symbols::create("Level" + std::to_string(levelNum) + "GraphMarkersClass");
        parseMarkers(*markers);

        LevelLandDef landDef;
        landDef.mapClass = mapClass;
        landDef.locationViews = locationViews_;

        level_->land().init(landDef);
        level_->land().redraw();

        fillLayer();
        level_->refreshAllIncomes();
        level_->assembleView();
        level_->linkKeyNodes();
    }

    void LevelParser::parseMarkers(engine::DisplayObjectContainer& markers) {

        // Crossings first
        for (int i = 0; i < markers.numChildren(); ++i) {
            engine::DisplayObject* marker = markers.getChildAt(i);
            if (marker->name().find("nd") != std::string::npos)
                addNode(*marker);
        }

        // roads between them
        for (int i = 0; i < markers.numChildren(); ++i) {
            engine::DisplayObject* marker = markers.getChildAt(i);
            if (marker->name().find("ed") != std::string::npos)
                addRoad(*marker);
        }
    }

    void LevelParser::addNode(engine::DisplayObject& marker) {

        const std::string& name = marker.name();
        const std::size_t pos = name.find('_');

        // "l12nd_xxx": location 12. (AS3 int("") is 0.)
        const std::string locationText = pos >= 3 ? name.substr(1, pos - 3) : "";
        const int locationNum = locationText.empty() ? 0 : std::atoi(locationText.c_str());
        const std::string nodeId = name.substr(pos + 1);

        auto node = std::make_shared<GraphNode>(std::make_shared<NodeKey>());
        node->pos = LevelVisual::getPosition(marker);
        node->locationNum = locationNum;
        nodes_[nodeId] = node;

        level_->getGraphNodes().addObject(node, *node->description);

        while (static_cast<int>(locationViews_.size()) < locationNum)
            locationViews_.push_back(0);

        // Marker
        if (KeyNodeView::isKeyNodeMarker(marker)) {

            const Race race = KeyNodeView::markerRace(marker);

            if (static_cast<int>(locationViews_.size()) <= locationNum)
                locationViews_.resize(static_cast<std::size_t>(locationNum) + 1, 0);

            locationViews_[static_cast<std::size_t>(locationNum)] = Combatant::raceTableIndex(race);
            level_->setLocationOwner(locationNum, Combatant::raceFromTableIndex(locationViews_[static_cast<std::size_t>(locationNum)]));

            Combatant* owner = level_->getCombatantByRace(race);
            if (owner == nullptr) {
                std::fprintf(stderr, "LevelParser: no combatant for stronghold %s\n", name.c_str());
                return;
            }

            auto keyNode = std::make_shared<FKeyNodeContent>(
                *resources_, *node, *owner, KeyNodeView::markerType(marker)
            );

            level_->getKeyNodes().addObject(keyNode, *keyNode->description);
        }

        // Chest
        else if (symbols::isInstance(marker, "NodeChestClass")) {
            auto chest = std::make_shared<ChestView>(*node);
            level_->getChests().addObject(chest, *chest->description);
        }

        // Build Place
        else if (symbols::isInstance(marker, "NodeBuildHereClass")) {
            auto place = std::make_shared<BuildPlace>(*resources_, *node);
            level_->getBuildPlaces().addObject(place, *place->description);
        }

        // Ogre
        else if (symbols::isInstance(marker, "NodeDefenderOgreClass")) {
            // A neutral ogre guarding the place.
            std::shared_ptr<FSquad> ogre = level_->createSquadFor(nullptr, ArmyStat::OGRE, ArmyStatBonus{});
            ogre->moveTo(marker.x(), marker.y() * LevelVisual::Y_KOEFF);
            level_->iconLayer()->addChild(ogre->iconPtr());
            ogre->setHoldPoint(ogre->getMainUnit()->getPosition().getCopy());
        }
    }

    void LevelParser::addRoad(engine::DisplayObject& marker) {

        const std::string& name = marker.name();
        const std::size_t pos = name.find('_');
        const std::size_t pos2 = name.find('_', pos + 1);
        const std::string idA = name.substr(pos + 1, pos2 - pos - 1);
        const std::string idB = name.substr(pos2 + 1);

        auto road = std::make_shared<Road>(std::make_shared<RoadKey>());
        const GraphNode& nodeA = *nodes_.at(idA);
        const GraphNode& nodeB = *nodes_.at(idB);

        road->nodeA = nodeA.description;
        road->nodeB = nodeB.description;
        road->wayPoints.emplace_back(nodeA.pos.x, nodeA.pos.y);

        if (auto* container = dynamic_cast<engine::DisplayObjectContainer*>(&marker)) {
            for (int i = 0; i < container->numChildren(); ++i) {
                if (auto* point = dynamic_cast<engine::DisplayObjectContainer*>(container->getChildAt(i)))
                    road->wayPoints.push_back(LevelVisual::getPosition(*point));
            }
        }

        road->wayPoints.emplace_back(nodeB.pos.x, nodeB.pos.y);
        road->length = road->computeLength();
        level_->getRoads().addObject(road, *road->description);
    }

    void LevelParser::fillLayer() {

        RasterGraphicLayer& layer = level_->rasterLayer();

        for (const auto& location : level_->land().allObjectsMatrix()) {

            for (const auto& object : location) {
                layer.addRGLObject(object, RasterGraphicLayer::LAYER_UNIT);
            }

            for (const auto& [index, node] : level_->getKeyNodes().objects()) {

                auto content = std::static_pointer_cast<FKeyNodeContent>(node);
                layer.addRGLObject(content, RasterGraphicLayer::LAYER_UNIT);

                content->setLocationType(
                    level_->land()
                    .locationView(level_->getGraphNodes()
                    .getObject(content->nodeDescription.get())->locationNum)
                );
            }
        }

        for (const auto& [index, chest] : level_->getChests().objects()) {

            layer.addRGLObject(
                std::static_pointer_cast<ChestView>(chest),
                RasterGraphicLayer::LAYER_UNIT
            );

            for (const auto& [index, place] : level_->getBuildPlaces().objects()) {

                auto buildPlace = std::static_pointer_cast<BuildPlace>(place);
                layer.addRGLObject(buildPlace, RasterGraphicLayer::LAYER_UNIT);

                buildPlace->setLocationType(
                    level_->land()
                    .locationView(
                        level_->getGraphNodes()
                        .getObject(buildPlace->nodeDescription.get())->locationNum)
                    );
            }
        }
    }

} // namespace takeover
