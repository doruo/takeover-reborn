// RasterLayer.h - the battlefield bitmap and the objects drawn into it
// (original: package _SafePkg_851 and _SafePkg_1310/RenderBitmapData).
//
// Every frame, RasterGraphicLayer sorts its objects by y (what is lower on
// screen is in front), clears its 800x600 bitmap and asks each object to
// draw itself, layer after layer: ground marks, corpses, paths, then units
// and buildings. An object leaves the layer by moving to DEAD_LINE_Y: it
// sorts last and is dropped during the next render.
#pragma once

#include "game/framework/core/Disposable.h"
#include "game/framework/raster/RasterObject.h"

#include <limits>
#include <memory>
#include <vector>

namespace takeover {

    /// An object of the battlefield layer (original: interface _SafeStr_850).
    class ILayerObject : public IRasterDrawable {
    public:
        [[nodiscard]] virtual double getY() const = 0;
        virtual void setY(double y) = 0;                       ///< _SafeStr_1039

        /// Draws `drawable` with this object, in front or behind (original: _SafeStr_1547).
        virtual void attach(std::shared_ptr<IRasterDrawable> drawable, bool above = true) = 0;
        virtual void detach(const IRasterDrawable* drawable) = 0; ///< _SafeStr_1548
        [[nodiscard]] virtual bool isVisible() const = 0;
        virtual void setVisible(bool visible) = 0;

        /// Drawn tinted, under the mouse (original: _SafeStr_849).
        [[nodiscard]] virtual bool isHighlighted() const = 0;
        virtual void setHighlighted(bool highlighted) = 0;

        /// Is (x, y) on a pixel of the object? (original: _SafeStr_1546).
        [[nodiscard]] virtual bool hitTest(double x, double y, double yScale = 1) const = 0;
    };

    /// The placeholder for removed objects (original: _SafeStr_845).
    class DeadObject : public ILayerObject {
    public:
        static constexpr double DEAD_LINE_Y = std::numeric_limits<double>::max();

        /// Removes an object from its layer at the next render (original: _SafeStr_848).
        static void kill(ILayerObject& object) { object.setY(DEAD_LINE_Y); }

        void drawTo(engine::BitmapData&, double = 0, double = 0, double = 1, const engine::ColorTransform* = nullptr) override {}
        [[nodiscard]] double getY() const override { return DEAD_LINE_Y; }
        void setY(double) override {}
        void attach(std::shared_ptr<IRasterDrawable>, bool = true) override {}
        void detach(const IRasterDrawable*) override {}
        [[nodiscard]] bool isVisible() const override { return false; }
        void setVisible(bool) override {}
        [[nodiscard]] bool isHighlighted() const override { return false; }
        void setHighlighted(bool) override {}
        [[nodiscard]] bool hitTest(double, double, double = 1) const override { return false; }
    };

    /// The objects of one layer (original: RenderableObjectsArray). Removed
    /// slots are filled with a DeadObject.
    class RenderableObjectsArray {
    public:
        RenderableObjectsArray();
        [[nodiscard]] ILayerObject* getObject(int index) const { return objects_[static_cast<std::size_t>(index)].get(); }
        [[nodiscard]] int getObjectsCount() const { return count_; }
        void sortObjects();
        void addRGLObject(std::shared_ptr<ILayerObject> object);
        bool removeRGLObject(const ILayerObject* object);

        /// Drops the objects from `startIndex` on (original: markObjectByNullFrom).
        void markObjectByNullFrom(int startIndex);

        /// The front-most object under (x, y) (original: findFirstHitTest).
        [[nodiscard]] ILayerObject* findFirstHitTest(double x, double y) const;
        void enableHighlightsForAll(bool enable);

    private:
        std::vector<std::shared_ptr<ILayerObject>> objects_; ///< Original: _SafeStr_2699.
        int count_ = 0;                                      ///< Original: _SafeStr_2698.
        std::shared_ptr<DeadObject> dead_;                   ///< Original: _SafeStr_2700.
    };

    /// Drawables attached to an object, behind and in front of it (original:
    /// RenderableObjectsContainer, with ArrayContainer).
    class RenderableObjectsContainer {
    public:
        void attach(std::shared_ptr<IRasterDrawable> drawable, bool above); ///< _SafeStr_1547
        void detach(const IRasterDrawable* drawable);                       ///< _SafeStr_1548
        void drawBelow(engine::BitmapData& place, double x, double y, double yScale, const engine::ColorTransform* color); ///< _SafeStr_2161
        void drawAbove(engine::BitmapData& place, double x, double y, double yScale, const engine::ColorTransform* color); ///< _SafeStr_2159
        void clear() { below_.clear(), above_.clear(); }

