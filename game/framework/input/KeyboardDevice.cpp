#include "game/framework/input/KeyboardDevice.h"

namespace takeover {

    using engine::KeyboardEvent;

    KeyboardDevice::KeyboardDevice(engine::DisplayObject* eventSource) : source_(eventSource->weak_from_this()) {
        reset();
        eventSource->addEventListener(KeyboardEvent::KEY_DOWN, this, &KeyboardDevice::onKeyDown);
        eventSource->addEventListener(KeyboardEvent::KEY_UP, this, &KeyboardDevice::onKeyUp);
    }

    void KeyboardDevice::customDispose() {
        if (auto source = source_.lock()) {
            source->removeEventListener(KeyboardEvent::KEY_DOWN, this, &KeyboardDevice::onKeyDown);
            source->removeEventListener(KeyboardEvent::KEY_UP, this, &KeyboardDevice::onKeyUp);
        }

        source_.reset();
        signalKeyDown_.dispose();
        signalKeyUp_.dispose();
        UpdatableObject::customDispose();
    }

    void KeyboardDevice::reset() {
        down_.clear();
        pressed_.clear();
        released_.clear();
    }

    void KeyboardDevice::update(double timeElapsed) {
        UpdatableObject::update(timeElapsed);

        // A press is reported for one frame after the event.
        for (auto* table : {&pressed_, &released_})
            for (auto it = table->begin(); it != table->end();)
                it = --it->second < 0 ? table->erase(it) : std::next(it);
    }

    void KeyboardDevice::onKeyDown(KeyboardEvent& event) {
        down_.insert(event.keyCode);
        pressed_[event.keyCode] = kCheckDelay;
        signalKeyDown_.dispatch(event);
    }

    void KeyboardDevice::onKeyUp(KeyboardEvent& event) {
        down_.erase(event.keyCode);
        released_[event.keyCode] = kCheckDelay;
        signalKeyUp_.dispatch(event);
    }

} // namespace takeover
