#include "BitmapDecoder.h"

#include "swf/BitReader.h"

#include <stb/stb_image.h>
#include <zlib.h>

#include <algorithm>
#include <memory>
#include <string>

namespace assetpack {

    namespace {

        using Bytes = std::span<const std::uint8_t>;

        std::vector<std::uint8_t> inflateExactly(Bytes compressed, std::size_t size) {
            std::vector<std::uint8_t> out(size);
            uLongf outSize = static_cast<uLongf>(size);
            const int rc = ::uncompress(out.data(), &outSize, compressed.data(), static_cast<uLong>(compressed.size()));

            // Z_BUF_ERROR: the stream holds more than we asked for (row padding of
            // the last row may be missing or extra). What we have is enough.
            if (rc != Z_OK && rc != Z_BUF_ERROR)
                throw swf::FormatError("zlib error " + std::to_string(rc) + " in bitmap");

            if (outSize < size)
                std::fill(out.begin() + static_cast<std::ptrdiff_t>(outSize), out.end(), 0);

            return out;
        }

        bool startsWith(Bytes data, std::initializer_list<std::uint8_t> prefix) {
            return data.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), data.begin());
        }

        /// Flash tolerates JPEG streams that start with an extra EOI+SOI pair
        /// (FF D9 FF D8) left over by old authoring tools. Decoders don't.
        std::vector<std::uint8_t> cleanJpeg(Bytes data) {
            std::vector<std::uint8_t> out;
            out.reserve(data.size());
            std::size_t i = 0;

            if (startsWith(data, {0xFF, 0xD9, 0xFF, 0xD8}))
                i = 4;

            out.insert(out.end(), data.begin() + static_cast<std::ptrdiff_t>(i), data.end());

            // Some files also embed an EOI+SOI pair in the middle (tables and image
            // were concatenated). Remove it so the stream is one JPEG.
            for (std::size_t k = 2; k + 3 < out.size(); ++k) {
                if (out[k] == 0xFF && out[k + 1] == 0xD9 && out[k + 2] == 0xFF && out[k + 3] == 0xD8) {
                    out.erase(out.begin() + static_cast<std::ptrdiff_t>(k), out.begin() + static_cast<std::ptrdiff_t>(k + 4));
                    break;
                }
            }

            return out;
        }

        RgbaImage decodeWithStb(const std::vector<std::uint8_t>& encoded) {
            int w = 0, h = 0, channels = 0;
            std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
                stbi_load_from_memory(encoded.data(), static_cast<int>(encoded.size()), &w, &h, &channels, 4),
                &stbi_image_free);
            if (!pixels)
                throw swf::FormatError(std::string("image decoding failed: ") + stbi_failure_reason());

            RgbaImage image;
            image.width = w;
            image.height = h;
            image.pixels.assign(pixels.get(), pixels.get() + static_cast<std::size_t>(w) * h * 4);

            return image;
        }

        /// Lossless2 colors are premultiplied by alpha. PNG wants straight alpha.
        void unpremultiply(std::uint8_t* rgba) {
            const int a = rgba[3];
            if (a == 0 || a == 255)
                return;

            for (int c = 0; c < 3; ++c)
                rgba[c] = static_cast<std::uint8_t>(std::min(255, (rgba[c] * 255 + a / 2) / a));
        }

        RgbaImage decodeLossless(const swf::BitmapDef& b) {
            const bool hasAlpha = b.kind == swf::BitmapDef::Kind::LosslessAlpha;
            RgbaImage image;
            image.width = b.width;
            image.height = b.height;
            image.pixels.resize(static_cast<std::size_t>(b.width) * b.height * 4);
            const std::size_t pixelCount = static_cast<std::size_t>(b.width) * b.height;

            switch (b.losslessFormat) {
            case 3: { // 8-bit palette indices, rows padded to 32 bits
                const std::size_t paletteSize = static_cast<std::size_t>(b.colorTableSize) + 1;
                const std::size_t entryBytes = hasAlpha ? 4 : 3;
                const std::size_t rowBytes = (static_cast<std::size_t>(b.width) + 3) & ~std::size_t{3};
                const auto raw = inflateExactly(b.imageData, paletteSize * entryBytes + rowBytes * b.height);

                for (int y = 0; y < b.height; ++y) {
                    for (int x = 0; x < b.width; ++x) {
                        const std::size_t index = raw[paletteSize * entryBytes + y * rowBytes + x];
                        std::uint8_t* out = &image.pixels[(static_cast<std::size_t>(y) * b.width + x) * 4];

                        if (index >= paletteSize) {
                            out[0] = out[1] = out[2] = out[3] = 0;
                            continue;
                        }

                        const std::uint8_t* entry = &raw[index * entryBytes];
                        out[0] = entry[0];
                        out[1] = entry[1];
                        out[2] = entry[2];
                        out[3] = hasAlpha ? entry[3] : 255;

                        if (hasAlpha)
                            unpremultiply(out);
                    }
                }
                break;
            }
            case 4: { // PIX15: 0RRRRRGGGGGBBBBB, big-endian, rows padded to 32 bits
                const std::size_t rowBytes = (static_cast<std::size_t>(b.width) * 2 + 3) & ~std::size_t{3};
                const auto raw = inflateExactly(b.imageData, rowBytes * b.height);

                for (int y = 0; y < b.height; ++y) {
                    for (int x = 0; x < b.width; ++x) {
                        const std::size_t i = y * rowBytes + static_cast<std::size_t>(x) * 2;
                        const unsigned v = (raw[i] << 8) | raw[i + 1];
                        std::uint8_t* out = &image.pixels[(static_cast<std::size_t>(y) * b.width + x) * 4];
                        out[0] = static_cast<std::uint8_t>(((v >> 10) & 0x1F) * 255 / 31);
                        out[1] = static_cast<std::uint8_t>(((v >> 5) & 0x1F) * 255 / 31);
                        out[2] = static_cast<std::uint8_t>((v & 0x1F) * 255 / 31);
                        out[3] = 255;
                    }
                }
                break;
            }
            case 5: { // 32-bit ARGB (alpha ignored for DefineBitsLossless)
                const auto raw = inflateExactly(b.imageData, pixelCount * 4);

                for (std::size_t i = 0; i < pixelCount; ++i) {
                    std::uint8_t* out = &image.pixels[i * 4];
                    out[0] = raw[i * 4 + 1];
                    out[1] = raw[i * 4 + 2];
                    out[2] = raw[i * 4 + 3];
                    out[3] = hasAlpha ? raw[i * 4] : 255;

                    if (hasAlpha)
                        unpremultiply(out);
                }
                break;
            }
            default:
                throw swf::FormatError("unknown lossless bitmap format " + std::to_string(b.losslessFormat));
            }

            return image;
        }

    } // namespace

    RgbaImage decodeBitmap(const swf::BitmapDef& bitmap) {
        using Kind = swf::BitmapDef::Kind;

        switch (bitmap.kind) {
        case Kind::Lossless:
        case Kind::LosslessAlpha:
            return decodeLossless(bitmap);
        case Kind::Jpeg: {
            // Tables (SOI ... EOI) + image (SOI ... EOI): drop the tables' EOI and
            // the image's SOI to form one stream.
            std::vector<std::uint8_t> joined = cleanJpeg(bitmap.jpegTables);
            if (joined.size() >= 2 && joined[joined.size() - 2] == 0xFF && joined.back() == 0xD9)
                joined.resize(joined.size() - 2);

            std::vector<std::uint8_t> image = cleanJpeg(bitmap.imageData);
            const std::size_t skip = (image.size() >= 2 && image[0] == 0xFF && image[1] == 0xD8 && !joined.empty()) ? 2 : 0;
            joined.insert(joined.end(), image.begin() + static_cast<std::ptrdiff_t>(skip), image.end());

            return decodeWithStb(joined);
        }
        case Kind::EncodedImage:
        case Kind::JpegWithAlpha: {
            RgbaImage image = decodeWithStb(cleanJpeg(bitmap.imageData));

            if (bitmap.kind == Kind::JpegWithAlpha && !bitmap.alphaData.empty()) {
                const std::size_t pixelCount = static_cast<std::size_t>(image.width) * image.height;

                // The authoring tool premultiplies the colors before JPEG
                // encoding, and compression noise can push a component above its
                // alpha. Flash clamps it. We do the same, then store straight
                // alpha like every other PNG.
                const auto alpha = inflateExactly(bitmap.alphaData, pixelCount);

                for (std::size_t i = 0; i < pixelCount; ++i) {
                    std::uint8_t* p = &image.pixels[i * 4];
                    p[3] = alpha[i];
                    for (int c = 0; c < 3; ++c)
                        p[c] = std::min(p[c], p[3]);

                    unpremultiply(p);
                }
            }

            return image;
        }
        }

        throw swf::FormatError("unhandled bitmap kind");
    }

} // namespace assetpack
