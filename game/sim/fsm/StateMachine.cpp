#include "game/sim/fsm/StateMachine.h"

namespace takeover {

bool StateMachine::handleEvent(const FsmEvent& event) {
    const auto it = transitions_.find(state_->getNumber());
    if (it == transitions_.end()) return false;
    for (FsmTransition& transition : it->second) {
        if (transition.matches(event)) {
            const std::shared_ptr<FsmState> to = transition.getTo(); // the action may add transitions
            transition.run();
            changeState(to);
            return true;
        }
    }
    return false;
}

void StateMachine::changeState(const std::shared_ptr<FsmState>& state) {
    if (!state_->compare(*state)) {
        state_->onLeave();
        state_ = state;
        state_->onEnter();
    }
}

void StateMachine::addTransition(FsmTransition transition) {
    transitions_[transition.getFrom()->getNumber()].push_back(std::move(transition));
}

void StateMachine::dispose() { transitions_.clear(); }

} // namespace takeover
