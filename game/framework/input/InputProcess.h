// InputProcess.h - a link of the battle's input chain (original:
// _SafePkg_1320/_SafeCls_1319 and _SafeCls_1936).
//
// Every frame the battle hands the input to each process in turn (HUD
// panel, building windows, spell casting, squad orders...). A process
// returns true when it used the input: the following ones then run
// "locked" and only tidy up.
#pragma once

#include "game/framework/core/Disposable.h"

#include <functional>

namespace takeover {

    class IInputProcess {
    public:
        virtual ~IInputProcess() = default;
        [[nodiscard]] virtual bool isEnabled() const = 0;
        virtual void setEnabled(bool enabled) = 0;

        /// Handles this frame's input. Returns true if it took it.
        virtual bool process(double timeElapsed, bool locked) = 0;
        virtual void reset() = 0;
    };

    /// A process made of functions (original: _SafeCls_1936).
    class FunctionProcess : public Disposable, public IInputProcess {
    public:
        using ProcessFunction = std::function<bool(double, bool)>;

        explicit FunctionProcess(ProcessFunction process, std::function<void()> reset = nullptr, bool enabled = true,
                                 std::function<void(bool)> setEnabled = nullptr, std::function<bool()> getEnabled = nullptr)
            : process_(std::move(process)), reset_(std::move(reset)), enabled_(enabled), setEnabled_(std::move(setEnabled)),
              getEnabled_(std::move(getEnabled)) {}

        [[nodiscard]] bool isEnabled() const override { return getEnabled_ ? getEnabled_() : enabled_; }
        void setEnabled(bool enabled) override {
            if (setEnabled_)
                setEnabled_(enabled);
            else
                enabled_ = enabled;
        }

        bool process(double timeElapsed, bool locked) override { return process_(timeElapsed, locked); }
        void reset() override {
            if (reset_)
                reset_();
        }

    protected:
        void customDispose() override {
            process_ = nullptr, reset_ = nullptr, setEnabled_ = nullptr, getEnabled_ = nullptr;
        }

    private:
        ProcessFunction process_;          ///< Original: _SafeStr_1940.
        std::function<void()> reset_;      ///< Original: _SafeStr_1939.
        bool enabled_;                     ///< Original: _SafeStr_1335.
        std::function<void(bool)> setEnabled_; ///< Original: _SafeStr_1937.
        std::function<bool()> getEnabled_;     ///< Original: _SafeStr_1938.
    };

} // namespace takeover
