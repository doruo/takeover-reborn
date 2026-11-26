// Matrix.h - 2D affine transform with flash.geom.Matrix semantics.
#pragma once

#include "swf/Types.h"

#include <algorithm>
#include <cmath>

namespace engine {

    struct Point {
        double x = 0;
        double y = 0;
    };

    /// Flash layout: x' = a*x + c*y + tx, y' = b*x + d*y + ty. Units are pixels.
    struct Matrix {
        double a = 1, b = 0, c = 0, d = 1, tx = 0, ty = 0;

        static Matrix identity() { return {}; }
        static Matrix translation(double x, double y) { return {1, 0, 0, 1, x, y}; }
        static Matrix scale(double sx, double sy) { return {sx, 0, 0, sy, 0, 0}; }

        /// Converts a SWF matrix (translation in twips) to pixels.
        static Matrix fromSwf(const swf::Matrix& m) {
            return {m.a, m.b, m.c, m.d, m.tx / static_cast<double>(swf::kTwipsPerPixel),
                    m.ty / static_cast<double>(swf::kTwipsPerPixel)};
        }

        /// Returns `parent * this`: first apply this matrix, then `parent`.
        /// For a display object, world = parentWorld.concat... is written as
        /// `child.then(parent)` to keep the reading order of the math.
        [[nodiscard]] Matrix then(const Matrix& parent) const {
            return {a * parent.a + b * parent.c,
                    a * parent.b + b * parent.d,
                    c * parent.a + d * parent.c,
                    c * parent.b + d * parent.d,
                    tx * parent.a + ty * parent.c + parent.tx,
                    tx * parent.b + ty * parent.d + parent.ty};
        }

        [[nodiscard]] Point apply(Point p) const { return {a * p.x + c * p.y + tx, b * p.x + d * p.y + ty}; }

        [[nodiscard]] double determinant() const { return a * d - b * c; }

        [[nodiscard]] Matrix inverted() const {
            const double det = determinant();
            if (det == 0)
                return {0, 0, 0, 0, -tx, -ty}; // what flash.geom.Matrix.invert yields

            const double ia = d / det, ib = -b / det, ic = -c / det, id = a / det;
            return {ia, ib, ic, id, -(ia * tx + ic * ty), -(ib * tx + id * ty)};
        }

        /// Largest scale factor along any axis (used to pick a raster resolution).
        [[nodiscard]] double maxScale() const { return std::max(std::hypot(a, b), std::hypot(c, d)); }
    };

    struct Rectangle {
        double x = 0, y = 0, width = 0, height = 0;

        [[nodiscard]] bool isEmpty() const { return width <= 0 || height <= 0; }
        [[nodiscard]] double right() const { return x + width; }
        [[nodiscard]] double bottom() const { return y + height; }

        [[nodiscard]] Rectangle united(const Rectangle& o) const {
            if (isEmpty())
                return o;

            if (o.isEmpty())
                return *this;

            const double l = std::min(x, o.x), t = std::min(y, o.y);
            return {l, t, std::max(right(), o.right()) - l, std::max(bottom(), o.bottom()) - t};
        }

        /// Bounding box of this rectangle after transformation by `m`.
        [[nodiscard]] Rectangle transformed(const Matrix& m) const {
            const Point p[4] = {m.apply({x, y}), m.apply({right(), y}), m.apply({x, bottom()}),
                                m.apply({right(), bottom()})};
            double l = p[0].x, r = p[0].x, t = p[0].y, btm = p[0].y;

            for (const Point& q : p) {
                l = std::min(l, q.x);
                r = std::max(r, q.x);
                t = std::min(t, q.y);
                btm = std::max(btm, q.y);
            }

            return {l, t, r - l, btm - t};
        }

        static Rectangle fromSwf(const swf::Rect& r) {
            constexpr double k = swf::kTwipsPerPixel;
            return {r.xMin / k, r.yMin / k, (r.xMax - r.xMin) / k, (r.yMax - r.yMin) / k};
        }
    };

    /// flash.geom.ColorTransform: channel' = channel * multiplier + offset, applied
    /// to straight (non-premultiplied) color, offsets in [-255, 255].
    struct ColorTransform {
        float redMultiplier = 1, greenMultiplier = 1, blueMultiplier = 1, alphaMultiplier = 1;
        float redOffset = 0, greenOffset = 0, blueOffset = 0, alphaOffset = 0;

        static ColorTransform fromSwf(const swf::ColorTransform& cx) {
            return {cx.rMul / 256.0f, cx.gMul / 256.0f, cx.bMul / 256.0f, cx.aMul / 256.0f,
                    static_cast<float>(cx.rAdd), static_cast<float>(cx.gAdd), static_cast<float>(cx.bAdd),
                    static_cast<float>(cx.aAdd)};
        }

        /// Composition as done by Flash when nesting: apply `this` first, then
        /// `parent`.
        [[nodiscard]] ColorTransform then(const ColorTransform& p) const {
            return {redMultiplier * p.redMultiplier,
                    greenMultiplier * p.greenMultiplier,
                    blueMultiplier * p.blueMultiplier,
                    alphaMultiplier * p.alphaMultiplier,
                    redOffset * p.redMultiplier + p.redOffset,
                    greenOffset * p.greenMultiplier + p.greenOffset,
                    blueOffset * p.blueMultiplier + p.blueOffset,
                    alphaOffset * p.alphaMultiplier + p.alphaOffset};
        }

        [[nodiscard]] bool isIdentity() const {
            return redMultiplier == 1 && greenMultiplier == 1 && blueMultiplier == 1 && alphaMultiplier == 1 &&
                   redOffset == 0 && greenOffset == 0 && blueOffset == 0 && alphaOffset == 0;
        }
    };

} // namespace engine
