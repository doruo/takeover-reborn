#include "game/GameBattle.h"

#include "game/framework/input/KeyboardDevice.h"
#include "game/hud/WindowController.h"
#include "game/level/Level.h"
#include "game/level/LevelParser.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/sim/spells/SpellInfo.h"

#include "engine/display/DisplayObjectContainer.h"

namespace takeover {

    GameBattle::GameBattle(const BattleDef& def) : def_(def) {
        initVisualBlock();
        initProcesses();
        startTutorials();
    }

    GameBattle::~GameBattle() = default;

    void GameBattle::customDispose() {
        stopTutorials();
        disposeVisualBlock();
        signalMissionOver.dispose();
        signalMenu.dispose();
        signalHelp.dispose();
        processes_.clear();
        GameState::customDispose();
    }

    void GameBattle::update(double timeElapsed) {
        timeElapsed *= speed_;
        processInput(timeElapsed);
        updateHud(timeElapsed);
        level_->update(timeElapsed);

        if (isMissionOver())
            signalMissionOver.dispatch(level_->getMissionResultPtr());

        GameState::update(timeElapsed);
    }

    std::shared_ptr<MissionResult> GameBattle::missionStats() const { return level_->getMissionResultPtr(); }
    Signal<Level*>& GameBattle::signalStatsChanged() { return level_->signalStatsChanged; }

    void GameBattle::initVisualBlock() {
        image_ = std::make_shared<engine::Sprite>();
        level_ = LevelParser::parse(def_);
        image_->addChild(level_->view());
        image_->addChild(level_->cursorLayer());
        image_->addChild(level_->frontLayer());
        image_->addChild(level_->topLayer());
        initHud();

        for (const auto& [index, node] : level_->getKeyNodes().objects())
            static_cast<FKeyNodeContent&>(*node).addViewsTo(*windows_->image());

        level_->precache(def_.levelNum);
    }

    void GameBattle::disposeVisualBlock() {
        disposeHud();

        if (level_)
            level_->dispose();

        level_.reset();

        if (image_ && image_->parent() != nullptr)
            image_->parent()->removeChild(image_.get());

        image_.reset();
    }

    void GameBattle::initProcesses() {
        processes_ = {panel_.get(), windows_.get(), level_->cursorProcess(), level_->spellProcess(),
                      level_->orderProcess(), level_->selectionProcess()};
        speed_ = 1;
    }

    void GameBattle::processInput(double dt) {
        bool lock = false;

        // Space or Escape cancel whatever the player was doing.
        if (def_.keyboard->isKeyDown(32) || def_.keyboard->isKeyDown(27)) {
            for (IInputProcess* process : processes_)
                process->reset();

            lock = true;
        }

        for (IInputProcess* process : processes_)
            lock = process->process(dt, lock) || lock;
    }

    bool GameBattle::isMissionOver() const { return level_->getMissionResult()->missionOverStatus != MissionResult::IN_PROGRESS; }

    void GameBattle::initHud() {
        hud_ = std::make_shared<engine::Sprite>();
        image_->addChild(hud_);
        panel_ = std::make_shared<GamePanel>();
        panel_->init(*level_);
        hud_->addChild(panel_->image());
        panel_->signalMenuButton.addListener(this, &GameBattle::onMenuButton);
        panel_->signalSpellButton.addListener(this, &GameBattle::onSpellButton);
        panel_->signalUltimate.addListener(this, &GameBattle::onUltimateButton);
        level_->signalSpellModeOff.addListener(this, &GameBattle::onSpellModeOff);
        windows_ = std::make_shared<WindowController>(*level_);
        image_->addChild(windows_->image());
        level_->signalNodeSelected.addListener(this, &GameBattle::onNodeSelected);
    }

    void GameBattle::disposeHud() {
        if (windows_)
            windows_->dispose();

        windows_.reset();

        if (panel_)
            panel_->dispose();

        panel_.reset();

        if (hud_ && hud_->parent() != nullptr)
            hud_->parent()->removeChild(hud_.get());

        hud_.reset();
    }

    void GameBattle::updateHud(double dt) {
        if (level_->isStatsDirty())
            panel_->showResources(*this);

        panel_->update(dt);
        windows_->update(dt);
        level_->setStatsDirty(false);
    }

    // /----- I RESOURCE SOURCE -----/

    int GameBattle::getGold() const { return level_->player().getGold(); }
    int GameBattle::getArmyPower() const { return level_->player().getArmyPower(); }
    int GameBattle::getSupplyPower() const { return level_->player().getSupplyPower(); }
    int GameBattle::getMana() const { return level_->player().getMana(); }
    double GameBattle::getUltimateFactor() const { return level_->player().getUSpeelFactor(); }
    int GameBattle::getManaSpeed() const { return level_->player().getManaSpeed(); }
    int GameBattle::getGoldSpeed() const { return level_->player().getGoldSpeed(); }
    int GameBattle::getGoldForSupply() const { return level_->player().getGoldForSupply(); }

