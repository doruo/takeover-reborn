#include "game/units/FUnit.h"

#include "game/framework/core/As3.h"
#include "game/framework/raster/ResourceManager.h"
#include "game/level/Level.h"
#include "game/media/SoundConst.h"
#include "game/sim/abilities/Abilities.h"
#include "game/sim/abilities/AbilityEffects.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/units/UnitNameModifiers.h"

#include <cmath>
#include <numbers>

namespace takeover {

// --- Unit3DAnimation ---------------------------------------------------------------------------------

Unit3DAnimation::Unit3DAnimation(ResourceManager& resources, const std::string& unitId, const INameModifier& directions,
                                 const INameModifier& phases)
    : phaseCount_(phases.count()) {
    for (int i = 0; i < directions.count(); ++i)
        directions_.push_back(std::make_shared<UnitAnimation>(resources, directions.name(unitId, i), phases));
    current_ = &directions_[0]->animation(0);
    show(0, 0);
    speed_ = 1;
}

void Unit3DAnimation::play(RasterAnimation& animation, int repeatCount, double startTime) {
    const bool wasRunning = current_->running();
    current_ = &animation;
    current_->restart(repeatCount, startTime);
    current_->setRunning(wasRunning);
}

void Unit3DAnimation::setSpeedFactor(double factor) {
    if (speed_ == factor) return;
    speed_ = factor;
    for (auto& animation : directions_) animation->setSpeedFactor(speed_);
}

void Unit3DAnimation::show(int phase, int direction, int repeatCount) {
    const double x = current_->getX(), y = current_->getY();
    play(directions_[static_cast<std::size_t>(direction)]->animation(phase), repeatCount, 0);
    current_->setX(x);
    current_->setY(y);
    direction_ = direction;
    phase_ = phase;
}

void Unit3DAnimation::turn(int direction) {
    const double x = current_->getX(), y = current_->getY();
    const double lifetime = current_->elapsedMs() / 1000;
    const int repeatCount = current_->repeatCounter();
    const int frame = current_->currentFrame();
    play(directions_[static_cast<std::size_t>(direction)]->animation(phase_), repeatCount, lifetime);
    current_->setX(x);
    current_->setY(y);
    current_->setTweenValue(frame);
    direction_ = direction;
}

void Unit3DAnimation::update(double timeElapsed) {
    current_->update(timeElapsed);
    UpdatableObject::update(timeElapsed);
}

// --- FUnit ------------------------------------------------------------------------------------------------

namespace {
BitSet& maskPoison() {
    static BitSet mask(1);
    return mask;
}
BitSet& maskFreeze() {
    static BitSet mask(2);
    return mask;
}
} // namespace

const engine::ColorTransform FUnit::COLOR_POISON{1, 1, 1, 1, 0, 150, 0, 0};
const engine::ColorTransform FUnit::COLOR_FREEZ{1, 1, 1, 1, 0, 0, 150, 0};

FUnit& FUnit::init(const FUnitDef& def) {
    def_ = def;
    const UnitDirectionNameModifier directions;
    animation_ = std::make_shared<Unit3DAnimation>(*def_.resources, def_.unitID, directions, *def_.phaseNames);
    Unit::init(def);
    initHealthBar();
    abilities_->addEventListener(Abilities::EVENT_PROPERTY_ADDED, getId(),
                                 [this](int t, BaseObject* s, const std::any& d) { onEffectAdded(t, s, d); });
    abilities_->addEventListener(Abilities::EVENT_PROPERTY_REMOVED, getId(),
                                 [this](int t, BaseObject* s, const std::any& d) { onEffectRemoved(t, s, d); });
    return *this;
}

void FUnit::update(double timeElapsed) {
    animation_->update(timeElapsed);
    animation_->setX(x_);
    animation_->setY(y_);
    Unit::update(timeElapsed);
}

void FUnit::customDispose() {
    if (!disposedOnce_) {
        signalDispose.dispatch(this);
        disposedOnce_ = true;
    }
    signalAnim.dispose();
    signalDispose.dispose();
    Unit::customDispose();
}

void FUnit::initHealthBar() {
    if (abilities_->size == UnitSize::Gaint) {
        healthBarWidth_ = 24;
        healthBarHeight_ = 2;
        healthBarOffset_ = -40;
    } else {
        healthBarWidth_ = 10;
        healthBarHeight_ = 1;
        healthBarOffset_ = -20;
    }
}

void FUnit::drawTo(engine::BitmapData& place, double x, double y, double yScale, const engine::ColorTransform* color) {
    if (!visible_ || hidden) return;
    const engine::ColorTransform* filter = tintFor(color);
    attached_.drawBelow(place, x + x_, y + y_, yScale, filter);
    animation_->drawTo(place, x, y, yScale, filter);
    attached_.drawAbove(place, x + x_, y + y_, yScale, filter);

    if (missileTimeLeft_ >= 0 && missilePos_) {
        const ArmyStat& stat = *getArmyStat();
        const double towardsFacing = std::numbers::pi * (90 * animation_->direction() + 45) / 180;
        switch (stat.unitType) {
        case UnitType::Siege:
            drawCatapultStone(place, x, y, yScale, color);
            break;
        case UnitType::Archers:
            if (stat.side == Race::TheCult) {
                // Robots shoot bullets from their gun. Original quirk: the
                // tier 1 robot draws its bullet twice (the code falls
                // through to the tier 2 case).
                if (stat.techLevel == 3) {
                    drawBullet(place, x + std::cos(towardsFacing) * 5, y + std::sin(towardsFacing) * 5,
                               "GiantbotBulletClass", yScale, color);
                } else if (stat.techLevel == 1 || stat.techLevel == 2) {
                    if (stat.techLevel == 1) drawBullet(place, x, y, "BulletClass", yScale, color);
                    drawBullet(place, x + std::cos(towardsFacing) * 10, y + std::sin(towardsFacing) * 10,
                               "GunbotBulletClass", yScale, color);
                }
            } else {
                drawArrow(place, x, y, yScale, color);
            }
            break;
        case UnitType::Magic:
            if (stat.side == Race::Westaria) drawBullet(place, x, y, "ZealotBulletClass", yScale, color);
            else if (stat.side == Race::TheCult) drawBullet(place, x, y, "AcolytesBulletClass", yScale, color);
            break;
        default:
            break;
        }
    } else {
        prevArrowPos1_.reset();
        prevArrowPos2_.reset();
    }

    if (Level::isModeOn(Level::ST_SHOW_UNIT_HEALTH)) {
        const double health = getHP();
        if (health == getMaxHP() || health <= 0) return;
        const double barY = (y + y_) * yScale + healthBarOffset_;
        const double length = health * healthBarWidth_ / getMaxHP();
        const double left = x + x_ - healthBarWidth_ / 2;
        place.fillRect({static_cast<int>(left), static_cast<int>(barY), static_cast<int>(length), static_cast<int>(healthBarHeight_)},
                       0xFF00FF00);
        place.fillRect({static_cast<int>(left + length), static_cast<int>(barY), static_cast<int>(healthBarWidth_ - length),
                        static_cast<int>(healthBarHeight_)},
                       0xFFFF0000);
    }
}

void FUnit::setVisible(bool visible) {
    if (isDead()) return;
    visible_ = visible;
}

void FUnit::changeAnimTo_Go() {
    if (animation_->phase() != UnitPhaseNameModifier::PHASE_ID_GO) {
        animation_->setPhase(UnitPhaseNameModifier::PHASE_ID_GO);
        const double startTime = as3::random() * animation_->current().transition().mscDuration / 1000;
        animation_->current().restart(-1, startTime);
        updateGoAnimSpeed();
    }
}

void FUnit::changeAnimTo_Special() {
    animation_->setPhase(UnitPhaseNameModifier::PHASE_ID_SPECIAL);
    animation_->current().restart(1, 0);
}

void FUnit::updateGoAnimSpeed() {
    setVisible(true);
    animation_->setSpeedFactor(getSpeed() / def_.armyStat->baseSpeedAnimWalk);
}

double FUnit::changeAnimTo_Attack(double attackSpeed) {
    double value = 0;
    if (animation_->phaseCount() > UnitPhaseNameModifier::PHASE_ID_ATTACK2) value = getRange() == 0 ? as3::random() : 1;
    return playAttack(value < 0.5 ? UnitPhaseNameModifier::PHASE_ID_ATTACK1 : UnitPhaseNameModifier::PHASE_ID_ATTACK2,
                      attackSpeed);
}

double FUnit::changeAnimTo_DistanceAttack(double attackSpeed) {
    return playAttack(UnitPhaseNameModifier::PHASE_ID_ATTACK1, attackSpeed);
}

double FUnit::playAttack(int phase, double attackSpeed) {
    setVisible(true);
    animation_->setPhase(phase);
    animation_->setSpeedFactor(1);
    animation_->current().restart(1);
    signalAnim.dispatch(this, ANIM_ATTACK);
    // The attack animation never lasts longer than the attack period.
    const double length = animation_->current().transition().mscDuration / 1000;
    if (length > attackSpeed) {
        animation_->setSpeedFactor(attackSpeed / length);
        return attackSpeed;
    }
    return length;
}

void FUnit::changeAnimTo_Stand() {
    // Standing still is the first frame of the death animation.
    animation_->setPhase(UnitPhaseNameModifier::PHASE_ID_DIE);
    animation_->current().setTweenValue(0);
    animation_->setRunning(false);
    setVisible(true);
}

bool FUnit::isLyingDead() const {
    return animation_->phase() == UnitPhaseNameModifier::PHASE_ID_DIE && !animation_->isRunning();
}

void FUnit::changeAnimTo_FallDown() {
    setVisible(true);
    if (animation_->phase() != UnitPhaseNameModifier::PHASE_ID_DIE || isLyingDead()) {
        animation_->setPhase(UnitPhaseNameModifier::PHASE_ID_DIE);
        animation_->current().restart(1);
        animation_->current().signalFinish.addListener(this, &FUnit::onFallDownFinished);
        animation_->setSpeedFactor(1);
        animation_->setRunning(true);
        signalAnim.dispatch(this, ANIM_FALL_DOWN);
    }
}

void FUnit::onFallDownFinished(Tween*) {
    setVisible(true);
    signalAnim.dispatch(this, ANIM_FALL_DOWN_FINISH);
}

void FUnit::updateAnimDir(double dx, double dy) {
    const int direction = directionOf(dx, dy);
    if (animation_->direction() != direction) animation_->turn(direction);
}

int FUnit::directionOf(double dx, double dy) const {
    if (dx < 0) return dy < 0 ? DIR_UP : DIR_LEFT;
    return dy < 0 ? DIR_RIGHT : DIR_DOWN;
}

void FUnit::drawLine(engine::BitmapData& place, int x1, int y1, int x2, int y2, std::uint32_t color) {
    const int deltaX = std::abs(x2 - x1), deltaY = std::abs(y2 - y1);
    const int signX = x1 < x2 ? 1 : -1, signY = y1 < y2 ? 1 : -1;
    int error = deltaX - deltaY;
    while (true) {
        place.setPixel32(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        const int error2 = error * 2;
        if (error2 > -deltaY) error -= deltaY, x1 += signX;
        if (error2 < deltaX) error += deltaX, y1 += signY;
    }
}

double FUnit::arcHeight(double time) const {
    const double half = missileFlightTime_ / 2;
    const double k = (missileFlightTime_ - time - half) / half;
    return 80 * (1 - k * k);
}

void FUnit::onEffectAdded(int, BaseObject*, const std::any& data) {
    const auto effect = std::dynamic_pointer_cast<AbilityEffect>(std::any_cast<std::shared_ptr<IProperty>>(data));
    if (!effect) return;
    if (effect->getName() == AbilityEffect::ATK_POISON) magicMarks_.unite(maskPoison());
    else if (effect->getName() == AbilityEffect::ATK_FREEZ) magicMarks_.unite(maskFreeze());
    SoundConst::playEffect(effect->getName());
}

void FUnit::onEffectRemoved(int, BaseObject*, const std::any& data) {
    const auto effect = std::dynamic_pointer_cast<AbilityEffect>(std::any_cast<std::shared_ptr<IProperty>>(data));
    if (!effect) return;
    if (effect->getName() == AbilityEffect::ATK_POISON) magicMarks_.subtract(maskPoison());
    else if (effect->getName() == AbilityEffect::ATK_FREEZ) magicMarks_.subtract(maskFreeze());
}

const engine::ColorTransform* FUnit::tintFor(const engine::ColorTransform* color) const {
    if (magicMarks_.intersects(maskPoison())) return &COLOR_POISON;
    if (magicMarks_.intersects(maskFreeze())) return &COLOR_FREEZ;
    return color;
}

void FUnit::drawCatapultStone(engine::BitmapData& place, double x, double y, double yScale,
                              const engine::ColorTransform* color) {
    if (!missilePicture_) missilePicture_ = def_.resources->createRasterObject("CatapultMissileClass");
    const double height = arcHeight(missileTimeLeft_) +
                          getArmyStat()->distAttackStartHeight * missileTimeLeft_ / missileFlightTime_;
    missilePicture_->drawTo(place, std::round(x + missilePos_->x), std::round(y + missilePos_->y - height), yScale, color);
}

void FUnit::drawBullet(engine::BitmapData& place, double x, double y, const std::string& className, double yScale,
                       const engine::ColorTransform* color) {
    if (!missilePicture_) missilePicture_ = def_.resources->createRasterObject(className);
    double height = getArmyStat()->distAttackStartHeight;
    height -= height / 2 * ((missileFlightTime_ - missileTimeLeft_) / missileFlightTime_);
    const double posX = std::round(x + missilePos_->x);
    const double posY = std::round(y + missilePos_->y - height);
    // Not drawn while still in the shooter's hands.
    if (getPosition().distanceTo(posX, posY + height) >= getArmyStat()->radAfterShowBullet)
        missilePicture_->drawTo(place, posX, posY, yScale, color);
}

void FUnit::drawArrow(engine::BitmapData& place, double x, double y, double yScale, const engine::ColorTransform*) {
    const double height = arcHeight(missileTimeLeft_);
    const double height2 = arcHeight(missileTimeLeft_ + 0.033);
    // The previous position is drawn faint: a short trail.
    if (prevArrowPos1_) {
        drawLine(place, static_cast<int>(prevArrowPos1_->x), static_cast<int>(prevArrowPos1_->y),
                 static_cast<int>(prevArrowPos2_->x), static_cast<int>(prevArrowPos2_->y), 0xA0000000);
    } else {
        prevArrowPos1_.emplace();
        prevArrowPos2_.emplace();
    }
    prevArrowPos1_->x = std::round(x + missilePos_->x);
    prevArrowPos1_->y = std::round((y + missilePos_->y - height) * yScale);
    prevArrowPos2_->x = std::round(x + missilePrevPos_->x);
    prevArrowPos2_->y = std::round((y + missilePrevPos_->y - height2) * yScale);
    drawLine(place, static_cast<int>(prevArrowPos1_->x), static_cast<int>(prevArrowPos1_->y),
             static_cast<int>(prevArrowPos2_->x), static_cast<int>(prevArrowPos2_->y), 0xB0000000);
}

void FUnit::missileAtTarget(ITarget* target) {
    Unit::missileAtTarget(target);
    SoundConst::playHit(def_.armyStat);
}

void FUnit::activateAttack() {
    Unit::activateAttack();
    SoundConst::playAttack(def_.armyStat, false);
}

void FUnit::activateDistanceAttack() {
    Unit::activateDistanceAttack();
    SoundConst::playAttack(def_.armyStat, true);
}

} // namespace takeover
