// HintWindow.h - tooltip windows (original: _SafeCls_2580, HintWindow2,
// HintLayer and the enums _SafeStr_2581 / _SafeStr_2583).
//
// A hint is a library clip with a "background" and a "text" field. The text
// field auto-sizes, the background is stretched around it with a margin,
// and the window docks to a side of a target rectangle, flipping to the
// other side when it would leave the allowed bounds.
#pragma once

#include "engine/display/MovieClip.h"
#include "engine/display/TextField.h"
#include "game/framework/core/Disposable.h"

#include <memory>
#include <string>
#include <vector>

namespace takeover {

    /// Which side of the target the hint goes (original: _SafeStr_2581).
    enum class DockSide { Left, Right, Top, Bottom };

    /// How it lines up along that side (original: _SafeStr_2583).
    enum class DockAlign { LeftTop, Center, RightBottom };

    DockSide opposite(DockSide side); ///< Original: _SafeStr_2600.

    class HintWindow {
    public:
        explicit HintWindow(std::shared_ptr<engine::Sprite> window, std::string_view backgroundName = "background",
                            std::string_view textName = "text");
        virtual ~HintWindow() = default;

        [[nodiscard]] engine::Sprite* image() const { return image_.get(); }
        [[nodiscard]] const std::shared_ptr<engine::Sprite>& imagePtr() const { return image_; }
        [[nodiscard]] engine::TextField* text() const { return text_; }
        [[nodiscard]] engine::DisplayObject* background() const { return background_; }

        /// With auto width the text field grows with its content; otherwise it
        /// keeps its authored width and wraps (original: _SafeStr_2593).
        [[nodiscard]] bool autoWidth() const { return autoWidth_; }
        void setAutoWidth(bool value);

        void setMargins(double horizontal, double vertical); ///< Original: _SafeStr_2597.
        void setText(const std::string& html);
        void setBounds(const engine::Rectangle& bounds);      ///< Original: _SafeStr_2599.

        /// Shows the hint docked to `target` (original: _SafeStr_2601).
        void show(const engine::Rectangle& target, DockSide side, DockAlign align, double margin);
        void hide();

    protected:
        void resize();
        void place(); ///< Original: _SafeStr_2589.
        void dock(engine::Point& position, DockSide side) const; ///< Original: _SafeStr_2596.
        [[nodiscard]] bool isOutOfBounds(DockSide side) const;
        void align(engine::Point& position, DockAlign alignment) const;

        std::shared_ptr<engine::Sprite> image_;
        engine::DisplayObject* background_ = nullptr;
        engine::TextField* text_ = nullptr;
        double authoredTextWidth_ = 0;
        bool autoWidth_ = true;
        double marginX_ = 0, marginY_ = 0;
        engine::Rectangle bounds_;
        engine::Rectangle target_;
        DockSide side_ = DockSide::Bottom;
        DockAlign align_ = DockAlign::LeftTop;
        double targetMargin_ = 0;
    };

    /// The hint used by the game: bounded to the stage, docked below its
    /// target by default (original: _SafePkg_773/HintWindow2).
    class HintWindow2 : public HintWindow {
    public:
        explicit HintWindow2(std::shared_ptr<engine::MovieClip> hint);

        DockSide dockSide = DockSide::Bottom;     ///< Original: _SafeStr_2602.
        DockAlign dockAlign = DockAlign::LeftTop; ///< Original: _SafeStr_2603.
        double margin = 5;

        void showAt(const engine::Rectangle& target) { show(target, dockSide, dockAlign, margin); } ///< _SafeStr_2604
        void dispose();
    };

    /// A sprite on top of everything that shows one hint at a time (original:
    /// _SafePkg_773/HintLayer).
    class HintLayer : public Disposable {
    public:
        HintLayer();

        [[nodiscard]] engine::Sprite* image() const { return image_.get(); }
        [[nodiscard]] const std::shared_ptr<engine::Sprite>& imagePtr() const { return image_; }

        [[nodiscard]] HintWindow2* getHintWindow(int id) const { return windows_[static_cast<std::size_t>(id)].get(); }
        int addHintWindow(std::shared_ptr<HintWindow2> window); ///< Original: _SafeStr_3070.

        void showHintAtObject(engine::DisplayObject* target, int hintId);
        void showWindowAtObject(engine::DisplayObject* target, HintWindow2* window); ///< _SafeStr_3156
        void showHintAtRect(const engine::Rectangle& rect, int hintId);
        void showHintWindowAtRect(const engine::Rectangle& rect, HintWindow2* window);
        void hideHint();

    protected:
        void customDispose() override;

    private:
        std::vector<std::shared_ptr<HintWindow2>> windows_;
        std::shared_ptr<engine::Sprite> image_;
    };

} // namespace takeover
