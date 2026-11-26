// Types.h - plain data types shared by every SWF record.
//
// Units: all coordinates stay in *twips* (1/20 pixel), exactly as stored in the
// file. Converting to pixels is the job of the consumer; keeping twips here
// avoids rounding drift when matrices are composed.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace swf {

    using CharacterId = std::uint16_t;
    using Depth = std::uint16_t;

    constexpr int kTwipsPerPixel = 20;

    struct Rect {
        std::int32_t xMin = 0;
        std::int32_t xMax = 0;
        std::int32_t yMin = 0;
        std::int32_t yMax = 0;

        [[nodiscard]] std::int32_t width() const { return xMax - xMin; }
        [[nodiscard]] std::int32_t height() const { return yMax - yMin; }
        [[nodiscard]] bool empty() const { return xMax <= xMin || yMax <= yMin; }
    };

    struct Rgba {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 255;
    };

    /// 2D affine transform, Flash convention:
    ///   x' = a * x + c * y + tx
    ///   y' = b * x + d * y + ty
    /// (a, d) are ScaleX/ScaleY, (b, c) are RotateSkew0/RotateSkew1 in the spec.
    /// Translation is in twips.
    struct Matrix {
        double a = 1.0;
        double b = 0.0;
        double c = 0.0;
        double d = 1.0;
        std::int32_t tx = 0;
        std::int32_t ty = 0;
    };

    /// Color transform with 8.8 fixed-point multipliers (256 == 1.0) and additive
    /// terms in [-255, 255], as stored by CXFORMWITHALPHA.
    struct ColorTransform {
        std::int16_t rMul = 256, gMul = 256, bMul = 256, aMul = 256;
        std::int16_t rAdd = 0, gAdd = 0, bAdd = 0, aAdd = 0;

        [[nodiscard]] bool isIdentity() const {
            return rMul == 256 && gMul == 256 && bMul == 256 && aMul == 256 && rAdd == 0 && gAdd == 0 && bAdd == 0 &&
                   aAdd == 0;
        }
    };

    /// Values match the SWF BlendMode byte (0 and 1 both mean "normal").
    enum class BlendMode : std::uint8_t {
        Normal = 1,
        Layer = 2,
        Multiply = 3,
        Screen = 4,
        Lighten = 5,
        Darken = 6,
        Difference = 7,
        Add = 8,
        Subtract = 9,
        Invert = 10,
        Alpha = 11,
        Erase = 12,
        Overlay = 13,
        HardLight = 14,
    };

    // /----- FILTERS -----/

    // Field names follow flash.filters.* so the runtime code reads like the AS3
    // it replaces. Angles are radians, distances and blur sizes are pixels.

    struct DropShadowFilter {
        Rgba color;
        float blurX = 4, blurY = 4;
        float angle = 0.785398f;
        float distance = 4;
        float strength = 1;
        bool inner = false;
        bool knockout = false;
        bool compositeSource = true;
        int passes = 1;
    };

    struct BlurFilter {
        float blurX = 4, blurY = 4;
        int passes = 1;
    };

    struct GlowFilter {
        Rgba color;
        float blurX = 6, blurY = 6;
        float strength = 2;
        bool inner = false;
        bool knockout = false;
        bool compositeSource = true;
        int passes = 1;
    };

    struct BevelFilter {
        Rgba shadowColor;
        Rgba highlightColor;
        float blurX = 4, blurY = 4;
        float angle = 0.785398f;
        float distance = 4;
        float strength = 1;
        bool inner = true;
        bool knockout = false;
        bool compositeSource = true;
        bool onTop = false;
        int passes = 1;
    };

    struct GradientStop {
        Rgba color;
        std::uint8_t ratio = 0;
    };

    /// GradientGlow and GradientBevel share their binary layout.
    struct GradientFilter {
        bool isBevel = false;
        std::vector<GradientStop> stops;
        float blurX = 4, blurY = 4;
        float angle = 0.785398f;
        float distance = 4;
        float strength = 1;
        bool inner = false;
        bool knockout = false;
        bool compositeSource = true;
        bool onTop = false;
        int passes = 1;
    };

    struct ConvolutionFilter {
        int matrixX = 0, matrixY = 0;
        float divisor = 1, bias = 0;
        std::vector<float> matrix;
        Rgba defaultColor;
        bool clamp = true;
        bool preserveAlpha = true;
    };

    struct ColorMatrixFilter {
        /// Row-major 4x5 matrix; the 5th column is an additive offset in [0,255].
        std::array<float, 20> matrix{1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0};
    };

    using Filter = std::variant<DropShadowFilter, BlurFilter, GlowFilter, BevelFilter, GradientFilter, ConvolutionFilter,
                                ColorMatrixFilter>;

} // namespace swf
