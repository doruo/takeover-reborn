#include "game/framework/input/MouseDevice.h"

namespace takeover {

    using engine::MouseEvent;

    MouseDevice::MouseDevice(engine::DisplayObject* eventSource) : source_(eventSource->weak_from_this()) {
        reset();
        eventSource->addEventListener(MouseEvent::MOUSE_DOWN, this, &MouseDevice::onMouseDown);
        eventSource->addEventListener(MouseEvent::MOUSE_UP, this, &MouseDevice::onMouseUp);
        eventSource->addEventListener(MouseEvent::MOUSE_MOVE, this, &MouseDevice::onMouseMove);
        eventSource->addEventListener(MouseEvent::CLICK, this, &MouseDevice::onClick);
        eventSource->addEventListener(MouseEvent::DOUBLE_CLICK, this, &MouseDevice::onDoubleClick);
    }

    void MouseDevice::customDispose() {
        if (auto source = source_.lock()) {
            source->removeEventListener(MouseEvent::MOUSE_DOWN, this, &MouseDevice::onMouseDown);
            source->removeEventListener(MouseEvent::MOUSE_UP, this, &MouseDevice::onMouseUp);
            source->removeEventListener(MouseEvent::MOUSE_MOVE, this, &MouseDevice::onMouseMove);
            source->removeEventListener(MouseEvent::CLICK, this, &MouseDevice::onClick);
            source->removeEventListener(MouseEvent::DOUBLE_CLICK, this, &MouseDevice::onDoubleClick);
        }

        source_.reset();

        for (auto* s : {&signalDown_, &signalUp_, &signalMove_, &signalClick_, &signalDoubleClick_})
            s->dispose();

        UpdatableObject::customDispose();
    }

    double MouseDevice::x() const {
        auto source = source_.lock();
        return source ? source->mouseX() : 0;
    }

    double MouseDevice::y() const {
        auto source = source_.lock();
        return source ? source->mouseY() : 0;
    }

    void MouseDevice::reset() {
        pressedCountdown_ = releasedCountdown_ = movedCountdown_ = clickedCountdown_ = doubleClickedCountdown_ = -1;
    }

    void MouseDevice::update(double timeElapsed) {
        UpdatableObject::update(timeElapsed);
        --pressedCountdown_;
        --releasedCountdown_;
        --movedCountdown_;
        --clickedCountdown_;
        --doubleClickedCountdown_;
    }

    void MouseDevice::onMouseDown(MouseEvent& event) {
        buttonDown_ = true;
        pressedCountdown_ = kCheckDelay;
        signalDown_.dispatch(event);
    }

    void MouseDevice::onMouseUp(MouseEvent& event) {
        buttonDown_ = false;
        releasedCountdown_ = kCheckDelay;
        signalUp_.dispatch(event);
    }

    void MouseDevice::onMouseMove(MouseEvent& event) {
        movedCountdown_ = kCheckDelay;
        signalMove_.dispatch(event);
    }

    void MouseDevice::onClick(MouseEvent& event) {
        clickedCountdown_ = kCheckDelay;
        signalClick_.dispatch(event);
    }

    void MouseDevice::onDoubleClick(MouseEvent& event) {
        doubleClickedCountdown_ = kCheckDelay;
        signalDoubleClick_.dispatch(event);
    }

} // namespace takeover
