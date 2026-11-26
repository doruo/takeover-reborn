#include "game/level/NodeViews.h"

#include "game/framework/Symbols.h"
#include "game/level/Level.h"
#include "game/media/SoundConst.h"
#include "game/resources/TOResManager.h"
#include "game/sim/battle/Combatant.h"

#include "engine/display/MovieClip.h"
#include "engine/display/TextField.h"

#include <algorithm>
#include <cmath>

namespace takeover {

namespace {

/// A building picture: library class, optional animation on top, and the
/// delay before the animation starts.
struct Look {
    const char* picture;
    const char* animation;
    double delay;
};

BuildingViewDef makeView(TOResManager& resources, const Look& look, const Point2D& pos) {
    BuildingViewDef def;
    def.view = resources.createRasterObject(look.picture, pos.x, pos.y);
    if (look.animation != nullptr)
        def.topAnimation = std::make_shared<RasterAnimation>(resources.getCachedData(look.animation),
                                                             resources.animTransitions(look.animation)[0], 0, 0, look.delay);
    return def;
}

} // namespace

// --- BuildingView -----------------------------------------------------------------------------------

BuildingView::BuildingView(BuildingViewDef def) : view_(std::move(def.view)), animation_(std::move(def.topAnimation)) {
    if (animation_) attached_.attach(animation_, true);
}

void BuildingView::update(double timeElapsed) {
    if (animation_) animation_->update(timeElapsed);
    UpdatableObject::update(timeElapsed);
}

void BuildingView::drawTo(engine::BitmapData& place, double x, double y, double yScale, const engine::ColorTransform* color) {
    if (!visible_) return;
    attached_.drawBelow(place, x + getX(), y + getY(), yScale, color);
    view_->drawTo(place, x, y, yScale, color);
    attached_.drawAbove(place, x + getX(), y + getY(), yScale, color);
}

void BuildingView::customDispose() {
    attached_.clear();
    if (animation_) animation_->dispose();
    animation_.reset();
    if (view_) view_->dispose();
    view_.reset();
    UpdatableObject::customDispose();
}

// --- KeyNodeView --------------------------------------------------------------------------------------

namespace {

// [kind: castle, tower, town, camp][color: red, green, blue, purple][intact, ruined]
const Look KEY_NODE_LOOKS[4][4][2] = {
    {{{"CastleRedClass", "CastleRedAnimClass", 6}, {"CastleRedRazeClass", "CastleRedRazeAnimClass", 0}},
     {{"CastleGreenClass", "CastleGreenAnimClass", 0}, {"CastleGreenRazeClass", "CastleGreenRazeAnimClass", 0}},
     {{"CastleBlueClass", "CastleBlueAnimClass", 6}, {"CastleBlueRazeClass", "CastleBlueRazeAnimClass", 0}},
     {{"CastlePurpleClass", "CastlePurpleAnimClass", 6}, {"CastlePurpleRazeClass", "CastlePurpleRazeAnimClass", 0}}},
    {{{"MageTowerRedClass", nullptr, 0}, {"MageTowerRedRazedClass", nullptr, 0}},
     {{"MageTowerGreenClass", nullptr, 0}, {"MageTowerGreenRazedClass", nullptr, 0}},
     {{"MageTowerBlueClass", nullptr, 0}, {"MageTowerBlueRazedClass", nullptr, 0}},
     {{"MageTowerPurpleClass", nullptr, 0}, {"MageTowerPurpleRazedClass", nullptr, 0}}},
    {{{"TownRedClass", "TownRedAnimClass", 4}, {"TownRedRazeClass", "TownRedRazeAnimClass", 0}},
     {{"TownGreenClass", "TownGreenAnimClass", 0}, {"TownGreenRazeClass", "TownGreenRazeAnimClass", 0}},
     {{"TownBlueClass", "TownBlueAnimClass", 6}, {"TownBlueRazeClass", "TownBlueRazeAnimClass", 0}},
     {{"TownPurpleClass", "TownPurpleAnimClass", 6}, {"TownPurpleRazeClass", nullptr, 0}}},
    {{{"WarcampRedClass", nullptr, 0}, {"WarcampRedRazeClass", nullptr, 0}},
     {{"WarcampGreenClass", nullptr, 0}, {"WarcampGreenRazeClass", nullptr, 0}},
     {{"WarcampBlueClass", nullptr, 0}, {"WarcampBlueRazeClass", nullptr, 0}},
     {{"WarcampPurpleClass", nullptr, 0}, {"WarcampPurpleRazeClass", nullptr, 0}}},
};

constexpr double INNER_RAD = 40;
constexpr double INNER_RAD2 = INNER_RAD * INNER_RAD;

// Stronghold markers of the map clips, by kind and race.
struct Marker {
    const char* className;
    KeyNodeType type;
    Race race;
};
const Marker MARKERS[] = {
    {"NodeCastleRedClass", KeyNodeType::Castle, Race::TheKhaganate},
    {"NodeCastleGreenClass", KeyNodeType::Castle, Race::Westaria},
    {"NodeCastleBlueClass", KeyNodeType::Castle, Race::TheCult},
    {"NodeCastlePurpleClass", KeyNodeType::Castle, Race::TheEmpire},
    {"NodeTownRedClass", KeyNodeType::Town, Race::TheKhaganate},
    {"NodeTownGreenClass", KeyNodeType::Town, Race::Westaria},
    {"NodeTownBlueClass", KeyNodeType::Town, Race::TheCult},
    {"NodeTownPurpleClass", KeyNodeType::Town, Race::TheEmpire},
    {"NodeCampRedClass", KeyNodeType::Camp, Race::TheKhaganate},
    {"NodeCampGreenClass", KeyNodeType::Camp, Race::Westaria},
    {"NodeCampBlueClass", KeyNodeType::Camp, Race::TheCult},
    {"NodeCampPurpleClass", KeyNodeType::Camp, Race::TheEmpire},
    {"NodeMageTowerRedClass", KeyNodeType::MagicTower, Race::TheKhaganate},
    {"NodeMageTowerGreenClass", KeyNodeType::MagicTower, Race::Westaria},
    {"NodeMageTowerBlueClass", KeyNodeType::MagicTower, Race::TheCult},
    {"NodeMageTowerPurpleClass", KeyNodeType::MagicTower, Race::TheEmpire},
};

const Marker* findMarker(const engine::DisplayObject& marker) {
    for (const Marker& m : MARKERS)
        if (symbols::isInstance(marker, m.className)) return &m;
    return nullptr;
}

} // namespace

KeyNodeView::KeyNodeView(TOResManager& resources, const GraphNode& node, KeyNodeType type)
    : selectedColor_(RasterSprite::DEFAULT_SELECTED_COLOR) {
    views_.resize(4);
    for (int kind = 0; kind < 4; ++kind) {
        views_[kind].resize(4);
        for (int color = 0; color < 4; ++color)
            for (int state = 0; state < 2; ++state)
                views_[kind][color].push_back(
                    std::make_shared<BuildingView>(makeView(resources, KEY_NODE_LOOKS[kind][color][state], node.pos)));
    }
    changeType(type);
}

KeyNodeView::~KeyNodeView() {
    for (auto& kind : views_)
        for (auto& color : kind)
            for (auto& view : color) view->dispose();
}

bool KeyNodeView::isKeyNodeMarker(const engine::DisplayObject& marker) { return findMarker(marker) != nullptr; }

KeyNodeType KeyNodeView::markerType(const engine::DisplayObject& marker) {
    const Marker* m = findMarker(marker);
    return m != nullptr ? m->type : KeyNodeType::Town;
}

Race KeyNodeView::markerRace(const engine::DisplayObject& marker) {
    const Marker* m = findMarker(marker);
    return m != nullptr ? m->race : Race::TheEmpire;
}

void KeyNodeView::changeType(KeyNodeType type) {
    switch (type) {
    case KeyNodeType::Castle: kind_ = 0; break;
    case KeyNodeType::MagicTower: kind_ = 1; break;
    case KeyNodeType::Town: kind_ = 2; break;
    case KeyNodeType::Camp: kind_ = 3; break;
    }
}

bool KeyNodeView::isInside(double x, double y) const {
    const double dx = x - current().getX(), dy = y - current().getY();
    return dx * dx + dy * dy < INNER_RAD2;
}

void KeyNodeView::drawTo(engine::BitmapData& place, double x, double y, double yScale, const engine::ColorTransform* color) {
    if (isHighlighted()) color = &selectedColor_;
    current().drawTo(place, x, y, yScale, color);
}

// --- FKeyNodeContent -----------------------------------------------------------------------------------------

FKeyNodeContent::FKeyNodeContent(TOResManager& resources, const GraphNode& node, const Combatant& ownerCombatant,
                                 KeyNodeType type)
    : KeyNode(std::make_shared<NodeContentKey>(), node.description, ownerCombatant, type, node.pos.getCopy()) {
    nodeDescription = node.description;
    view_ = std::make_unique<KeyNodeView>(resources, node, type);
    iconPlace_ = std::make_shared<engine::Sprite>();
    healthBar_ = symbols::create("KeyNodeHealthBarClass");
    anchor_ = node.pos.getCopy();
    anchor_.y /= Level::Y_KOEFF;
    switch (getType()) {
    case KeyNodeType::MagicTower: anchor_.y -= 35; break;
    case KeyNodeType::Castle: anchor_.y -= 20; break;
    case KeyNodeType::Town: anchor_.y -= 10; break;
    default: break;
    }
    healthBar_->setX(anchor_.x);
    healthBar_->setY(anchor_.y - 60);
    updateHealthBar(getHP(), getMaxHP());
    setLevel(level_);
}

void FKeyNodeContent::customDispose() {
    if (iconPlace_ && iconPlace_->parent() != nullptr) iconPlace_->parent()->removeChild(iconPlace_.get());
    iconPlace_.reset();
    view_.reset();
    KeyNode::customDispose();
}

void FKeyNodeContent::customUpdate(double dt) {
    view_->update(dt);
    KeyNode::customUpdate(dt);
}

void FKeyNodeContent::setLevel(int level) {
    KeyNode::setLevel(level);
    if (auto* star = symbols::child(*healthBar_, "mcStarIcon")) star->gotoAndStop(3 - level_);
}

void FKeyNodeContent::updateIconView() {
    const int count = iconPlace_->numChildren();
    double gap = 4;
    double iconsWidth = count * 30 + gap * (count - 1);
    if (iconsWidth > 120) {
        iconsWidth = 120;
        gap = (iconsWidth - count * 30) / (count - 1);
    }
    int x = static_cast<int>(anchor_.x - iconsWidth / 2 + 30.0 / 2);
    const int y = static_cast<int>(anchor_.y - 85);
    for (int i = 0; i < count; ++i) {
        engine::DisplayObject* icon = iconPlace_->getChildAt(i);
        icon->setX(x);
        icon->setY(y);
        x = static_cast<int>(x + 30 + gap);
    }
}

void FKeyNodeContent::addViewsTo(engine::DisplayObjectContainer& place) {
    place.addChild(iconPlace_);
    place.addChild(healthBar_);
}

void FKeyNodeContent::addIcon(std::shared_ptr<engine::DisplayObject> icon) {
    iconPlace_->addChild(std::move(icon));
    updateIconView();
}

Point2D FKeyNodeContent::viewPosition() const { return {view_->getX(), view_->getY() / Level::Y_KOEFF}; }

void FKeyNodeContent::setLocationType(int locationType) {
    view_->setLocationType(locationType);
    if (auto* color = symbols::child(*healthBar_, "mcColor")) color->gotoAndStop(locationType + 1);
    locationType_ = locationType;
    updateHealthBar(hp_, getMaxHP());
}

void FKeyNodeContent::setHP(int hp) {
    KeyNode::setHP(hp);
    if (healthBar_) updateHealthBar(hp_, getMaxHP());
}

void FKeyNodeContent::updateHealthBar(int hp, int maxHP) {
    healthBar_->gotoAndStop(static_cast<int>(std::floor(static_cast<double>(healthBar_->totalFrames() - 1) * hp / maxHP)) + 1);
    if (hp <= 0) {
        showStatus(2);
    } else {
        FsmState* state = getFSMState();
        if (hp == maxHP) state = stateActive_.get();
        if (state == stateActive_.get()) showStatus(1);
        else if (state == stateRebuilding_.get()) showStatus(2);
    }
    if (auto* text = symbols::find<engine::TextField>(*healthBar_, "mcStatusIcon.txtStatus"))
        text->setText(std::to_string(static_cast<int>(std::ceil(hp / 100.0))));
}

void FKeyNodeContent::showStatus(int id) {
    auto* status = symbols::child(*healthBar_, "mcStatusIcon");
    if (status == nullptr) return;
    status->gotoAndStop(id);
    if (id == 1) {
        if (auto* shield = symbols::child(*status, "mcShield")) shield->gotoAndStop(locationType_ + 1);
    } else if (id == 2) {
        if (auto* rebuild = symbols::child(*status, "mcRebuild")) rebuild->gotoAndStop(locationType_ + 1);
    }
}

void FKeyNodeContent::changeViewToActive() {
    KeyNode::changeViewToActive();
    view_->showActive();
    updateHealthBar(hp_, getMaxHP());
}

void FKeyNodeContent::changeViewToRize() {
    KeyNode::changeViewToRize();
    view_->showRuined();
    SoundConst::play(SoundId::DESTRUCTION_STRONGHOLD);
}

// --- BuildPlace ------------------------------------------------------------------------------------------------

namespace {

// [kind: empty, village, mine, obelisk][color: red, green, blue, purple]
const Look BUILD_PLACE_LOOKS[4][4] = {
    {{"BuildHereTopClass", nullptr, 0}, {"BuildHereTopClass", nullptr, 0}, {"BuildHereTopClass", nullptr, 0},
     {"BuildHereTopClass", nullptr, 0}},
    {{"VillageRedClass", "VillageRedAnimClass", 0}, {"VillageGreenClass", "VillageGreenAnimClass", 1},
     {"VillageBlueClass", "VillageBlueAnimClass", 0}, {"VillagePurpleClass", "VillagePurpleAnimClass", 2.5}},
    {{"MineRedClass", "MineRedAnimClass", 0}, {"MineGreenClass", "MineGreenAnimClass", 0.5},
     {"MineBlueClass", "MineBlueAnimClass", 0}, {"MinePurpleClass", "MinePurpleAnimClass", 0}},
    {{"ObeliskRedClass", "ObeliskRedAnimClass", 1}, {"ObeliskGreenClass", "ObeliskGreenAnimClass", 1},
     {"ObeliskBlueClass", "ObeliskBlueAnimClass", 1}, {"ObeliskPurpleClass", "ObeliskPurpleAnimClass", 1}},
};

} // namespace

BuildPlace::BuildPlace(TOResManager& resources, const GraphNode& node)
    : NodeContent(std::make_shared<NodeContentKey>(), node.description),
      selectedColor_(RasterSprite::DEFAULT_SELECTED_COLOR) {
    views_.resize(4);
    for (int kind = 0; kind < 4; ++kind)
        for (int color = 0; color < 4; ++color)
            views_[kind].push_back(std::make_shared<BuildingView>(makeView(resources, BUILD_PLACE_LOOKS[kind][color], node.pos)));
    initBySupplyBuilding(SupplyBuildingType::Empty, nullptr);
}

void BuildPlace::customDispose() {
    for (auto& kind : views_)
        for (auto& view : kind) view->dispose();
    views_.clear();
    NodeContent::customDispose();
}

void BuildPlace::customUpdate(double dt) {
    current().update(dt);
    NodeContent::customUpdate(dt);
}

void BuildPlace::drawTo(engine::BitmapData& place, double x, double y, double yScale, const engine::ColorTransform* color) {
    if (isHighlighted()) color = &selectedColor_;
    current().drawTo(place, x, y, yScale, color);
}

void BuildPlace::initBySupplyBuilding(SupplyBuildingType type, const Combatant* owner) {
    NodeContent::initBySupplyBuilding(type, owner);
    anchor_ = Point2D(current().getX(), current().getY() / Level::Y_KOEFF);
    anchor_.y -= 11;
    anchor_.x -= 5;
    type_ = type;
    kind_ = static_cast<int>(type);
}

// --- ChestView --------------------------------------------------------------------------------------------------

ChestView::ChestView(const GraphNode& node) : Chest(std::make_shared<NodeContentKey>(), node.description) {
    std::shared_ptr<engine::MovieClip> clip = symbols::create("ChestClass");
    sprite_ = std::make_shared<RasterSprite>(RasterObject::fromDisplayObject(*clip, node.pos.x, node.pos.y));
}

bool ChestView::collect(Squad& squad, Combatant& combatant) {
    if (Chest::collect(squad, combatant)) {
        DeadObject::kill(*this);
        SoundConst::play(SoundId::CHEST_OPEN);
        SoundConst::play(SoundId::CHEST_GOLD);
        return true;
    }
    return false;
}

engine::Rectangle ChestView::area() const {
    return {sprite_->getX() - 15, sprite_->getY() / Level::Y_KOEFF - 25, 35, 35};
}

} // namespace takeover