    private:
        static void drawAll(const std::vector<std::shared_ptr<IRasterDrawable>>& list, engine::BitmapData& place, double x,
                            double y, double yScale, const engine::ColorTransform* color); ///< _SafeStr_2442
        static void removeSwapLast(std::vector<std::shared_ptr<IRasterDrawable>>& list, std::size_t index);

        std::vector<std::shared_ptr<IRasterDrawable>> below_; ///< Original: _SafeStr_2441.
        std::vector<std::shared_ptr<IRasterDrawable>> above_; ///< Original: _SafeStr_2440.
    };

    /// A RasterObject on the battlefield, with attachments and highlight
    /// (original: _SafeStr_1962).
    class RasterSprite : public Disposable, public ILayerObject {
    public:
        /// The tint of highlighted objects (original: DEFAULT_SELECTED_COLOR).
        static const engine::ColorTransform DEFAULT_SELECTED_COLOR;

        explicit RasterSprite(std::shared_ptr<RasterObject> rasterObject);

        /// Pixel hit test of a raster object (original: _SafeStr_2160).
        static bool hitTestRaster(const RasterObject& raster, double x, double y, double yScale = 1);

        [[nodiscard]] RasterObject& rasterObject() { return *raster_; }
        [[nodiscard]] const engine::ColorTransform& selectedColor() const { return selectedColor_; } ///< _SafeStr_2800
        void setSelectedColor(const engine::ColorTransform& color) { selectedColor_ = color; }

        [[nodiscard]] double getX() const { return raster_->x; }
        void setX(double x) { raster_->x = x; }

        void drawTo(engine::BitmapData& place, double x = 0, double y = 0, double yScale = 1,
                    const engine::ColorTransform* color = nullptr) override;
        [[nodiscard]] double getY() const override { return raster_->y; }
        void setY(double y) override { raster_->y = y; }
        void attach(std::shared_ptr<IRasterDrawable> drawable, bool above = true) override { attached_.attach(std::move(drawable), above); }
        void detach(const IRasterDrawable* drawable) override { attached_.detach(drawable); }
        [[nodiscard]] bool isVisible() const override { return visible_; }
        void setVisible(bool visible) override { visible_ = visible; }
        [[nodiscard]] bool isHighlighted() const override { return highlighted_; }
        void setHighlighted(bool highlighted) override { highlighted_ = highlighted; }
        [[nodiscard]] bool hitTest(double x, double y, double yScale = 1) const override {
            return hitTestRaster(*raster_, x, y, yScale);
        }

    protected:
        void customDispose() override;

    private:
        std::shared_ptr<RasterObject> raster_;   ///< Original: _SafeStr_1807.
        engine::ColorTransform selectedColor_;   ///< Original: _SafeStr_2135.
        RenderableObjectsContainer attached_;    ///< Original: _SafeStr_2152.
        bool visible_ = true;                    ///< Original: _SafeStr_2154.
        bool highlighted_ = false;               ///< Original: _SafeStr_2155.
    };

    /// The battlefield bitmap, drawn layer by layer (original: RasterGraphicLayer
    /// and its base RenderBitmapData).
    class RasterGraphicLayer : public Disposable {
    public:
        static constexpr int LAYER_GROUND = 0;
        static constexpr int LAYER_CORPSE = 1;
        static constexpr int LAYER_PATH = 2;
        static constexpr int LAYER_UNIT = 3;
        static constexpr int LAYER_COUNT = 4;

        RasterGraphicLayer(int width, int height);

        [[nodiscard]] const std::shared_ptr<engine::BitmapData>& bitmap() const { return bitmap_; }
        void addRGLObject(std::shared_ptr<ILayerObject> object, int layer);
        void removeRGLObject(const ILayerObject* object);

        /// Sorts, clears and redraws everything (original: _SafeStr_2880).
        void render();

        [[nodiscard]] ILayerObject* findFirstHitTest(double x, double y, int layer) const {
            return layers_[static_cast<std::size_t>(layer)].findFirstHitTest(x, y);
        }

        void allHighLightsOff(int layer) { layers_[static_cast<std::size_t>(layer)].enableHighlightsForAll(false); }

    protected:
        void customDispose() override;

    private:
        std::shared_ptr<engine::BitmapData> bitmap_;
        std::vector<RenderableObjectsArray> layers_; ///< Original: _SafeStr_2699.
    };

} // namespace takeover
