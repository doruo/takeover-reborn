#include "game/gui/PreloaderScreen.h"

#include "game/framework/Symbols.h"
#include "game/gui/GameScreen.h"

#include <cmath>

namespace takeover {

PreloaderScreen::PreloaderScreen(int statusTextNum, bool showButtonAfterComplete) : showButton_(showButtonAfterComplete) {
    clip_ = symbols::create("blackBack");
    clip_->removeChildAt(clip_->numChildren() - 1);
    clip_->gotoAndStop(FRAME_LOADING);
    // Children with short obfuscated names: _lrstf IriySoft logo, _bragsm
    // Armor Games logo, tprc_ percent, _tstt status, dt0_/dt2_ status texts,
    // dtp_ the "%" suffix, rbt_ progress bar, cmt_ advertisement place.
    branding_ = std::make_unique<Branding>(clip("_lrstf"), clip("_bragsm"), clip("mcLogoA10Com"), true);
    engine::TextField* percent = text("tprc_");
    engine::TextField* status = text("_tstt");
    status->setHtmlText(statusTextNum == 0 ? text("dt0_")->text() : text("dt2_")->text());
    const engine::Rectangle r = status->getBounds(clip_.get());
    percent->setX((r.x + r.right() + status->textWidth() + 10) / 2);
    const double delta = (10 + percent->width()) / 2;
    percent->setX(percent->x() - delta);
    status->setX(status->x() - delta);
    percent->setHtmlText("0" + text("dtp_")->text());
    if (auto* ads = symbols::child<engine::DisplayObject>(*clip_, "cmt_")) ads->setVisible(false);
}

engine::TextField* PreloaderScreen::text(std::string_view name) const {
    return symbols::child<engine::TextField>(*clip_, name);
}

engine::MovieClip* PreloaderScreen::clip(std::string_view name) const { return symbols::child(*clip_, name); }

void PreloaderScreen::customDispose() {
    signalStart.dispose();
    branding_.reset();
    if (startButton_) {
        startButton_->button()->removeEventListener(engine::MouseEvent::CLICK, this, &PreloaderScreen::onStartClick);
        startButton_.reset();
    }
    if (clip_ && clip_->parent()) clip_->parent()->removeChild(clip_.get());
    Disposable::customDispose();
}

void PreloaderScreen::setProgress(double fraction) {
    const int percent = static_cast<int>(std::round(fraction * 100));
    text("tprc_")->setHtmlText(std::to_string(percent) + text("dtp_")->text());
    engine::MovieClip* bar = clip("rbt_");
    bar->gotoAndStop(static_cast<int>(std::round(bar->totalFrames() * fraction + 1)));
}

void PreloaderScreen::complete() {
    if (showButton_) {
        clip_->gotoAndStop(FRAME_COMPLETE);
        startButton_ = std::make_unique<MovieClipButton>(sharedClip(clip("butStart")));
        startButton_->button()->addEventListener(engine::MouseEvent::CLICK, this, &PreloaderScreen::onStartClick);
    } else {
        signalStart.dispatch();
    }
}

void PreloaderScreen::onStartClick(engine::MouseEvent&) { signalStart.dispatch(); }

std::shared_ptr<engine::MovieClip> createVersionLabel() {
    auto label = symbols::create("VersionNumClass");
    symbols::setHtmlText(*label, "txtCaption", "ver. 1.1.5");
    return label;
}

} // namespace takeover
