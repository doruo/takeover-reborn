// Point2D.h - a 2D point of the simulation (original: base/_SafeStr_867).
#pragma once

#include <cmath>
#include <string>

namespace takeover {

    struct Point2D {
        double x = 0;
        double y = 0;

        Point2D() = default;
        Point2D(double px, double py) : x(px), y(py) {}

        /// Distance to (px, py), or its square (original: _SafeStr_2781).
        [[nodiscard]] double distanceTo(double px, double py, bool squared = false) const {
            const double dx = px - x, dy = py - y, d2 = dx * dx + dy * dy;
            return squared ? d2 : std::sqrt(d2);
        }

        [[nodiscard]] double getDistanceTo(const Point2D& p, bool squared = false) const {
            return distanceTo(p.x, p.y, squared);
        }

        [[nodiscard]] Point2D getCopy() const { return *this; }
        /// Angle of the direction towards (px, py), radians (original: _SafeStr_2957).
        [[nodiscard]] double angleTo(double px, double py) const { return std::atan2(py - y, px - x); }
        [[nodiscard]] double angleTo(const Point2D& p) const { return angleTo(p.x, p.y); } ///< _SafeStr_2349
        void copyFrom(const Point2D& p) { *this = p; }                                    ///< _SafeStr_2112
        [[nodiscard]] bool compare(const Point2D& p) const { return x == p.x && y == p.y; }
        void add(const Point2D& p) { x += p.x, y += p.y; } ///< _SafeStr_2165
        [[nodiscard]] std::string toString() const;
    };

    /// Helpers on points (original: base/_SafeStr_2347).
    namespace geometry {

        /// Foot of the perpendicular from `p` to the line through `a` and `b`
        /// (original: _SafeStr_2384). Like the original, vertical or horizontal
        /// lines divide by zero.
        Point2D projectOnLine(const Point2D& p, const Point2D& a, const Point2D& b);

        /// True when `p` is strictly inside the bounding box of p, a, b: neither
        /// its x nor its y is an extreme (original: _SafeStr_2379).
        bool isInsideBox(const Point2D& p, const Point2D& a, const Point2D& b);

    } // namespace geometry

} // namespace takeover
