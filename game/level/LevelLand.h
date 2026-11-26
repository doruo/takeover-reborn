// LevelLand.h - the ground of a battle map (original: level/LevelLand,
// level/LevelLandObject and level/_SafeCls_1767).
//
// A map clip ("Level3Class") has, per location (a stronghold and its
// surroundings), an area, roads and objects (trees, houses...), each with
// one frame per owner color. The ground (areas and roads) is drawn once
// into a bitmap, redrawn when a location changes hands. The objects are
// raster objects of the battlefield layer, so that soldiers walk behind
// them.
#pragma once

#include "game/framework/raster/RasterLayer.h"

#include <string>
#include <vector>

namespace engine {
    class Bitmap;
    class DisplayObject;
    class Sprite;
}

namespace takeover {

    /// What a LevelLand is made from (original: _SafeCls_1767).
    struct LevelLandDef {
        double width = 800;
        double height = 600;
        std::string mapClass;               ///< The map clip (original: _SafeStr_1768).
        std::vector<int> locationViews;     ///< Color frame of each location (original: _SafeStr_1769).
    };

    /// A map object of one location, in each owner color (original: LevelLandObject).
    class LevelLandObject : public Disposable, public ILayerObject {
    public:
        explicit LevelLandObject(int maxObjectIndex = 4);

        [[nodiscard]] const std::vector<std::shared_ptr<RasterObject>>& objects() const { return objects_; } ///< _SafeStr_3439
        [[nodiscard]] int currentIndex() const { return index_; } ///< _SafeStr_3438
        /// Shows the picture of `index`, rendering `source` for it the first time
        /// (original: _SafeStr_1559).
        void setView(int index, engine::DisplayObject* source = nullptr);

        void drawTo(engine::BitmapData& place, double x = 0, double y = 0, double yScale = 1,
                    const engine::ColorTransform* color = nullptr) override;
        [[nodiscard]] double getX() const { return objects_[static_cast<std::size_t>(index_)]->x; }
        [[nodiscard]] double getY() const override { return objects_[static_cast<std::size_t>(index_)]->y; }
        void setY(double) override {}
        void attach(std::shared_ptr<IRasterDrawable> drawable, bool above = true) override { attached_.attach(std::move(drawable), above); }
        void detach(const IRasterDrawable* drawable) override { attached_.detach(drawable); }
        [[nodiscard]] bool isVisible() const override { return visible_; }
        void setVisible(bool visible) override { visible_ = visible; }
        [[nodiscard]] bool isHighlighted() const override { return highlighted_; }
        void setHighlighted(bool highlighted) override { highlighted_ = highlighted; }
        [[nodiscard]] bool hitTest(double, double, double = 1) const override { return false; }

    protected:
        void customDispose() override;

    private:
        static constexpr double TRANSFORM_Y = 0.5;
        std::vector<std::shared_ptr<RasterObject>> objects_; ///< Original: _SafeStr_3436.
        int index_ = 0;                                      ///< Original: _SafeStr_3437.
        RenderableObjectsContainer attached_;                ///< Original: _SafeStr_2152.
        bool visible_ = true;
        bool highlighted_ = false;
    };

    class LevelLand : public Disposable {
    public:
        LevelLand() = default;
        LevelLand& init(LevelLandDef def); ///< _SafeStr_2733

        /// The ground picture (original: _SafeStr_2737).
        [[nodiscard]] const std::shared_ptr<engine::Bitmap>& groundBitmap() const { return groundBitmap_; }
        /// Objects by location (original: allObjectsMatrix).
        [[nodiscard]] const std::vector<std::vector<std::shared_ptr<LevelLandObject>>>& allObjectsMatrix() const { return objects_; }
        [[nodiscard]] const LevelLandDef& def() const { return def_; }
        void setLocationView(int location, int view) { ///< _SafeStr_2736
            if (location >= static_cast<int>(def_.locationViews.size()))
                def_.locationViews.resize(static_cast<std::size_t>(location) + 1, 0);

            def_.locationViews[static_cast<std::size_t>(location)] = view;
        }

        /// The color of a location, 0 for a location without a stronghold (an
        /// undefined array entry in the original) (original: _SafeStr_2735).
        [[nodiscard]] int locationView(int location) const {
            if (location < 0 || location >= static_cast<int>(def_.locationViews.size()))
                return 0;

            return def_.locationViews[static_cast<std::size_t>(location)];
        }

        /// Redraws the ground and switches the objects to the current colors
        /// (original: _SafeStr_2734).
        void redraw();

    protected:
        void customDispose() override;

    private:
        void showLocation(engine::Sprite& map, int location, int view); ///< _SafeStr_2732

        std::shared_ptr<engine::BitmapData> ground_;  ///< Original: _SafeStr_2730.
        std::shared_ptr<engine::Bitmap> groundBitmap_; ///< Original: _SafeStr_2731.
        std::vector<std::vector<std::shared_ptr<LevelLandObject>>> objects_; ///< Original: _SafeStr_2729.
        LevelLandDef def_;                            ///< Original: _SafeStr_839.
    };

} // namespace takeover
