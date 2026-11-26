#include "game/media/SoundConst.h"

#include "game/framework/core/As3.h"

namespace takeover {

    SoundConst::SoundConst() : MediaToggle("butSoundOn", "butSoundOff") {
        setEnabled(!MODE_MUTE);

        const std::pair<int, const char*> table[] = {
            {SoundId::BUTTON_OVER, "sndButtonOver"},
            {SoundId::BUTTON_CLICK, "sndButtonClick"},
            {SoundId::WINDOW_CLOSE, "sndWindowClose"},
            {SoundId::ATTACK_BOW_1, "sndAttackBow1"},
            {SoundId::ATTACK_BOW_2, "sndAttackBow2"},
            {SoundId::ATTACK_SWORD_1, "sndAttackSword1"},
            {SoundId::ATTACK_SWORD_2, "sndAttackSword2"},
            {SoundId::ATTACK_SWORD_3, "sndAttackSword3"},
            {SoundId::ATTACK_STAFF_1, "sndAttackStaff1"},
            {SoundId::ATTACK_STAFF_2, "sndAttackStaff2"},
            {SoundId::ATTACK_MAGIC_1, "sndAttackMagic1"},
            {SoundId::ATTACK_MAGIC_2, "sndAttackMagic2"},
            {SoundId::ATTACK_GUN_1, "sndAttackGun1"},
            {SoundId::ATTACK_GUN_2, "sndAttackGun2"},
            {SoundId::ATTACK_SIEGE, "sndAttackSiege"},
            {SoundId::ATTACK_OGRE, "sndAttackOgre"},
            {SoundId::HIT_ARROW_1, "sndHitArrow1"},
            {SoundId::HIT_ARROW_2, "sndHitArrow2"},
            {SoundId::HIT_SIEGE, "sndHitSiege"},
            {SoundId::DEAD_MAN_1, "sndDeadMan1"},
            {SoundId::DEAD_MAN_2, "sndDeadMan2"},
            {SoundId::DEAD_MAN_3, "sndDeadMan3"},
            {SoundId::DEAD_HORSE_1, "sndDeadHorse1"},
            {SoundId::DEAD_HORSE_2, "sndDeadHorse2"},
            {SoundId::DEAD_REPTILE, "sndDeadReptile"},
            {SoundId::DEAD_BOT, "sndDeadBot"},
            {SoundId::DEAD_SIEGE, "sndDeadSiege"},
            {SoundId::DEAD_UNDEAD_1, "sndDeadUndead1"},
            {SoundId::DEAD_UNDEAD_2, "sndDeadUndead2"},
            {SoundId::DEAD_OGRE, "sndDeadOgre"},
            {SoundId::DESTRUCTION_STRONGHOLD, "sndDestructionStronghold"},
            {SoundId::DESTRUCTION_BUILDING, "sndDestructionBuilding"},
            {SoundId::SEND_SOLDIERS, "sndSendSoldiers"},
            {SoundId::SEND_CROWD, "sndSendCrowd"},
            {SoundId::SEND_HORSES, "sndSendHorses"},
            {SoundId::SEND_BOTS, "sndSendBots"},
            {SoundId::SEND_SIEGE, "sndSendSiege"},
            {SoundId::SEND_REPTILES, "sndSendReptiles"},
            {SoundId::BEGIN_BATTLE_SOLDIERS, "sndBeginBattleSoldiers"},
            {SoundId::BEGIN_BATTLE_OGRE, "sndBeginBattleOgre"},
            {SoundId::BEGIN_BATTLE_UNDEAD, "sndBeginBattleUndead"},
            {SoundId::BEGIN_BATTLE_ARCHERS, "sndBeginBattleArchers"},
            {SoundId::BEGIN_BATTLE_BOTS, "sndBeginBattleBots"},
            {SoundId::SPELL_LORDS_JUDGEMENT, "sndSpellLordsJudgement"},
            {SoundId::SPELL_SUMMON_STONEWORMS, "sndSpellSummonStoneworms"},
            {SoundId::SPELL_DISENTOMB, "sndSpellDisentomb"},
            {SoundId::SPELL_ICE_BLAST_LAUNCH, "sndSpellIceBlastLaunch"},
            {SoundId::SPELL_ICE_BLAST, "sndSpellIceBlast"},
            {SoundId::SPELL_FROST_ENCHANTMENT, "sndSpellFrostEnchantment"},
            {SoundId::SPELL_TERROR, "sndSpellTerror"},
            {SoundId::BANNER_SET, "sndBannerSet"},
            {SoundId::BANNER_REMOVE, "sndBannerRemove"},
            {SoundId::ULTIMATE_DECLARATION, "sndUltimateDeclaration"},
            {SoundId::ULTIMATE_DECLARE_TRANSFORMATION, "sndUltimateDeclareTransformation"},
            {SoundId::ULTIMATE_DECLARE_CATACLYSM, "sndUltimateDeclareCataclysm"},
            {SoundId::ULTIMATE_DECLARE_AVATAR, "sndUltimateDeclareAvatar"},
            {SoundId::ULTIMATE_DECLARE_UNDEAD, "sndUltimateDeclareUndead"},
            {SoundId::UNIT_FROZEN, "sndUnitFrozen"},
            {SoundId::UNIT_FRIGHTENED, "sndUnitFeared"},
            {SoundId::UNIT_HEROIC, "sndUnitHeroic"},
            {SoundId::BUILD_FARM, "sndBuildFarm"},
            {SoundId::BUILD_MINE, "sndBuildMine"},
            {SoundId::BUILD_OBELISK, "sndBuildObelisk"},
            {SoundId::BUILD_GRAVEYARD, "sndBuildGraveyard"},
            {SoundId::CHEST_OPEN, "sndChestOpen"},
            {SoundId::CHEST_GOLD, "sndChestGold"},
            {SoundId::SQUAD_LEVEL_UP, "sndSquadLevelUp"},
        };

        for (const auto& [id, soundClass] : table)
            sounds_[id] = SoundDef{soundClass};
    }

    SoundConst& SoundConst::instance() {
        static SoundConst sounds;
        return sounds;
    }

    std::shared_ptr<engine::SoundChannel> SoundConst::play(int id) {
        if (id < 0)
            return nullptr;

        SoundConst& self = instance();
        const auto it = self.sounds_.find(id);
        if (it == self.sounds_.end())
            return nullptr; // the original would throw here

        const SoundDef& def = it->second;
        auto channel = self.manager().play(def.soundClass, def.startTime, def.loops);

        if (def.volume < 1)
            self.manager().setVolume(def.soundClass, def.volume);

        return channel;
    }

    std::shared_ptr<engine::SoundChannel> SoundConst::playRandom(std::initializer_list<int> ids) {
        const auto index = static_cast<std::size_t>(std::floor(as3::random() * static_cast<double>(ids.size())));
        return play(*(ids.begin() + index));
    }

} // namespace takeover
