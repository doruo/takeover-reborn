#include "engine/display/BitmapData.h"

#include "engine/display/DisplayObject.h"
#include "engine/platform/Application.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace engine {

namespace {

// Premultiplied BGRA <-> straight 0xAARRGGBB.
inline std::uint32_t toArgb(const std::uint8_t* p) {
    const unsigned a = p[3];
    if (a == 0) return 0;
    const auto unmultiply = [a](unsigned c) { return std::min(255u, (c * 255 + a / 2) / a); };
    return (a << 24) | (unmultiply(p[2]) << 16) | (unmultiply(p[1]) << 8) | unmultiply(p[0]);
}

inline void fromArgb(std::uint32_t argb, std::uint8_t* p) {
    const unsigned a = argb >> 24;
    const auto multiply = [a](unsigned c) { return static_cast<std::uint8_t>((c * a + 127) / 255); };
    p[0] = multiply(argb & 0xFF);
    p[1] = multiply((argb >> 8) & 0xFF);
    p[2] = multiply((argb >> 16) & 0xFF);
    p[3] = static_cast<std::uint8_t>(a);
}

} // namespace

BitmapData::BitmapData(int width, int height, bool transparent, std::uint32_t fillColor)
    : image_(std::max(1, width), std::max(1, height)), transparent_(transparent) {
    fillRect(rect(), fillColor);
}

std::shared_ptr<BitmapData> BitmapData::clone() const {
    auto copy = std::make_shared<BitmapData>(width(), height(), transparent_, 0);
    copy->image_ = image_;
    return copy;
}

PixelBox BitmapData::clip(const PixelBox& r) const {
    const int x0 = std::max(r.x, 0), y0 = std::max(r.y, 0);
    const int x1 = std::min(r.x + r.width, width()), y1 = std::min(r.y + r.height, height());
    return {x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
}

std::uint32_t BitmapData::getPixel(int x, int y) const { return getPixel32(x, y) & 0xFFFFFF; }

std::uint32_t BitmapData::getPixel32(int x, int y) const {
    if (x < 0 || y < 0 || x >= width() || y >= height()) return 0;
    return toArgb(&image_.pixels[(static_cast<std::size_t>(y) * width() + x) * 4]);
}

void BitmapData::setPixel32(int x, int y, std::uint32_t argb) {
    if (x < 0 || y < 0 || x >= width() || y >= height()) return;
    if (!transparent_) argb |= 0xFF000000;
    fromArgb(argb, &image_.pixels[(static_cast<std::size_t>(y) * width() + x) * 4]);
    ++version_;
}

void BitmapData::fillRect(const PixelBox& r, std::uint32_t argb) {
    if (!transparent_) argb |= 0xFF000000;
    const PixelBox box = clip(r);
    std::uint8_t pixel[4];
    fromArgb(argb, pixel);
    std::uint32_t value = 0;
    std::memcpy(&value, pixel, 4);
    for (int y = box.y; y < box.y + box.height; ++y) {
        auto* row = reinterpret_cast<std::uint32_t*>(&image_.pixels[(static_cast<std::size_t>(y) * width() + box.x) * 4]);
        std::fill(row, row + box.width, value);
    }
    ++version_;
}

void BitmapData::copyPixels(const BitmapData& source, const PixelBox& sourceRect, Point destPoint, bool mergeAlpha) {
    // Flash rounds the destination point down (Point is Number but pixels are int).
    int dx = static_cast<int>(std::floor(destPoint.x)), dy = static_cast<int>(std::floor(destPoint.y));
    PixelBox src = source.clip(sourceRect);
    // Offset changes caused by clipping the source.
    dx += src.x - sourceRect.x;
    dy += src.y - sourceRect.y;
    // Clip to the destination.
    if (dx < 0) src.x -= dx, src.width += dx, dx = 0;
    if (dy < 0) src.y -= dy, src.height += dy, dy = 0;
    src.width = std::min(src.width, width() - dx);
    src.height = std::min(src.height, height() - dy);
    if (src.width <= 0 || src.height <= 0) return;

    const bool blend = mergeAlpha && source.transparent_;
    for (int y = 0; y < src.height; ++y) {
        const std::uint8_t* s = &source.image_.pixels[(static_cast<std::size_t>(src.y + y) * source.width() + src.x) * 4];
        std::uint8_t* d = &image_.pixels[(static_cast<std::size_t>(dy + y) * width() + dx) * 4];
        if (!blend) {
            std::memcpy(d, s, static_cast<std::size_t>(src.width) * 4);
            continue;
        }
        for (int x = 0; x < src.width; ++x, s += 4, d += 4) {
            const unsigned a = s[3];
            if (a == 0) continue;
            if (a == 255) {
                std::memcpy(d, s, 4);
                continue;
            }
            const unsigned inverse = 255 - a;
            for (int c = 0; c < 4; ++c) d[c] = static_cast<std::uint8_t>(s[c] + (d[c] * inverse + 127) / 255);
        }
    }
    if (!transparent_) {
        for (int y = 0; y < src.height; ++y)
            for (int x = 0; x < src.width; ++x)
                image_.pixels[((static_cast<std::size_t>(dy + y) * width() + dx + x) * 4) + 3] = 255;
    }
    ++version_;
}

void BitmapData::colorTransform(const PixelBox& r, const ColorTransform& t) {
    const PixelBox box = clip(r);
    const auto channel = [](double value, float multiplier, float offset) {
        return static_cast<std::uint32_t>(std::clamp(value * multiplier + offset, 0.0, 255.0));
    };
    for (int y = box.y; y < box.y + box.height; ++y) {
        for (int x = box.x; x < box.x + box.width; ++x) {
            std::uint8_t* p = &image_.pixels[(static_cast<std::size_t>(y) * width() + x) * 4];
            const std::uint32_t argb = toArgb(p);
            const std::uint32_t a = channel(argb >> 24, t.alphaMultiplier, t.alphaOffset);
            const std::uint32_t red = channel((argb >> 16) & 0xFF, t.redMultiplier, t.redOffset);
            const std::uint32_t green = channel((argb >> 8) & 0xFF, t.greenMultiplier, t.greenOffset);
            const std::uint32_t blue = channel(argb & 0xFF, t.blueMultiplier, t.blueOffset);
            fromArgb((transparent_ ? a << 24 : 0xFF000000u) | (red << 16) | (green << 8) | blue, p);
        }
    }
    ++version_;
}

void BitmapData::draw(DisplayObject& source, const Matrix& matrix, const ColorTransform& color) {
    const vector::Image rendered = Application::instance().renderToImage(source, matrix, color, width(), height());
    // Composite the rendering over the current pixels.
    const std::size_t count = static_cast<std::size_t>(width()) * height();
    for (std::size_t i = 0; i < count; ++i) {
        const std::uint8_t* s = &rendered.pixels[i * 4];
        std::uint8_t* d = &image_.pixels[i * 4];
        const unsigned a = s[3];
        if (a == 0) continue;
        const unsigned inverse = 255 - a;
        for (int c = 0; c < 4; ++c) d[c] = static_cast<std::uint8_t>(std::min(255u, s[c] + (d[c] * inverse + 127) / 255));
        if (!transparent_) d[3] = 255;
    }
    ++version_;
}

void BitmapData::dispose() {
    image_ = vector::Image();
    ++version_;
}

} // namespace engine
