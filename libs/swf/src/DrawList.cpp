#include "swf/DrawList.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace swf {

    namespace {

        struct Point {
            std::int32_t x = 0;
            std::int32_t y = 0;
            bool operator==(const Point&) const = default;
        };

        /// One edge in absolute twips. Integer coordinates let us join edges by exact
        /// endpoint equality, which is how the SWF format guarantees connectivity.
        struct Edge {
            Point from;
            Point control;
            Point to;
            bool curved = false;

            [[nodiscard]] Edge reversed() const { return {to, control, from, curved}; }
        };

        std::uint64_t pointKey(Point p) {
            return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(p.x)) << 32) | static_cast<std::uint32_t>(p.y);
        }

        PathPoint toPathPoint(Point p) { return {static_cast<float>(p.x), static_cast<float>(p.y)}; }

        void appendEdge(Path& path, const Edge& e) {
            if (e.curved)
                path.commands.push_back({PathCommand::Op::QuadTo, toPathPoint(e.control), toPathPoint(e.to)});
            else
                path.commands.push_back({PathCommand::Op::LineTo, {}, toPathPoint(e.to)});
        }

        /// Chains the edges of one fill region into closed contours.
        Path assembleFill(const std::vector<Edge>& edges) {
            std::unordered_multimap<std::uint64_t, std::size_t> byStart;
            byStart.reserve(edges.size());

            for (std::size_t i = 0; i < edges.size(); ++i)
                byStart.emplace(pointKey(edges[i].from), i);

            std::vector<bool> used(edges.size(), false);
            Path path;

            for (std::size_t first = 0; first < edges.size(); ++first) {
                if (used[first])
                    continue;

                const Point contourStart = edges[first].from;
                path.commands.push_back({PathCommand::Op::MoveTo, {}, toPathPoint(contourStart)});
                std::size_t current = first;

                for (;;) {
                    used[current] = true;
                    appendEdge(path, edges[current]);
                    const Point end = edges[current].to;
                    if (end == contourStart)
                        break;

                    // Continue with the earliest unused edge starting where we are.
                    std::size_t next = edges.size();
                    auto [it, last] = byStart.equal_range(pointKey(end));
                    for (; it != last; ++it) {
                        if (!used[it->second] && it->second < next)
                            next = it->second;
                    }

                    if (next == edges.size())
                        break; // open contour. Filling closes it implicitly

                    current = next;
                }

                path.commands.push_back({PathCommand::Op::Close, {}, {}});
            }

            return path;
        }

        /// Edges grouped by style for one style-table "layer".
        class LayerBuilder {
        public:
            explicit LayerBuilder(StyleTables styles)
                : styles_(std::move(styles)), fills_(styles_.fills.size()), strokes_(styles_.lines.size()) {}

            void addEdge(const Edge& e, std::uint32_t fill0, std::uint32_t fill1, std::uint32_t line) {
                if (fill0 >= 1 && fill0 <= fills_.size())
                    fills_[fill0 - 1].push_back(e.reversed());

                if (fill1 >= 1 && fill1 <= fills_.size())
                    fills_[fill1 - 1].push_back(e);

                if (line >= 1 && line <= strokes_.size()) {
                    Path& path = strokes_[line - 1];
                    const Point lastPoint = lastStrokePoint_[line - 1];
                    if (path.commands.empty() || !(lastPoint == e.from)) {
                        closeIfLooped(line - 1);
                        path.commands.push_back({PathCommand::Op::MoveTo, {}, toPathPoint(e.from)});
                        subpathStart_[line - 1] = e.from;
                    }

                    appendEdge(path, e);
                    lastStrokePoint_[line - 1] = e.to;
                }
            }

            void flushInto(DrawList& out, bool nonZeroWinding) {
                for (std::size_t i = 0; i < fills_.size(); ++i) {
                    if (fills_[i].empty())
                        continue;

                    out.commands.emplace_back(FillCommand{styles_.fills[i], assembleFill(fills_[i]), nonZeroWinding});
                }

                for (std::size_t i = 0; i < strokes_.size(); ++i) {
                    if (strokes_[i].commands.empty())
                        continue;

                    closeIfLooped(i);
                    out.commands.emplace_back(StrokeCommand{styles_.lines[i], std::move(strokes_[i])});
                }
            }

        private:
            /// A stroke that ends where it started is a closed outline: Flash joins
            /// the ends instead of drawing two caps (unless the style says NoClose).
            void closeIfLooped(std::size_t line) {
                Path& path = strokes_[line];
                if (path.commands.empty() || styles_.lines[line].noClose)
                    return;

                if (lastStrokePoint_[line] == subpathStart_[line] && path.commands.back().op != PathCommand::Op::Close &&
                    path.commands.back().op != PathCommand::Op::MoveTo) {
                    path.commands.push_back({PathCommand::Op::Close, {}, {}});
                }
            }

            StyleTables styles_; // declared first: the other members are sized from it
            std::vector<std::vector<Edge>> fills_;
            std::vector<Path> strokes_;
            std::unordered_map<std::size_t, Point> lastStrokePoint_;
            std::unordered_map<std::size_t, Point> subpathStart_;
        };

    } // namespace

    DrawList buildDrawList(const ShapeGeometry& geometry, bool nonZeroWinding) {
        DrawList out;
        auto builder = std::make_unique<LayerBuilder>(geometry.styles);

        Point pen;
        std::uint32_t fill0 = 0, fill1 = 0, line = 0;

        for (const auto& record : geometry.records) {
            if (const auto* change = std::get_if<StyleChangeRecord>(&record)) {
                if (change->newStyles) {
                    builder->flushInto(out, nonZeroWinding);
                    builder = std::make_unique<LayerBuilder>(*change->newStyles);
                    fill0 = fill1 = line = 0;
                }

                if (change->moveTo)
                    pen = {change->moveTo->first, change->moveTo->second};

                if (change->fill0)
                    fill0 = *change->fill0;

                if (change->fill1)
                    fill1 = *change->fill1;

                if (change->line)
                    line = *change->line;
            } else if (const auto* s = std::get_if<StraightEdgeRecord>(&record)) {
                const Point to{pen.x + s->dx, pen.y + s->dy};
                builder->addEdge({pen, pen, to, false}, fill0, fill1, line);
                pen = to;
            } else {
                const auto& c = std::get<CurvedEdgeRecord>(record);
                const Point control{pen.x + c.controlDx, pen.y + c.controlDy};
                const Point to{control.x + c.anchorDx, control.y + c.anchorDy};
                builder->addEdge({pen, control, to, true}, fill0, fill1, line);
                pen = to;
            }
        }

        builder->flushInto(out, nonZeroWinding);
        return out;
    }

} // namespace swf