    // /----- HUD EVENTS -----/

    void GameBattle::onMenuButton(int id) {
        switch (id) {
        case GamePanel::BUT_MENU: signalMenu.dispatch(); break;
        case GamePanel::BUT_HELP: signalHelp.dispatch(); break;
        case GamePanel::BUT_FAST: speed_ = 2; break;
        case GamePanel::BUT_FAST_ON: speed_ = 1; break;
        default: break;
        }
    }

    void GameBattle::onSpellButton(int id) {
        switch (id) {
        case 0:
        case 1:
        case 2: level_->selectSpellButton(id); break;
        case GamePanel::BUT_CHEAT_0:
            level_->player().addMana(70);
            level_->player().addGold(150);
            level_->player().forceEnableUltimate();
            level_->setStatsDirty(true);
            break;
        case GamePanel::BUT_CHEAT_1: level_->selectSpellButton(3); break;
        default: break;
        }
    }

    void GameBattle::onUltimateButton(int id) {
        if (id == GamePanel::BUT_USPELL)
            level_->castUltimateSpell(level_->player());
    }

    void GameBattle::onSpellModeOff() { panel_->releaseSpellButtons(); }

    void GameBattle::onNodeSelected(NodeContent* node) { windows_->openWindow(node, &level_->player()); }

    // /----- TUTORIALS -----/

    void GameBattle::startTutorials() {
        savedSpells_ = level_->player().getSpells();

        if (def_.levelUID == 1) {
            windows_->signalWindowOpened.addListener(this, &GameBattle::onWindowOpened_Tutorial1);
            windows_->signalWindowClosed.addListener(this, &GameBattle::onWindowClosed_Tutorial1);
            level_->signalSquadBought.addListener(this, &GameBattle::onBuySquad_Tutorial1);
            level_->signalSquadKilled.addListener(this, &GameBattle::onKillSquad_Tutorial1);
            level_->signalNodeCaptured.addListener(this, &GameBattle::onKeyNodeCaptured_Tutorial1);
            level_->aiDisabled = true;
            enableSpells(false);

            // The Empire starts with exactly two squads of undying warriors.
            Combatant* enemy = level_->getCombatantByRace(Race::TheEmpire);
            level_->clearNodeIncomes(enemy);
            enemy->setMana(0);
            enemy->setGoldForDebug(ArmyStat::UNDYING_WARRIORS.cost + ArmyStat::UNDYING_WARRIORS.cost);
            WindowContentManager::LOCK_UNIT_ABOVE = 0;
            ArmyStat::UNDYING_WARRIORS.upkeep = 0;

            for (const auto& node : level_->keyNodesOf(enemy->description.get())) {
                node->supplyIncome = 1;

                if (node->getType() == KeyNodeType::Town) {
                    tutor1EnemySquad_ = level_->buyUnit(&ArmyStat::UNDYING_WARRIORS, *node).get();
                    tutor1EnemyTown_ = node.get();
                } else {
                    level_->buyUnit(&ArmyStat::UNDYING_WARRIORS, *node);
                }
            }

            ArmyStat::UNDYING_WARRIORS.upkeep = 1;
        } else if (def_.levelUID == 2) {
            level_->signalSupplyBuilt.addListener(this, &GameBattle::onSupplyBuilt_Tutorial2);
            level_->signalSquadBought.addListener(this, &GameBattle::onBuySquad_Tutorial2);
            level_->signalUnitAttack.addListener(this, &GameBattle::onUnitAttack_Tutorial2);
            level_->signalSquadKilled.addListener(this, &GameBattle::onKillSquad_Tutorial2);
            level_->signalNodeCaptured.addListener(this, &GameBattle::onKeyNodeCaptured_Tutorial2);
            level_->signalUltimateCast.addListener(this, &GameBattle::onUltimateCast_Tutorial2);
            Combatant& player = level_->player();
            player.setGoldForDebug(player.getGold() + player.getSupplyCost(SupplyBuildingType::Mine) +
                                   player.getSupplyCost(SupplyBuildingType::Obelisk));
            level_->buildSupplyOfType(player, SupplyBuildingType::Mine);
            level_->buildSupplyOfType(player, SupplyBuildingType::Obelisk);
            WindowContentManager::UNLOCK_OBELISK = false;
            WindowContentManager::UNLOCK_MINE = false;
            enableSpells(false);
            Combatant* enemy = level_->getCombatantByRace(Race::TheKhaganate);
            enemy->setGoldForDebug(ArmyStat::WARRIORS.cost * 3 + ArmyStat::HUNTERS.cost +
                                   enemy->getSupplyCost(SupplyBuildingType::Village) + enemy->getSupplyCost(SupplyBuildingType::Mine));
            level_->buildSupplyOfType(*enemy, SupplyBuildingType::Village);
            level_->refreshIncome(enemy);

            for (const auto& node : level_->keyNodesOf(enemy->description.get())) {
                if (node->getType() == KeyNodeType::MagicTower) {
                    level_->buyUnit(&ArmyStat::WARRIORS, *node);
                } else {
                    level_->buyUnit(&ArmyStat::WARRIORS, *node);
                    level_->buyUnit(&ArmyStat::WARRIORS, *node);
                    level_->buyUnit(&ArmyStat::HUNTERS, *node);
                    node->aiDefendersWanted = 3;
                }
            }

            level_->aiDisabled = true;
        }
    }

