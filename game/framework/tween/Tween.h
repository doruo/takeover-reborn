// Tween.h - value animations over time (original: package _SafePkg_1640).
//
//   Transition  an easing function with a start value, a change and a
//               duration (original: _SafeStr_1612; the easings are the
//               classic Robert Penner ones)
//   Tween       plays a Transition when updated, after an optional delay,
//               possibly several times (original: _SafeCls_1782)
//   TargetTween a Tween that pushes its value into an ITweenTarget
//               (original: _SafeCls_1800 and the interface _SafeStr_1802)
#pragma once

#include "game/framework/core/Disposable.h"

#include <functional>

namespace takeover {

/// Easing: (time, begin, change, duration) -> value.
using Easing = double (*)(double t, double b, double c, double d);

namespace easing {
double linear(double t, double b, double c, double d);         ///< _SafeStr_1631
double sineIn(double t, double b, double c, double d);         ///< _SafeStr_2829
double sineOut(double t, double b, double c, double d);        ///< _SafeStr_2820
double sineInOut(double t, double b, double c, double d);      ///< _SafeStr_2836
double quadIn(double t, double b, double c, double d);         ///< _SafeStr_2830
double quadOut(double t, double b, double c, double d);        ///< _SafeStr_2823
double quadInOut(double t, double b, double c, double d);      ///< _SafeStr_2825
double cubicIn(double t, double b, double c, double d);        ///< _SafeStr_2822
double cubicOut(double t, double b, double c, double d);       ///< _SafeStr_2833
double cubicInOut(double t, double b, double c, double d);     ///< _SafeStr_2817
double quartIn(double t, double b, double c, double d);        ///< _SafeStr_2814
double quartOut(double t, double b, double c, double d);       ///< _SafeStr_2831
double quartInOut(double t, double b, double c, double d);     ///< _SafeStr_2812
double quintIn(double t, double b, double c, double d);        ///< _SafeStr_2813
double quintOut(double t, double b, double c, double d);       ///< _SafeStr_2824
double quintInOut(double t, double b, double c, double d);     ///< _SafeStr_2834
double expoIn(double t, double b, double c, double d);         ///< _SafeStr_2819
double expoOut(double t, double b, double c, double d);        ///< _SafeStr_2827
double expoInOut(double t, double b, double c, double d);      ///< _SafeStr_2826
double backIn(double t, double b, double c, double d);         ///< _SafeStr_2835
double backOut(double t, double b, double c, double d);        ///< _SafeStr_2821
double backInOut(double t, double b, double c, double d);      ///< _SafeStr_2815
double bounceOut(double t, double b, double c, double d);      ///< _SafeStr_2828
double elasticIn(double t, double b, double c, double d);      ///< _SafeStr_2832
double elasticOut(double t, double b, double c, double d);     ///< _SafeStr_2816
double elasticInOut(double t, double b, double c, double d);   ///< _SafeStr_2818
} // namespace easing

/// Original: _SafeStr_1612.
struct Transition {
    Transition() = default;
    /// `duration` in seconds; times are kept in milliseconds like the original.
    Transition(Easing function, double from, double to, double duration)
        : easing(function), x0(from), dx(to - from), mscDuration(duration * 1000) {}

    Easing easing = easing::linear;
    double x0 = 0;
    double dx = 0;
    double mscDuration = 0;

    /// Value at `milliseconds` (original: _SafeStr_1795).
    [[nodiscard]] double valueAt(double milliseconds) const { return easing(milliseconds, x0, dx, mscDuration); }
};

/// Original: _SafeCls_1782.
class Tween : public UpdatableObject {
public:
    Tween() = default;
    explicit Tween(Transition transition, double delayBeforeStart = 0, bool disposeAfterFinish = true,
                   int repeatCounter = 1) {
        init(transition, delayBeforeStart, disposeAfterFinish, repeatCounter);
    }

    Signal<Tween*> signalStart;  ///< Original: _SafeStr_1791.
    Signal<Tween*> signalUpdate; ///< Original: _SafeStr_1792.
    Signal<Tween*> signalFinish; ///< Original: _SafeStr_1681.

    Tween& init(Transition transition, double delayBeforeStart = 0, bool disposeAfterFinish = true,
                int repeatCounter = 1); ///< _SafeStr_1793
    /// Restarts with a repeat count, from `startTime` seconds (original: _SafeStr_1680).
    void restart(int repeatCounter, double startTime = 0);

    [[nodiscard]] double value() const { return value_; }          ///< _SafeStr_1796
    [[nodiscard]] double elapsedMs() const { return elapsedMs_; }  ///< _SafeStr_1797
    [[nodiscard]] bool finished() const { return finished_; }      ///< _SafeStr_1798
    [[nodiscard]] bool running() const { return running_; }        ///< _SafeStr_1794
    void setRunning(bool running) { running_ = running; }
    [[nodiscard]] int repeatCounter() const { return repeatCounter_; }
    [[nodiscard]] const Transition& transition() const { return transition_; } ///< _SafeStr_1679
    void setDelay(double seconds) { delay_ = seconds; }            ///< _SafeStr_1799
    void setDisposeAfterFinish(bool value) { disposeAfterFinish_ = value; } ///< _SafeStr_1667

    void update(double timeElapsed) override;

protected:
    void customDispose() override;
    virtual void startTween() {}
    virtual void processTween() { value_ = transition_.valueAt(elapsedMs_); }
    virtual void finishTween() { value_ = transition_.x0 + transition_.dx; }

    Transition transition_;
    double value_ = 0;
    double elapsedMs_ = 0;
    double delay_ = 0;
    bool finished_ = false;
    bool disposeAfterFinish_ = true;
    int repeatCounter_ = 1;
    bool running_ = true;
};

/// Something a TargetTween animates (original: interface _SafeStr_1802).
class ITweenTarget {
public:
    virtual ~ITweenTarget() = default;
    virtual void setTweenValue(double value) = 0; ///< _SafeStr_1804
    [[nodiscard]] virtual int tweenStart() const = 0; ///< _SafeStr_1805
    [[nodiscard]] virtual int tweenEnd() const = 0;   ///< _SafeStr_1806
};

/// Original: _SafeCls_1800.
class TargetTween : public Tween {
public:
    TargetTween() = default;
    TargetTween(ITweenTarget* target, double duration, int repeatCounter = -1, double delayBeforeStart = 0,
                bool disposeAfterFinish = true) {
        init(target, duration, repeatCounter, delayBeforeStart, disposeAfterFinish);
    }
    TargetTween& init(ITweenTarget* target, double duration, int repeatCounter = -1, double delayBeforeStart = 0,
                      bool disposeAfterFinish = true); ///< _SafeStr_1803

protected:
    void processTween() override;
    void finishTween() override;

private:
    ITweenTarget* target_ = nullptr;
};

} // namespace takeover
