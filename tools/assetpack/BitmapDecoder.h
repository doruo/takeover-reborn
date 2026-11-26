// BitmapDecoder.h - turns any SWF bitmap tag into plain RGBA pixels.
#pragma once

#include "swf/Characters.h"

#include <cstdint>
#include <vector>

namespace assetpack {

/// Straight (non-premultiplied) 8-bit RGBA, rows top to bottom. This is what
/// PNG stores, so decoded bitmaps can be edited with any image editor.
struct RgbaImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;
};

/// Decodes every bitmap kind found in SWF files:
///   * DefineBits + JPEGTables, DefineBitsJPEG2 (JPEG, PNG or GIF payload),
///   * DefineBitsJPEG3 (JPEG + separate zlib alpha plane),
///   * DefineBitsLossless / Lossless2 (palette, 15-bit and 32-bit formats).
/// Throws swf::FormatError on corrupt data.
RgbaImage decodeBitmap(const swf::BitmapDef& bitmap);

} // namespace assetpack
