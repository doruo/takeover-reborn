// SoundConst.h - the sound effects table (original: media/SoundConst).
//
//     SoundConst::play(SoundId::BUTTON_CLICK);
//
// Each id maps to a library sound class with a volume, start time and loop
// count (original: _SafeStr_1898). The battle-specific choosers (which
// sound a unit makes when it attacks, dies, ...) are at the end. They live
// in SoundConstBattle.cpp next to the simulation types they need.
#pragma once

#include "game/media/MediaToggle.h"
#include "game/media/SoundId.h"

#include <initializer_list>
#include <map>
#include <memory>
#include <string>

namespace takeover {

    class ArmyStat;
    class SpellInfo;
    enum class MoraleStatus;
    enum class Race;
    enum class SupplyBuildingType;

    class SoundConst : public MediaToggle {
    public:
        static SoundConst& instance(); ///< Original: _SafeStr_50.

        static inline bool MODE_MUTE = false;

        /// Plays a sound effect by id. Ids < 0 mean "no sound" (original: _SafeStr_1303).
        static std::shared_ptr<engine::SoundChannel> play(int id);

        /// Plays one of the ids at random (original: _SafeStr_1900).
        static std::shared_ptr<engine::SoundChannel> playRandom(std::initializer_list<int> ids);

        /// Sound of an ability effect by name (original: _SafeStr_1931).
        static std::shared_ptr<engine::SoundChannel> playEffect(const std::string& effectName);

        // /----- BATTLE (SOUNDCONSTBATTLE.CPP) -----/

        static std::shared_ptr<engine::SoundChannel> playAttack(const ArmyStat* stat, bool ranged);          ///< _SafeStr_1929
        static std::shared_ptr<engine::SoundChannel> playHit(const ArmyStat* stat);                          ///< _SafeStr_1922
        static std::shared_ptr<engine::SoundChannel> playDeath(const ArmyStat* stat);                        ///< _SafeStr_1299
        static std::shared_ptr<engine::SoundChannel> playSend(const ArmyStat* stat);                         ///< _SafeStr_1924
        static std::shared_ptr<engine::SoundChannel> playBattleCry(const ArmyStat* stat);                    ///< _SafeStr_1296
        static std::shared_ptr<engine::SoundChannel> playSpell(const SpellInfo* spell);                      ///< _SafeStr_1928
        static std::shared_ptr<engine::SoundChannel> playMorale(const ArmyStat* stat, MoraleStatus status);  ///< _SafeStr_1284
        static std::shared_ptr<engine::SoundChannel> playBuild(SupplyBuildingType type, Race race);          ///< _SafeStr_1934
        static std::shared_ptr<engine::SoundChannel> playUltimate(Race race);                                ///< _SafeStr_1930

    private:
        SoundConst();

        struct SoundDef { ///< Original: _SafeStr_1898.
            std::string soundClass;
            double volume = 1;
            double startTime = 0;
            int loops = 0;
        };

        std::map<int, SoundDef> sounds_;
    };

} // namespace takeover
