// NodeViews.h - what stands on the crossings of a battle map, as drawn on
// the battlefield: strongholds, build places and chests (original:
// level/FKeyNodeContent, FKeyNodeContentView, BuildPlace, _SafeCls_1961,
// _SafeStr_2131 and _SafeStr_2133).
//
// Each of them keeps a picture for every look it can take (kind of
// building x owner color x intact/ruined) and draws the current one.
#pragma once

#include "game/framework/raster/RasterAnimation.h"
#include "game/framework/raster/RasterLayer.h"
#include "game/sim/level/KeyNode.h"
#include "game/sim/level/NodeContent.h"
#include "game/sim/battle/Race.h"

#include <memory>
#include <optional>
#include <vector>

namespace engine {
class MovieClip;
class Sprite;
}

namespace takeover {

class TOResManager;

/// A building picture with its optional animation on top (original: _SafeStr_2133).
struct BuildingViewDef {
    std::shared_ptr<RasterObject> view;
    std::shared_ptr<RasterAnimation> topAnimation; ///< Original: _SafeStr_2137.
};

/// One look of a building (original: _SafeStr_2131).
class BuildingView : public UpdatableObject, public ILayerObject {
public:
    explicit BuildingView(BuildingViewDef def);

    void update(double timeElapsed) override;
    [[nodiscard]] double getX() const { return view_->x; }
    void drawTo(engine::BitmapData& place, double x = 0, double y = 0, double yScale = 1,
                const engine::ColorTransform* color = nullptr) override;
    [[nodiscard]] double getY() const override { return view_->y; }
    void setY(double) override {}
    void attach(std::shared_ptr<IRasterDrawable> drawable, bool above = true) override { attached_.attach(std::move(drawable), above); }
    void detach(const IRasterDrawable* drawable) override { attached_.detach(drawable); }
    [[nodiscard]] bool isVisible() const override { return visible_; }
    void setVisible(bool visible) override { visible_ = visible; }
    [[nodiscard]] bool isHighlighted() const override { return highlighted_; }
    void setHighlighted(bool highlighted) override { highlighted_ = highlighted; }
    [[nodiscard]] bool hitTest(double x, double y, double yScale = 1) const override {
        return RasterSprite::hitTestRaster(*view_, x, y, yScale);
    }

protected:
    void customDispose() override;

private:
    std::shared_ptr<RasterObject> view_;         ///< Original: _SafeStr_1549.
    std::shared_ptr<RasterAnimation> animation_; ///< Original: _SafeStr_2153.
    RenderableObjectsContainer attached_;        ///< Original: _SafeStr_2152.
    bool visible_ = true;
    bool highlighted_ = false;
};

/// The looks of a stronghold: kind x color x intact/ruined (original:
/// FKeyNodeContentView).
class KeyNodeView : public ILayerObject {
public:
    KeyNodeView(TOResManager& resources, const GraphNode& node, KeyNodeType type);
    ~KeyNodeView() override;

    /// Is the marker a stronghold, of which kind and race? (original:
    /// _SafeStr_2844, _SafeStr_2845, _SafeStr_2843).
    static bool isKeyNodeMarker(const engine::DisplayObject& marker);
    static KeyNodeType markerType(const engine::DisplayObject& marker);
    static Race markerRace(const engine::DisplayObject& marker);

    void update(double timeElapsed) { current().update(timeElapsed); }
    void setLocationType(int locationType) { color_ = locationType; } ///< _SafeStr_1559
    void changeType(KeyNodeType type);
    void showRuined() { state_ = 1; } ///< _SafeStr_1562
    void showActive() { state_ = 0; } ///< _SafeStr_1560
    /// Within the central disc of the building? (original: _SafeStr_1556).
    [[nodiscard]] bool isInside(double x, double y) const;
    [[nodiscard]] double getX() const { return current().getX(); }

