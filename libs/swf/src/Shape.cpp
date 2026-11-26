#include "swf/Shape.h"

#include "Records.h"
#include "swf/BitReader.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace swf {

    namespace {

        using detail::readMatrix;
        using detail::readRgb;
        using detail::readRgba;

        std::size_t readStyleCount(BitReader& in, bool allowExtended) {
            std::size_t count = in.u8();
            if (count == 0xFF && allowExtended)
                count = in.u16();

            return count;
        }

        Gradient readGradient(BitReader& in, int shapeVersion, bool focal) {
            Gradient g;
            in.align();
            g.spread = static_cast<SpreadMode>(in.ub(2));
            g.interpolation = static_cast<InterpolationMode>(in.ub(2));
            const unsigned count = in.ub(4);
            g.records.resize(count);

            for (auto& r : g.records) {
                r.ratio = in.u8();
                r.color = shapeVersion >= 3 ? readRgba(in) : readRgb(in);
            }

            if (focal)
                g.focalPoint = in.fixed8();

            return g;
        }

        FillStyle readFillStyle(BitReader& in, int shapeVersion) {
            FillStyle f;
            f.type = static_cast<FillStyle::Type>(in.u8());
            switch (f.type) {
            case FillStyle::Type::Solid:
                f.color = shapeVersion >= 3 ? readRgba(in) : readRgb(in);
                break;
            case FillStyle::Type::LinearGradient:
            case FillStyle::Type::RadialGradient:
            case FillStyle::Type::FocalRadialGradient:
            {
                const Matrix gradientMatrix = readMatrix(in);
                f.gradient = readGradient(in, shapeVersion, f.type == FillStyle::Type::FocalRadialGradient);
                f.gradient.matrix = gradientMatrix;
                break;
            }
            case FillStyle::Type::RepeatingBitmap:
            case FillStyle::Type::ClippedBitmap:
            case FillStyle::Type::NonSmoothedRepeatingBitmap:
            case FillStyle::Type::NonSmoothedClippedBitmap:
                f.bitmapId = in.u16();
                f.bitmapMatrix = readMatrix(in);
                break;
            default:
                throw FormatError("unknown fill style type " + std::to_string(static_cast<int>(f.type)));
            }

            return f;
        }

        LineStyle readLineStyle(BitReader& in, int shapeVersion) {
            LineStyle l;
            l.width = in.u16();

            if (shapeVersion < 4) {
                l.color = shapeVersion >= 3 ? readRgba(in) : readRgb(in);
                return l;
            }

            l.startCap = static_cast<CapStyle>(in.ub(2));
            l.join = static_cast<JoinStyle>(in.ub(2));
            const bool hasFill = in.flag();
            l.noHScale = in.flag();
            l.noVScale = in.flag();
            l.pixelHinting = in.flag();
            in.ub(5);
            l.noClose = in.flag();
            l.endCap = static_cast<CapStyle>(in.ub(2));

            if (l.join == JoinStyle::Miter)
                l.miterLimit = in.fixed8();

            if (hasFill) {
                l.fill = readFillStyle(in, shapeVersion);
                if (l.fill->type == FillStyle::Type::Solid)
                    l.color = l.fill->color;
            } else {
                l.color = readRgba(in);
            }

            return l;
        }

        StyleTables readStyleTables(BitReader& in, int shapeVersion) {
            StyleTables t;
            const bool extended = shapeVersion >= 2;
            t.fills.resize(readStyleCount(in, extended));

            for (auto& f : t.fills)
                f = readFillStyle(in, shapeVersion);

            t.lines.resize(readStyleCount(in, extended));

            for (auto& l : t.lines)
                l = readLineStyle(in, shapeVersion);

            return t;
        }

        /// Reads NumFillBits/NumLineBits and the record stream that follows.
        /// `shapeVersion` 0 means "glyph or morph edges": no style tables can appear.
        std::vector<ShapeRecord> readShapeRecords(BitReader& in, int shapeVersion) {
            in.align();
            unsigned fillBits = in.ub(4);
            unsigned lineBits = in.ub(4);
            std::vector<ShapeRecord> records;
            std::int32_t penX = 0, penY = 0;

            for (;;) {
                if (in.bytePosition() >= in.size())
                    break; // tolerate a missing end record

                const bool isEdge = in.flag();
                if (!isEdge) {
                    const unsigned flags = in.ub(5);
                    if (flags == 0)
                        break; // EndShapeRecord

                    StyleChangeRecord change;

                    if (flags & 0x01) {
                        const unsigned bits = in.ub(5);
                        penX = in.sb(bits);
                        penY = in.sb(bits);
                        change.moveTo = std::make_pair(penX, penY);
                    }

                    if (flags & 0x02)
                        change.fill0 = in.ub(fillBits);

                    if (flags & 0x04)
                        change.fill1 = in.ub(fillBits);

                    if (flags & 0x08)
                        change.line = in.ub(lineBits);

                    if ((flags & 0x10) && shapeVersion >= 2) {
                        change.newStyles = readStyleTables(in, shapeVersion);
                        fillBits = in.ub(4);
                        lineBits = in.ub(4);
                    }

                    records.emplace_back(std::move(change));
                    continue;
                }

                const bool straight = in.flag();
                const unsigned bits = in.ub(4) + 2;

                if (straight) {
                    StraightEdgeRecord e;

                    if (in.flag()) { // general line
                        e.dx = in.sb(bits);
                        e.dy = in.sb(bits);
                    } else if (in.flag()) { // vertical
                        e.dy = in.sb(bits);
                    } else {
                        e.dx = in.sb(bits);
                    }

                    penX += e.dx;
                    penY += e.dy;
                    records.emplace_back(e);
                } else {
                    CurvedEdgeRecord e;
                    e.controlDx = in.sb(bits);
                    e.controlDy = in.sb(bits);
                    e.anchorDx = in.sb(bits);
                    e.anchorDy = in.sb(bits);
                    penX += e.controlDx + e.anchorDx;
                    penY += e.controlDy + e.anchorDy;
                    records.emplace_back(e);
                }
            }

            return records;
        }

        // /----- MORPH STYLES -----/

        MorphFillStyle readMorphFillStyle(BitReader& in) {
            MorphFillStyle m;
            const auto type = static_cast<FillStyle::Type>(in.u8());
            m.start.type = m.end.type = type;
            switch (type) {
            case FillStyle::Type::Solid:
                m.start.color = readRgba(in);
                m.end.color = readRgba(in);
                break;
            case FillStyle::Type::LinearGradient:
            case FillStyle::Type::RadialGradient:
            case FillStyle::Type::FocalRadialGradient: {
                m.start.gradient.matrix = readMatrix(in);
                m.end.gradient.matrix = readMatrix(in);
                in.align();
                const SpreadMode spread = static_cast<SpreadMode>(in.ub(2));
                const InterpolationMode interpolation = static_cast<InterpolationMode>(in.ub(2));
                const unsigned count = in.ub(4);

                for (Gradient* g : {&m.start.gradient, &m.end.gradient}) {
                    g->spread = spread;
                    g->interpolation = interpolation;
                    g->records.resize(count);
                }

                for (unsigned i = 0; i < count; ++i) {
                    m.start.gradient.records[i].ratio = in.u8();
                    m.start.gradient.records[i].color = readRgba(in);
                    m.end.gradient.records[i].ratio = in.u8();
                    m.end.gradient.records[i].color = readRgba(in);
                }

                if (type == FillStyle::Type::FocalRadialGradient) {
                    m.start.gradient.focalPoint = in.fixed8();
                    m.end.gradient.focalPoint = in.fixed8();
                }
                break;
            }
            default:
                if (!m.start.isBitmap())
                    throw FormatError("unknown morph fill type");

                m.start.bitmapId = m.end.bitmapId = in.u16();
                m.start.bitmapMatrix = readMatrix(in);
                m.end.bitmapMatrix = readMatrix(in);
                break;
            }

            return m;
        }

        MorphLineStyle readMorphLineStyle(BitReader& in, int morphVersion) {
            MorphLineStyle m;
            m.start.width = in.u16();
            m.end.width = in.u16();

            if (morphVersion == 1) {
                m.start.color = readRgba(in);
                m.end.color = readRgba(in);

                return m;
            }

            LineStyle shared;
            shared.startCap = static_cast<CapStyle>(in.ub(2));
            shared.join = static_cast<JoinStyle>(in.ub(2));
            const bool hasFill = in.flag();
            shared.noHScale = in.flag();
            shared.noVScale = in.flag();
            shared.pixelHinting = in.flag();
            in.ub(5);
            shared.noClose = in.flag();
            shared.endCap = static_cast<CapStyle>(in.ub(2));

            if (shared.join == JoinStyle::Miter)
                shared.miterLimit = in.fixed8();

            const std::uint16_t startWidth = m.start.width, endWidth = m.end.width;
            m.start = m.end = shared;
            m.start.width = startWidth;
            m.end.width = endWidth;

            if (hasFill) {
                const MorphFillStyle fill = readMorphFillStyle(in);
                m.start.fill = fill.start;
                m.end.fill = fill.end;
                m.start.color = fill.start.color;
                m.end.color = fill.end.color;
            } else {
                m.start.color = readRgba(in);
                m.end.color = readRgba(in);
            }

            return m;
        }

        // /----- INTERPOLATION -----/

        double lerp(double a, double b, double t) { return a + (b - a) * t; }

        std::int32_t lerpInt(std::int32_t a, std::int32_t b, double t) {
            return static_cast<std::int32_t>(std::lround(lerp(a, b, t)));
        }

        std::uint8_t lerpByte(std::uint8_t a, std::uint8_t b, double t) {
            return static_cast<std::uint8_t>(std::clamp<long>(std::lround(lerp(a, b, t)), 0, 255));
        }

        Rgba lerpColor(Rgba a, Rgba b, double t) {
            return {lerpByte(a.r, b.r, t), lerpByte(a.g, b.g, t), lerpByte(a.b, b.b, t), lerpByte(a.a, b.a, t)};
        }

        Matrix lerpMatrix(const Matrix& a, const Matrix& b, double t) {
            Matrix m;
            m.a = lerp(a.a, b.a, t);
            m.b = lerp(a.b, b.b, t);
            m.c = lerp(a.c, b.c, t);
            m.d = lerp(a.d, b.d, t);
            m.tx = lerpInt(a.tx, b.tx, t);
            m.ty = lerpInt(a.ty, b.ty, t);

            return m;
        }

        FillStyle lerpFill(const FillStyle& a, const FillStyle& b, double t) {
            FillStyle f = a;
            f.color = lerpColor(a.color, b.color, t);
            f.gradient.matrix = lerpMatrix(a.gradient.matrix, b.gradient.matrix, t);
            f.gradient.focalPoint = static_cast<float>(lerp(a.gradient.focalPoint, b.gradient.focalPoint, t));

            for (std::size_t i = 0; i < f.gradient.records.size() && i < b.gradient.records.size(); ++i) {
                f.gradient.records[i].ratio = lerpByte(a.gradient.records[i].ratio, b.gradient.records[i].ratio, t);
                f.gradient.records[i].color = lerpColor(a.gradient.records[i].color, b.gradient.records[i].color, t);
            }

            f.bitmapMatrix = lerpMatrix(a.bitmapMatrix, b.bitmapMatrix, t);
            return f;
        }

        /// An edge in absolute coordinates. Straight edges are stored as curves whose
        /// control point is the midpoint, which is how Flash morphs straight<->curved.
        struct AbsoluteEdge {
            double x0, y0, cx, cy, x1, y1;
            bool straight;
        };

        /// Flattens the edges of a record stream into absolute coordinates, in order.
        /// Style changes contribute only their moveTo.
        std::vector<AbsoluteEdge> absoluteEdges(const std::vector<ShapeRecord>& records) {
            std::vector<AbsoluteEdge> edges;
            double x = 0, y = 0;

            for (const auto& record : records) {
                if (const auto* change = std::get_if<StyleChangeRecord>(&record)) {
                    if (change->moveTo) {
                        x = change->moveTo->first;
                        y = change->moveTo->second;
                    }
                } else if (const auto* s = std::get_if<StraightEdgeRecord>(&record)) {
                    const double nx = x + s->dx, ny = y + s->dy;
                    edges.push_back({x, y, (x + nx) / 2, (y + ny) / 2, nx, ny, true});
                    x = nx;
                    y = ny;
                } else {
                    const auto& c = std::get<CurvedEdgeRecord>(record);
                    const double cx = x + c.controlDx, cy = y + c.controlDy;
                    const double nx = cx + c.anchorDx, ny = cy + c.anchorDy;
                    edges.push_back({x, y, cx, cy, nx, ny, false});
                    x = nx;
                    y = ny;
                }
            }

            return edges;
        }

    } // namespace

    ShapeGeometry parseShapeWithStyle(std::span<const std::uint8_t> data, int shapeVersion) {
        BitReader in(data);
        ShapeGeometry g;
        g.styles = readStyleTables(in, shapeVersion);
        g.records = readShapeRecords(in, shapeVersion);

        return g;
    }

    ShapeGeometry parseGlyph(std::span<const std::uint8_t> data) {
        BitReader in(data);
        ShapeGeometry g;
        FillStyle solid;
        solid.color = {0, 0, 0, 255};
        g.styles.fills.push_back(solid);
        g.records = readShapeRecords(in, 0);

        return g;
    }

    MorphShapeGeometry parseMorphShape(std::span<const std::uint8_t> data, int morphVersion) {
        BitReader in(data);
        MorphShapeGeometry m;
        const std::uint32_t endEdgesOffset = in.u32();
        const std::size_t afterOffset = in.bytePosition();

        m.fills.resize(readStyleCount(in, true));

        for (auto& f : m.fills)
            f = readMorphFillStyle(in);

        m.lines.resize(readStyleCount(in, true));

        for (auto& l : m.lines)
            l = readMorphLineStyle(in, morphVersion);

        m.startEdges.records = readShapeRecords(in, 0);

        if (endEdgesOffset != 0) {
            in.seek(afterOffset + endEdgesOffset);
            m.endEdges.records = readShapeRecords(in, 0);
        }

        return m;
    }

    ShapeGeometry interpolateMorph(const MorphShapeGeometry& morph, double t) {
        ShapeGeometry out;

        for (const auto& f : morph.fills)
            out.styles.fills.push_back(lerpFill(f.start, f.end, t));

        for (const auto& l : morph.lines) {
            LineStyle s = l.start;
            s.width = static_cast<std::uint16_t>(lerpInt(l.start.width, l.end.width, t));
            s.color = lerpColor(l.start.color, l.end.color, t);

            if (l.start.fill && l.end.fill)
                s.fill = lerpFill(*l.start.fill, *l.end.fill, t);

            out.styles.lines.push_back(s);
        }

        const std::vector<AbsoluteEdge> endEdges = absoluteEdges(morph.endEdges.records);
        std::size_t edgeIndex = 0;
        double startX = 0, startY = 0; // pen in start space, to detect implicit moves

        // Rebuild a record stream with interpolated absolute positions. Every
        // edge is re-emitted relative to the interpolated pen, and each style
        // change gets an explicit interpolated moveTo so both streams stay in sync.
        double penX = 0, penY = 0;

        for (const auto& record : morph.startEdges.records) {
            if (const auto* change = std::get_if<StyleChangeRecord>(&record)) {
                StyleChangeRecord c = *change;

                if (change->moveTo) {
                    startX = change->moveTo->first;
                    startY = change->moveTo->second;

                    // The end shape's matching move is the start of its next edge.
                    double endX = startX, endY = startY;

                    if (edgeIndex < endEdges.size()) {
                        endX = endEdges[edgeIndex].x0;
                        endY = endEdges[edgeIndex].y0;
                    }

                    penX = lerp(startX, endX, t);
                    penY = lerp(startY, endY, t);
                    c.moveTo = std::make_pair(static_cast<std::int32_t>(std::lround(penX)),
                                              static_cast<std::int32_t>(std::lround(penY)));
                    penX = c.moveTo->first;
                    penY = c.moveTo->second;
                }

                out.records.emplace_back(std::move(c));
                continue;
            }

            AbsoluteEdge s{};

            if (const auto* straight = std::get_if<StraightEdgeRecord>(&record)) {
                s = {startX, startY, startX + straight->dx / 2.0, startY + straight->dy / 2.0, startX + straight->dx,
                     startY + straight->dy, true};
            } else {
                const auto& c = std::get<CurvedEdgeRecord>(record);
                const double cx = startX + c.controlDx, cy = startY + c.controlDy;
                s = {startX, startY, cx, cy, cx + c.anchorDx, cy + c.anchorDy, false};
            }

            startX = s.x1;
            startY = s.y1;
            const AbsoluteEdge e = edgeIndex < endEdges.size() ? endEdges[edgeIndex] : s;
            ++edgeIndex;

            const double x1 = std::round(lerp(s.x1, e.x1, t));
            const double y1 = std::round(lerp(s.y1, e.y1, t));

            if (s.straight && e.straight) {
                out.records.emplace_back(StraightEdgeRecord{static_cast<std::int32_t>(x1 - penX),
                                                            static_cast<std::int32_t>(y1 - penY)});
            } else {
                const double cx = std::round(lerp(s.cx, e.cx, t));
                const double cy = std::round(lerp(s.cy, e.cy, t));

                out.records.emplace_back(CurvedEdgeRecord{
                    static_cast<std::int32_t>(cx - penX), static_cast<std::int32_t>(cy - penY),
                    static_cast<std::int32_t>(x1 - cx), static_cast<std::int32_t>(y1 - cy)});
            }

            penX = x1;
            penY = y1;
        }

        return out;
    }

} // namespace swf
