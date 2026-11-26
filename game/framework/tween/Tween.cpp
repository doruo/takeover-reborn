#include "game/framework/tween/Tween.h"

#include <cmath>
#include <numbers>

namespace takeover {

namespace easing {

using std::numbers::pi;

double linear(double t, double b, double c, double d) { return c * t / d + b; }
double sineIn(double t, double b, double c, double d) { return -c * std::cos(t / d * (pi / 2)) + b + c; }
double sineOut(double t, double b, double c, double d) { return c * std::sin(t / d * (pi / 2)) + b; }
double sineInOut(double t, double b, double c, double d) { return -c / 2 * (std::cos(t / d * pi) - 1) + b; }
double quadIn(double t, double b, double c, double d) { t /= d; return c * t * t + b; }
double quadOut(double t, double b, double c, double d) { t /= d; return -c * t * (t - 2) + b; }
double quadInOut(double t, double b, double c, double d) {
    t /= d / 2;
    if (t < 1) return c / 2 * t * t + b;
    --t;
    return -c / 2 * (t * (t - 2) - 1) + b;
}
double cubicIn(double t, double b, double c, double d) { t /= d; return c * t * t * t + b; }
double cubicOut(double t, double b, double c, double d) { t = t / d - 1; return c * (t * t * t + 1) + b; }
double cubicInOut(double t, double b, double c, double d) {
    t /= d / 2;
    if (t < 1) return c / 2 * t * t * t + b;
    t -= 2;
    return c / 2 * (t * t * t + 2) + b;
}
double quartIn(double t, double b, double c, double d) { t /= d; return c * t * t * t * t + b; }
double quartOut(double t, double b, double c, double d) { t = t / d - 1; return -c * (t * t * t * t - 1) + b; }
double quartInOut(double t, double b, double c, double d) {
    t /= d / 2;
    if (t < 1) return c / 2 * t * t * t * t + b;
    t -= 2;
    return -c / 2 * (t * t * t * t - 2) + b;
}
double quintIn(double t, double b, double c, double d) { t /= d; return c * t * t * t * t * t + b; }
double quintOut(double t, double b, double c, double d) { t = t / d - 1; return c * (t * t * t * t * t + 1) + b; }
double quintInOut(double t, double b, double c, double d) {
    t /= d / 2;
    if (t < 1) return c / 2 * t * t * t * t * t + b;
    t -= 2;
    return c / 2 * (t * t * t * t * t + 2) + b;
}
double expoIn(double t, double b, double c, double d) { return t == 0 ? b : c * std::pow(2, 10 * (t / d - 1)) + b; }
double expoOut(double t, double b, double c, double d) { return t == d ? b + c : c * (-std::pow(2, -10 * t / d) + 1) + b; }
double expoInOut(double t, double b, double c, double d) {
    if (t == 0) return b;
    if (t == d) return b + c;
    t /= d / 2;
    if (t < 1) return c / 2 * std::pow(2, 10 * (t - 1)) + b;
    --t;
    return c / 2 * (-std::pow(2, -10 * t) + 2) + b;
}
constexpr double kOvershoot = 1.70158;
double backIn(double t, double b, double c, double d) { t /= d; return c * t * t * ((kOvershoot + 1) * t - kOvershoot) + b; }
double backOut(double t, double b, double c, double d) {
    t = t / d - 1;
    return c * (t * t * ((kOvershoot + 1) * t + kOvershoot) + 1) + b;
}
double backInOut(double t, double b, double c, double d) {
    const double s = kOvershoot * 1.525;
    t /= d / 2;
    if (t < 1) return c / 2 * (t * t * ((s + 1) * t - s)) + b;
    t -= 2;
    return c / 2 * (t * t * ((s + 1) * t + s) + 2) + b;
}
double bounceOut(double t, double b, double c, double d) {
    t /= d;
    if (t < 1 / 2.75) return c * (7.5625 * t * t) + b;
    if (t < 2 / 2.75) { t -= 1.5 / 2.75; return c * (7.5625 * t * t + 0.75) + b; }
    if (t < 2.5 / 2.75) { t -= 2.25 / 2.75; return c * (7.5625 * t * t + 0.9375) + b; }
    t -= 2.625 / 2.75;
    return c * (7.5625 * t * t + 0.984375) + b;
}

// The original's amplitude constant is `undefined` (NaN), so the amplitude
// is always |c| and the period is 400 ms.
constexpr double kElasticPeriod = 400;
double elasticIn(double t, double b, double c, double d) {
    if (t == 0) return b;
    t /= d;
    if (t == 1) return b + c;
    const double p = kElasticPeriod, a = c, s = p / 4;
    t -= 1;
    return -(a * std::pow(2, 10 * t) * std::sin((t * d - s) * (2 * pi) / p)) + b;
}
double elasticOut(double t, double b, double c, double d) {
    if (t == 0) return b;
    t /= d;
    if (t == 1) return b + c;
    const double p = kElasticPeriod, a = c, s = p / 4;
    return a * std::pow(2, -10 * t) * std::sin((t * d - s) * (2 * pi) / p) + c + b;
}
double elasticInOut(double t, double b, double c, double d) {
    if (t == 0) return b;
    t /= d / 2;
    if (t == 2) return b + c;
    const double p = kElasticPeriod, a = c, s = p / 4;
    if (t < 1) {
        t -= 1;
        return -0.5 * (a * std::pow(2, 10 * t) * std::sin((t * d - s) * (2 * pi) / p)) + b;
    }
    t -= 1;
    return a * std::pow(2, -10 * t) * std::sin((t * d - s) * (2 * pi) / p) * 0.5 + c + b;
}

} // namespace easing

Tween& Tween::init(Transition transition, double delayBeforeStart, bool disposeAfterFinish, int repeatCounter) {
    transition_ = transition;
    delay_ = delayBeforeStart;
    disposeAfterFinish_ = disposeAfterFinish;
    restart(repeatCounter);
    return *this;
}

void Tween::restart(int repeatCounter, double startTime) {
    repeatCounter_ = repeatCounter;
    lifetime_ = startTime;
    elapsedMs_ = lifetime_ * 1000;
    finished_ = false;
    value_ = transition_.valueAt(elapsedMs_);
    running_ = true;
}

void Tween::update(double timeElapsed) {
    if (!running_) return;
    if (!finished_ && lifetime_ >= delay_) {
        if (lifetime_ - timeElapsed < delay_) {
            startTween();
            signalStart.dispatch(this);
        }
        if (elapsedMs_ < transition_.mscDuration + delay_) {
            processTween();
            signalUpdate.dispatch(this);
        } else {
            finishTween();
            --repeatCounter_;
            if (repeatCounter_ == 0) {
                finished_ = true;
                running_ = false;
                signalFinish.dispatch(this);
                if (disposeAfterFinish_) dispose();
            } else {
                restart(repeatCounter_);
            }
        }
        elapsedMs_ += timeElapsed * 1000;
    }
    UpdatableObject::update(timeElapsed);
}

void Tween::customDispose() {
    signalStart.dispose();
    signalUpdate.dispose();
    signalFinish.dispose();
    UpdatableObject::customDispose();
}

TargetTween& TargetTween::init(ITweenTarget* target, double duration, int repeatCounter, double delayBeforeStart,
                               bool disposeAfterFinish) {
    target_ = target;
    Tween::init(Transition(easing::linear, target_->tweenStart(), target_->tweenEnd(), duration), delayBeforeStart,
                disposeAfterFinish, repeatCounter);
    return *this;
}

void TargetTween::processTween() {
    Tween::processTween();
    target_->setTweenValue(value_);
}

void TargetTween::finishTween() {
    Tween::finishTween();
    target_->setTweenValue(value_);
}

} // namespace takeover
