#include "game/framework/states/StateManager.h"

#include "game/framework/core/Trash.h"

#include <algorithm>

namespace takeover {

GameState* StateManager::topState() const { return states_.empty() ? nullptr : states_.back().get(); }

GameState* StateManager::stateBelowTop() const {
    return states_.size() < 2 ? nullptr : states_[states_.size() - 2].get();
}

void StateManager::update(double timeElapsed) {
    UpdatableObject::update(timeElapsed);
    // States may dispose themselves (or others) while updating: the index
    // is adjusted by onStateDisposed, as in the original.
    updating_ = true;
    updateIndex_ = static_cast<int>(states_.size()) - 1;
    while (updateIndex_ >= 0) {
        std::shared_ptr<GameState> state = states_[static_cast<std::size_t>(updateIndex_)];
        if (!state->isPaused()) state->update(timeElapsed);
        --updateIndex_;
        updateIndex_ = std::min(updateIndex_, static_cast<int>(states_.size()) - 1);
    }
    updating_ = false;
}

void StateManager::pushState(std::shared_ptr<GameState> state, bool modal) {
    if (modal)
        for (auto& s : states_)
            if (!s->isPaused()) s->pause();
    state->signalDisposed().addListener(this, &StateManager::onStateDisposed);
    state->modal_ = modal;
    states_.push_back(std::move(state));
}

std::shared_ptr<GameState> StateManager::popState() {
    if (states_.empty()) return nullptr;
    std::shared_ptr<GameState> state = states_.back();
    state->dispose();
    for (int s = static_cast<int>(states_.size()) - 1; s >= 0; --s) {
        states_[static_cast<std::size_t>(s)]->resume();
        if (states_[static_cast<std::size_t>(s)]->modal()) break;
    }
    return state;
}

void StateManager::clear() {
    while (!states_.empty()) states_.back()->dispose();
}

void StateManager::setState(std::shared_ptr<GameState> state) {
    pushState(std::move(state), false);
    while (states_.size() > 1) states_[states_.size() - 2]->dispose();
}

void StateManager::customDispose() {
    clear();
    UpdatableObject::customDispose();
}

void StateManager::onStateDisposed(Disposable* disposed) {
    const auto it = std::find_if(states_.begin(), states_.end(), [&](const auto& s) { return s.get() == disposed; });
    if (it == states_.end()) return;
    const int index = static_cast<int>(it - states_.begin());
    releaseLater(*it); // it may be running right now
    states_.erase(it);
    if (updating_ && index < updateIndex_) --updateIndex_;
}

} // namespace takeover