// /----- HIT TESTING -----/

namespace swf {

    namespace {

        /// Calls `segment(from, to)` for every straight piece of the path, with
        /// quadratic curves flattened into a few segments (enough for picking).
        template <typename SegmentFn>
        void forEachSegment(const Path& path, SegmentFn&& segment) {
            PathPoint pen{}, contourStart{};

            for (const PathCommand& c : path.commands) {
                switch (c.op) {
                case PathCommand::Op::MoveTo:
                    pen = contourStart = c.to;
                    break;
                case PathCommand::Op::LineTo:
                    segment(pen, c.to);
                    pen = c.to;
                    break;
                case PathCommand::Op::QuadTo: {
                    constexpr int kSteps = 8;
                    PathPoint previous = pen;

                    for (int i = 1; i <= kSteps; ++i) {
                        const float t = static_cast<float>(i) / kSteps, u = 1 - t;
                        const PathPoint p{u * u * pen.x + 2 * u * t * c.control.x + t * t * c.to.x,
                                          u * u * pen.y + 2 * u * t * c.control.y + t * t * c.to.y};
                        segment(previous, p);
                        previous = p;
                    }

                    pen = c.to;
                    break;
                }
                case PathCommand::Op::Close:
                    segment(pen, contourStart);
                    pen = contourStart;
                    break;
                }
            }
        }

        bool insideFill(const FillCommand& fill, float x, float y) {
            // Ray casting towards +x, counting signed crossings for the winding rule.
            int winding = 0;

            forEachSegment(fill.path, [&](PathPoint a, PathPoint b) {
                if ((a.y <= y) == (b.y <= y))
                    return; // doesn't cross the ray's line

                const float crossX = a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y);
                if (crossX > x)
                    winding += b.y > a.y ? 1 : -1;
            });

            return fill.nonZeroWinding ? winding != 0 : (winding & 1) != 0;
        }

        bool onStroke(const StrokeCommand& stroke, float x, float y) {
            const float halfWidth = std::max(static_cast<float>(stroke.style.width), 20.0f) / 2;
            bool hit = false;

            forEachSegment(stroke.path, [&](PathPoint a, PathPoint b) {
                if (hit)
                    return;

                const float dx = b.x - a.x, dy = b.y - a.y;
                const float lengthSquared = dx * dx + dy * dy;
                float t = lengthSquared > 0 ? ((x - a.x) * dx + (y - a.y) * dy) / lengthSquared : 0;
                t = std::clamp(t, 0.0f, 1.0f);
                const float px = a.x + t * dx - x, py = a.y + t * dy - y;
                hit = px * px + py * py <= halfWidth * halfWidth;
            });

            return hit;
        }

    } // namespace

    bool hitTest(const DrawList& drawList, float x, float y) {
        for (const DrawCommand& command : drawList.commands) {
            if (const auto* fill = std::get_if<FillCommand>(&command)) {
                if (insideFill(*fill, x, y))
                    return true;
            } else if (onStroke(std::get<StrokeCommand>(command), x, y)) {
                return true;
            }
        }

        return false;
    }

} // namespace swf
