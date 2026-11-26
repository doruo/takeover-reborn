// ButtonSet.h - a group of buttons with shared signals, and a two-state
// button (original: _SafePkg_773/ButtonSet and _SafeCls_1709).
#pragma once

#include "game/framework/ui/Button.h"
#include "game/media/SoundConst.h"

#include <memory>
#include <vector>

namespace takeover {

    class ButtonSet : public Disposable {
    public:
        Signal<Button*> signalClick;    ///< Original: _SafeStr_2961.
        Signal<Button*> signalRollOver; ///< Original: _SafeStr_2959.
        Signal<Button*> signalRollOut;  ///< Original: _SafeStr_2960.

        void addButton(std::shared_ptr<Button> button) { ///< _SafeStr_1847
            button->signalClick.addListener(this, &ButtonSet::onClick);
            button->signalRollOver.addListener(this, &ButtonSet::onRollOver);
            button->signalRollOut.addListener(this, &ButtonSet::onRollOut);
            buttons_.push_back(std::move(button));
        }

        [[nodiscard]] Button* getButton(int index) const { return buttons_[static_cast<std::size_t>(index)].get(); } ///< _SafeStr_2964
        [[nodiscard]] int count() const { return static_cast<int>(buttons_.size()); } ///< _SafeStr_2965

    protected:
        void customDispose() override {
            for (auto& button : buttons_)
                if (button)
                    button->dispose();
            buttons_.clear();
            signalClick.dispose();
            signalRollOver.dispose();
            signalRollOut.dispose();
        }

    private:
        void onClick(Button* button) { signalClick.dispatch(button); }
        void onRollOver(Button* button) { signalRollOver.dispatch(button); }
        void onRollOut(Button* button) { signalRollOut.dispatch(button); }

        std::vector<std::shared_ptr<Button>> buttons_; ///< Original: _SafeStr_1415.
    };

    /// A button with a second look for its "on" state; the second clip
    /// replaces the first while on (original: _SafeCls_1709).
    class SwitchButton : public Button {
    public:
        SwitchButton(ButtonDef off, ButtonDef on) {
            init(std::move(off));
            onButton_ = std::make_shared<Button>(std::move(on));
            onButton_->signalClick.addListener([this](Button*) { forwardClick(); });
            onButton_->signalRollOver.addListener([this](Button*) { signalRollOver.dispatch(this); });
            onButton_->signalRollOut.addListener([this](Button*) { signalRollOut.dispatch(this); });
            setHidden(false);
        }

        /// Places both looks (original: _SafeStr_1719).
        void moveTo(int x, int y) {
            image()->setX(x), image()->setY(y);
            onButton_->image()->setX(x), onButton_->image()->setY(y);
        }

        /// Hides the button completely, or shows it "off" (original: _SafeStr_1716).
        void setHidden(bool hidden) {
            if (!hidden) {
                show(*onButton_, false);
                show(*this, true);
                on_ = false;
            } else {
                show(*this, false);
                show(*onButton_, false);
            }
        }

        [[nodiscard]] bool isOn() const { return on_; } ///< _SafeStr_1713
        void setOn(bool on) {
            if (on_ == on)
                return;

            if (on_) {
                show(*onButton_, false);
                show(*this, true);
                on_ = false;
            } else {
                show(*onButton_, true);
                show(*this, false);
                on_ = true;
            }
        }

    protected:
        void customDispose() override {
            if (onButton_)
                onButton_->dispose();

            onButton_.reset();
            Button::customDispose();
        }

    private:
        void forwardClick() {
            signalClick.dispatch(this);
            SoundConst::play(clickSound);
        }

        static void show(Button& button, bool enable) { ///< _SafeStr_1711
            button.setEnabled(enable);
            button.image()->setVisible(enable);
        }

        std::shared_ptr<Button> onButton_; ///< Original: _SafeStr_1710.
        bool on_ = false;                  ///< Original: _SafeStr_1712.
    };

} // namespace takeover
