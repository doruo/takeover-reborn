#include "game/progress/Conditions.h"

#include <algorithm>

namespace takeover {

void Metric::setValue(double value) {
    double merged = value;
    if (mode_ == MetricMode::Max) merged = std::max(value_, value);
    else if (mode_ == MetricMode::Min) merged = std::min(value_, value);
    if (merged != value_) {
        previous_ = value_;
        value_ = merged;
        signalChanged.dispatch(this, key_);
    }
}

void Metric::touch() {
    previous_ = value_;
    signalChanged.dispatch(this, key_);
}

void Metric::customDispose() {
    signalChanged.dispose();
    Disposable::customDispose();
}

void Condition::setFulfilled(bool value) {
    const bool old = fulfilled_;
    fulfilled_ = value;
    if (old != fulfilled_) signalChanged.dispatch(this);
}

void Condition::customDispose() {
    signalChanged.dispose();
    Disposable::customDispose();
}

MetricCondition::MetricCondition(std::shared_ptr<Metric> metric, Comparison comparison, double constant)
    : metric_(std::move(metric)), comparison_(comparison), constant_(constant) {
    metric_->signalChanged.addListener(this, &MetricCondition::onMetricChanged);
    evaluate();
}

void MetricCondition::evaluate() {
    const double v = metric_->value();
    bool result = false;
    switch (comparison_) {
    case Comparison::Equal: result = v == constant_; break;
    case Comparison::NotEqual: result = v != constant_; break;
    case Comparison::Less: result = v < constant_; break;
    case Comparison::LessOrEqual: result = v <= constant_; break;
    case Comparison::Greater: result = v > constant_; break;
    case Comparison::GreaterOrEqual: result = v >= constant_; break;
    }
    setFulfilled(result);
}

void MetricCondition::onMetricChanged(Metric*, const std::string&) { evaluate(); }

LogicalCondition::LogicalCondition(LogicalOperator op, std::vector<std::shared_ptr<Condition>> operands)
    : op_(op), operands_(std::move(operands)) {
    for (auto& condition : operands_) condition->signalChanged.addListener(this, &LogicalCondition::onOperandChanged);
    evaluate();
}

void LogicalCondition::evaluate() {
    // As the original: the accumulator starts at true, even for OR.
    bool result = true;
    for (const auto& condition : operands_)
        result = op_ == LogicalOperator::And ? (result && condition->fulfilled()) : (result || condition->fulfilled());
    setFulfilled(result);
}

Achievement::Achievement(std::shared_ptr<Condition> condition, bool manualVerify)
    : condition_(std::move(condition)), manualVerify_(manualVerify) {
    if (!manualVerify_) condition_->signalChanged.addListener(this, &Achievement::onConditionChanged);
}

void Achievement::verify() {
    if (!fulfilled_ && condition_->fulfilled()) unlock();
}

void Achievement::reset() {
    if (fulfilled_) {
        fulfilled_ = false;
        signalChanged.dispatch(this);
    }
}

void Achievement::unlock() {
    if (!fulfilled_) {
        fulfilled_ = true;
        signalUnlocked.dispatch(this);
        signalChanged.dispatch(this);
    }
}

void Achievement::customDispose() {
    signalUnlocked.dispose();
    Condition::customDispose();
}

} // namespace takeover
