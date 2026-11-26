// SpellInfo.h - spell definitions (original: _SafePkg_1566/_SafeStr_1901)
// and a spell owned by a combatant, with its edict bonuses applied
// (original: _SafePkg_1566/_SafeStr_1565).
#pragma once

#include "game/sim/base/Point2D.h"
#include "game/sim/battle/Race.h"

#include <string>
#include <vector>

namespace takeover {

class EdictBonuses;

class SpellInfo {
public:
    static constexpr int TYPE_SPELL = 0;    ///< Original: _SafeStr_3046.
    static constexpr int TYPE_BANNER = 1;   ///< Original: _SafeStr_1927.
    static constexpr int TYPE_ULTIMATE = 2; ///< Original: _SafeStr_3042.

    SpellInfo() = default;
    SpellInfo(int id, int level, std::string name, Race race, int type, int cost, int radius, double power,
              double time, std::string hint);
    virtual ~SpellInfo() = default;

    int id = 0;
    int level = 0;   ///< Position in the race's spell bar (1..4).
    std::string name;
    Race race = Race::Westaria;
    int type = TYPE_SPELL;
    int cost = 0;    ///< Mana.
    int radius = 0;
    double power = 0;
    double time = 0; ///< Duration in seconds.
    std::string hint;

    void copyFrom(const SpellInfo& other); ///< Original: _SafeStr_2112.

    /// Every spell, in id order (original: _SafeStr_3132).
    static const std::vector<const SpellInfo*>& all();

    static const SpellInfo LORDS_JUDGEMENT;
    static const SpellInfo BANNER_OF_CONSECRATION; ///< _SafeStr_2424
    static const SpellInfo BANNER_OF_HEROISM;      ///< _SafeStr_2421
    static const SpellInfo SIGN_OF_THE_ADVENT;     ///< _SafeStr_3135
    static const SpellInfo SUMMON_STONEWORMS;      ///< _SafeStr_1925
    static const SpellInfo DISENTOMB;
    static const SpellInfo BANNER_OF_BLOODLUST;    ///< _SafeStr_2419
    static const SpellInfo CATACLYSM;
    static const SpellInfo ICE_BLAST;
    static const SpellInfo FROST_ENCHANTMENT;      ///< _SafeStr_1920
    static const SpellInfo BANNER_OF_LIBERATION;   ///< _SafeStr_2420
    static const SpellInfo GRAND_TRANSFORMATION;   ///< _SafeStr_3134
    static const SpellInfo TERROR;
    static const SpellInfo RAISE_THE_DEAD;         ///< _SafeStr_1923
    static const SpellInfo BANNER_OF_DESECRATION;  ///< _SafeStr_2418
    static const SpellInfo CALL_OF_THE_GRAVE;      ///< _SafeStr_3133
};

class Spell : public SpellInfo {
public:
    Spell() = default;
    Spell(const SpellInfo& info, const EdictBonuses* bonuses);

    Point2D position;

    /// The definition with this id (original: _SafeStr_3021).
    static const SpellInfo* byId(int id);
    void applyBonuses(const SpellInfo& info, const EdictBonuses& bonuses); ///< _SafeStr_3136
    void copyFrom(const Spell& other);                                       ///< _SafeStr_3137
};

} // namespace takeover
