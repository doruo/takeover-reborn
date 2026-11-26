// GameScreen.h - base of the menu and battle screens (original: gui/GameScreen).
//
// A screen is a GameState whose view is a library clip. Building one runs
// three steps that subclasses extend: initVisualBlock (create the clip),
// the input devices, initButtonsBlock (wire the clip's buttons). Every
// button reports its id through signalButton.
//
// C++ note: AS3 constructors call virtual methods freely; C++ ones can't
// (the subclass isn't built yet). So subclass constructors call initScreen()
// as their last statement, which is what the AS3 code did too.
#pragma once

#include "game/framework/input/KeyboardDevice.h"
#include "game/framework/input/MouseDevice.h"
#include "game/framework/states/StateManager.h"
#include "game/framework/ui/Button.h"
#include "game/gui/ScreenDefs.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace takeover {

    class MediaSwitchers;

    class GameScreen : public GameState {
    public:
        GameScreen() = default;
        ~GameScreen() override;

        /// The id of the button clicked (original: _SafeStr_667).
        Signal<int> signalButton;

        [[nodiscard]] engine::Sprite* image() const { return image_.get(); }
        [[nodiscard]] const std::shared_ptr<engine::Sprite>& imagePtr() const { return image_; }

        void update(double timeElapsed) override;

    protected:
        GameScreen& initScreen(std::shared_ptr<ScreenDef> def); ///< _SafeStr_3449

        void customDispose() override;

        virtual void initVisualBlock();
        virtual void disposeVisualBlock();
        virtual void initButtonsBlock() {}
        virtual void disposeButtonsBlock();
        void initDevices();     ///< _SafeStr_3377
        void disposeDevices();  ///< _SafeStr_3363
        void updateDevices(double timeElapsed); ///< _SafeStr_3382

        /// Per-frame work of the screen (original: _SafeStr_1428).
        virtual void updateScreen(double /*timeElapsed*/) {}

        [[nodiscard]] MouseDevice* mouse() const { return mouse_.get(); }          ///< _SafeStr_3450
        [[nodiscard]] KeyboardDevice* keyboard() const { return keyboard_.get(); } ///< _SafeStr_3451

        /// A button on a child clip of the view, reporting `id` (original: _SafeStr_1417).
        Button* addButton(engine::MovieClip* clip, std::optional<std::string> caption, int id);
        Button* addButton(std::string_view childName, std::optional<std::string> caption, int id);
        virtual void onButtonClick(Button* button);
        void sendButton(int id) { signalButton.dispatch(id); } ///< _SafeStr_3448

        /// The view's child clip by instance name.
        [[nodiscard]] engine::MovieClip* clip(std::string_view name) const;

        std::shared_ptr<ScreenDef> def_;
        std::shared_ptr<engine::Sprite> image_;
        std::shared_ptr<MediaSwitchers> mediaSwitchers_;
        std::vector<std::shared_ptr<Button>> buttons_;

    private:
        std::shared_ptr<KeyboardDevice> keyboard_;
        std::shared_ptr<MouseDevice> mouse_;
    };

    /// Shared pointer to a clip found in a view: Button needs ownership.
    std::shared_ptr<engine::MovieClip> sharedClip(engine::DisplayObject* clip);

} // namespace takeover
