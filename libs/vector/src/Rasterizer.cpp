#include "vector/Rasterizer.h"

#include <cairo.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace vector {

    namespace {

        // RAII owners for cairo objects.
        struct CairoDeleter {
            void operator()(cairo_t* p) const { cairo_destroy(p); }
            void operator()(cairo_surface_t* p) const { cairo_surface_destroy(p); }
            void operator()(cairo_pattern_t* p) const { cairo_pattern_destroy(p); }
        };

        using CairoContext = std::unique_ptr<cairo_t, CairoDeleter>;
        using CairoSurface = std::unique_ptr<cairo_surface_t, CairoDeleter>;
        using CairoPattern = std::unique_ptr<cairo_pattern_t, CairoDeleter>;

        /// Gradients are defined on a 32768-twip square centred on the origin.
        constexpr double kGradientHalfSize = 16384.0;

        cairo_matrix_t toCairo(const swf::Matrix& m) {
            cairo_matrix_t cm;
            cairo_matrix_init(&cm, m.a, m.b, m.c, m.d, m.tx, m.ty);

            return cm;
        }

        /// Cairo pattern matrices map user space to pattern space: the inverse of the
        /// SWF matrix, which maps pattern space to shape space. A degenerate matrix
        /// (zero scale) gets a tiny scale instead, as Flash still paints a color.
        void setPatternMatrix(cairo_pattern_t* pattern, const swf::Matrix& m) {
            cairo_matrix_t cm = toCairo(m);
            if (cairo_matrix_invert(&cm) != CAIRO_STATUS_SUCCESS) {
                swf::Matrix fixed = m;
                if (std::abs(fixed.a * fixed.d - fixed.b * fixed.c) < 1e-9) {
                    fixed.a = fixed.a == 0 ? 1e-4 : fixed.a;
                    fixed.d = fixed.d == 0 ? 1e-4 : fixed.d;
                }

                cm = toCairo(fixed);
                if (cairo_matrix_invert(&cm) != CAIRO_STATUS_SUCCESS)
                    cairo_matrix_init_identity(&cm);
            }

            cairo_pattern_set_matrix(pattern, &cm);
        }

        void addStops(cairo_pattern_t* pattern, const swf::Gradient& g) {
            for (const auto& r : g.records) {
                cairo_pattern_add_color_stop_rgba(pattern, r.ratio / 255.0, r.color.r / 255.0, r.color.g / 255.0,
                                                  r.color.b / 255.0, r.color.a / 255.0);
            }

            switch (g.spread) {
            case swf::SpreadMode::Pad: cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD); break;
            case swf::SpreadMode::Reflect: cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REFLECT); break;
            case swf::SpreadMode::Repeat: cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT); break;
            }
        }

        /// Builds the cairo source for a fill style. Returns an empty pointer for a
        /// solid color (set directly on the context) or an unresolvable bitmap.
        CairoPattern makePattern(cairo_t* cr, const swf::FillStyle& fill, const BitmapResolver& bitmaps,
                                 CairoSurface& bitmapSurfaceOut) {
            using Type = swf::FillStyle::Type;

            switch (fill.type) {
            case Type::Solid:
                cairo_set_source_rgba(cr, fill.color.r / 255.0, fill.color.g / 255.0, fill.color.b / 255.0,
                                      fill.color.a / 255.0);
                return {};
            case Type::LinearGradient: {
                CairoPattern p(cairo_pattern_create_linear(-kGradientHalfSize, 0, kGradientHalfSize, 0));
                addStops(p.get(), fill.gradient);
                setPatternMatrix(p.get(), fill.gradient.matrix);

                return p;
            }
            case Type::RadialGradient:
            case Type::FocalRadialGradient: {
                const double focalX = fill.type == Type::FocalRadialGradient ? fill.gradient.focalPoint * kGradientHalfSize : 0;
                CairoPattern p(cairo_pattern_create_radial(focalX, 0, 0, 0, 0, kGradientHalfSize));
                addStops(p.get(), fill.gradient);
                setPatternMatrix(p.get(), fill.gradient.matrix);

                return p;
            }
            default: {
                const Image* image = bitmaps ? bitmaps(fill.bitmapId) : nullptr;
                if (!image || image->width == 0) {
                    // Flash paints a missing bitmap fill as nothing; keep that.
                    cairo_set_source_rgba(cr, 0, 0, 0, 0);
                    return {};
                }

                bitmapSurfaceOut.reset(cairo_image_surface_create_for_data(
                    const_cast<unsigned char*>(image->pixels.data()), CAIRO_FORMAT_ARGB32, image->width, image->height,
                    image->width * 4));
                CairoPattern p(cairo_pattern_create_for_surface(bitmapSurfaceOut.get()));
                cairo_pattern_set_extend(p.get(), fill.bitmapRepeats() ? CAIRO_EXTEND_REPEAT : CAIRO_EXTEND_PAD);
                cairo_pattern_set_filter(p.get(), fill.bitmapSmoothed() ? CAIRO_FILTER_BILINEAR : CAIRO_FILTER_NEAREST);
                setPatternMatrix(p.get(), fill.bitmapMatrix);

                return p;
            }
            }
        }

        void tracePath(cairo_t* cr, const swf::Path& path) {
            cairo_new_path(cr);
            double x = 0, y = 0;

            for (const auto& c : path.commands) {
                switch (c.op) {
                case swf::PathCommand::Op::MoveTo:
                    x = c.to.x;
                    y = c.to.y;
                    cairo_move_to(cr, x, y);
                    break;
                case swf::PathCommand::Op::LineTo:
                    x = c.to.x;
                    y = c.to.y;
                    cairo_line_to(cr, x, y);
                    break;
                case swf::PathCommand::Op::QuadTo: {
                    // Quadratic -> cubic: control points at 2/3 towards the quad control.
                    const double c1x = x + 2.0 / 3.0 * (c.control.x - x), c1y = y + 2.0 / 3.0 * (c.control.y - y);
                    const double c2x = c.to.x + 2.0 / 3.0 * (c.control.x - c.to.x);
                    const double c2y = c.to.y + 2.0 / 3.0 * (c.control.y - c.to.y);
                    x = c.to.x;
                    y = c.to.y;
                    cairo_curve_to(cr, c1x, c1y, c2x, c2y, x, y);
                    break;
                }
                case swf::PathCommand::Op::Close:
                    cairo_close_path(cr);
                    break;
                }
            }
        }

        void fill(cairo_t* cr, const swf::FillCommand& command, const BitmapResolver& bitmaps) {
            CairoSurface bitmapSurface;
            CairoPattern pattern = makePattern(cr, command.style, bitmaps, bitmapSurface);
            if (pattern)
                cairo_set_source(cr, pattern.get());

            tracePath(cr, command.path);
            cairo_set_fill_rule(cr, command.nonZeroWinding ? CAIRO_FILL_RULE_WINDING : CAIRO_FILL_RULE_EVEN_ODD);
            cairo_fill(cr);
        }

        void stroke(cairo_t* cr, const swf::StrokeCommand& command, const BitmapResolver& bitmaps) {
            const swf::LineStyle& style = command.style;
            CairoSurface bitmapSurface;
            CairoPattern pattern;

            if (style.fill && style.fill->type != swf::FillStyle::Type::Solid) {
                pattern = makePattern(cr, *style.fill, bitmaps, bitmapSurface);
                if (pattern)
                    cairo_set_source(cr, pattern.get());
            } else {
                cairo_set_source_rgba(cr, style.color.r / 255.0, style.color.g / 255.0, style.color.b / 255.0,
                                      style.color.a / 255.0);
            }

            switch (style.startCap) {
            case swf::CapStyle::Round: cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND); break;
            case swf::CapStyle::None: cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT); break;
            case swf::CapStyle::Square: cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE); break;
            }

            switch (style.join) {
            case swf::JoinStyle::Round: cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND); break;
            case swf::JoinStyle::Bevel: cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL); break;
            case swf::JoinStyle::Miter:
                cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
                cairo_set_miter_limit(cr, std::max(1.0f, style.miterLimit));
                break;
            }

            tracePath(cr, command.path);

            // Stroke width in device pixels. Flash never draws a line thinner than one
            // pixel (width 0 is the explicit "hairline"), and non-scaling strokes keep
            // their nominal width whatever the transform.
            cairo_matrix_t ctm;
            cairo_get_matrix(cr, &ctm);
            const double scaleX = std::hypot(ctm.xx, ctm.yx);
            const double scaleY = std::hypot(ctm.xy, ctm.yy);
            const double nominalPx = style.width / 20.0;
            double widthPx;

            if (style.noHScale && style.noVScale)
                widthPx = nominalPx;
            else if (style.noHScale)
                widthPx = style.width * scaleY;
            else if (style.noVScale)
                widthPx = style.width * scaleX;
            else
                widthPx = style.width * std::sqrt(std::abs(ctm.xx * ctm.yy - ctm.xy * ctm.yx));

            widthPx = std::max(widthPx, 1.0);

            cairo_save(cr);
            cairo_identity_matrix(cr); // the path is already in device space
            cairo_set_line_width(cr, widthPx);
            cairo_stroke(cr);
            cairo_restore(cr);
        }

    } // namespace

    void rasterize(const swf::DrawList& drawList, const Affine& toPixels, Image& target, const BitmapResolver& bitmaps) {
        if (target.width <= 0 || target.height <= 0)
            return;

        CairoSurface surface(cairo_image_surface_create_for_data(target.pixels.data(), CAIRO_FORMAT_ARGB32, target.width,
                                                                 target.height, target.width * 4));
        CairoContext cr(cairo_create(surface.get()));
        if (cairo_status(cr.get()) != CAIRO_STATUS_SUCCESS)
            throw std::runtime_error("cairo context creation failed");

        cairo_matrix_t m;
        cairo_matrix_init(&m, toPixels.a, toPixels.b, toPixels.c, toPixels.d, toPixels.tx, toPixels.ty);
        cairo_set_matrix(cr.get(), &m);
        cairo_set_antialias(cr.get(), CAIRO_ANTIALIAS_GOOD);

        for (const auto& command : drawList.commands) {
            if (const auto* f = std::get_if<swf::FillCommand>(&command))
                fill(cr.get(), *f, bitmaps);
            else
                stroke(cr.get(), std::get<swf::StrokeCommand>(command), bitmaps);
        }

        cairo_surface_flush(surface.get());
    }

} // namespace vector
