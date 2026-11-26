#include "game/level/LevelVisual.h"

#include "game/framework/Symbols.h"
#include "game/framework/input/MouseDevice.h"
#include "game/framework/ui/HintWindow.h"
#include "game/media/SoundConst.h"
#include "game/resources/TOResManager.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/sim/spells/SpellInfo.h"

#include "engine/display/MovieClip.h"
#include "engine/display/Shape.h"

#include <cmath>

namespace takeover {

// --- Static helpers ------------------------------------------------------------------------------------

Point2D LevelVisual::getPosition(const engine::DisplayObject& marker) {
    const engine::Point p = marker.localToGlobal({0, 0});
    return {p.x, p.y * Y_KOEFF};
}

std::vector<Point2D> LevelVisual::resamplePath(const std::vector<Point2D>& points, double distance) {
    std::vector<Point2D> result;
    Point2D p = points[0];
    Point2D current = p;
    double d = 0, carried = 0;
    result.push_back(p);
    for (std::size_t i = 1; i < points.size(); ++i) {
        const Point2D next = points[i];
        d = current.getDistanceTo(next);
        while (d >= distance - carried) {
            // flash.geom.Point.interpolate(current, next, f) = next + (current - next) * f
            const double f = (d - distance + carried) / d;
            current = Point2D(next.x + (current.x - next.x) * f, next.y + (current.y - next.y) * f);
            d = current.getDistanceTo(next);
            result.push_back(current);
            carried = 0;
        }
        current = next;
        carried = d + carried;
    }
    if (d > distance / 2) result.push_back(current);
    return result;
}

Point2D LevelVisual::convertToLandPoint(const Point2D& position) { return {position.x, position.y * Y_KOEFF}; }
Point2D LevelVisual::convertToScreenPoint(const Point2D& position) { return {position.x, position.y / Y_KOEFF}; }

int LevelVisual::raceIconIndex(Race race) {
    switch (race) {
    case Race::Westaria: return 1;
    case Race::TheKhaganate: return 2;
    case Race::TheCult: return 3;
    default: return 0;
    }
}

int LevelVisual::getRaceID(Race race) {
    switch (race) {
    case Race::Westaria: return 0;
    case Race::TheKhaganate: return 1;
    case Race::TheCult: return 2;
    case Race::TheEmpire: return 3;
    default: return 4;
    }
}

// --- Setup ---------------------------------------------------------------------------------------------

LevelVisual& LevelVisual::initVisual(const BattleDef& def) {
    def_ = def;
    addPlayers();
    initVisualBlock(def_.hintLayer);
    afterInit();
    return *this;
}

void LevelVisual::customDispose() {
    LevelAi::customDispose();
    disposeVisualBlock();
}

void LevelVisual::update(double dt) {
    LevelAi::update(dt);
    renderLayer(dt);
    // The screen shakes (Cataclysm) calm down.
    if (view_->x() != 0) view_->setX(-std::round(view_->x() / 2));
    if (view_->y() != 0) view_->setY(-std::round(view_->y() / 2));
}

void LevelVisual::initVisualBlock(HintLayer* hintLayer) {
    land_ = std::make_shared<LevelLand>();
    rasterLayer_ = std::make_shared<RasterGraphicLayer>(800, 600);
    view_ = std::make_shared<engine::Sprite>();
    debugSprite_ = std::make_shared<engine::Sprite>();
    iconLayer_ = std::make_shared<engine::Sprite>();
    cursorLayer_ = std::make_shared<engine::Sprite>();
    frontLayer_ = std::make_shared<engine::Sprite>();
    topLayer_ = std::make_shared<engine::Sprite>();
    hitMask_ = std::make_shared<engine::BitmapData>(800, 600);
    hitMask_->draw(*symbols::create("Level" + std::to_string(def_.levelUID) + "HitMaskClass"));
    hintLayer_ = hintLayer;

    auto addHint = [&](const char* className, DockSide side, DockAlign align, bool autoWidth) {
        auto hint = std::make_shared<HintWindow2>(symbols::create(className));
        hint->dockSide = side;
        hint->dockAlign = align;
        if (!autoWidth) hint->setAutoWidth(false);
        return hintLayer_->addHintWindow(hint);
    };
    hintBottom_ = addHint("Hint2", DockSide::Bottom, DockAlign::LeftTop, true);
    hintOverPanel_ = addHint("HintOverPanel", DockSide::Top, DockAlign::RightBottom, false);
    hintUnit_ = addHint("HintUnit", DockSide::Bottom, DockAlign::LeftTop, false);
    hintUnit2_ = addHint("HintUnit2Class", DockSide::Top, DockAlign::RightBottom, false);
    hintSpell_ = addHint("HintSpell", DockSide::Bottom, DockAlign::Center, false);
}

void LevelVisual::disposeVisualBlock() {
    for (auto* sprite : {&topLayer_, &frontLayer_, &debugSprite_, &iconLayer_})
        if (*sprite && (*sprite)->parent() != nullptr) (*sprite)->parent()->removeChild(sprite->get());
    if (land_) land_->dispose();
    if (rasterLayer_) rasterLayer_->dispose();
    hintLayer_ = nullptr;
}

void LevelVisual::renderLayer(double dt) {
    rasterLayer_->render();
    afterRender(dt);
}

bool LevelVisual::isHitColor(int x, int y, std::uint32_t color) const { return hitMask_->getPixel(x, y) == color; }
std::uint32_t LevelVisual::hitColorAt(int x, int y) const { return hitMask_->getPixel(x, y); }

void LevelVisual::drawWay(const std::vector<Point2D>& wayPoints) {
    engine::Graphics& g = graphics();
    g.moveTo(wayPoints[0].x, wayPoints[0].y / Y_KOEFF);
    for (std::size_t i = 1; i < wayPoints.size(); ++i) g.lineTo(wayPoints[i].x, wayPoints[i].y / Y_KOEFF);
    for (const Point2D& p : wayPoints) g.drawCircle(p.x, p.y / Y_KOEFF, 3);
}

void LevelVisual::drawGraph() {
    engine::Graphics& g = graphics();
    g.lineStyle(2, 0, 1);
    for (const auto& [index, road] : roads_.objects()) drawWay(road->wayPoints);
    g.lineStyle(1, 0xFF0000, 1);
    for (const auto& [index, road] : roads_.objects())
        for (std::size_t i = 1; i + 1 < road->wayPoints.size(); ++i)
            g.drawCircle(road->wayPoints[i].x, road->wayPoints[i].y / Y_KOEFF, 3);
    g.lineStyle(1, 0, 1);
    g.beginFill(0x00FF00);
    for (const auto& [index, node] : graphNodes_.objects()) g.drawRect(node->pos.x - 5, node->pos.y / Y_KOEFF - 5, 10, 10);
    g.endFill();
}

void LevelVisual::assembleView() {
    view_->addChild(land_->groundBitmap());
    view_->addChild(debugSprite_);
    view_->addChild(std::make_shared<engine::Bitmap>(rasterLayer_->bitmap()));
    view_->addChild(iconLayer_);
    if (isModeOn(ST_SHOW_HIT_LAYER)) {
        auto mask = std::make_shared<engine::Bitmap>(hitMask_);
        mask->setAlpha(0.2);
        view_->addChild(mask);
    }
    view_->addChild(frontLayer_);
}

void LevelVisual::setLocationView(int location, int locationType) {
    land_->setLocationView(location, locationType);
    land_->redraw();
    for (const auto& [index, node] : keyNodes_.objects())
        if (graphNodes_.getObject(node->nodeDescription.get())->locationNum == location)
            static_cast<FKeyNodeContent&>(*node).setLocationType(locationType);
    for (const auto& [index, place] : buildPlaces_.objects())
        if (graphNodes_.getObject(place->nodeDescription.get())->locationNum == location)
            static_cast<BuildPlace&>(*place).setLocationType(locationType);
}

// --- Players ---------------------------------------------------------------------------------------------

void LevelVisual::addPlayers() {
    playerKey_ = def_.player->description;
    addCombatant(def_.player);
    for (const auto& enemy : def_.enemyPlayers) addCombatant(enemy);
    setPlayer(def_.player->description);
    playerRaceIndex_ = raceIconIndex(player().getRace());
    statsDirty_ = true;
}

void LevelVisual::setPlayer(const std::shared_ptr<CombatantKey>& key) { playerKey_ = key; }

bool LevelVisual::isPlayerRace(Race race) const {
    if (mode.test(ST_LOCK_AI)) return true;
    return race == def_.player->getRace();
}

bool LevelVisual::isPlayer(const CombatantKey* key) const {
    if (mode.test(ST_LOCK_AI)) return true;
    return key != nullptr && def_.player->description->compare(*key);
}

Point2D LevelVisual::mouseLandPosition() const { return {def_.mouse->x(), def_.mouse->y() * Y_KOEFF}; }

bool LevelVisual::updateBaseCombotantLogic(double dt) {
    const bool result = LevelAi::updateBaseCombotantLogic(dt);
    statsDirty_ = result || statsDirty_;
    return result;
}

bool LevelVisual::updateCpuCombotantLogic(double dt) {
    if (mode.test(ST_LOCK_AI)) return false;
    return LevelAi::updateCpuCombotantLogic(dt);
}

// --- Squads ----------------------------------------------------------------------------------------------

std::shared_ptr<FSquad> LevelVisual::makeSquad(const std::shared_ptr<CombatantKey>& owner, const ArmyStat& stat,
                                               const ArmyStatBonus& bonus) {
    FSquadDef def;
    def.resources = def_.resources;
    def.levelVisual = this;
    def.armyStat = &stat;
    def.armyStatBonus = bonus;
    def.owner = owner;
    def.level = this;
    auto squad = std::make_shared<FSquad>();
    squad->init(def);
    squad->signalIcon.addListener(this, &LevelVisual::onSquadIconEvent);
    return squad;
}

std::shared_ptr<FSquad> LevelVisual::buySquadFor(const ArmyStat& stat, Combatant& combatant) {
    std::shared_ptr<FSquad> squad = makeSquad(combatant.description, stat, combatant.getArmyStatBonus(stat));
    if (buySquad(squad, combatant)) {
        signalSquadBought.dispatch(squad.get());
        return squad;
    }
    squad->dispose();
    return nullptr;
}

std::shared_ptr<FSquad> LevelVisual::createSquadFor(const std::shared_ptr<CombatantKey>& owner, const ArmyStat& stat,
                                                    const ArmyStatBonus& bonus) {
    std::shared_ptr<FSquad> squad = makeSquad(owner, stat, bonus);
    addSquad(squad);
    return squad;
}

std::shared_ptr<Squad> LevelVisual::buyUnit(const ArmyStat* stat, KeyNode& node) {
    Combatant* combatant = getCombatantByRace(stat->side);
    if (combatant == nullptr) return nullptr;
    std::shared_ptr<FSquad> squad = buySquadFor(*stat, *combatant);
    if (squad != nullptr) {
        listenSquad(*squad);
        squad->enterNode(node);
        squad->addXP(SquadBase::xpForLevel(node.getLevel()));
        statsDirty_ = true;
    }
    return squad;
}

void LevelVisual::listenSquad(FSquad& squad) {
    squad.signalLeftNode.addListener(this, &LevelVisual::onSquadLeftNode);
    squad.signalAtPosition.addListener(this, &LevelVisual::onSquadAtPosition);
}

void LevelVisual::onSquadLeftNode(FSquad* squad) {
    if (squad->icon() != nullptr) iconLayer_->addChild(squad->iconPtr());
}

void LevelVisual::killSquad(Squad& squad) {
    signalSquadKilled.dispatch(&squad);
    LevelAi::killSquad(squad);
}

void LevelVisual::disposeSignals() {
    signalSquadBought.dispose();
    signalSquadKilled.dispose();
    signalUnitAttack.dispose();
    signalNodeCaptured.dispose();
    signalSupplyBuilt.dispose();
    signalUltimateCast.dispose();
    signalHealBanner.dispose();
    signalNodeChecked.dispose();
}

// --- Effects ----------------------------------------------------------------------------------------------

void LevelVisual::addEffect(const std::shared_ptr<TimedEffect>& effect, int layer) {
    rasterLayer_->addRGLObject(effect->layerObject(), layer);
    levelObjects_.addObject(effect, effect->getKey());
}

void LevelVisual::playEffect(const std::string& key, double x, double y, int layer, int repeatCounter, double delay) {
    (void)delay;
    AnimEffectDef def;
    def.anim = std::make_shared<RasterAnimation>(def_.resources->getCachedData(key), def_.resources->animTransitions(key)[0],
                                                 x, y, 0, repeatCounter);
    def.time = def.anim->transition().mscDuration / 1000 * (repeatCounter > 0 ? repeatCounter : 1);
    addEffect(std::make_shared<AnimEffect>(def), layer);
}

void LevelVisual::showRizeEffect(const Point2D& position) {
    playEffect("BuildingRizeEffectClass", position.x, position.y, RasterGraphicLayer::LAYER_UNIT);
}

// --- Overrides of the rules ---------------------------------------------------------------------------------

void LevelVisual::captureNode(const std::shared_ptr<CombatantKey>& newOwner, KeyNode& node) {
    LevelAi::captureNode(newOwner, node);
    Combatant* owner = combatants_.getObject(newOwner.get());
    GraphNode* crossing = graphNodes_.getObject(node.nodeDescription.get());
    setLocationView(crossing->locationNum, Combatant::raceTableIndex(owner->getRace()));
    signalNodeCaptured.dispatch(&node);
}

void LevelVisual::buildSupplyBuilding(NodeContent& place, SupplyBuildingType type) {
    Combatant* owner = getLocationOwner(*place.nodeDescription);
    owner->spendGold(owner->getSupplyCost(type));
    SoundConst::playBuild(type, owner->getRace());
    signalSupplyBuilt.dispatch(owner, type);
    LevelAi::buildSupplyBuilding(place, type);
}

bool LevelVisual::tryBuySupply(NodeContent& place, Combatant&, SupplyBuildingType type) {
    if (auto* buildPlace = dynamic_cast<BuildPlace*>(&place)) {
        if (buildPlace->getType() == SupplyBuildingType::Empty) {
            buildSupplyBuilding(*buildPlace, type);
            return true;
        }
    }
    return false;
}

void LevelVisual::resetBuildPlace(NodeContent& place) {
    if (auto* buildPlace = dynamic_cast<BuildPlace*>(&place)) {
        if (buildPlace->getType() != SupplyBuildingType::Empty) {
            buildSupplyBuilding(*buildPlace, SupplyBuildingType::Empty);
            showRizeEffect(convertToLandPoint(buildPlace->anchor()));
        }
    }
}

bool LevelVisual::checkNodeActivState(KeyNode& node) {
    const bool changed = LevelAi::checkNodeActivState(node);
    if (changed && node.isRuined()) {
        Point2D position = node.getPosition();
        position.y += 3;
        showRizeEffect(position);
    }
    signalNodeChecked.dispatch(&node);
    return changed;
}

SupplyBuildingType LevelVisual::getBuildPlaceType(NodeContent& place) {
    if (auto* buildPlace = dynamic_cast<BuildPlace*>(&place)) return buildPlace->getType();
    return LevelAi::getBuildPlaceType(place);
}

// --- Caching --------------------------------------------------------------------------------------------------

void LevelVisual::precache(int levelNum) {
    forEachObject(combatants_, [&](const std::shared_ptr<Combatant>& combatant) {
        // Creating a squad renders its animations; the squad is dropped.
        for (int i = 0; i < 5; ++i) {
            const ArmyStat* stat = combatant->getArmyStat(unit_types::fromInt(i));
            makeSquad(combatant->description, *stat, combatant->getArmyStatBonus(*stat))->dispose();
        }
        if (levelNum != 0 && combatant->getRace() == Race::Westaria)
            makeSquad(combatant->description, ArmyStat::AVATAR, combatant->getArmyStatBonus(ArmyStat::AVATAR))->dispose();
        for (const auto& spell : combatant->getSpells()) {
            if (spell == nullptr) continue;
            for (const std::string& key : spellEffectKeys(spell->id)) def_.resources->getCachedData(key);
        }
    });
}

std::vector<std::string> LevelVisual::spellEffectKeys(int spellId) {
    if (spellId == SpellInfo::LORDS_JUDGEMENT.id)
        return {"SpellThunderClass", "CursorOn_LordsJudgementClass", "CursorOff_LordsJudgementClass"};
    if (spellId == SpellInfo::BANNER_OF_CONSECRATION.id)
        return {"BannerAuraHealClass", "BannerHealClass", "CursorOn_HealClass", "CursorOff_HealClass"};
    if (spellId == SpellInfo::BANNER_OF_HEROISM.id)
        return {"BannerAuraHeroismClass", "BannerHeroismClass", "CursorOn_HeroismClass", "CursorOff_HeroismClass"};
    if (spellId == SpellInfo::BANNER_OF_DESECRATION.id) return {"BannerAuraDesecrationClass", "BannerDesecrationClass"};
    if (spellId == SpellInfo::RAISE_THE_DEAD.id) return {"RaiseDeadSpellEffectClass"};
    if (spellId == SpellInfo::TERROR.id)
        return {"TerrorSpellEffectClass", "TerrorFlyingBatEffectClass", "TerrorSplashEffectClass"};
    if (spellId == SpellInfo::SUMMON_STONEWORMS.id)
        return {"StoneWormsEffectClass", "StonewormsExplosionEffect", "CursorOn_StoneWormClass", "CursorOff_StoneWormClass"};
    if (spellId == SpellInfo::DISENTOMB.id)
        return {"RaiseDeadSpellEffectClass", "CursorOn_DisentombClass", "CursorOff_DisentombClass"};
    if (spellId == SpellInfo::BANNER_OF_BLOODLUST.id)
        return {"BannerEffectBloodlustClass", "BannerBloodlustClass", "CursorOn_BloodlustClass", "CursorOff_BloodlustClass"};
    if (spellId == SpellInfo::ICE_BLAST.id) return {"IceBlastEffectClass", "IceBlastExplosionClass", "CursorOff_IceBlastClass"};
    if (spellId == SpellInfo::FROST_ENCHANTMENT.id)
        return {"FrostEncahntmentEffectClass", "CursorOn_MistyClonesClass", "CursorOff_MistyClonesClass"};
    if (spellId == SpellInfo::BANNER_OF_LIBERATION.id)
        return {"BannerLiberationClass", "BannerLiberationEffectClass", "CursorOn_LiberationClass", "CursorOff_LiberationClass"};
    return {};
}

} // namespace takeover
