// KeyboardDevice.h - polled keyboard state (original: _SafePkg_775).
//
// Listens to key events on a display object and answers, during the frame
// after the event, "is this key down / was it just pressed / released?".
#pragma once

#include "engine/display/DisplayObject.h"
#include "game/framework/core/Disposable.h"

#include <map>
#include <set>

namespace takeover {

    class KeyboardDevice : public UpdatableObject {
    public:
        explicit KeyboardDevice(engine::DisplayObject* eventSource);

        Signal<engine::KeyboardEvent&>& signalKeyDown() { return signalKeyDown_; } ///< Original: _SafeStr_1686.
        Signal<engine::KeyboardEvent&>& signalKeyUp() { return signalKeyUp_; }     ///< Original: _SafeStr_1685.

        void reset();
        void update(double timeElapsed) override;

        [[nodiscard]] bool isKeyDown(unsigned keyCode) const { return down_.contains(keyCode); }       ///< _SafeStr_1687
        [[nodiscard]] bool wasKeyPressed(unsigned keyCode) const { return pressed_.contains(keyCode); } ///< _SafeStr_1688
        [[nodiscard]] bool wasKeyReleased(unsigned keyCode) const { return released_.contains(keyCode); } ///< _SafeStr_1684

    protected:
        void customDispose() override;

    private:
        static constexpr int kCheckDelay = 1;
        void onKeyDown(engine::KeyboardEvent& event);
        void onKeyUp(engine::KeyboardEvent& event);

        std::weak_ptr<engine::DisplayObject> source_;
        Signal<engine::KeyboardEvent&> signalKeyDown_;
        Signal<engine::KeyboardEvent&> signalKeyUp_;
        std::set<unsigned> down_;
        std::map<unsigned, int> pressed_;  ///< key -> frames left to report it
        std::map<unsigned, int> released_;
    };

} // namespace takeover
