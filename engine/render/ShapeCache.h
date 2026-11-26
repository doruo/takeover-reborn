// ShapeCache.h - vector characters rasterized to textures, on demand.
//
// A shape is rasterized at the resolution it is displayed at, rounded up to
// the next quarter octave (2^(k/4)), so small scale changes reuse a texture
// while large ones get a sharp one. The texture covers the shape's bounds plus
// a one-pixel transparent border that keeps bilinear filtering from cutting
// the anti-aliased edge.
#pragma once

#include "engine/assets/Library.h"
#include "engine/geom/Matrix.h"
#include "engine/render/Texture.h"

#include <cstdint>
#include <unordered_map>

namespace engine {

    class ShapeCache {
    public:
        explicit ShapeCache(Library& library) : library_(library) {}

        struct Entry {
            Texture texture;
            Matrix textureToLocal; ///< Maps texels to the character's own space (pixels).
        };

        /// Returns the raster of character `id` (shape, morph shape at `ratio`, or
        /// static text) suited to being drawn with `localToDevice`, or nullptr if
        /// the character has nothing to draw.
        const Entry* get(std::uint16_t id, std::uint16_t ratio, const Matrix& localToDevice);

        /// Rasterizes any draw list (twips, bounds `bounds`) at the resolution
        /// suited to `localToDevice`. Used for content that is not a library
        /// character, such as dynamic text.
        [[nodiscard]] Entry rasterize(const swf::DrawList& drawList, const swf::Rect& bounds,
                                      const Matrix& localToDevice) const;

        /// The quarter-octave resolution bucket rasterize() would use.
        [[nodiscard]] static int scaleBucket(const swf::Rect& bounds, const Matrix& localToDevice);

        /// Drops every texture (e.g. when the window scale changes a lot).
        void clear() { entries_.clear(); }

        [[nodiscard]] std::size_t size() const { return entries_.size(); }

    private:
        Library& library_;
        std::unordered_map<std::uint64_t, Entry> entries_;
    };

} // namespace engine
