// Property.h - timed modifiers attached to a unit (original: package
// _SafePkg_897).
//
//   IProperty          interface of a modifier (original: _SafeCls_896)
//   Property           base with a unique id, a name and a "finished" flag
//                      (original: _SafeCls_903)
//   PropertyContainer  owns the properties of one unit, updates them and
//                      removes the finished ones. A property may veto the
//                      addition of another (poison immunity...) or alter it
//                      (shield wall reduces arrows) (original: _SafeCls_2428)
//   BoundedValue       an int with a base value, a current value and
//                      bounds (original: _SafeStr_1093)
#pragma once

#include "game/sim/base/BaseObject.h"

#include <map>
#include <memory>
#include <string>

namespace takeover {

    class IProperty {
    public:
        virtual ~IProperty() = default;
        virtual void updateProperty(double dt) = 0; ///< update
        virtual void finish() = 0;                  ///< _SafeStr_898
        virtual void onPropertyRemoved(IProperty& removed) = 0; ///< _SafeStr_902

        /// Called for every property already present when `added` arrives (and
        /// the other way round). Returning true vetoes the addition.
        virtual bool makeAddPropertyCollision(IProperty& added) = 0;
        [[nodiscard]] virtual bool isFinished() const = 0;   ///< _SafeStr_901
        [[nodiscard]] virtual int getPropertyId() const = 0; ///< _SafeStr_899
        [[nodiscard]] virtual const std::string& getName() const = 0; ///< _SafeStr_900
        virtual void disposeProperty() = 0;
    };

    class Property : public BaseObject, public IProperty {
    public:
        explicit Property(std::string name = "");

        void updateProperty(double dt) override { update(dt); }
        void finish() override { finished_ = true; }
        void onPropertyRemoved(IProperty&) override {}
        bool makeAddPropertyCollision(IProperty&) override { return false; }
        [[nodiscard]] bool isFinished() const override { return finished_; }
        [[nodiscard]] int getPropertyId() const override { return propertyId_; }
        [[nodiscard]] const std::string& getName() const override { return name_; }
        void disposeProperty() override { dispose(); }

    protected:
        int propertyId_;     ///< Original: _SafeStr_905.
        std::string name_;   ///< Original: _SafeStr_906.
        bool finished_ = false; ///< Original: _SafeStr_904.

    private:
        static inline int nextPropertyId_ = 0; ///< Original: _SafeStr_907.
    };

    class PropertyContainer : public BaseObject {
    public:
        PropertyContainer() = default;

        virtual void removeProperty(std::shared_ptr<IProperty> property);
        virtual bool addProperty(std::shared_ptr<IProperty> property);

        /// In id order (the original iterated an IntMap).
        [[nodiscard]] const std::map<int, std::shared_ptr<IProperty>>& properties() const { return properties_; } ///< _SafeStr_2430

        void customUpdate(double dt) override;
        void customDispose() override;

    protected:
        std::map<int, std::shared_ptr<IProperty>> properties_; ///< Original: _SafeStr_2429.
    };

    class BoundedValue {
    public:
        BoundedValue(std::string name = "", int base = 0, int min = 0, int max = 1, int value = 0)
            : name_(std::move(name)), base_(base), min_(min), max_(max) {
            setValue(value);
        }

        static int min(int a, int b) { return a > b ? b : a; } ///< _SafeStr_1735
        static int max(int a, int b) { return a < b ? b : a; } ///< _SafeStr_1730

        void setValue(int v) { value_ = clamp(v); }
        void setMin(int v) { min_ = v, setValue(value_); }     ///< _SafeStr_1733
        void setMax(int v) { max_ = v, setValue(value_); }     ///< _SafeStr_1734
        void setBase(int v) { base_ = clamp(v); }              ///< _SafeStr_1318
        void reset() { value_ = base_; }
        void add(int v) { setValue(value_ + v); }              ///< _SafeStr_651
        void addBase(int v) { setBase(base_ + v); }            ///< _SafeStr_1732
        [[nodiscard]] int getValue() const { return value_; }
        [[nodiscard]] int getBase() const { return base_; }
        [[nodiscard]] int getMin() const { return min_; }      ///< _SafeStr_1110 / _SafeStr_1731
        [[nodiscard]] int getMax() const { return max_; }      ///< _SafeStr_1102
        [[nodiscard]] const std::string& getName() const { return name_; }

    private:
        [[nodiscard]] int clamp(int v) const {
            const int low = v < min_ ? min_ : v;
            return low > max_ ? max_ : low;
        }

        std::string name_;
        int value_ = 0; ///< Original: _SafeStr_1720.
        int base_;      ///< Original: _SafeStr_1729.
        int min_;       ///< Original: _SafeStr_1727.
        int max_;       ///< Original: _SafeStr_1728.
    };

} // namespace takeover