    void GameBattle::stopTutorials() {
        savedSpells_.clear();
        tutor1EnemySquad_ = nullptr;
        tutor1EnemyTown_ = nullptr;
        WindowContentManager::LOCK_UNIT_ABOVE = 999;
        WindowContentManager::UNLOCK_OBELISK = true;
        WindowContentManager::UNLOCK_MINE = true;
        WindowContentManager::UNLOCK_VILAGE = true;

        if (level_)
            level_->aiDisabled = false;

        signalTutorialEvent.dispose();
    }

    void GameBattle::enableSpells(bool enable) {
        if (enable)
            level_->player().setSpellsForDebug(savedSpells_);
        else
            level_->player().setSpellsForDebug({nullptr, nullptr, nullptr});

        panel_->setUltimateVisible(false);
        panel_->layoutSpellButtons();
    }

    bool GameBattle::isOwnCastle(NodeContent* node) const {
        auto* keyNode = dynamic_cast<FKeyNodeContent*>(node);
        if (keyNode == nullptr || keyNode->getType() != KeyNodeType::Castle)
            return false;

        return keyNode->owner->compare(*level_->player().description);
    }

    void GameBattle::onWindowOpened_Tutorial1(WindowInfo* info) {
        if (isOwnCastle(info->node))
            signalTutorialEvent.dispatch(EVENT_LVL1_CASTLE_CLICKED);
    }

    void GameBattle::onWindowClosed_Tutorial1(WindowInfo* info) {
        if (isOwnCastle(info->node))
            signalTutorialEvent.dispatch(EVENT_LVL1_CASTLE_MENU_CLOSES);
    }

    void GameBattle::onBuySquad_Tutorial1(FSquad* squad) {
        if (squad->getRace() != Race::Westaria)
            return;

        signalTutorialEvent.dispatch(EVENT_LVL1_GUARDS_PURCHASING_STARTED);
        signalTutorialEvent.dispatch(EVENT_LVL1_GUARDS_PURCHASING_FINISHED);
        level_->signalSquadBought.removeListener(this, &GameBattle::onBuySquad_Tutorial1);
        level_->setStatsDirty(true);

        // The enemy squad comes out to fight.
        tutor1EnemySquad_->commandMoveHome(Point2D(340, 290 * LevelVisual::Y_KOEFF));
    }

    void GameBattle::onKillSquad_Tutorial1(Squad* squad) {
        if (squad != tutor1EnemySquad_)
            return;

        tutor1EnemySquad_ = nullptr;
        signalTutorialEvent.dispatch(EVENT_LVL1_ENEMY_SQUAD_DESTROYED);
        level_->signalSquadKilled.removeListener(this, &GameBattle::onKillSquad_Tutorial1);
        WindowContentManager::LOCK_UNIT_ABOVE = 1;
        Combatant* enemy = level_->getCombatantByRace(Race::TheEmpire);
        enemy->addGold(ArmyStat::UNDYING_WARRIORS.cost);
        level_->buyUnit(&ArmyStat::UNDYING_WARRIORS, *tutor1EnemyTown_);
        level_->aiDisabled = false;
    }

    void GameBattle::onKeyNodeCaptured_Tutorial1(KeyNode* node) {
        if (tutor1EnemyTown_ != node)
            return;

        tutor1EnemyTown_ = nullptr;
        signalTutorialEvent.dispatch(EVENT_LVL1_TOWN_SIEZED);
        level_->signalNodeCaptured.removeListener(this, &GameBattle::onKeyNodeCaptured_Tutorial1);
        Combatant* enemy = level_->getCombatantByRace(Race::TheEmpire);
        enemy->addGold(ArmyStat::UNDYING_WARRIORS.cost);
        level_->reinitNodes(enemy);
        level_->refreshIncome(enemy);
        level_->setStatsDirty(true);
    }

