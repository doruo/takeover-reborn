// StateManager.h - a stack of game states (original: _SafePkg_774).
//
// The top state is the active one. Pushing a *modal* state pauses every
// state below it; popping resumes them down to the first modal one. Every
// state that isn't paused is updated, from the top of the stack down. A
// state leaves the stack by being disposed, from anywhere.
#pragma once

#include "game/framework/core/Disposable.h"

#include <memory>
#include <vector>

namespace takeover {

    /// Original: _SafePkg_774/_SafeStr_742.
    class GameState : public UpdatableObject {
    public:
        [[nodiscard]] bool modal() const { return modal_; }

    private:
        friend class StateManager;
        bool modal_ = false;
    };

    class StateManager : public UpdatableObject {
    public:
        StateManager() = default;

        /// Original: _SafeStr_1471.
        [[nodiscard]] GameState* topState() const;

        /// The state just below the top (original: _SafeStr_1473).
        [[nodiscard]] GameState* stateBelowTop() const;

        void update(double timeElapsed) override;

        virtual void pushState(std::shared_ptr<GameState> state, bool modal = true);

        /// Disposes the top state and resumes the ones below it, down to the
        /// first modal one (original: _SafeStr_1476).
        std::shared_ptr<GameState> popState();

        /// Disposes every state (original: _SafeStr_1475).
        void clear();

        /// Replaces the whole stack with `state` (original: _SafeStr_743).
        void setState(std::shared_ptr<GameState> state);

    protected:
        void customDispose() override;

    private:
        void onStateDisposed(Disposable* state);

        std::vector<std::shared_ptr<GameState>> states_;
        int updateIndex_ = 0;
        bool updating_ = false;
    };

} // namespace takeover
