// Shape.h - vector geometry of DefineShape / DefineMorphShape / font glyphs.
//
// SWF shapes are stored as a stream of drawing records. Each edge separates two
// fill regions: `fill0` on its left and `fill1` on its right (relative to the
// drawing direction), and is optionally stroked with `line`. A record stream
// can switch styles, move the pen, and even replace the style tables midway.
//
// This header only *decodes* that stream (parseShape). Turning edges into
// closed, fillable paths is done by buildPaths() in ShapePaths.h.
#pragma once

#include "swf/Types.h"

#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace swf {

    enum class SpreadMode : std::uint8_t { Pad = 0, Reflect = 1, Repeat = 2 };
    enum class InterpolationMode : std::uint8_t { Rgb = 0, LinearRgb = 1 };

    struct GradientRecord {
        std::uint8_t ratio = 0; ///< Position along the gradient, 0..255.
        Rgba color;
    };

    /// The gradient square spans -16384..16384 twips in both axes before
    /// `matrix` maps it to shape space.
    struct Gradient {
        Matrix matrix;
        SpreadMode spread = SpreadMode::Pad;
        InterpolationMode interpolation = InterpolationMode::Rgb;
        std::vector<GradientRecord> records;
        float focalPoint = 0; ///< Focal radial gradients only, -1..1.
    };

    struct FillStyle {
        enum class Type : std::uint8_t {
            Solid = 0x00,
            LinearGradient = 0x10,
            RadialGradient = 0x12,
            FocalRadialGradient = 0x13,
            RepeatingBitmap = 0x40,
            ClippedBitmap = 0x41,
            NonSmoothedRepeatingBitmap = 0x42,
            NonSmoothedClippedBitmap = 0x43,
        };

        Type type = Type::Solid;
        Rgba color;               ///< Solid fills.
        Gradient gradient;        ///< Gradient fills.
        CharacterId bitmapId = 0; ///< Bitmap fills, 0xFFFF means "no bitmap".
        Matrix bitmapMatrix;      ///< Maps bitmap pixels (in twips) to shape space.

        [[nodiscard]] bool isGradient() const {
            return type == Type::LinearGradient || type == Type::RadialGradient || type == Type::FocalRadialGradient;
        }

        [[nodiscard]] bool isBitmap() const { return static_cast<std::uint8_t>(type) >= 0x40; }
        [[nodiscard]] bool bitmapRepeats() const {
            return type == Type::RepeatingBitmap || type == Type::NonSmoothedRepeatingBitmap;
        }

        [[nodiscard]] bool bitmapSmoothed() const {
            return type == Type::RepeatingBitmap || type == Type::ClippedBitmap;
        }
    };

    enum class CapStyle : std::uint8_t { Round = 0, None = 1, Square = 2 };
    enum class JoinStyle : std::uint8_t { Round = 0, Bevel = 1, Miter = 2 };

    struct LineStyle {
        std::uint16_t width = 0; ///< Twips, 0 is a hairline (always 1 px).
        Rgba color;
        CapStyle startCap = CapStyle::Round;
        CapStyle endCap = CapStyle::Round;
        JoinStyle join = JoinStyle::Round;
        float miterLimit = 3; ///< Multiple of the half width (LINESTYLE2 only).
        bool noHScale = false;
        bool noVScale = false;
        bool pixelHinting = false;
        bool noClose = false;
        std::optional<FillStyle> fill; ///< LINESTYLE2 may stroke with any fill.
    };

    struct StyleTables {
        std::vector<FillStyle> fills;
        std::vector<LineStyle> lines;
    };

    /// Pen state change. Style indices are 1-based into the current tables, 0
    /// means "none". std::nullopt means "unchanged".
    struct StyleChangeRecord {
        std::optional<std::pair<std::int32_t, std::int32_t>> moveTo; ///< Absolute, twips.
        std::optional<std::uint32_t> fill0;
        std::optional<std::uint32_t> fill1;
        std::optional<std::uint32_t> line;
        std::optional<StyleTables> newStyles; ///< Replaces the tables from here on.
    };

    struct StraightEdgeRecord {
        std::int32_t dx = 0, dy = 0;
    };

    struct CurvedEdgeRecord {
        std::int32_t controlDx = 0, controlDy = 0;
        std::int32_t anchorDx = 0, anchorDy = 0;
    };

    using ShapeRecord = std::variant<StyleChangeRecord, StraightEdgeRecord, CurvedEdgeRecord>;

    struct ShapeGeometry {
        StyleTables styles; ///< Initial tables (empty for glyphs).
        std::vector<ShapeRecord> records;
    };

    /// Decodes SHAPEWITHSTYLE as found in DefineShape1..4 (`shapeVersion` 1..4).
    ShapeGeometry parseShapeWithStyle(std::span<const std::uint8_t> records, int shapeVersion);

    /// Decodes a font glyph (SHAPE without style tables). Glyphs implicitly use a
    /// single solid fill, index 1.
    ShapeGeometry parseGlyph(std::span<const std::uint8_t> records);

    // /----- MORPH SHAPES -----/

    /// Morph fill/line styles carry a start and an end value for every field.
    struct MorphFillStyle {
        FillStyle start;
        FillStyle end;
    };

    struct MorphLineStyle {
        LineStyle start;
        LineStyle end;
    };

    struct MorphShapeGeometry {
        std::vector<MorphFillStyle> fills;
        std::vector<MorphLineStyle> lines;
        ShapeGeometry startEdges; ///< Carries the style changes.
        ShapeGeometry endEdges;   ///< Same edges in the same order, end positions only.
    };

    /// Decodes the payload of DefineMorphShape (version 1) / DefineMorphShape2.
    MorphShapeGeometry parseMorphShape(std::span<const std::uint8_t> records, int morphVersion);

    /// Interpolates a morph shape at `ratio` in [0, 1] into a regular shape.
    ShapeGeometry interpolateMorph(const MorphShapeGeometry& morph, double ratio);

} // namespace swf
