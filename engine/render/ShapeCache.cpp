#include "engine/render/ShapeCache.h"

#include <algorithm>
#include <cmath>

namespace engine {

    namespace {

        constexpr int kPadding = 1;
        constexpr int kMaxTextureSize = 4096;

    } // namespace

    int ShapeCache::scaleBucket(const swf::Rect& bounds, const Matrix& localToDevice) {
        const double scale = std::clamp(localToDevice.maxScale(), 1.0 / 64.0, 64.0);
        int bucket = static_cast<int>(std::ceil(std::log2(scale) * 4.0 - 1e-9));
        const double widthPx = bounds.width() / 20.0, heightPx = bounds.height() / 20.0;

        // Never exceed the texture size limit: lower the resolution instead.
        while (bucket > -24 && std::max(widthPx, heightPx) * std::exp2(bucket / 4.0) + 2 * kPadding > kMaxTextureSize)
            --bucket;

        return bucket;
    }

    ShapeCache::Entry ShapeCache::rasterize(const swf::DrawList& drawList, const swf::Rect& bounds,
                                            const Matrix& localToDevice) const {
        const double scale = std::exp2(scaleBucket(bounds, localToDevice) / 4.0);
        const int w = static_cast<int>(std::ceil(bounds.width() / 20.0 * scale)) + 2 * kPadding;
        const int h = static_cast<int>(std::ceil(bounds.height() / 20.0 * scale)) + 2 * kPadding;
        vector::Image image(w, h);
        vector::Affine toPixels;
        toPixels.a = toPixels.d = scale / 20.0;
        toPixels.tx = -bounds.xMin / 20.0 * scale + kPadding;
        toPixels.ty = -bounds.yMin / 20.0 * scale + kPadding;
        vector::rasterize(drawList, toPixels, image, [this](std::uint16_t bitmapId) { return library_.bitmap(bitmapId); });

        Entry entry;
        entry.texture = Texture(image);
        entry.textureToLocal = {1.0 / scale, 0, 0, 1.0 / scale, bounds.xMin / 20.0 - kPadding / scale,
                                bounds.yMin / 20.0 - kPadding / scale};
        return entry;
    }

    const ShapeCache::Entry* ShapeCache::get(std::uint16_t id, std::uint16_t ratio, const Matrix& localToDevice) {
        const swf::Rect bounds = library_.bounds(id, ratio);
        const int bucket = scaleBucket(bounds, localToDevice);
        const std::uint64_t key = (static_cast<std::uint64_t>(id) << 32) | (static_cast<std::uint64_t>(ratio) << 8) |
                                  static_cast<std::uint8_t>(bucket);
        if (auto it = entries_.find(key); it != entries_.end())
            return &it->second;

        const swf::DrawList* drawList = library_.drawList(id, ratio);
        if (!drawList || bounds.empty())
            return nullptr;

        return &entries_.emplace(key, rasterize(*drawList, bounds, localToDevice)).first->second;
    }

} // namespace engine
