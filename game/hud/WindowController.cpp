#include "game/hud/WindowController.h"

#include "game/framework/Symbols.h"
#include "game/framework/ui/HintWindow.h"
#include "game/hud/GamePanel.h"
#include "game/level/Level.h"
#include "game/media/SoundConst.h"
#include "game/sim/abilities/Abilities.h"
#include "game/sim/abilities/AbilityEffects.h"
#include "game/sim/abilities/UnitParams.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/sim/battle/Combatant.h"
#include "game/squads/FSquad.h"

#include "engine/audio/Sound.h"
#include "engine/display/MovieClip.h"
#include "engine/display/TextField.h"
#include "engine/events/Event.h"

#include <algorithm>
#include <cctype>
#include <optional>

namespace takeover {

namespace {

using Positions = std::vector<std::pair<std::string, double>>;
const Positions ENEMY_WINDOW_POSITION_DATA = {{"lbLeft", 300}, {"lbRight", 0}};
const Positions DESTROY_SUPPLY_WINDOW_POSITION_DATA = {{"lbLeft", 580}, {"lbRight", 0}};
const Positions CASTLE_WINDOW_POSITION_DATA = {{"lbLeftBorder", 215}, {"lbLeft", 370}, {"lbRight", 580}, {"lbRightBorder", 0}};
const Positions TOWN_WINDOW_POSITION_DATA = {{"lbLeftBorder", 215}, {"lbLeft", 320}, {"lbRight", 580}, {"lbRightBorder", 0}};

/// Where the bottom block of the unit hint was authored (original: _savedHintUnitBottomBlockPos).
std::optional<engine::Point> savedBottomBlockPos;

std::string upper(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return text;
}

} // namespace

// --- WindowInfo -----------------------------------------------------------------------------------------

void WindowInfo::dispose() {
    if (window) window->dispose();
    window.reset();
    node = nullptr;
    owner.reset();
}

bool WindowInfo::stateChanged() {
    auto* keyNode = dynamic_cast<FKeyNodeContent*>(node);
    if (keyNode == nullptr) return false;
    FsmState* state = keyNode->getFSMState();
    if (lastState_ != state) {
        lastState_ = state;
        return true;
    }
    level = keyNode->getLevel();
    return false;
}

// --- Static helpers ---------------------------------------------------------------------------------------

std::string WindowController::levelBonusText(int squadLevel) {
    int bonus = 0;
    if (squadLevel == 1) bonus = 10;
    else if (squadLevel >= 2) bonus = 20;
    if (bonus > 0) return "HP and DMG are increased by " + std::to_string(bonus) + "%";
    return "Have no any rank bonuses";
}

void WindowController::fillUnitHint(const ArmyStat& stat, const ArmyStatBonus& bonus, HintWindow2& hint, int squadLevel) {
    engine::DisplayObjectContainer& image = *hint.image();
    if (!savedBottomBlockPos) {
        engine::DisplayObject* bottom = image.getChildByName("mcBottomBlock");
        savedBottomBlockPos = engine::Point{bottom->x(), bottom->y()};
    }
    const int unitsCount = stat.unitsCount + bonus.addUnit;
    const int hp = UnitParams::levelHP(stat, bonus, squadLevel);
    const int minDamage = UnitParams::levelDamageMin(stat, bonus, squadLevel);
    const int maxDamage = UnitParams::levelDamageMax(stat, bonus, squadLevel);
    const int morale = bonus.getBaseMorale(stat.morale);
    const int speed = bonus.getSpeed(stat.speed);
    const std::vector<std::string> abilities = Abilities::splitAbilities(stat.abilities);

    // Room for the stat lines drawn in the window.
    std::string text;
    for (int n = 0, lines = stat.range == 0 ? 9 : 12; n < lines; ++n) text += "\n";
    symbols::setHtmlText(image, "_-y3", upper(stat.name));
    symbols::setHtmlText(image, "textSupplyPts", "-" + std::to_string(stat.upkeep));
    symbols::setHtmlText(image, "textUnitsNum", std::to_string(unitsCount) + " UNITS");
    if (auto* number = symbols::child(image, "mcUnitNumber")) number->gotoAndStop(unitsCount);
    symbols::setHtmlText(image, "textHp", levelColored(squadLevel, std::to_string(hp)));
    symbols::setHtmlText(image, "textTotalHp", std::to_string(hp * unitsCount));
    std::string damage = levelColored(squadLevel, std::to_string(minDamage) + " - " + std::to_string(maxDamage));
    symbols::setHtmlText(image, "textTotalDamage",
                         std::to_string(minDamage * unitsCount) + " - " + std::to_string(maxDamage * unitsCount));
    const std::string attackSpeed = FSquad::attackSpeedText(stat.attackSpeed);
    if (!attackSpeed.empty()) {
        // Like the original, the colored text is read back from the field.
        auto* field = symbols::child<engine::TextField>(image, "textDamage");
        field->setHtmlText(damage);
        damage = field->htmlText() + "  (" + attackSpeed + ")";
    }
    symbols::setHtmlText(image, "textDamage", damage);
    auto* rangeBlock = symbols::child<engine::DisplayObjectContainer>(image, "mcRangedBlock");
    auto* bottomBlock = symbols::child<engine::DisplayObjectContainer>(image, "mcBottomBlock");
    if (stat.range == 0) {
        rangeBlock->setVisible(false);
        bottomBlock->setY(rangeBlock->y());
    } else {
        rangeBlock->setVisible(true);
        bottomBlock->setY(savedBottomBlockPos->y);
        symbols::setHtmlText(*rangeBlock, "textRange", FSquad::rangeText(stat.range));
        symbols::setHtmlText(*rangeBlock, "textMeleePenalty", "- " + std::to_string(stat.meleePenalty) + "%");
    }
    symbols::setHtmlText(*bottomBlock, "textSpeed", FSquad::speedText(speed));
    if (squadLevel > 0)
        text += "<br>" + levelColored(squadLevel, upper(levelName(squadLevel))) + " ( <FONT COLOR='#999999'>" +
                levelBonusText(squadLevel) + "</FONT>)";
    bool moraleNone = false;
    if (!abilities.empty() && !abilities[0].empty()) {
        for (const std::string& ability : abilities) {
            const std::vector<std::string> parts = Abilities::splitAbility(ability);
            const std::string param = parts.size() > 1 ? parts[1] : "";
            const std::string name = AbilityEffect::displayName(parts[0], param, bonus);
            const std::string description = AbilityEffect::description(parts[0], param, bonus);
            text += "<br>" + name + " ( <FONT COLOR='#999999'>" + description + "</FONT>)";
            if (name == AbilityEffect::NAME_MECHANISM || name == AbilityEffect::NAME_UNDEAD) moraleNone = true;
        }
    }
    symbols::setHtmlText(*bottomBlock, "textMorale", FSquad::moraleText(morale, moraleNone));
    hint.setText(text);
}

// --- Controller ----------------------------------------------------------------------------------------------

WindowController::WindowController(Level& level) : level_(&level), image_(std::make_shared<engine::Sprite>()) {
    enabled_ = false;
    pendingAction_ = -1;
}

void WindowController::customDispose() {
    stopSound();
    mouseOverWindow_ = true;
    if (current_) current_->dispose();
    current_.reset();
    if (image_ && image_->parent() != nullptr) image_->parent()->removeChild(image_.get());
    level_ = nullptr;
    signalWindowOpened.dispose();
    signalWindowClosed.dispose();
    UpdatableObject::customDispose();
}

void WindowController::update(double timeElapsed) {
    if (current_) {
        if (level_->isStatsDirty() && level_->player().description->compare(*current_->owner)) current_->window->refresh();
        if (current_ && current_->stateChanged())
            openWindow(current_->node, level_->getCombatants().getObject(current_->owner.get()));
        if (level_->isDraggingOrder() || (enabled_ && level_->isFraming())) closeWindow();
    }
    UpdatableObject::update(timeElapsed);
}

void WindowController::stopSound() {
    if (sound_) sound_->stop();
    sound_.reset();
}

void WindowController::listenWindow(WindowBase* window, bool enable) {
    if (window == nullptr) return;
    engine::MovieClip* sprite = window->image().get();
    // (The original adds the listeners again instead of removing them.)
    sprite->addEventListener(engine::MouseEvent::ROLL_OUT, this, &WindowController::onWindowRollOut);
    sprite->addEventListener(engine::MouseEvent::ROLL_OVER, this, &WindowController::onWindowRollOver);
    if (enable) window->signalAction.addListener([this](int action) { pendingAction_ = action; });
    else window->signalAction.removeAllListeners();
}

void WindowController::onWindowRollOut(engine::MouseEvent&) { mouseOverWindow_ = false; }
void WindowController::onWindowRollOver(engine::MouseEvent&) { mouseOverWindow_ = true; }

void WindowController::show(std::unique_ptr<WindowInfo> info) {
    closeWindow();
    current_ = std::move(info);
    listenWindow(current_->window.get(), true);
    signalWindowOpened.dispatch(current_.get());
    enabled_ = true;
    image_->addChild(current_->window->image());
    if (auto* keyNode = dynamic_cast<FKeyNodeContent*>(current_->node)) keyNode->addViewsTo(*image_);
    stopSound();
    sound_ = SoundConst::play(current_->openSound);
}

void WindowController::closeWindow() {
    enabled_ = false;
    if (current_) {
        signalWindowClosed.dispatch(current_.get());
        SoundConst::play(SoundId::WINDOW_CLOSE);
        listenWindow(current_->window.get(), false);
        current_->dispose();
        current_.reset();
        level_->hintLayer().hideHint();
    }
    stopSound();
}

int WindowController::frameColorOf(const NodeKey& node) {
    switch (level_->getLocationOwner(node)->getRace()) {
    case Race::Westaria: return 1;
    case Race::TheKhaganate: return 2;
    case Race::TheCult: return 3;
    case Race::TheEmpire: return 4;
    default: return 1;
    }
}

void WindowController::openWindow(NodeContent* node, Combatant* player) {
    (void)player;
    if (node == nullptr || (current_ && current_->node->description == node->description)) {
        closeWindow();
        return;
    }
    Combatant* owner = level_->getLocationOwner(*node->nodeDescription);
    if (owner == nullptr) {
        closeWindow();
        return;
    }
    std::unique_ptr<WindowInfo> info;
    if (auto* place = dynamic_cast<BuildPlace*>(node)) {
        info = buildPlaceWindow(*place, *owner);
    } else if (auto* keyNode = dynamic_cast<FKeyNodeContent*>(node)) {
        info = keyNodeWindow(*keyNode, *owner);
    } else {
        return;
    }
    info->window->refresh();
    show(std::move(info));
}

CenterPanelDef WindowController::centerOf(NodeContent& node, bool active, int frame, const Point2D& position) {
    return {active, node.name, frame, node.goldIncome, node.supplyIncome, node.manaIncome, statsOnRight(position.x)};
}

std::unique_ptr<WindowInfo> WindowController::keyNodeWindow(FKeyNodeContent& node, Combatant& owner) {
    auto info = std::make_unique<WindowInfo>(&node);
    const Point2D position = node.anchor();
    if (!level_->isPlayer(owner.description.get())) {
        const std::string text = "It belongs to<br>" + Combatant::raceName(owner.getRace()) + ".<br>We must capture it!";
        info->window = enemyWindow(node, position, text);
    } else {
        switch (node.getType()) {
        case KeyNodeType::MagicTower: info->window = towerWindow(node, position); break;
        case KeyNodeType::Castle: info->window = castleWindow(node, position); break;
        case KeyNodeType::Town:
        case KeyNodeType::Camp: info->window = townWindow(node, position); break;
        }
    }
    info->owner = node.owner;
    return info;
}

std::unique_ptr<WindowInfo> WindowController::buildPlaceWindow(BuildPlace& place, Combatant& owner) {
    auto info = std::make_unique<WindowInfo>(&place);
    const Point2D position = place.anchor();
    if (!level_->isPlayer(owner.description.get())) {
        const std::string raceName = Combatant::raceName(owner.getRace());
        const std::string text = place.getType() == SupplyBuildingType::Empty
                                     ? "It belongs to<br>" + raceName + ".<br>You can`t build here."
                                     : "It belongs to<br>" + raceName + ".";
        info->window = enemyWindow(place, position, text);
    } else if (place.getType() == SupplyBuildingType::Empty) {
        info->window = buildWindow(place, position);
    } else {
        info->window = destroyWindow(place, position);
    }
    info->owner = owner.description;
    return info;
}

void WindowController::build(SupplyBuildingType type) {
    if (auto* place = dynamic_cast<BuildPlace*>(current_->node)) {
        level_->buildSupplyBuilding(*place, type);
        closeWindow();
    }
}

void WindowController::buyUnit(int unitId) {
    auto* keyNode = dynamic_cast<FKeyNodeContent*>(current_->node);
    Combatant* combatant = level_->getLocationOwner(*keyNode->nodeDescription);
    level_->buyUnit(combatant->getArmyStat(unit_types::fromInt(unitId)), *keyNode);
    closeWindow();
}

void WindowController::upgrade() {
    auto* keyNode = dynamic_cast<FKeyNodeContent*>(current_->node);
    Combatant* combatant = level_->getLocationOwner(*keyNode->nodeDescription);
    combatant->upgradeKeyNode(*keyNode);
    closeWindow();
}

bool WindowController::process(double, bool locked) {
    if (locked) {
        closeWindow();
        return locked;
    }
    if (!enabled_) return locked;
    if (pendingAction_ >= 0) {
        doAction();
        pendingAction_ = -1;
        return true;
    }
    return mouseOverWindow_;
}

void WindowController::doAction() {
    switch (pendingAction_) {
    case WindowBase::BUT_ID_UNIT_0:
    case WindowBase::BUT_ID_UNIT_1:
    case WindowBase::BUT_ID_UNIT_2:
    case WindowBase::BUT_ID_UNIT_3:
    case WindowBase::BUT_ID_UNIT_4: buyUnit(pendingAction_); break;
    case WindowBase::BUT_ID_UPGRADE: upgrade(); break;
    case WindowBase::BUT_ID_ULTIMATE_SPELL:
        level_->castUltimateSpell(level_->player());
        closeWindow();
        break;
    case WindowBase::BUT_ID_BUILD_FARM: build(SupplyBuildingType::Village); break;
    case WindowBase::BUT_ID_BUILD_MINE: build(SupplyBuildingType::Mine); break;
    case WindowBase::BUT_ID_BUILD_OBELISK: build(SupplyBuildingType::Obelisk); break;
    case WindowBase::BUT_ID_DESTROY: build(SupplyBuildingType::Empty); break;
    default: break;
    }
}

// --- The windows ------------------------------------------------------------------------------------------

std::shared_ptr<WindowBase> WindowController::enemyWindow(NodeContent& node, const Point2D& position, const std::string& text) {
    const int frame = frameColorOf(*node.nodeDescription);
    WindowDef def;
    def.center = centerOf(node, true, frame, position);
    def.position = position;
    def.panelColorFrame = frame;
    def.text = text;
    def.windowClass = "WindowEnemyPanelClass";
    def.positions = ENEMY_WINDOW_POSITION_DATA;
    auto window = std::make_shared<WindowBase>(std::move(def));
    listenButtons(*window);
    return window;
}

std::shared_ptr<WindowBase> WindowController::castleWindow(KeyNode& node, const Point2D& position) {
    const int frame = frameColorOf(*node.nodeDescription);
    WindowDef def;
    def.center = centerOf(node, node.isActive(), frame, position);
    def.position = position;
    def.panelColorFrame = frame;
    if (!node.isActive()) def.text = "Restore the stronghold to train new troops.";
    def.content = std::make_shared<WindowContentManager>(*level_, node);
    def.windowClass = "WindowCastleClass";
    def.positions = CASTLE_WINDOW_POSITION_DATA;
    auto window = std::make_shared<WindowBase>(std::move(def));
    for (int i = 0; i < 4; ++i) window->addUnitButton(i, WindowBase::BUT_ID_UNIT_0 + i);
    window->addUpgradeButton();
    listenButtons(*window);
    return window;
}

std::shared_ptr<WindowBase> WindowController::townWindow(KeyNode& node, const Point2D& position) {
    const int frame = frameColorOf(*node.nodeDescription);
    WindowDef def;
    def.center = centerOf(node, node.isActive(), frame, position);
    def.position = position;
    def.panelColorFrame = frame;
    if (!node.isActive()) def.text = "Restore the stronghold to train new troops.";
    def.content = std::make_shared<WindowContentManager>(*level_, node);
    def.windowClass = "WindowTownClass";
    def.positions = TOWN_WINDOW_POSITION_DATA;
    auto window = std::make_shared<WindowBase>(std::move(def));
    window->addUnitButton(0, WindowBase::BUT_ID_UNIT_0);
    window->addUnitButton(1, WindowBase::BUT_ID_UNIT_1);
    window->addUpgradeButton();
    listenButtons(*window);
    return window;
}

std::shared_ptr<WindowBase> WindowController::towerWindow(KeyNode& node, const Point2D& position) {
    const int frame = frameColorOf(*node.nodeDescription);
    WindowDef def;
    def.center = centerOf(node, node.isActive(), frame, position);
    def.position = position;
    def.panelColorFrame = frame;
    if (!node.isActive()) def.text = "Restore the stronghold to train new troops.";
    def.content = std::make_shared<WindowContentManager>(*level_, node);
    def.windowClass = "WindowMageTowerClass";
    def.positions = CASTLE_WINDOW_POSITION_DATA;
    auto window = std::make_shared<WindowBase>(std::move(def));
    window->addUnitButton(0, WindowBase::BUT_ID_UNIT_0);
    window->addUnitButton(1, WindowBase::BUT_ID_UNIT_4);
    window->addUpgradeButton();
    level_->setUltimateIcon(window->addUltimateButton());
    listenButtons(*window);
    return window;
}

std::shared_ptr<WindowBase> WindowController::buildWindow(NodeContent& node, const Point2D& position) {
    const int frame = frameColorOf(*node.nodeDescription);
    WindowDef def;
    def.center = centerOf(node, true, frame, position);
    def.position = position;
    def.panelColorFrame = frame;
    def.content = std::make_shared<WindowContentManager>(*level_, node);
    def.windowClass = "WindowBuildingSiteClass";
    def.positions = ENEMY_WINDOW_POSITION_DATA;
    auto window = std::make_shared<WindowBase>(std::move(def));
    window->addBuildButton(0, WindowBase::BUT_ID_BUILD_FARM);
    window->addBuildButton(1, WindowBase::BUT_ID_BUILD_MINE);
    window->addBuildButton(2, WindowBase::BUT_ID_BUILD_OBELISK);
    listenButtons(*window);
    return window;
}

std::shared_ptr<WindowBase> WindowController::destroyWindow(NodeContent& node, const Point2D& position) {
    const int frame = frameColorOf(*node.nodeDescription);
    WindowDef def;
    def.center = centerOf(node, true, frame, position);
    def.position = position;
    def.panelColorFrame = frame;
    def.content = std::make_shared<WindowContentManager>(*level_, node);
    def.windowClass = "WindowVillageClass";
    def.positions = DESTROY_SUPPLY_WINDOW_POSITION_DATA;
    auto window = std::make_shared<WindowBase>(std::move(def));
    window->addDestroyButton();
    listenButtons(*window);
    return window;
}

// --- Tooltips ------------------------------------------------------------------------------------------------

void WindowController::listenButtons(WindowBase& window) {
    for (const auto& button : window.buttons()) {
        button->signalRollOver.addListener(this, &WindowController::onButtonOver);
        button->signalRollOut.addListener(this, &WindowController::onButtonOut);
    }
    if (USpellButton* ultimate = window.ultimateButton()) {
        ultimate->signalRollOver.addListener(this, &WindowController::onButtonOver);
        ultimate->signalRollOut.addListener(this, &WindowController::onButtonOut);
    }
}

void WindowController::onButtonOver(Button* button) {
    const int id = button->id();
    Combatant* combatant = level_->getCombatants().getObject(current_->owner.get());
    auto* keyNode = dynamic_cast<FKeyNodeContent*>(current_->node);
    std::string text;
    switch (id) {
    case WindowBase::BUT_ID_UNIT_0:
    case WindowBase::BUT_ID_UNIT_1:
    case WindowBase::BUT_ID_UNIT_2:
    case WindowBase::BUT_ID_UNIT_3:
    case WindowBase::BUT_ID_UNIT_4:
        showUnitHint(*combatant, id, current_->level);
        showHintNear(level_->hintUnitId(), button->image());
        return;
    case WindowBase::BUT_ID_UPGRADE:
        if (keyNode == nullptr) return;
        text = upgradeText(keyNode->getLevel() + 1);
        break;
    case WindowBase::BUT_ID_BUILD_FARM: text = buildingText(*combatant, SupplyBuildingType::Village); break;
    case WindowBase::BUT_ID_BUILD_MINE: text = buildingText(*combatant, SupplyBuildingType::Mine); break;
    case WindowBase::BUT_ID_BUILD_OBELISK: text = buildingText(*combatant, SupplyBuildingType::Obelisk); break;
    case WindowBase::BUT_ID_ULTIMATE_SPELL:
        GamePanel::showSpellHint(3, *level_);
        showHintNear(level_->hintSpellId(), button->image());
        return;
    default: return;
    }
    level_->hintLayer().getHintWindow(level_->hintBottomId())->setText(text);
    showHintNear(level_->hintBottomId(), button->image());
}

void WindowController::showHintNear(int hintId, engine::DisplayObject* buttonImage) {
    const engine::Point p = buttonImage->localToGlobal({0, 0});
    level_->hintLayer().showHintAtRect({p.x - 23, p.y - 23, 46, 56}, hintId);
}

void WindowController::onButtonOut(Button*) { level_->hintLayer().hideHint(); }

std::string WindowController::upgradeText(int toLevel) {
    switch (toLevel) {
    case 1: return "<FONT SIZE='+1'>+50% stronghold's HP<br>The stronghold will train VETERAN units</FONT>";
    case 2: return "<FONT SIZE='+1'>+50% stronghold's HP<br>The stronghold will train ELITE units</FONT>";
    default: return "<FONT SIZE='+1' COLOR='#FF3300'>Reached MAX level of the stronghold</FONT>";
    }
}

std::string WindowController::buildingText(const Combatant& combatant, SupplyBuildingType type) {
    NodeContent node(nullptr, nullptr);
    node.initBySupplyBuilding(type, &combatant);
    std::string text = "<FONT COLOR='#999999'>" + upper(node.name) + ": </FONT>";
    std::string br;
    if (node.goldIncome > 0) {
        text += "+" + std::to_string(node.goldIncome) + " <FONT COLOR='#FFFF00'>GOLD</FONT> income";
        br = "<br>";
    }
    if (node.supplyIncome > 0) {
        text += br + "+" + std::to_string(node.supplyIncome) + " <FONT COLOR='#65FF00'>SUPPLY POINT</FONT>";
        br = "<br>";
    }
    if (node.manaIncome > 0) text += br + "+" + std::to_string(node.manaIncome) + " <FONT COLOR='#00CBFF'>MANA</FONT> generation";
    return text;
}

void WindowController::showUnitHint(Combatant& combatant, int unitId, int level) {
    const ArmyStat& stat = *combatant.getArmyStat(unit_types::fromInt(unitId));
    fillUnitHint(stat, combatant.getArmyStatBonus(stat), *level_->hintLayer().getHintWindow(level_->hintUnitId()), level);
}

} // namespace takeover
