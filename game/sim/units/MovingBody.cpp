#include "game/sim/units/MovingBody.h"

#include "game/framework/core/As3.h"

#include <cmath>

namespace takeover {

    double MovingBody::distanceSquaredTo(const MovingBody& other) const {
        const double dx = other.getX() - x_, dy = other.getY() - y_;
        return dx * dx + dy * dy;
    }

    void MovingBody::applyVelocity() {
        setX(getX() + vx_);
        setY(getY() + vy_);
    }

    bool MovingBody::pushAwayFrom(const Point2D& point, double radius, double force) {
        const double dx = getX() - point.x, dy = getY() - point.y;
        const double d2 = dx * dx + dy * dy, r2 = radius * radius;
        if (d2 > r2)
            return false;

        // A little randomness keeps stacked soldiers from staying aligned.
        const double angle = std::atan2(dy, dx) + as3::random() * 2;
        const double strength = force * (1 - d2 / r2);
        addVelocity(std::cos(angle) * strength, std::sin(angle) * strength);

        return true;
    }

    bool MovingBody::pushAway(MovingBody& other, double radius, double force) {
        const double dx = other.getX() - getX(), dy = other.getY() - getY();
        const double d2 = dx * dx + dy * dy, r2 = radius * radius;
        if (d2 > r2)
            return false;

        const double angle = std::atan2(dy, dx) + as3::random() * 2;
        const double strength = force * (1 - d2 / r2);
        other.addVelocity(std::cos(angle) * strength, std::sin(angle) * strength);

        return true;
    }

    bool MovingBody::pullCloser(MovingBody& other, double radius, double force) {
        const double dx = getX() - other.getX(), dy = getY() - other.getY();
        const double d2 = dx * dx + dy * dy, r2 = radius * radius;
        if (d2 < r2)
            return false;

        const double angle = std::atan2(dy, dx) + as3::random() * 2;
        const double strength = -force * (1 - d2 / r2);
        other.addVelocity(std::cos(angle) * strength, std::sin(angle) * strength);

        return true;
    }

} // namespace takeover
