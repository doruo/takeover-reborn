// Button.h - a game button: a MovieClipButton with signals, a caption and
// sounds (original: _SafePkg_773/Button and its definition _SafeStr_686).
#pragma once

#include "game/framework/core/Disposable.h"
#include "game/framework/ui/MovieClipButton.h"

#include <memory>
#include <optional>
#include <string>

namespace takeover {

/// What a Button is built from (original: _SafeStr_686).
struct ButtonDef {
    std::shared_ptr<engine::MovieClip> movie; ///< Original: _SafeStr_1572.
    std::optional<std::string> caption;       ///< Shown in the clip's mcText.txt.
    int id = -1;                              ///< Original: _SafeStr_1573, "additional button id".

    static ButtonDef make(std::shared_ptr<engine::MovieClip> movie, std::optional<std::string> caption, int id = -1) {
        return {std::move(movie), std::move(caption), id};
    }
};

class Button : public Disposable {
public:
    Button() = default;
    explicit Button(ButtonDef def) { init(std::move(def)); }

    Button& init(ButtonDef def); ///< Original: _SafeStr_1706.

    Signal<Button*> signalClick;
    Signal<Button*> signalRollOver; ///< Original: _SafeStr_733.
    Signal<Button*> signalRollOut;  ///< Original: _SafeStr_714.

    /// Sound ids played on roll over and click (media::SoundId).
    int overSound = 0;  ///< Original: _SafeStr_1701.
    int clickSound = 0; ///< Original: _SafeStr_1704.

    [[nodiscard]] const ButtonDef& def() const { return def_; }
    [[nodiscard]] int id() const { return def_.id; } ///< Original: _SafeStr_1401.
    [[nodiscard]] bool enabled() const { return behavior_->isEnabled(); }
    void setEnabled(bool value) { behavior_->setEnabled(value); }
    [[nodiscard]] engine::MovieClip* image() const { return behavior_ ? behavior_->image() : nullptr; }

    [[nodiscard]] std::string caption() const;
    void setCaption(const std::string& html);

    /// Read-only mode: the button can't be clicked but still lights up
    /// under the cursor, so its hint can be shown (original: _SafeStr_1707).
    void setReadOnly(bool readOnly = true);
    [[nodiscard]] bool isClickable() const { return clickable_; } ///< Original: _SafeStr_1708.

protected:
    void customDispose() override;
    void onMouseClick(engine::MouseEvent& event);
    void onRollOver(engine::MouseEvent& event); ///< Original: _SafeStr_1700.
    void onRollOut(engine::MouseEvent& event);  ///< Original: _SafeStr_1703.

private:
    void onReadOnlyRollOver(engine::MouseEvent& event);
    void onReadOnlyRollOut(engine::MouseEvent& event);

    ButtonDef def_;
    std::unique_ptr<MovieClipButton> behavior_;
    bool clickable_ = true;
};

} // namespace takeover