    void drawTo(engine::BitmapData& place, double x = 0, double y = 0, double yScale = 1,
                const engine::ColorTransform* color = nullptr) override;
    [[nodiscard]] double getY() const override { return current().getY(); }
    void setY(double y) override { current().setY(y); }
    void attach(std::shared_ptr<IRasterDrawable> drawable, bool above = true) override { current().attach(std::move(drawable), above); }
    void detach(const IRasterDrawable* drawable) override { current().detach(drawable); }
    [[nodiscard]] bool isVisible() const override { return current().isVisible(); }
    void setVisible(bool visible) override { current().setVisible(visible); }
    [[nodiscard]] bool isHighlighted() const override { return current().isHighlighted(); }
    void setHighlighted(bool highlighted) override { current().setHighlighted(highlighted); }
    [[nodiscard]] bool hitTest(double x, double y, double yScale = 1) const override { return current().hitTest(x, y, yScale); }

private:
    [[nodiscard]] BuildingView& current() const { ///< _SafeStr_2130
        return *views_[static_cast<std::size_t>(kind_)][static_cast<std::size_t>(color_)][static_cast<std::size_t>(state_)];
    }

    int color_ = 0; ///< Original: _SafeStr_2134.
    int state_ = 0; ///< Intact or ruined (original: _SafeStr_2842).
    int kind_ = 0;  ///< Original: _SafeStr_2132.
    std::vector<std::vector<std::vector<std::shared_ptr<BuildingView>>>> views_; ///< Original: _SafeStr_2129.
    engine::ColorTransform selectedColor_;
};

/// A stronghold on the battlefield, with its health bar and squad icons
/// (original: level/FKeyNodeContent).
class FKeyNodeContent : public KeyNode, public ILayerObject {
public:
    FKeyNodeContent(TOResManager& resources, const GraphNode& node, const Combatant& owner, KeyNodeType type);

    /// The sprite holding the icons of the squads inside (original: iconPlace).
    [[nodiscard]] const std::shared_ptr<engine::Sprite>& iconPlace() const { return iconPlace_; }
    /// Where the HUD points at the building (original: _SafeStr_1330).
    [[nodiscard]] const Point2D& anchor() const { return anchor_; }
    void updateIconView();
    /// Puts the icons and the health bar in `place` (original: _SafeStr_1402).
    void addViewsTo(engine::DisplayObjectContainer& place);
    void addIcon(std::shared_ptr<engine::DisplayObject> icon); ///< _SafeStr_1285
    /// Position of the building picture, in screen coordinates (original: _SafeStr_1561).
    [[nodiscard]] Point2D viewPosition() const;
    void setLocationType(int locationType); ///< _SafeStr_1559
    [[nodiscard]] bool isInside(double x, double y) const { return view_->isInside(x, y); } ///< _SafeStr_1556

    void setLevel(int level) override;
    void setHP(int hp) override;
    void changeViewToActive() override;
    void changeViewToRize() override;
    void changeType(KeyNodeType type) override { view_->changeType(type); }
    void customUpdate(double dt) override;
    void customDispose() override;

    // ILayerObject
    void drawTo(engine::BitmapData& place, double x = 0, double y = 0, double yScale = 1,
                const engine::ColorTransform* color = nullptr) override { view_->drawTo(place, x, y, yScale, color); }
    [[nodiscard]] double getY() const override { return view_->getY(); }
    void setY(double y) override { view_->setY(y); }
    void attach(std::shared_ptr<IRasterDrawable> drawable, bool above = true) override { view_->attach(std::move(drawable), above); }
    void detach(const IRasterDrawable* drawable) override { view_->detach(drawable); }
    [[nodiscard]] bool isVisible() const override { return view_->isVisible(); }
    void setVisible(bool visible) override { view_->setVisible(visible); }
    [[nodiscard]] bool isHighlighted() const override { return view_->isHighlighted(); }
    void setHighlighted(bool highlighted) override { view_->setHighlighted(highlighted); }
    [[nodiscard]] bool hitTest(double x, double y, double yScale = 1) const override { return view_->hitTest(x, y, yScale); }

protected:
    void updateHealthBar(int hp, int maxHP); ///< _SafeStr_1553
    void showStatus(int id);                ///< _SafeStr_1555

