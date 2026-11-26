#include "engine/events/EventDispatcher.h"

#include <algorithm>

namespace engine {

    namespace {

        /// Dispatchers with at least one ENTER_FRAME listener, in registration order.
        std::vector<EventDispatcher*>& enterFrameListeners() {
            static std::vector<EventDispatcher*> list;
            return list;
        }

        void unregisterEnterFrame(EventDispatcher* dispatcher) {
            auto& list = enterFrameListeners();
            std::replace(list.begin(), list.end(), dispatcher, static_cast<EventDispatcher*>(nullptr));
        }

    } // namespace

    EventDispatcher::~EventDispatcher() { unregisterEnterFrame(this); }

    void EventDispatcher::broadcastEnterFrame() {
        auto& list = enterFrameListeners();

        // Entries are nulled (not erased) on removal so this loop stays valid
        // while listeners come and go; compact afterwards.
        for (std::size_t i = 0; i < list.size(); ++i) {
            if (EventDispatcher* dispatcher = list[i]) {
                Event event(Event::ENTER_FRAME);
                dispatcher->dispatchEvent(event);
            }
        }

        std::erase(list, nullptr);
    }

    void EventDispatcher::addEventListener(std::string_view type, Handler handler) {
        for (auto& r : registrations_)
            if (!r.removed && r.type == type && r.handler.key == handler.key)
                return;
        registrations_.push_back({std::string(type), std::move(handler), false});

        if (type == Event::ENTER_FRAME) {
            auto& list = enterFrameListeners();
            if (std::find(list.begin(), list.end(), this) == list.end())
                list.push_back(this);
        }
    }

    void EventDispatcher::removeEventListener(std::string_view type, const ListenerKey& key) {
        for (auto& r : registrations_)
            if (r.type == type && r.handler.key == key)
                r.removed = true;
        if (dispatchDepth_ == 0)
            std::erase_if(registrations_, [](const Registration& r) { return r.removed; });

        if (type == Event::ENTER_FRAME && !hasEventListener(Event::ENTER_FRAME))
            unregisterEnterFrame(this);
    }

    bool EventDispatcher::hasEventListener(std::string_view type) const {
        return std::any_of(registrations_.begin(), registrations_.end(),
                           [&](const Registration& r) { return !r.removed && r.type == type; });
    }

    bool EventDispatcher::dispatchEvent(Event& event) {
        event.target_ = this;
        notifyListeners(event);

        return true;
    }

    void EventDispatcher::notifyListeners(Event& event) {
        event.currentTarget_ = this;

        // Flash snapshots the listener list: listeners added during dispatch
        // don't run for this event; removed ones are skipped.
        const std::size_t count = registrations_.size();
        ++dispatchDepth_;

        for (std::size_t i = 0; i < count && i < registrations_.size(); ++i) {
            if (registrations_[i].removed || registrations_[i].type != event.type())
                continue;

            auto call = registrations_[i].handler.call; // copy: the vector may grow
            call(event);

            if (event.isImmediatePropagationStopped())
                break;
        }

        if (--dispatchDepth_ == 0)
            std::erase_if(registrations_, [](const Registration& r) { return r.removed; });
    }

} // namespace engine