    void GameBattle::onSupplyBuilt_Tutorial2(Combatant* owner, SupplyBuildingType type) {
        if (owner->description->compare(*level_->player().description) && type == SupplyBuildingType::Village) {
            level_->signalSupplyBuilt.removeListener(this, &GameBattle::onSupplyBuilt_Tutorial2);
            signalTutorialEvent.dispatch(EVENT_LVL2_FARM_CONSTRUCTION_STARTED);
            signalTutorialEvent.dispatch(EVENT_LVL2_FARM_CONSTRUCTION_FINISHED);
            WindowContentManager::UNLOCK_OBELISK = true;
            WindowContentManager::UNLOCK_MINE = true;
        }
    }

    void GameBattle::onBuySquad_Tutorial2(FSquad* squad) {
        if (squad->getRace() == Race::Westaria) {
            level_->signalSquadBought.removeListener(this, &GameBattle::onBuySquad_Tutorial2);
            signalTutorialEvent.dispatch(EVENT_LVL2_SQUAD_PURCHASING_STARTED);
        }
    }

    void GameBattle::onUnitAttack_Tutorial2(FUnit* unit) {
        if (unit->getArmyStat() == &ArmyStat::OGRE) {
            signalTutorialEvent.dispatch(EVENT_LVL2_FIGHT_WITH_OGRE_STARTED);
            level_->signalUnitAttack.removeListener(this, &GameBattle::onUnitAttack_Tutorial2);
            level_->signalHealBanner.addListener(this, &GameBattle::onHealBanner_Tutorial2);
            enableSpells(true);
        }
    }

    void GameBattle::onKillSquad_Tutorial2(Squad* squad) {
        if (squad->getArmyStat() == &ArmyStat::OGRE) {
            level_->signalSquadKilled.removeListener(this, &GameBattle::onKillSquad_Tutorial2);
            level_->signalHealBanner.removeListener(this, &GameBattle::onHealBanner_Tutorial2);
            signalTutorialEvent.dispatch(EVENT_LVL2_FIGHT_WITH_OGRE_FINISHED);
            level_->aiDisabled = false;
            level_->getCombatantByRace(Race::TheKhaganate)->setGoldForDebug(0);
        } else if (level_->player().getArmyPower() == squad->getArmyStat()->upkeep) {
            // The player's last squad died: back to buying one.
            signalTutorialEvent.dispatch(EVENT_LVL2_SQUAD_PURCHASING_STARTED);
            level_->signalUnitAttack.addListener(this, &GameBattle::onUnitAttack_Tutorial2);
            level_->signalHealBanner.removeListener(this, &GameBattle::onHealBanner_Tutorial2);
        }
    }

    void GameBattle::onHealBanner_Tutorial2() {
        level_->signalSquadKilled.removeListener(this, &GameBattle::onKillSquad_Tutorial2);
        level_->signalHealBanner.removeListener(this, &GameBattle::onHealBanner_Tutorial2);
        level_->aiDisabled = false;
        signalTutorialEvent.dispatch(EVENT_LVL2_BANNER_SET);
    }

    void GameBattle::onKeyNodeCaptured_Tutorial2(KeyNode* node) {
        Combatant* owner = level_->getCombatants().getObject(node->owner.get());
        if (owner == nullptr || owner->getRace() != Race::Westaria)
            return;

        switch (node->getType()) {
        case KeyNodeType::MagicTower:
            signalTutorialEvent.dispatch(EVENT_LVL2_MAGIC_TOWER_SIEZED);
            level_->player().forceEnableUltimate();
            level_->player().forceMaxUltimateTime();
            panel_->setUltimateVisible(true);
            panel_->layoutSpellButtons();
            level_->signalNodeChecked.addListener(this, &GameBattle::onTowerChecked_Tutorial2);
            level_->setStatsDirty(true);
            break;
        case KeyNodeType::Castle:
            signalTutorialEvent.dispatch(EVENT_LVL2_CASTLE_SIEZED);
            break;
        default:
            return;
        }

        level_->signalNodeCaptured.removeListener(this, &GameBattle::onKeyNodeCaptured_Tutorial2);
    }

    void GameBattle::onTowerChecked_Tutorial2(KeyNode* node) {
        if (node->getType() != KeyNodeType::MagicTower)
            return;

        Combatant* owner = level_->getCombatants().getObject(node->owner.get());
        if (owner->description == level_->player().description && node->isActive()) {
            level_->player().forceEnableUltimate(0);
            level_->signalNodeChecked.removeListener(this, &GameBattle::onTowerChecked_Tutorial2);
        }
    }

    void GameBattle::onUltimateCast_Tutorial2(const SpellInfo* spell) {
        if (spell == &SpellInfo::SIGN_OF_THE_ADVENT) {
            level_->signalNodeChecked.removeListener(this, &GameBattle::onTowerChecked_Tutorial2);
            level_->signalUltimateCast.removeListener(this, &GameBattle::onUltimateCast_Tutorial2);
            signalTutorialEvent.dispatch(EVENT_LVL2_ULTIMATE_CAST);
        }
    }

} // namespace takeover