    std::unique_ptr<KeyNodeView> view_;            ///< Original: _SafeStr_1549.
    std::shared_ptr<engine::Sprite> iconPlace_;    ///< Original: _SafeStr_1550.
    std::shared_ptr<engine::MovieClip> healthBar_; ///< Original: _SafeStr_1552.
    Point2D anchor_;                               ///< Original: _SafeStr_1551.
    int locationType_ = 0;                         ///< Original: _SafeStr_1554.
};

/// A place for a farm, mine or obelisk (original: level/BuildPlace).
class BuildPlace : public NodeContent, public ILayerObject {
public:
    BuildPlace(TOResManager& resources, const GraphNode& node);

    [[nodiscard]] SupplyBuildingType getType() const { return type_; } ///< _SafeStr_1380
    [[nodiscard]] const Point2D& anchor() const { return anchor_; }     ///< _SafeStr_1330
    void setLocationType(int locationType) { color_ = locationType; }  ///< _SafeStr_1559
    void initBySupplyBuilding(SupplyBuildingType type, const Combatant* owner) override;
    void customUpdate(double dt) override;
    void customDispose() override;

    void drawTo(engine::BitmapData& place, double x = 0, double y = 0, double yScale = 1,
                const engine::ColorTransform* color = nullptr) override;
    [[nodiscard]] double getY() const override { return current().getY(); }
    void setY(double y) override { current().setY(y); }
    void attach(std::shared_ptr<IRasterDrawable> drawable, bool above = true) override { current().attach(std::move(drawable), above); }
    void detach(const IRasterDrawable* drawable) override { current().detach(drawable); }
    [[nodiscard]] bool isVisible() const override { return current().isVisible(); }
    void setVisible(bool visible) override { current().setVisible(visible); }
    [[nodiscard]] bool isHighlighted() const override { return current().isHighlighted(); }
    void setHighlighted(bool highlighted) override { current().setHighlighted(highlighted); }
    [[nodiscard]] bool hitTest(double x, double y, double yScale = 1) const override { return current().hitTest(x, y, yScale); }

private:
    [[nodiscard]] BuildingView& current() const { ///< _SafeStr_2130
        return *views_[static_cast<std::size_t>(kind_)][static_cast<std::size_t>(color_)];
    }

    std::vector<std::vector<std::shared_ptr<BuildingView>>> views_; ///< Original: _SafeStr_2129.
    int kind_ = 0;                          ///< Original: _SafeStr_2132.
    int color_ = 0;                         ///< Original: _SafeStr_2134.
    SupplyBuildingType type_ = SupplyBuildingType::Empty; ///< Original: _SafeStr_1504.
    engine::ColorTransform selectedColor_;  ///< Original: _SafeStr_2135.
    Point2D anchor_;                        ///< Original: _SafeStr_1551.
};

/// A chest on the battlefield (original: level/_SafeCls_1961).
class ChestView : public Chest, public ILayerObject {
public:
    explicit ChestView(const GraphNode& node);

    bool collect(Squad& squad, Combatant& combatant) override;
    /// The clickable area, in screen coordinates (original: _SafeStr_1964).
    [[nodiscard]] engine::Rectangle area() const;

    void drawTo(engine::BitmapData& place, double x = 0, double y = 0, double yScale = 1,
                const engine::ColorTransform* color = nullptr) override { sprite_->drawTo(place, x, y, yScale, color); }
    [[nodiscard]] double getY() const override { return sprite_->getY(); }
    void setY(double y) override { sprite_->setY(y); }
    void attach(std::shared_ptr<IRasterDrawable> drawable, bool above = true) override { sprite_->attach(std::move(drawable), above); }
    void detach(const IRasterDrawable* drawable) override { sprite_->detach(drawable); }
    [[nodiscard]] bool isVisible() const override { return sprite_->isVisible(); }
    void setVisible(bool visible) override { sprite_->setVisible(visible); }
    [[nodiscard]] bool isHighlighted() const override { return sprite_->isHighlighted(); }
    void setHighlighted(bool highlighted) override { sprite_->setHighlighted(highlighted); }
    [[nodiscard]] bool hitTest(double x, double y, double yScale = 1) const override { return sprite_->hitTest(x, y, yScale); }

private:
    std::shared_ptr<RasterSprite> sprite_; ///< Original: _SafeStr_1549.
};

} // namespace takeover
