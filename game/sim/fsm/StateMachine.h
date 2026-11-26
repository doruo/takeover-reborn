// StateMachine.h - a small table-driven state machine (original: package
// _SafePkg_836, plus _SafePkg_1542 for the stronghold flavour).
//
// States are objects with a unique number. Events are objects compared by
// identity. A transition says "in state S, event E leads to state T and runs
// action A". Strongholds use it to go Active -> Ruined -> Rebuilding -> Active.
//
//     StateMachine fsm(idle);
//     fsm.addTransition(FsmTransition(idle, &EV_GO, moving, std::make_shared<CallbackAction>([&] { start(); })));
//     fsm.handleEvent(EV_GO);  // runs start(), now in `moving`
#pragma once

#include "game/sim/base/BaseObject.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace takeover {

    /// An event of the machine. Events are compared by identity (original:
    /// _SafeCls_2207).
    class FsmEvent {
    public:
        virtual ~FsmEvent() = default;
        [[nodiscard]] bool compare(const FsmEvent& other) const { return this == &other; }
    };

    /// An event with a priority: when several happen in the same frame, the
    /// strongest wins (original: _SafePkg_1542/_SafeStr_1488).
    class PriorityEvent : public FsmEvent {
    public:
        explicit PriorityEvent(int eventPriority = 0) : priority(eventPriority) {}
        /// 1 if this one is stronger than `other`, -1 if weaker, else 0
        /// (original: _SafeStr_1537).
        [[nodiscard]] int comparePriority(const PriorityEvent& other) const {
            return priority > other.priority ? 1 : priority < other.priority ? -1 : 0;
        }

        [[nodiscard]] std::string toString() const { return "KnodeEvent: priority " + std::to_string(priority); }

        int priority; ///< Original: _SafeStr_2552.
    };

    /// A state. Every state created gets the next number (original: _SafeStr_828).
    class FsmState : public BaseObject {
    public:
        FsmState() : number_(nextNumber_++) {}
        /// Restarts the numbering (original: _SafeStr_888).
        static void resetCounter() { nextNumber_ = 0; }

        virtual void onEnter() {} ///< Original: _SafeStr_2673.
        virtual void onLeave() {} ///< Original: _SafeStr_2740.
        [[nodiscard]] int getNumber() const { return number_; } ///< Original: _SafeStr_2741.
        [[nodiscard]] bool compare(const FsmState& other) const { return number_ == other.number_; }

    private:
        static inline int nextNumber_ = 0; ///< Original: _SafeStr_2739.
        int number_;                       ///< Original: _SafeStr_2738.
    };

    class FsmTransition;

    /// What a transition does (original: interface _SafeCls_2269).
    class FsmAction {
    public:
        virtual ~FsmAction() = default;
        virtual void action(FsmTransition& transition) = 0;
    };

    /// Calls a function (original: _SafeStr_1498, which applied a method to an
    /// object with an argument list).
    class CallbackAction : public FsmAction {
    public:
        explicit CallbackAction(std::function<void()> callback) : callback_(std::move(callback)) {}
        void action(FsmTransition&) override { callback_(); }

    private:
        std::function<void()> callback_;
    };

    /// Does nothing (original: _SafeStr_1523).
    class NoAction : public FsmAction {
    public:
        void action(FsmTransition&) override {}
    };

    /// From `from`, `event` leads to `to` running `action` (original: _SafeStr_1492).
    class FsmTransition {
    public:
        FsmTransition(std::shared_ptr<FsmState> from, const FsmEvent* event, std::shared_ptr<FsmState> to,
                   std::shared_ptr<FsmAction> action)
            : from_(std::move(from)), event_(event), to_(std::move(to)), action_(std::move(action)) {}

        [[nodiscard]] const std::shared_ptr<FsmState>& getTo() const { return to_; }     ///< _SafeStr_3149
        [[nodiscard]] const std::shared_ptr<FsmState>& getFrom() const { return from_; } ///< _SafeStr_3151
        [[nodiscard]] const FsmEvent* getEvent() const { return event_; }                ///< _SafeStr_658
        [[nodiscard]] FsmAction* getAction() const { return action_.get(); }
        void run() { action_->action(*this); }                                           ///< _SafeStr_3150
        [[nodiscard]] bool matches(const FsmEvent& event) const { return event_->compare(event); } ///< _SafeStr_2770

    private:
        std::shared_ptr<FsmState> from_;   ///< Original: _SafeStr_3311.
        const FsmEvent* event_;            ///< Original: _SafeStr_3309.
        std::shared_ptr<FsmState> to_;     ///< Original: _SafeStr_3312.
        std::shared_ptr<FsmAction> action_; ///< Original: _SafeStr_3310.
    };

    /// The machine (original: _SafeStr_1515).
    class StateMachine {
    public:
        explicit StateMachine(std::shared_ptr<FsmState> initial) : state_(std::move(initial)) {}

        /// Runs the first transition of the current state matching `event`.
        /// Returns false when there is none (original: _SafeStr_1540).
        bool handleEvent(const FsmEvent& event);
        [[nodiscard]] FsmState* getState() const { return state_.get(); } ///< _SafeStr_1516
        /// Switches state, calling onLeave/onEnter (original: _SafeStr_743).
        void changeState(const std::shared_ptr<FsmState>& state);
        void addTransition(FsmTransition transition); ///< _SafeStr_822
        void dispose();

    private:
        std::map<int, std::vector<FsmTransition>> transitions_; ///< By source state number (original: _SafeStr_3148).
        std::shared_ptr<FsmState> state_;                    ///< Original: _SafeStr_1116.
    };

} // namespace takeover
