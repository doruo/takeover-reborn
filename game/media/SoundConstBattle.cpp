// The battle sounds of SoundConst: which sound a unit makes when it
// attacks, is hit, dies, is sent, starts fighting... (original: the
// battle functions of media/SoundConst).
#include "game/media/SoundConst.h"

#include "game/sim/abilities/AbilityEffects.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/sim/level/SupplyBuildingType.h"
#include "game/sim/spells/SpellInfo.h"
#include "game/sim/squads/MoraleStatus.h"

#include <initializer_list>

namespace takeover {

    namespace {

        bool isOneOf(const ArmyStat* stat, std::initializer_list<const ArmyStat*> stats) {
            for (const ArmyStat* candidate : stats)
                if (stat == candidate)
                    return true;
            return false;
        }

        bool isUndead(const ArmyStat* stat) {
            return isOneOf(stat, {&ArmyStat::UNDYING_WARRIORS, &ArmyStat::ZOMBIE_WARRIORS, &ArmyStat::UNDYING_ARCHERS,
                                  &ArmyStat::SKELETON_ARCHERS, &ArmyStat::UNDYING_HORSEMEN, &ArmyStat::DARK_KNIGHTS,
                                  &ArmyStat::UNDYING_ASSASSINS, &ArmyStat::IMMORTALS, &ArmyStat::ANCIENT_RAIDERS});
        }

        bool isSiegeEngine(const ArmyStat* stat) {
            return isOneOf(stat, {&ArmyStat::TREBUCHET_T1_B, &ArmyStat::TREBUCHET_T1_W, &ArmyStat::TREBUCHET_T1_D,
                                  &ArmyStat::TREBUCHET_T1_E, &ArmyStat::POISON_CATAPULT_T2_D, &ArmyStat::POISON_CATAPULT_T2_E});
        }

    } // namespace

    std::shared_ptr<engine::SoundChannel> SoundConst::playEffect(const std::string& effectName) {
        int id = -1;

        if (effectName == AbilityEffect::ATK_FREEZ)
            id = SoundId::UNIT_FROZEN;

        return play(id);
    }

    std::shared_ptr<engine::SoundChannel> SoundConst::playAttack(const ArmyStat* stat, bool ranged) {
        if (!ranged) {
            if (stat == &ArmyStat::OGRE)
                return play(SoundId::ATTACK_OGRE);

            if (isOneOf(stat, {&ArmyStat::MONKS, &ArmyStat::ZEALOTS, &ArmyStat::ACOLYTES, &ArmyStat::WIZARDS, &ArmyStat::GUNSMITHS}))
                return playRandom({SoundId::ATTACK_STAFF_1, SoundId::ATTACK_STAFF_2});

            return playRandom({SoundId::ATTACK_SWORD_1, SoundId::ATTACK_SWORD_2, SoundId::ATTACK_SWORD_3});
        }

        if (isOneOf(stat, {&ArmyStat::GUNSMITHS, &ArmyStat::GUNBOTS, &ArmyStat::GIANTBOTS}))
            return playRandom({SoundId::ATTACK_GUN_1, SoundId::ATTACK_GUN_2});

        if (isOneOf(stat, {&ArmyStat::ZEALOTS, &ArmyStat::ACOLYTES, &ArmyStat::WIZARDS}))
            return playRandom({SoundId::ATTACK_MAGIC_1, SoundId::ATTACK_MAGIC_2});

        if (isSiegeEngine(stat))
            return play(SoundId::ATTACK_SIEGE);

        return playRandom({SoundId::ATTACK_BOW_1, SoundId::ATTACK_BOW_2});
    }

    std::shared_ptr<engine::SoundChannel> SoundConst::playHit(const ArmyStat* stat) {
        if (stat->unitType == UnitType::Siege)
            return play(SoundId::HIT_SIEGE);

        // Magic and bullets hit silently.
        if (isOneOf(stat, {&ArmyStat::ZEALOTS, &ArmyStat::ACOLYTES, &ArmyStat::WIZARDS, &ArmyStat::GUNSMITHS,
                           &ArmyStat::GUNBOTS, &ArmyStat::GIANTBOTS}))
            return nullptr;
        return playRandom({SoundId::HIT_ARROW_1, SoundId::HIT_ARROW_2});
    }

    std::shared_ptr<engine::SoundChannel> SoundConst::playDeath(const ArmyStat* stat) {
        if (stat == &ArmyStat::OGRE)
            return play(SoundId::DEAD_OGRE);

        if (isOneOf(stat, {&ArmyStat::GUNBOTS, &ArmyStat::GIANTBOTS}))
            return play(SoundId::DEAD_BOT);

        if (stat == &ArmyStat::DRAGON_LANCERS)
            return play(SoundId::DEAD_REPTILE);

        if (isUndead(stat))
            return playRandom({SoundId::DEAD_UNDEAD_1, SoundId::DEAD_UNDEAD_2});

        switch (stat->unitType) {
        case UnitType::Siege: return play(SoundId::DEAD_SIEGE);
        case UnitType::Cavalry: return playRandom({SoundId::DEAD_HORSE_1, SoundId::DEAD_HORSE_2});
        default: return playRandom({SoundId::DEAD_MAN_1, SoundId::DEAD_MAN_2, SoundId::DEAD_MAN_3});
        }
    }

