// SquadIcon.h - the round icon floating above a squad (original:
// _SafePkg_868/_SafeStr_1257).
//
// It shows the unit portrait in the race color, a health ring, the level
// stars, and two small animations: what the squad is doing (moving,
// fighting, repairing...) and its morale.
#pragma once

#include "game/sim/battle/Race.h"
#include "game/sim/squads/MoraleStatus.h"

#include <memory>
#include <optional>
#include <string>

namespace engine {
    class DisplayObjectContainer;
    class MovieClip;
    class Sprite;
}

namespace takeover {

    class ArmyStat;
    class FSquad;
    class SpriteAnimation;
    class TOResManager;
    class View;

    class SquadIcon {
    public:
        SquadIcon(FSquad& squad, TOResManager& resources);
        ~SquadIcon();

        /// Frame of the race color in icon clips (original: _SafeStr_2459).
        static int raceColorFrame(Race race);

        /// Library class of a unit portrait, "IconT2_W_ArchersClass" (original: _SafeStr_2460).
        static std::string unitIconClass(UnitType type, Race race, int techLevel);

        /// Replaces the content of a container by a picture of it (original:
        /// Utils._SafeStr_2466).
        static void rasterizeContents(engine::DisplayObjectContainer& container);

        [[nodiscard]] const std::shared_ptr<engine::Sprite>& icon() const { return icon_; }
        /// The clickable part (original: _SafeStr_1273).
        [[nodiscard]] const std::shared_ptr<engine::Sprite>& body() const { return body_; }
        void dispose();
        void update(double timeElapsed, FSquad& squad);
        void setActionStatus(std::optional<SquadState> status); ///< _SafeStr_1275
        void showMorale(MoraleStatus status);                   ///< _SafeStr_1267
        void removeMorale();                                    ///< _SafeStr_1277
        void makelevelUP(int level);

    private:
        std::shared_ptr<engine::Sprite> createBody(FSquad& squad); ///< _SafeStr_2464
        void showAction(std::optional<SquadState> status); ///< _SafeStr_2465
        void showLevel(int level);                         ///< _SafeStr_2461
        void removeAction();                               ///< _SafeStr_2458
        void removeLevel();                                ///< _SafeStr_2462
        void removeAll();                                  ///< _SafeStr_2463
        std::shared_ptr<SpriteAnimation> addAnimation(const std::string& key, double delay);

        const ArmyStat* stat_;                      ///< Original: _SafeStr_977.
        TOResManager* resources_;                   ///< Original: _SafeStr_1252.
        std::shared_ptr<engine::Sprite> icon_;      ///< Original: _SafeStr_2452.
        std::shared_ptr<engine::Sprite> body_;      ///< Original: _SafeStr_2457.
        std::shared_ptr<engine::MovieClip> round_;  ///< SquadRoundIconClass (original: _SafeStr_2454).
        std::shared_ptr<SpriteAnimation> action_;   ///< Original: _SafeStr_2456.
        std::shared_ptr<SpriteAnimation> morale_;   ///< Original: _SafeStr_2455.
        std::shared_ptr<SpriteAnimation> star_;     ///< Original: _SafeStr_2453.
        std::shared_ptr<View> view_;                ///< Original: _SafeStr_2451.
    };

} // namespace takeover
