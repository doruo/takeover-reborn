// MovingBody.h - a point with a velocity, and the pushes soldiers give
// each other to stay apart (original: _SafePkg_805/_SafeCls_2180).
#pragma once

#include "game/sim/base/BaseObject.h"
#include "game/sim/base/Point2D.h"

namespace takeover {

    class MovingBody : public BaseObject {
    public:
        void setX(double x) { x_ = x; } ///< _SafeStr_1070
        void setY(double y) { y_ = y; } ///< _SafeStr_1039
        [[nodiscard]] double getX() const { return x_; } ///< _SafeStr_1024
        [[nodiscard]] double getY() const { return y_; } ///< _SafeStr_1020
        void moveTo(double x, double y) { setX(x), setY(y); }
        void resetVelocity() { vx_ = vy_ = 0; } ///< _SafeStr_1075
        [[nodiscard]] double distanceSquaredTo(const MovingBody& other) const; ///< _SafeStr_2183
        [[nodiscard]] bool isAt(const Point2D& p) const { return x_ == p.x && y_ == p.y; } ///< _SafeStr_2184
        /// Moves by the velocity (original: _SafeStr_2185).
        void applyVelocity();
        void addVelocity(double dx, double dy) { vx_ += dx, vy_ += dy; } ///< _SafeStr_1052
        /// Pushed away from a point when closer than `radius` (original: _SafeStr_1240).
        bool pushAwayFrom(const Point2D& point, double radius, double force);

        /// Pushes `other` away when closer than `radius` (original: _SafeStr_1239).
        bool pushAway(MovingBody& other, double radius, double force);

        /// Pulls `other` closer when farther than `radius` (original: _SafeStr_1227).
        bool pullCloser(MovingBody& other, double radius, double force);

    protected:
        double y_ = 0;  ///< Original: _SafeStr_1944.
        double x_ = 0;  ///< Original: _SafeStr_1945.
        double vy_ = 0; ///< Original: _SafeStr_2181.
        double vx_ = 0; ///< Original: _SafeStr_2182.
    };

} // namespace takeover
