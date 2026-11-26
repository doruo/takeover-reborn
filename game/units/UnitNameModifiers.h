// UnitNameModifiers.h - how the names of unit animations are built
// (original: _SafePkg_805/UnitAnimNameModifier, UnitDirectionNameModifier,
// UnitPhaseNameModifier).
//
//     "Red" + "Archers2" + "Down" + "_Attack"  ->  RedArchers2Down_Attack
#pragma once

#include "game/framework/raster/RasterAnimation.h"

#include <array>
#include <string>

namespace takeover {

    /// Color prefix + base + phase (original: UnitAnimNameModifier).
    class UnitAnimNameModifier : public INameModifier {
    public:
        static constexpr int PHASE_ID_GO = 0;
        static constexpr int PHASE_ID_DIE = 1;
        static constexpr int PHASE_ID_ATTACK1 = 2;
        static constexpr int PHASE_ID_ATTACK2 = 3;
        static constexpr int PHASE_ID_SPECIAL = 4;

        UnitAnimNameModifier(int animCount = 3, std::string prefix = "") : prefix_(std::move(prefix)), count_(animCount) {}
        [[nodiscard]] int count() const override { return count_; }
        [[nodiscard]] std::string name(const std::string& base, int index) const override {
            static const std::array<const char*, 8> POSTFIX = {"_Go", "_Die", "_Attack", "SpecialA",
                                                               "SpecialB", "SpecialC", "SpecialD", "SpecialE"};
            return prefix_ + base + POSTFIX[static_cast<std::size_t>(index)];
        }

    private:
        std::string prefix_; ///< Original: _SafeStr_2449.
        int count_;          ///< Original: _SafeStr_2951.
    };

    /// Base + direction (original: UnitDirectionNameModifier).
    class UnitDirectionNameModifier : public INameModifier {
    public:
        [[nodiscard]] int count() const override { return 4; }
        [[nodiscard]] std::string name(const std::string& base, int index) const override {
            static const std::array<const char*, 4> DIRECTIONS = {"Down", "Left", "Up", "Right"};
            return base + DIRECTIONS[static_cast<std::size_t>(index)];
        }
    };

    /// "Clip_" + color + base + phase: the naming of an older art set, not in
    /// the final SWF (original: UnitPhaseNameModifier).
    class UnitPhaseNameModifier : public INameModifier {
    public:
        static constexpr int PHASE_ID_GO = 0;
        static constexpr int PHASE_ID_DIE = 1;
        static constexpr int PHASE_ID_ATTACK1 = 2;
        static constexpr int PHASE_ID_ATTACK2 = 3;
        static constexpr int PHASE_ID_SPECIAL = 4;

        explicit UnitPhaseNameModifier(const std::string& color = "") : prefix_("Clip_" + color) {}
        [[nodiscard]] int count() const override { return 4; }
        [[nodiscard]] std::string name(const std::string& base, int index) const override {
            static const std::array<const char*, 4> POSTFIX = {"_Go", "_Die", "_Attack1", "_Attack2"};
            return prefix_ + base + POSTFIX[static_cast<std::size_t>(index)];
        }

    private:
        std::string prefix_;
    };

} // namespace takeover
