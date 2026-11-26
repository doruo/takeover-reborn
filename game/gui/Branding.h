// Branding.h - the IriySoft / Armor Games / A10 logos and their links
// (original: Branding). The port always uses the Armor Games branding (the
// original picked A10 when served from a10.com).
#pragma once

#include "engine/display/MovieClip.h"
#include "game/framework/ui/MovieClipButton.h"

#include <memory>

namespace takeover {

    class Branding {
    public:
        static constexpr const char* URL_ARMOR_GAMES = "https://armorgames.com/";
        static constexpr const char* URL_A10COM = "https://www.a10.com/";
        static constexpr const char* URL_IRIYSOFT = "https://www.iriysoft.com";
        static constexpr const char* URL_IRIYSOFT_TWITTER = "https://x.com/iriysoft";
        static constexpr const char* URL_IRIYSOFT_FACEBOOK = "https://www.facebook.com/IriySoft";

        /// Original: Preloader.A10Com.
        static constexpr bool A10_COM = false;

        Branding(engine::MovieClip* iriy, engine::MovieClip* armor, engine::MovieClip* a10com, bool preloader = false);
        ~Branding() { dispose(); }
        void dispose();

    private:
        void enableIriyLinks(engine::MovieClip* logo, bool enable); ///< _SafeStr_97
        void enableA10Link(engine::MovieClip* logo, bool enable);   ///< _SafeStr_96
        void onA10(engine::MouseEvent&);
        void onTwitter(engine::MouseEvent&);
        void onFacebook(engine::MouseEvent&);
        void onIriySoft(engine::MouseEvent&);
        void onArmorGames(engine::MouseEvent&);

        std::shared_ptr<engine::MovieClip> iriy_;
        std::shared_ptr<engine::MovieClip> a10_;
        std::unique_ptr<MovieClipButton> armor_;
    };

} // namespace takeover
