// Conditions.h - numbers that badges watch, and conditions on them
// (original: package _SafePkg_1601).
//
//   Metric             a named number with an update rule (keep the last,
//                      the max or the min value written)
//   MetricCondition    "metric >= 8"
//   LogicalCondition   and/or of other conditions
//   Achievement        becomes unlocked, once, when its condition holds
#pragma once

#include "game/framework/core/Disposable.h"

#include <memory>
#include <string>
#include <vector>

namespace takeover {

/// How a Metric merges a new value (original: _SafeCls_1600).
enum class MetricMode { Last, Max, Min };

/// Original: _SafePkg_1601/Metric.
class Metric : public Disposable {
public:
    explicit Metric(std::string key = "", MetricMode mode = MetricMode::Last) : key_(std::move(key)), mode_(mode) {}

    /// Dispatched with (metric, key) when the value changes (original: _SafeStr_1722).
    Signal<Metric*, const std::string&> signalChanged;

    [[nodiscard]] double value() const { return value_; }
    void setValue(double value);
    [[nodiscard]] double previousValue() const { return previous_; } ///< _SafeStr_1726
    [[nodiscard]] const std::string& key() const { return key_; }
    [[nodiscard]] MetricMode mode() const { return mode_; }
    void setMode(MetricMode mode) { mode_ = mode; }
    void reset() { value_ = previous_ = 0; }
    /// Re-sends the current value (original: _SafeStr_1725).
    void touch();

protected:
    void customDispose() override;

private:
    std::string key_;
    MetricMode mode_;
    double value_ = 0;
    double previous_ = 0;
};

/// Original: interface _SafeCls_3138.
class Condition : public Disposable {
public:
    [[nodiscard]] bool fulfilled() const { return fulfilled_; }
    Signal<Condition*> signalChanged; ///< Original: _SafeStr_1722.

protected:
    void setFulfilled(bool value);
    void customDispose() override;
    bool fulfilled_ = false;
};

enum class Comparison { Equal, NotEqual, Less, LessOrEqual, Greater, GreaterOrEqual }; ///< _SafeCls_2958

/// Original: _SafeCls_3317.
class MetricCondition : public Condition {
public:
    MetricCondition(std::shared_ptr<Metric> metric, Comparison comparison, double constant);
    [[nodiscard]] Metric* metric() const { return metric_.get(); }
    [[nodiscard]] double constant() const { return constant_; } ///< _SafeStr_3325

private:
    void evaluate(); ///< _SafeStr_3323
    void onMetricChanged(Metric*, const std::string&);
    std::shared_ptr<Metric> metric_;
    Comparison comparison_;
    double constant_;
};

enum class LogicalOperator { And, Or }; ///< _SafeCls_1860

/// Original: _SafePkg_1601/LogicalCondition.
class LogicalCondition : public Condition {
public:
    LogicalCondition(LogicalOperator op, std::vector<std::shared_ptr<Condition>> operands);

private:
    void evaluate();
    void onOperandChanged(Condition*) { evaluate(); }
    LogicalOperator op_;
    std::vector<std::shared_ptr<Condition>> operands_;
};

/// A condition that latches: once fulfilled it stays unlocked until reset
/// (original: _SafeCls_3336).
class Achievement : public Condition {
public:
    explicit Achievement(std::shared_ptr<Condition> condition, bool manualVerify = false);

    Signal<Achievement*> signalUnlocked; ///< Original: _SafeStr_3338.
    [[nodiscard]] Condition* condition() const { return condition_.get(); }

    void verify(); ///< _SafeStr_3323
    void reset();
    void unlock();
    void setUnlockedSilently() { fulfilled_ = true; } ///< _SafeStr_3341

protected:
    void customDispose() override;

private:
    void onConditionChanged(Condition*) { verify(); }
    std::shared_ptr<Condition> condition_;
    bool manualVerify_;
};

} // namespace takeover
