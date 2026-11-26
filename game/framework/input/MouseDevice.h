// MouseDevice.h - polled mouse state (original: _SafePkg_776/_SafeStr_704).
//
// Listens to the mouse events reaching a display object (they bubble up
// from whatever is under the cursor) and answers, during the frame after
// the event, "was the button pressed / released / clicked, did it move?".
#pragma once

#include "engine/display/DisplayObject.h"
#include "game/framework/core/Disposable.h"

namespace takeover {

    class MouseDevice : public UpdatableObject {
    public:
        explicit MouseDevice(engine::DisplayObject* eventSource);

        // Original names: _SafeStr_1837, _SafeStr_1835, _SafeStr_1836, click, _SafeStr_1834.
        Signal<engine::MouseEvent&>& signalMouseDown() { return signalDown_; }
        Signal<engine::MouseEvent&>& signalMouseUp() { return signalUp_; }
        Signal<engine::MouseEvent&>& signalMouseMove() { return signalMove_; }
        Signal<engine::MouseEvent&>& signalClick() { return signalClick_; }
        Signal<engine::MouseEvent&>& signalDoubleClick() { return signalDoubleClick_; }

        /// Mouse position in the source object's space.
        [[nodiscard]] double x() const;
        [[nodiscard]] double y() const;

        void reset();
        void update(double timeElapsed) override;

        [[nodiscard]] bool buttonIsDown() const { return buttonDown_; }
        [[nodiscard]] bool buttonWasPressed() const { return pressedCountdown_ == 0; }
        [[nodiscard]] bool buttonWasReleased() const { return releasedCountdown_ == 0; }
        [[nodiscard]] bool wasMoved() const { return movedCountdown_ == 0; }
        [[nodiscard]] bool buttonWasClicked() const { return clickedCountdown_ == 0; }
        [[nodiscard]] bool buttonWasDoubleClicked() const { return doubleClickedCountdown_ == 0; } ///< _SafeStr_1838

    protected:
        void customDispose() override;

    private:
        static constexpr int kCheckDelay = 1;
        void onMouseDown(engine::MouseEvent& event);
        void onMouseUp(engine::MouseEvent& event);
        void onMouseMove(engine::MouseEvent& event);
        void onClick(engine::MouseEvent& event);
        void onDoubleClick(engine::MouseEvent& event);

        std::weak_ptr<engine::DisplayObject> source_;
        Signal<engine::MouseEvent&> signalDown_, signalUp_, signalMove_, signalClick_, signalDoubleClick_;
        bool buttonDown_ = false;

        // Frames since the event, counted down from kCheckDelay; 0 = "this frame".
        int pressedCountdown_ = -1, releasedCountdown_ = -1, movedCountdown_ = -1, clickedCountdown_ = -1,
            doubleClickedCountdown_ = -1;
    };

} // namespace takeover
