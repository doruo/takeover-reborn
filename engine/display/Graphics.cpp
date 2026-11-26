#include "engine/display/Graphics.h"

#include "engine/display/DisplayObject.h"
#include "engine/render/Renderer.h"
#include "engine/render/ShapeCache.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace engine {

namespace {
swf::Rgba toRgba(std::uint32_t rgb, double alpha) {
    return {static_cast<std::uint8_t>((rgb >> 16) & 0xFF), static_cast<std::uint8_t>((rgb >> 8) & 0xFF),
            static_cast<std::uint8_t>(rgb & 0xFF), static_cast<std::uint8_t>(std::clamp(alpha, 0.0, 1.0) * 255 + 0.5)};
}
swf::PathPoint twips(double x, double y) { return {static_cast<float>(x * 20), static_cast<float>(y * 20)}; }
} // namespace

void Graphics::clear() {
    drawList_.commands.clear();
    pendingStroke_.reset();
    pendingFill_.reset();
    line_.reset();
    hasBounds_ = false;
    penX_ = penY_ = 0;
    dirty_ = true;
}

void Graphics::extend(double x, double y, double margin) {
    if (!hasBounds_) {
        minX_ = maxX_ = x, minY_ = maxY_ = y;
        hasBounds_ = true;
    }
    minX_ = std::min(minX_, x - margin), maxX_ = std::max(maxX_, x + margin);
    minY_ = std::min(minY_, y - margin), maxY_ = std::max(maxY_, y + margin);
}

void Graphics::flushStroke() {
    if (pendingStroke_ && pendingStroke_->path.commands.size() > 1) drawList_.commands.emplace_back(std::move(*pendingStroke_));
    pendingStroke_.reset();
}

void Graphics::flushFill() {
    if (pendingFill_ && pendingFill_->path.commands.size() > 1) {
        pendingFill_->path.commands.push_back({swf::PathCommand::Op::Close, {}, {}});
        drawList_.commands.emplace_back(std::move(*pendingFill_));
    }
    pendingFill_.reset();
}

void Graphics::lineStyle(double thickness, std::uint32_t rgb, double alpha) {
    flushStroke();
    if (thickness < 0) {
        line_.reset();
        return;
    }
    swf::LineStyle style;
    style.width = static_cast<std::uint16_t>(thickness * 20);
    style.color = toRgba(rgb, alpha);
    line_ = style;
}

void Graphics::beginFill(std::uint32_t rgb, double alpha) {
    flushFill();
    swf::FillCommand fill;
    fill.style.type = swf::FillStyle::Type::Solid;
    fill.style.color = toRgba(rgb, alpha);
    fill.path.commands.push_back({swf::PathCommand::Op::MoveTo, {}, twips(penX_, penY_)});
    pendingFill_ = std::move(fill);
}

void Graphics::endFill() {
    flushStroke();
    flushFill();
    dirty_ = true;
}

void Graphics::moveTo(double x, double y) {
    flushStroke();
    penX_ = x, penY_ = y;
    if (pendingFill_) pendingFill_->path.commands.push_back({swf::PathCommand::Op::MoveTo, {}, twips(x, y)});
    dirty_ = true;
}

void Graphics::lineTo(double x, double y) {
    const double margin = line_ ? line_->width / 40.0 + 1 : 0;
    if (line_) {
        if (!pendingStroke_) {
            pendingStroke_ = swf::StrokeCommand{*line_, {}};
            pendingStroke_->path.commands.push_back({swf::PathCommand::Op::MoveTo, {}, twips(penX_, penY_)});
        }
        pendingStroke_->path.commands.push_back({swf::PathCommand::Op::LineTo, {}, twips(x, y)});
    }
    if (pendingFill_) pendingFill_->path.commands.push_back({swf::PathCommand::Op::LineTo, {}, twips(x, y)});
    extend(penX_, penY_, margin);
    extend(x, y, margin);
    penX_ = x, penY_ = y;
    dirty_ = true;
}

void Graphics::curveTo(double controlX, double controlY, double anchorX, double anchorY) {
    const double margin = line_ ? line_->width / 40.0 + 1 : 0;
    if (line_) {
        if (!pendingStroke_) {
            pendingStroke_ = swf::StrokeCommand{*line_, {}};
            pendingStroke_->path.commands.push_back({swf::PathCommand::Op::MoveTo, {}, twips(penX_, penY_)});
        }
        pendingStroke_->path.commands.push_back({swf::PathCommand::Op::QuadTo, twips(controlX, controlY), twips(anchorX, anchorY)});
    }
    if (pendingFill_)
        pendingFill_->path.commands.push_back({swf::PathCommand::Op::QuadTo, twips(controlX, controlY), twips(anchorX, anchorY)});
    extend(penX_, penY_, margin);
    extend(controlX, controlY, margin);
    extend(anchorX, anchorY, margin);
    penX_ = anchorX, penY_ = anchorY;
    dirty_ = true;
}

void Graphics::drawRect(double x, double y, double width, double height) {
    moveTo(x, y);
    lineTo(x + width, y);
    lineTo(x + width, y + height);
    lineTo(x, y + height);
    lineTo(x, y);
}

void Graphics::drawCircle(double x, double y, double radius) {
    // Eight quadratic arcs, like the Flash Player.
    constexpr int SEGMENTS = 8;
    const double step = 2 * std::numbers::pi / SEGMENTS;
    const double controlRadius = radius / std::cos(step / 2);
    moveTo(x + radius, y);
    for (int i = 1; i <= SEGMENTS; ++i) {
        const double angle = step * i, middle = angle - step / 2;
        curveTo(x + std::cos(middle) * controlRadius, y + std::sin(middle) * controlRadius, x + std::cos(angle) * radius,
                y + std::sin(angle) * radius);
    }
}

Rectangle Graphics::bounds() const {
    if (!hasBounds_) return {};
    return {minX_, minY_, maxX_ - minX_, maxY_ - minY_};
}

swf::Rect Graphics::twipsBounds() const {
    return {static_cast<std::int32_t>(std::floor(minX_ * 20)), static_cast<std::int32_t>(std::ceil(maxX_ * 20)),
            static_cast<std::int32_t>(std::floor(minY_ * 20)), static_cast<std::int32_t>(std::ceil(maxY_ * 20))};
}

void Graphics::draw(RenderContext& context, const Matrix& world, const ColorTransform& color) {
    if (!hasBounds_) return;
    // Pending paths are drawn as they are, like in Flash.
    swf::DrawList all = drawList_;
    if (pendingFill_ && pendingFill_->path.commands.size() > 1) {
        swf::FillCommand fill = *pendingFill_;
        fill.path.commands.push_back({swf::PathCommand::Op::Close, {}, {}});
        all.commands.emplace_back(std::move(fill));
    }
    if (pendingStroke_ && pendingStroke_->path.commands.size() > 1) all.commands.emplace_back(*pendingStroke_);
    const swf::Rect rect = twipsBounds();
    if (rect.empty()) return;
    const Matrix toDevice = world.then(Matrix::scale(context.deviceScale, context.deviceScale));
    const int bucket = ShapeCache::scaleBucket(rect, toDevice);
    if (dirty_ || !texture_ || bucket != cachedBucket_) {
        ShapeCache::Entry entry = context.shapes.rasterize(all, rect, toDevice);
        texture_ = std::make_unique<Texture>(std::move(entry.texture));
        textureToLocal_ = entry.textureToLocal;
        cachedBucket_ = bucket;
        dirty_ = false;
    }
    context.renderer.drawTexture(*texture_, textureToLocal_.then(world), color);
}

} // namespace engine