    std::shared_ptr<engine::SoundChannel> SoundConst::playSend(const ArmyStat* stat) {
        if (stat == &ArmyStat::DRAGON_LANCERS)
            return play(SoundId::SEND_REPTILES);

        if (isOneOf(stat, {&ArmyStat::GUNBOTS, &ArmyStat::GIANTBOTS}))
            return play(SoundId::SEND_BOTS);

        switch (stat->unitType) {
        case UnitType::Cavalry: return play(SoundId::SEND_HORSES);
        case UnitType::Siege: return play(SoundId::SEND_SIEGE);
        case UnitType::Magic: return play(SoundId::SEND_CROWD);
        case UnitType::Infantry:
        case UnitType::Archers: return play(SoundId::SEND_SOLDIERS);
        }

        return nullptr;
    }

    std::shared_ptr<engine::SoundChannel> SoundConst::playBattleCry(const ArmyStat* stat) {
        if (isUndead(stat))
            return play(SoundId::BEGIN_BATTLE_UNDEAD);

        if (isOneOf(stat, {&ArmyStat::ARCHERS, &ArmyStat::CROSSBOWMEN, &ArmyStat::HUNTERS, &ArmyStat::SERPENT_ARCHERS,
                           &ArmyStat::GUNSMITHS, &ArmyStat::MONKS, &ArmyStat::ZEALOTS, &ArmyStat::ACOLYTES, &ArmyStat::WIZARDS}))
            return play(SoundId::BEGIN_BATTLE_ARCHERS);
        if (stat == &ArmyStat::OGRE)
            return play(SoundId::BEGIN_BATTLE_OGRE);

        if (isOneOf(stat, {&ArmyStat::GUNBOTS, &ArmyStat::GIANTBOTS}))
            return play(SoundId::BEGIN_BATTLE_BOTS);

        if (isSiegeEngine(stat))
            return nullptr;

        return play(SoundId::BEGIN_BATTLE_SOLDIERS);
    }

    std::shared_ptr<engine::SoundChannel> SoundConst::playSpell(const SpellInfo* spell) {
        int id = -1;

        if (spell->type == SpellInfo::TYPE_BANNER)
            play(SoundId::BANNER_SET);

        if (spell->id == SpellInfo::LORDS_JUDGEMENT.id)
            id = SoundId::SPELL_LORDS_JUDGEMENT;
        else if (spell->id == SpellInfo::ICE_BLAST.id)
            id = SoundId::SPELL_ICE_BLAST;
        else if (spell->id == SpellInfo::FROST_ENCHANTMENT.id)
            id = SoundId::SPELL_FROST_ENCHANTMENT;
        else if (spell->id == SpellInfo::TERROR.id)
            id = SoundId::SPELL_TERROR;
        else if (spell->id == SpellInfo::DISENTOMB.id)
            id = SoundId::SPELL_DISENTOMB;
        else if (spell->id == SpellInfo::RAISE_THE_DEAD.id)
            id = SoundId::SPELL_DISENTOMB;
        else if (spell->id == SpellInfo::SUMMON_STONEWORMS.id)
            id = SoundId::SPELL_SUMMON_STONEWORMS;

        return play(id);
    }

    std::shared_ptr<engine::SoundChannel> SoundConst::playMorale(const ArmyStat*, MoraleStatus status) {
        switch (status) {
        case MoraleStatus::Heroism: return play(SoundId::UNIT_HEROIC);
        case MoraleStatus::Feared: return play(SoundId::UNIT_FRIGHTENED);
        default: return nullptr;
        }
    }

    std::shared_ptr<engine::SoundChannel> SoundConst::playBuild(SupplyBuildingType type, Race race) {
        if (type == SupplyBuildingType::Village && race == Race::TheEmpire)
            return play(SoundId::BUILD_GRAVEYARD);

        switch (type) {
        case SupplyBuildingType::Mine: return play(SoundId::BUILD_MINE);
        case SupplyBuildingType::Village: return play(SoundId::BUILD_FARM);
        case SupplyBuildingType::Obelisk: return play(SoundId::BUILD_OBELISK);
        case SupplyBuildingType::Empty: return play(SoundId::DESTRUCTION_BUILDING);
        }

        return nullptr;
    }

    std::shared_ptr<engine::SoundChannel> SoundConst::playUltimate(Race race) {
        play(SoundId::ULTIMATE_DECLARATION);

        switch (race) {
        case Race::TheKhaganate: return play(SoundId::ULTIMATE_DECLARE_CATACLYSM);
        case Race::TheCult: return play(SoundId::ULTIMATE_DECLARE_TRANSFORMATION);
        case Race::TheEmpire: return play(SoundId::ULTIMATE_DECLARE_UNDEAD);
        case Race::Westaria: return play(SoundId::ULTIMATE_DECLARE_AVATAR);
        default: return nullptr;
        }
    }

} // namespace takeover
