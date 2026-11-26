#include "game/framework/ui/Button.h"

#include "game/framework/Symbols.h"
#include "game/media/SoundConst.h"

namespace takeover {

using engine::MouseEvent;

Button& Button::init(ButtonDef def) {
    def_ = std::move(def);
    behavior_ = std::make_unique<MovieClipButton>(def_.movie);
    if (def_.caption) setCaption(*def_.caption);
    engine::MovieClip* clip = behavior_->button();
    clip->addEventListener(MouseEvent::CLICK, this, &Button::onMouseClick);
    clip->addEventListener(MouseEvent::ROLL_OVER, this, &Button::onRollOver);
    clip->addEventListener(MouseEvent::ROLL_OUT, this, &Button::onRollOut);
    overSound = SoundId::BUTTON_OVER;
    clickSound = SoundId::BUTTON_CLICK;
    clickable_ = true;
    return *this;
}

void Button::customDispose() {
    if (behavior_) {
        setReadOnly(false);
        engine::MovieClip* clip = behavior_->button();
        clip->removeEventListener(MouseEvent::CLICK, this, &Button::onMouseClick);
        clip->removeEventListener(MouseEvent::ROLL_OVER, this, &Button::onRollOver);
        clip->removeEventListener(MouseEvent::ROLL_OUT, this, &Button::onRollOut);
        behavior_.reset();
    }
    def_.movie.reset();
    signalClick.dispose();
    signalRollOver.dispose();
    signalRollOut.dispose();
    Disposable::customDispose();
}

std::string Button::caption() const {
    auto* field = symbols::find<engine::TextField>(*behavior_->button(), "mcText.txt");
    return field ? field->htmlText() : std::string();
}

void Button::setCaption(const std::string& html) { symbols::setHtmlText(*behavior_->button(), "mcText.txt", html); }

void Button::onMouseClick(MouseEvent&) {
    signalClick.dispatch(this);
    SoundConst::play(clickSound);
}

void Button::onRollOver(MouseEvent&) {
    signalRollOver.dispatch(this);
    SoundConst::play(overSound);
}

void Button::onRollOut(MouseEvent&) { signalRollOut.dispatch(this); }

void Button::setReadOnly(bool readOnly) {
    engine::MovieClip* clip = behavior_->button();
    behavior_->setEnabled(!readOnly);
    clip->setButtonMode(!readOnly);
    clip->setUseHandCursor(!readOnly);
    clickable_ = !readOnly;
    clip->removeEventListener(MouseEvent::ROLL_OVER, this, &Button::onReadOnlyRollOver);
    clip->removeEventListener(MouseEvent::ROLL_OUT, this, &Button::onReadOnlyRollOut);
    if (readOnly) {
        clip->addEventListener(MouseEvent::ROLL_OVER, this, &Button::onReadOnlyRollOver);
        clip->addEventListener(MouseEvent::ROLL_OUT, this, &Button::onReadOnlyRollOut);
        clip->gotoAndStop("_up");
    }
}

void Button::onReadOnlyRollOut(MouseEvent&) { behavior_->button()->gotoAndStop("_up"); }
void Button::onReadOnlyRollOver(MouseEvent&) { behavior_->button()->gotoAndStop("_over"); }

} // namespace takeover
