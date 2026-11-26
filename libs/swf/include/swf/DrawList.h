// DrawList.h - a shape as an ordered list of filled and stroked paths.
//
// This is the form every renderer understands: "fill this closed path with
// that paint, then stroke this polyline with that pen". buildDrawList()
// produces it from the SWF edge representation (Shape.h); the asset pack
// stores it; the engine rasterizes it.
//
// Coordinates are twips, in the shape's own space.
#pragma once

#include "swf/Shape.h"

#include <cstdint>
#include <variant>
#include <vector>

namespace swf {

    struct PathPoint {
        float x = 0;
        float y = 0;
    };

    struct PathCommand {
        enum class Op : std::uint8_t { MoveTo, LineTo, QuadTo, Close };
        Op op = Op::MoveTo;
        PathPoint control; ///< QuadTo only.
        PathPoint to;
    };

    struct Path {
        std::vector<PathCommand> commands;
    };

    struct FillCommand {
        FillStyle style;
        Path path;
        bool nonZeroWinding = false; ///< Even-odd unless the shape asks otherwise.
    };

    struct StrokeCommand {
        LineStyle style;
        Path path;
    };

    using DrawCommand = std::variant<FillCommand, StrokeCommand>;

    struct DrawList {
        std::vector<DrawCommand> commands; ///< Painted in order.
    };

    /// Converts decoded edges into closed fill paths and stroke polylines.
    ///
    /// Flash rules reproduced here:
    ///   * an edge belongs to the region of `fill1` as drawn and to the region of
    ///     `fill0` reversed, so every region boundary is assembled from edges
    ///     that may have been drawn in any order;
    ///   * a style-table change starts a new layer painted above the previous
    ///     ones;
    ///   * within a layer, fills are painted in style order, then strokes.
    DrawList buildDrawList(const ShapeGeometry& geometry, bool nonZeroWinding = false);

} // namespace swf

namespace swf {

    /// Does the point (twips, shape space) fall inside a filled region or on a
    /// stroke of the draw list? This is Flash's "shape flag" hit test, used to
    /// find what is under the mouse. Curves are flattened; strokes count with
    /// their full width (hairlines as 1 px).
    bool hitTest(const DrawList& drawList, float x, float y);

} // namespace swf
