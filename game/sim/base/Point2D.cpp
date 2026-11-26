#include "game/sim/base/Point2D.h"

#include "game/framework/core/As3.h"

#include <algorithm>

namespace takeover {

std::string Point2D::toString() const { return "(" + as3::toString(x) + "," + as3::toString(y) + ")"; }

namespace geometry {

Point2D projectOnLine(const Point2D& p, const Point2D& a, const Point2D& b) {
    const double slope = (b.y - a.y) / (b.x - a.x);
    const double intercept = -a.x * slope + a.y;
    const double normalSlope = (a.x - b.x) / (b.y - a.y);
    const double normalIntercept = -p.x * normalSlope + p.y;
    const double x = (normalIntercept - intercept) / (slope - normalSlope);
    return {x, slope * x + intercept};
}

bool isInsideBox(const Point2D& p, const Point2D& a, const Point2D& b) {
    if (std::min({p.x, a.x, b.x}) == p.x) return false;
    if (std::max({p.x, a.x, b.x}) == p.x) return false;
    if (std::min({p.y, a.y, b.y}) == p.y) return false;
    if (std::max({p.y, a.y, b.y}) == p.y) return false;
    return true;
}

} // namespace geometry

} // namespace takeover
