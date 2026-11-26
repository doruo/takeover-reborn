// Event.h - flash.events.Event, MouseEvent and KeyboardEvent.
//
// Only what the game uses: no capture phase, no priorities, no weak
// references. Display objects add bubbling (see DisplayObject::dispatchEvent).
#pragma once

#include <string>
#include <string_view>

namespace engine {

    class DisplayObject;
    class EventDispatcher;

    class Event {
    public:
        // Event types, same strings as Flash.
        static constexpr std::string_view ENTER_FRAME = "enterFrame";
        static constexpr std::string_view ADDED = "added";
        static constexpr std::string_view ADDED_TO_STAGE = "addedToStage";
        static constexpr std::string_view REMOVED_FROM_STAGE = "removedFromStage";
        static constexpr std::string_view COMPLETE = "complete";
        static constexpr std::string_view SOUND_COMPLETE = "soundComplete";

        explicit Event(std::string_view type, bool bubbles = false, bool cancelable = false)
            : type_(type), bubbles_(bubbles), cancelable_(cancelable) {}
        virtual ~Event() = default;

        [[nodiscard]] const std::string& type() const { return type_; }
        [[nodiscard]] bool bubbles() const { return bubbles_; }
        [[nodiscard]] bool cancelable() const { return cancelable_; }

        /// The object the event was dispatched to.
        [[nodiscard]] EventDispatcher* target() const { return target_; }
        /// The object whose listeners are running now (differs while bubbling).
        [[nodiscard]] EventDispatcher* currentTarget() const { return currentTarget_; }

        /// Stops bubbling after the listeners of the current object.
        void stopPropagation() { propagationStopped_ = true; }
        /// Stops bubbling and skips the remaining listeners of the current object.
        void stopImmediatePropagation() {
            propagationStopped_ = true;
            immediatePropagationStopped_ = true;
        }

        [[nodiscard]] bool isPropagationStopped() const { return propagationStopped_; }
        [[nodiscard]] bool isImmediatePropagationStopped() const { return immediatePropagationStopped_; }

        /// Flash asks for a render between frames; we always render once per
        /// frame, so this does nothing (kept so ported code reads the same).
        void updateAfterEvent() {}

    private:
        friend class EventDispatcher;
        friend class DisplayObject;

        std::string type_;
        bool bubbles_ = false;
        bool cancelable_ = false;
        EventDispatcher* target_ = nullptr;
        EventDispatcher* currentTarget_ = nullptr;
        bool propagationStopped_ = false;
        bool immediatePropagationStopped_ = false;
    };

    class MouseEvent : public Event {
    public:
        static constexpr std::string_view CLICK = "click";
        static constexpr std::string_view DOUBLE_CLICK = "doubleClick";
        static constexpr std::string_view MOUSE_DOWN = "mouseDown";
        static constexpr std::string_view MOUSE_UP = "mouseUp";
        static constexpr std::string_view MOUSE_MOVE = "mouseMove";
        static constexpr std::string_view MOUSE_OVER = "mouseOver";
        static constexpr std::string_view MOUSE_OUT = "mouseOut";
        static constexpr std::string_view ROLL_OVER = "rollOver";
        static constexpr std::string_view ROLL_OUT = "rollOut";
        static constexpr std::string_view MOUSE_WHEEL = "mouseWheel";

        explicit MouseEvent(std::string_view type, bool bubbles = true, bool cancelable = false)
            : Event(type, bubbles, cancelable) {}

        double localX = 0; ///< Position in the target's own space.
        double localY = 0;
        double stageX = 0;
        double stageY = 0;
        bool buttonDown = false;
        bool ctrlKey = false;
        bool altKey = false;
        bool shiftKey = false;
        int delta = 0; ///< Wheel lines.
        DisplayObject* relatedObject = nullptr;
    };

    class KeyboardEvent : public Event {
    public:
        static constexpr std::string_view KEY_DOWN = "keyDown";
        static constexpr std::string_view KEY_UP = "keyUp";

        explicit KeyboardEvent(std::string_view type) : Event(type, true, false) {}

        unsigned keyCode = 0;  ///< flash.ui.Keyboard code (see engine/platform/Keyboard.h).
        unsigned charCode = 0; ///< Character typed, 0 if none.
        bool ctrlKey = false;
        bool altKey = false;
        bool shiftKey = false;
    };

} // namespace engine
