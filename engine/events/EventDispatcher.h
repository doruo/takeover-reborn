// EventDispatcher.h - flash.events.EventDispatcher.
//
//     button->addEventListener(MouseEvent::CLICK, this, &Button::onMouseClick);
//     button->removeEventListener(MouseEvent::CLICK, this, &Button::onMouseClick);
//
// A handler may take the concrete event type (`void onMouseClick(MouseEvent&)`);
// it must then only be registered for event types of that class.
#pragma once

#include "engine/core/Listener.h"
#include "engine/events/Event.h"

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace engine {

class EventDispatcher {
public:
    using Handler = Listener<Event&>;

    EventDispatcher() = default;
    virtual ~EventDispatcher();
    EventDispatcher(const EventDispatcher&) = delete;
    EventDispatcher& operator=(const EventDispatcher&) = delete;

    /// Registering the same handler twice for a type keeps one registration,
    /// as in Flash.
    void addEventListener(std::string_view type, Handler handler);
    void removeEventListener(std::string_view type, const ListenerKey& key);
    [[nodiscard]] bool hasEventListener(std::string_view type) const;

    template <typename Object, typename EventType>
    void addEventListener(std::string_view type, Object* object, void (Object::*method)(EventType&)) {
        static_assert(std::is_base_of_v<Event, EventType>);
        addEventListener(type, Handler{ListenerKey::of(object, method), [object, method](Event& event) {
                                           (object->*method)(static_cast<EventType&>(event));
                                       }});
    }

    template <typename Object, typename EventType>
    void removeEventListener(std::string_view type, Object* object, void (Object::*method)(EventType&)) {
        removeEventListener(type, ListenerKey::of(object, method));
    }

    /// Sends `event` to this object's listeners. Display objects override it
    /// to bubble up the display list. Returns false if a cancelable event was
    /// cancelled (never, here: kept for API fidelity).
    virtual bool dispatchEvent(Event& event);

    /// Sends Event::ENTER_FRAME to every dispatcher that listens to it, on
    /// the display list or not (Flash broadcasts this event).
    static void broadcastEnterFrame();

protected:
    /// Runs the listeners registered on this object for `event`.
    void notifyListeners(Event& event);

private:
    struct Registration {
        std::string type;
        Handler handler;
        bool removed = false;
    };
    std::vector<Registration> registrations_;
    int dispatchDepth_ = 0; ///< > 0 while listeners run: removal is deferred.
};

} // namespace engine
