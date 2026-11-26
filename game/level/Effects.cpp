#include "game/level/Effects.h"

#include "game/level/Level.h"
#include "game/sim/battle/Combatant.h"
#include "game/squads/FSquad.h"

namespace takeover {

    // /----- RASTER ANIM SPRITE -----/

    const engine::ColorTransform RasterAnimSprite::DEFAULT_SELECTED_COLOR{0.1f, 0.6f, 0.1f, 1, 0, 110, 0, 0};

    RasterAnimSprite::RasterAnimSprite(std::shared_ptr<RasterAnimation> animation)
        : animation_(std::move(animation)), selectedColor_(DEFAULT_SELECTED_COLOR) {}

    void RasterAnimSprite::update(double timeElapsed) {
        animation_->update(timeElapsed);
        UpdatableObject::update(timeElapsed);
    }

    void RasterAnimSprite::drawTo(engine::BitmapData& place, double x, double y, double yScale,
                                  const engine::ColorTransform* color) {
        if (!visible_)
            return;

        if (highlighted_)
            color = &selectedColor_;

        attached_.drawBelow(place, x + getX(), y + getY(), yScale, color);
        animation_->drawTo(place, x, y, yScale, color);
        attached_.drawAbove(place, x + getX(), y + getY(), yScale, color);
    }

    bool RasterAnimSprite::hitTest(double x, double y, double yScale) const {
        return RasterSprite::hitTestRaster(animation_->rasterObject(), x, y, yScale);
    }

    void RasterAnimSprite::customDispose() {
        attached_.clear();

        if (animation_)
            animation_->dispose();

        UpdatableObject::customDispose();
    }

    // /----- TIMED EFFECT -----/

    TimedEffect::TimedEffect(std::shared_ptr<ILayerObject> object, double showTime)
        : object_(std::move(object)), timeLeft_(showTime) {}

    void TimedEffect::update(double timeElapsed) {
        UpdatableObject::update(timeElapsed);
        object_->setHighlighted(false);

        if (timeLeft_ < 0)
            finished_ = true;

        timeLeft_ -= timeElapsed;
    }

    void TimedEffect::customDispose() {
        DeadObject::kill(*object_);
        UpdatableObject::customDispose();
    }

    // /----- ANIM EFFECT -----/

    AnimEffect::AnimEffect(const AnimEffectDef& def)
        : TimedEffect(std::make_shared<RasterAnimSprite>(def.anim), def.time) {
        sprite_ = std::static_pointer_cast<RasterAnimSprite>(object_);
    }

    void AnimEffect::update(double timeElapsed) {
        sprite_->update(timeElapsed);
        TimedEffect::update(timeElapsed);
    }

    void AnimEffect::customDispose() {
        TimedEffect::customDispose();
        data.reset();
    }

    // /----- BANNERS -----/

    BannerHeal::BannerHeal(BannerDef def) : AnimEffect(def.animDef), def_(std::move(def)) {}

    void BannerHeal::customDispose() {
        def_.level = nullptr;
        AnimEffect::customDispose();
    }

    void BannerHeal::update(double timeElapsed) {
        AnimEffect::update(timeElapsed);
        nextAura_ -= timeElapsed;

        if (nextAura_ <= 0) {
            updateAuraEffect();
            nextAura_ = 0.5;
        }
    }

    namespace {
        /// Calls `f` on every squad of the level.
        template <typename F>
        void forEachSquad(Level& level, F&& f) {
            forEachObject(level.getSquads(), [&](const std::shared_ptr<Squad>& squad) { f(static_cast<FSquad&>(*squad)); });
        }
    } // namespace

    void BannerHeal::updateAuraEffect() {
        if (def_.level == nullptr)
            return;

        const Race race = def_.owner->getRace();
        const Spell& spell = *def_.spellData;

        forEachSquad(*def_.level, [&](FSquad& squad) {
            if (squad.getRace() == race && !squad.isMechanism() && squad.isNear(spell.position, spell.radius))
                squad.heal(static_cast<int>(spell.power));
        });
    }

    void BannerBloodlust::updateAuraEffect() {
        if (def_.level == nullptr)
            return;

        const Race race = def_.owner->getRace();
        const Spell& spell = *def_.spellData;

        forEachSquad(*def_.level, [&](FSquad& squad) {
            if (squad.getRace() == race)
                squad.setDamageFactor(spell.position, spell.radius, spell.power);
        });
    }

    void BannerHeroism::updateAuraEffect() {
        if (def_.level == nullptr)
            return;

        const Race race = def_.owner->getRace();
        const Spell& spell = *def_.spellData;

        forEachSquad(*def_.level, [&](FSquad& squad) {
            if (squad.getRace() == race && squad.isNear(spell.position, spell.radius))
                squad.makeHeroic();
        });
    }

    void BannerOfLiberation::updateAuraEffect() {
        if (def_.level == nullptr)
            return;

        const Race race = def_.owner->getRace();
        const Spell& spell = *def_.spellData;

        forEachSquad(*def_.level, [&](FSquad& squad) {
            if (squad.getRace() == race)
                squad.setSpellDefence(spell.position, spell.radius);
        });
    }

    RepeatingSpellEffect::RepeatingSpellEffect(BannerDef def) : BannerHeal(std::move(def)) {
        lastRepeat_ = sprite_->animation().repeatCounter() + 1;
    }

    void RepeatingSpellEffect::updateAuraEffect() {
        if (lastRepeat_ != sprite_->animation().repeatCounter()) {
            lastRepeat_ = sprite_->animation().repeatCounter();
            def_.level->spellStrike(*def_.spellData, *def_.owner);
        }
    }

} // namespace takeover
