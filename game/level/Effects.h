// Effects.h - timed things on the battlefield: animations of spells and
// buildings, and banners (original: package _SafePkg_838 and _SafeStr_854).
//
// A TimedEffect shows a layer object for some seconds; the level removes it
// once it is finished. Banners are effects that, every half second, act on
// the squads around them: heal, heroism, bloodlust, liberation.
#pragma once

#include "game/framework/raster/RasterAnimation.h"
#include "game/framework/raster/RasterLayer.h"
#include "game/sim/base/TypedKey.h"
#include "game/sim/level/LevelLogic.h"
#include "game/sim/spells/SpellInfo.h"

#include <any>
#include <memory>

namespace takeover {

    class Combatant;
    class Level;

    /// A RasterAnimation on the battlefield (original: _SafeStr_854).
    class RasterAnimSprite : public UpdatableObject, public ILayerObject {
    public:
        static const engine::ColorTransform DEFAULT_SELECTED_COLOR;

        explicit RasterAnimSprite(std::shared_ptr<RasterAnimation> animation);

        [[nodiscard]] RasterAnimation& animation() { return *animation_; } ///< rasterObject
        void update(double timeElapsed) override;
        [[nodiscard]] double getX() const { return animation_->getX(); }

        void drawTo(engine::BitmapData& place, double x = 0, double y = 0, double yScale = 1,
                    const engine::ColorTransform* color = nullptr) override;
        [[nodiscard]] double getY() const override { return animation_->getY(); }
        void setY(double y) override { animation_->setY(y); }
        void attach(std::shared_ptr<IRasterDrawable> drawable, bool above = true) override { attached_.attach(std::move(drawable), above); }
        void detach(const IRasterDrawable* drawable) override { attached_.detach(drawable); }
        [[nodiscard]] bool isVisible() const override { return visible_; }
        void setVisible(bool visible) override { visible_ = visible; }
        [[nodiscard]] bool isHighlighted() const override { return highlighted_; }
        void setHighlighted(bool highlighted) override { highlighted_ = highlighted; }
        [[nodiscard]] bool hitTest(double x, double y, double yScale = 1) const override;

    protected:
        void customDispose() override;

    private:
        std::shared_ptr<RasterAnimation> animation_; ///< Original: _SafeStr_3055.
        engine::ColorTransform selectedColor_;       ///< Original: _SafeStr_2135.
        RenderableObjectsContainer attached_;
        bool visible_ = true;
        bool highlighted_ = false;
    };

    /// Shows a layer object for a while (original: _SafeCls_837).
    class TimedEffect : public UpdatableObject, public ILevelObject {
    public:
        TimedEffect(std::shared_ptr<ILayerObject> object, double showTime);

        [[nodiscard]] const std::shared_ptr<ILayerObject>& layerObject() const { return object_; } ///< _SafeStr_841
        void update(double timeElapsed) override;
        [[nodiscard]] bool isFinished() const override { return finished_; }
        [[nodiscard]] const TypedKey& getKey() const override { return key_; }
        void dispose() override { UpdatableObject::dispose(); }

    protected:
        void customDispose() override;

        std::shared_ptr<ILayerObject> object_;
        double timeLeft_;        ///< Original: _SafeStr_840.
        TypedKey key_{"vsleff"}; ///< Original: _SafeStr_843.
        bool finished_ = false;  ///< Original: _SafeStr_842.
    };

    /// An animation definition and how long it shows (original: _SafeStr_855).
    struct AnimEffectDef {
        std::shared_ptr<RasterAnimation> anim;
        double time = 0;
    };

    /// A TimedEffect playing an animation (original: _SafeCls_852).
    class AnimEffect : public TimedEffect {
    public:
        explicit AnimEffect(const AnimEffectDef& def);

        [[nodiscard]] RasterAnimSprite& rglAnim() { return *sprite_; }
        void update(double timeElapsed) override;
        std::any data;

    protected:
        void customDispose() override;

        std::shared_ptr<RasterAnimSprite> sprite_; ///< Original: _SafeStr_853.
    };

    /// What a banner is made from (original: _SafeStr_858).
    struct BannerDef {
        std::shared_ptr<Spell> spellData;
        AnimEffectDef animDef;
        Level* level = nullptr;        ///< Original: _SafeStr_859.
        Combatant* owner = nullptr;    ///< Original: _SafeStr_862.
    };

    /// Heals the owner's living squads around it (original: BannerHeal).
    class BannerHeal : public AnimEffect {
    public:
        explicit BannerHeal(BannerDef def);
        void update(double timeElapsed) override;

    protected:
        void customDispose() override;

        /// Acts on the squads, every half second (original: updateAuraEffect).
        virtual void updateAuraEffect();

        BannerDef def_;           ///< Original: _SafeStr_839.
        double nextAura_ = 0;     ///< Original: _SafeStr_857.
    };

    /// Raises the damage of the owner's squads around it (original: BannerBloodlust).
    class BannerBloodlust : public BannerHeal {
    public:
        using BannerHeal::BannerHeal;

    protected:
        void updateAuraEffect() override;
    };

    /// Makes the owner's squads around it heroic (original: BannerHeroism).
    class BannerHeroism : public BannerHeal {
    public:
        using BannerHeal::BannerHeal;

    protected:
        void updateAuraEffect() override;
    };

    /// Protects the owner's squads around it from spells (original: BannerOfLiberation).
    class BannerOfLiberation : public BannerHeal {
    public:
        using BannerHeal::BannerHeal;

    protected:
        void updateAuraEffect() override;
    };

    /// Strikes again at each loop of its animation (original: _SafeCls_1965).
    class RepeatingSpellEffect : public BannerHeal {
    public:
        explicit RepeatingSpellEffect(BannerDef def);

    protected:
        void updateAuraEffect() override;

    private:
        int lastRepeat_; ///< Original: _SafeStr_1966.
    };

} // namespace takeover
