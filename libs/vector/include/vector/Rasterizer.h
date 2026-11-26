// Rasterizer.h - draws a swf::DrawList into an RGBA image with cairo.
//
// Vector art is rasterized on the CPU (cairo: exact-coverage anti-aliasing,
// every gradient/stroke feature SWF needs) and the result is uploaded to the
// GPU as a texture. Each shape is rasterized once per display scale, which is
// cheap compared to tessellating and anti-aliasing on an old integrated GPU.
#pragma once

#include "swf/DrawList.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace vector {

/// 2D affine transform in the Flash layout (see swf::Matrix) with a floating
/// point translation.
struct Affine {
    double a = 1, b = 0, c = 0, d = 1, tx = 0, ty = 0;
};

/// Premultiplied 8-bit pixels, rows top to bottom, no padding, stored as one
/// native-endian 0xAARRGGBB word per pixel: B, G, R, A bytes on x86. That is
/// cairo's ARGB32 layout and uploads to OpenGL as GL_BGRA without conversion.
struct Image {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels; ///< size = width * height * 4

    Image() = default;
    Image(int w, int h) : width(w), height(h), pixels(static_cast<std::size_t>(w) * h * 4, 0) {}
};

/// Supplies the pixels of a bitmap used as a fill, or
/// nullptr if the id is unknown.
using BitmapResolver = std::function<const Image*(std::uint16_t bitmapId)>;

/// Paints `drawList` into `target`. `toPixels` maps shape space (twips) to
/// target pixels. The target is not cleared, so several shapes can be
/// composed into one image.
void rasterize(const swf::DrawList& drawList, const Affine& toPixels, Image& target,
               const BitmapResolver& bitmaps = {});

} // namespace vector
