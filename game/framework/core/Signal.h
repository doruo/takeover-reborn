// Signal.h - the framework's typed observer (original: _SafePkg_29/Signal).
//
//     Signal<int> signalButton;
//     signalButton.addListener(this, &Main::onTitleButton);
//     signalButton.dispatch(TitleScreen::BUT_START);
//     signalButton.removeListener(this, &Main::onTitleButton);
//
// Same semantics as the original: adding the same listener twice keeps one
// registration; listeners added while dispatching don't run for that
// dispatch; removed ones stop at once. The original iterated a Dictionary
// (unspecified order); we use registration order.
#pragma once

#include "engine/core/Listener.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace takeover {

using engine::ListenerKey;

template <typename... Args>
class Signal {
public:
    using Listener = engine::Listener<Args...>;

    Signal() = default;
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    /// `signal.addListener(this, &Class::method)`; the method may take
    /// parameters the signal's arguments convert to.
    template <typename Object, typename Result, typename... Params>
    void addListener(Object* object, Result (Object::*method)(Params...)) {
        addListener(Listener{ListenerKey::of(object, method),
                             [object, method](Args... args) { (object->*method)(std::forward<Args>(args)...); }});
    }

    /// A lambda; keep the returned key to remove it.
    template <typename Callable>
    ListenerKey addListener(Callable&& callable) {
        Listener listener = Listener::function(std::forward<Callable>(callable));
        ListenerKey key = listener.key;
        addListener(std::move(listener));
        return key;
    }

    void addListener(Listener listener) {
        if (disposed_ || hasListener(listener.key)) return;
        listeners_.push_back({std::move(listener), false});
    }

    template <typename Object, typename Method>
    void removeListener(Object* object, Method method) {
        removeListener(ListenerKey::of(object, method));
    }

    void removeListener(const ListenerKey& key) {
        for (auto& entry : listeners_)
            if (entry.listener.key == key) entry.removed = true;
        compact();
    }

    template <typename Object, typename Method>
    [[nodiscard]] bool hasListener(Object* object, Method method) const {
        return hasListener(ListenerKey::of(object, method));
    }

    [[nodiscard]] bool hasListener(const ListenerKey& key) const {
        return std::any_of(listeners_.begin(), listeners_.end(),
                           [&](const Entry& e) { return !e.removed && e.listener.key == key; });
    }

    /// Original: _SafeStr_81.
    void removeAllListeners() {
        for (auto& entry : listeners_) entry.removed = true;
        compact();
    }

    void dispatch(Args... args) {
        const std::size_t count = listeners_.size();
        ++dispatching_;
        for (std::size_t i = 0; i < count && i < listeners_.size(); ++i) {
            if (listeners_[i].removed) continue;
            auto call = listeners_[i].listener.call; // copy: the list may change
            call(args...);
        }
        --dispatching_;
        compact();
    }

    /// Drops every listener; the signal accepts no new ones afterwards.
    void dispose() {
        removeAllListeners();
        disposed_ = true;
    }
    [[nodiscard]] bool isDisposed() const { return disposed_; }
    [[nodiscard]] std::size_t listenerCount() const {
        return static_cast<std::size_t>(
            std::count_if(listeners_.begin(), listeners_.end(), [](const Entry& e) { return !e.removed; }));
    }

private:
    struct Entry {
        Listener listener;
        bool removed = false;
    };
    void compact() {
        if (dispatching_ == 0) std::erase_if(listeners_, [](const Entry& e) { return e.removed; });
    }

    std::vector<Entry> listeners_;
    int dispatching_ = 0;
    bool disposed_ = false;
};

} // namespace takeover
