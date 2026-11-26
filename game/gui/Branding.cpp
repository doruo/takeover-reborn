#include "game/gui/Branding.h"

#include "engine/platform/Application.h"
#include "game/framework/Symbols.h"
#include "game/gui/GameScreen.h"

namespace takeover {

    using engine::MouseEvent;

    Branding::Branding(engine::MovieClip* iriy, engine::MovieClip* armor, engine::MovieClip* a10com, bool preloader)
        : iriy_(sharedClip(iriy)), a10_(sharedClip(a10com)) {
        if (A10_COM) {
            enableA10Link(a10_.get(), true);

            if (iriy_)
                iriy_->setVisible(false);
        } else {
            enableIriyLinks(iriy_.get(), true);

            if (a10_)
                a10_->setVisible(false);
        }

        if (preloader && A10_COM) {
            armor->setVisible(false);
        } else {
            armor_ = std::make_unique<MovieClipButton>(sharedClip(armor));
            armor_->button()->addEventListener(MouseEvent::CLICK, this, &Branding::onArmorGames);
        }
    }

    void Branding::dispose() {
        if (armor_) {
            armor_->button()->removeEventListener(MouseEvent::CLICK, this, &Branding::onArmorGames);
            armor_.reset();
        }

        if (iriy_) {
            enableIriyLinks(iriy_.get(), false);
            iriy_.reset();
        }

        if (a10_) {
            enableA10Link(a10_.get(), false);
            a10_.reset();
        }
    }

    void Branding::enableIriyLinks(engine::MovieClip* logo, bool enable) {
        if (logo == nullptr)
            return;

        const auto link = [&](std::string_view name, void (Branding::*handler)(MouseEvent&)) {
            engine::MovieClip* button = symbols::child(logo, name);
            if (button == nullptr)
                return;

            if (enable)
                button->addEventListener(MouseEvent::CLICK, this, handler);
            else
                button->removeEventListener(MouseEvent::CLICK, this, handler);
        };

        link("btnIriySoft", &Branding::onIriySoft);
        link("btnIriyFacebook", &Branding::onFacebook);
        link("btnIriyTwitter", &Branding::onTwitter);
    }

    void Branding::enableA10Link(engine::MovieClip* logo, bool enable) {
        engine::MovieClip* button = symbols::child(logo, "btnA10Com");
        if (button == nullptr)
            return;

        if (enable)
            button->addEventListener(MouseEvent::CLICK, this, &Branding::onA10);
        else
            button->removeEventListener(MouseEvent::CLICK, this, &Branding::onA10);
    }

    void Branding::onA10(MouseEvent&) { engine::navigateToURL(URL_A10COM); }
    void Branding::onTwitter(MouseEvent&) { engine::navigateToURL(URL_IRIYSOFT_TWITTER); }
    void Branding::onFacebook(MouseEvent&) { engine::navigateToURL(URL_IRIYSOFT_FACEBOOK); }
    void Branding::onIriySoft(MouseEvent&) { engine::navigateToURL(URL_IRIYSOFT); }
    void Branding::onArmorGames(MouseEvent&) { engine::navigateToURL(URL_ARMOR_GAMES); }

} // namespace takeover
