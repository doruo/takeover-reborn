// Graphics.h - flash.display.Graphics: drawing lines and fills from code.
//
//     sprite->graphics().lineStyle(2, 0x00FF00);
//     sprite->graphics().drawRect(10, 10, 100, 50);
//
// The commands build a swf::DrawList, the same form as the library shapes,
// rasterized when drawn (and again only when it changes or the scale does).
#pragma once

#include "engine/geom/Matrix.h"
#include "engine/render/Texture.h"
#include "swf/DrawList.h"

#include <cstdint>
#include <memory>
#include <optional>

namespace engine {

    struct RenderContext;

    class Graphics {
    public:
        void clear();

        /// `thickness` in pixels (0 = hairline). A NaN-free way to stop
        /// stroking is lineStyle(-1).
        void lineStyle(double thickness, std::uint32_t rgb = 0, double alpha = 1);
        void beginFill(std::uint32_t rgb, double alpha = 1);
        void endFill();
        void moveTo(double x, double y);
        void lineTo(double x, double y);
        void curveTo(double controlX, double controlY, double anchorX, double anchorY);
        void drawRect(double x, double y, double width, double height);
        void drawCircle(double x, double y, double radius);

        [[nodiscard]] bool empty() const { return drawList_.commands.empty() && !pendingFill_ && !pendingStroke_; }
        [[nodiscard]] Rectangle bounds() const;
        void draw(RenderContext& context, const Matrix& world, const ColorTransform& color);

    private:
        void flushStroke();
        void flushFill();
        void extend(double x, double y, double margin = 0);
        [[nodiscard]] swf::Rect twipsBounds() const;

        swf::DrawList drawList_;
        std::optional<swf::StrokeCommand> pendingStroke_;
        std::optional<swf::FillCommand> pendingFill_;
        std::optional<swf::LineStyle> line_;
        double penX_ = 0, penY_ = 0;
        double minX_ = 0, minY_ = 0, maxX_ = 0, maxY_ = 0;
        bool hasBounds_ = false;

        // Cached raster.
        std::unique_ptr<Texture> texture_;
        Matrix textureToLocal_;
        int cachedBucket_ = -1000;
        bool dirty_ = true;
    };

} // namespace engine
