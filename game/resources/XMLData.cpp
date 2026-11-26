#include "game/resources/XMLData.h"

#include "game/sim/battle/ArmyStat.h"
#include "game/units/UnitNameModifiers.h"

namespace takeover {

XMLData& XMLData::instance() {
    static XMLData data;
    return data;
}

XMLData::XMLData() {
    addAllUnits();
    preloadClips.push_back({"ObeliskRedClass", "", 1});
}

std::string XMLData::raceColor(Race race) {
    switch (race) {
    case Race::TheKhaganate: return "Red";
    case Race::Westaria: return "Green";
    case Race::TheCult: return "Blue";
    case Race::TheEmpire: return "Purple";
    default: return "Gray";
    }
}

std::string XMLData::unitTypeName(UnitType type) {
    switch (type) {
    case UnitType::Archers: return "Bowman";
    case UnitType::Cavalry: return "Horseman";
    case UnitType::Magic: return "Pikeman";
    case UnitType::Infantry: return "Swordsman";
    case UnitType::Siege: return "Trebuchet";
    }
    return "unknow";
}

void XMLData::addSpecialUnit(ArmyStat& stat, int animCount, double scale) {
    auto names = std::make_shared<UnitAnimNameModifier>(animCount, raceColor(stat.side));
    stat.animModifier = names;
    const UnitDirectionNameModifier directions;
    const std::string animId =
        unit_types::toString(stat.unitType) + (stat.techLevel > 1 ? std::to_string(stat.techLevel) : "");
    for (int i = 0; i < directions.count(); ++i)
        for (int k = 0; k < names->count(); ++k)
            battleClips.push_back({names->name(directions.name(animId, i), k), "", scale});
}

void XMLData::addAllUnits() {
    addSpecialUnit(ArmyStat::TREBUCHET_T1_D, 3, 0.8);
    addSpecialUnit(ArmyStat::GUARDS, 3);
    addSpecialUnit(ArmyStat::SWORDSMEN, 3);
    addSpecialUnit(ArmyStat::LEGIONARIES, 4);
    addSpecialUnit(ArmyStat::UNDYING_ARCHERS, 4);
    addSpecialUnit(ArmyStat::ZOMBIE_WARRIORS, 3);
    addSpecialUnit(ArmyStat::KNIGHTS, 3);
    addSpecialUnit(ArmyStat::ACOLYTES, 4);
    addSpecialUnit(ArmyStat::ASSASSINS, 6);
    addSpecialUnit(ArmyStat::MONKS, 4);
    addSpecialUnit(ArmyStat::SHADES, 5);
    addSpecialUnit(ArmyStat::UNDYING_ASSASSINS, 5);
    addSpecialUnit(ArmyStat::WIZARDS, 4);
    addSpecialUnit(ArmyStat::ZEALOTS, 4);
    addSpecialUnit(ArmyStat::OGRE, 3);
    addSpecialUnit(ArmyStat::NOMADS, 4);
    addSpecialUnit(ArmyStat::GUNSMITHS, 4);
    addSpecialUnit(ArmyStat::ARCHERS, 4);
    addSpecialUnit(ArmyStat::HUNTERS, 4);
    addSpecialUnit(ArmyStat::SKELETON_ARCHERS, 4);
    addSpecialUnit(ArmyStat::TREBUCHET_T1_B, 3, 0.8);
    addSpecialUnit(ArmyStat::TREBUCHET_T1_W, 3, 0.8);
    addSpecialUnit(ArmyStat::POISON_CATAPULT_T2_D, 3, 0.8);
    addSpecialUnit(ArmyStat::TREBUCHET_T1_E, 3, 0.8);
    addSpecialUnit(ArmyStat::POISON_CATAPULT_T2_E, 3, 0.8);
    addSpecialUnit(ArmyStat::REBELS, 3);
    addSpecialUnit(ArmyStat::LANCERS, 3);
    addSpecialUnit(ArmyStat::WARRIORS, 3);
    addSpecialUnit(ArmyStat::UNDYING_WARRIORS, 3);
    addSpecialUnit(ArmyStat::UNDYING_HORSEMEN, 3);
    addSpecialUnit(ArmyStat::DARK_KNIGHTS, 3);
    addSpecialUnit(ArmyStat::CROSSBOWMEN, 4);
    addSpecialUnit(ArmyStat::GUNBOTS, 4);
    addSpecialUnit(ArmyStat::GIANTBOTS, 4, 1);
    addSpecialUnit(ArmyStat::CHAMPIONS, 3);
    addSpecialUnit(ArmyStat::MYRMIDONS, 3);
    addSpecialUnit(ArmyStat::DRAGON_LANCERS, 3, 0.9);
    addSpecialUnit(ArmyStat::SERPENT_ARCHERS, 4);
    addSpecialUnit(ArmyStat::RAIDERS, 4);
    addSpecialUnit(ArmyStat::IMMORTALS, 3);
    addSpecialUnit(ArmyStat::ANCIENT_RAIDERS, 3);
    addSpecialUnit(ArmyStat::AVATAR, 3);

    // Units without a naming rule would use the older art set ("Clip_..."),
    // which the final game no longer has: every unit has a rule above.
    const UnitDirectionNameModifier directions;
    for (Race race : {Race::Westaria, Race::TheCult, Race::TheKhaganate, Race::TheEmpire}) {
        for (UnitType type : {UnitType::Infantry, UnitType::Archers, UnitType::Cavalry, UnitType::Siege, UnitType::Magic}) {
            for (int tier = 1; tier <= 4; ++tier) {
                const ArmyStat* stat = ArmyStat::find(race, type, tier);
                if (stat == nullptr || stat->animModifier != nullptr) continue;
                const UnitPhaseNameModifier phases(raceColor(stat->side));
                for (int i = 0; i < directions.count(); ++i)
                    for (int k = 0; k < phases.count(); ++k)
                        battleClips.push_back({phases.name(directions.name(unitTypeName(stat->unitType), i), k), "", 1});
            }
        }
    }

    for (const char* name : {
        "CastleRedClass", "CastleRedAnimClass", "CastleRedRazeClass", "CastleRedRazeAnimClass",
        "CastleGreenClass", "CastleGreenAnimClass", "CastleGreenRazeClass", "CastleGreenRazeAnimClass",
        "CastleBlueClass", "CastleBlueAnimClass", "CastleBlueRazeClass", "CastleBlueRazeAnimClass",
        "CastlePurpleClass", "CastlePurpleAnimClass", "CastlePurpleRazeClass", "CastlePurpleRazeAnimClass",
        "TownRedClass", "TownRedAnimClass", "TownRedRazeClass", "TownRedRazeAnimClass", "TownGreenClass",
        "TownGreenAnimClass", "TownGreenRazeClass", "TownGreenRazeAnimClass", "TownBlueClass",
        "TownBlueAnimClass", "TownBlueRazeClass", "TownBlueRazeAnimClass", "TownPurpleClass",
        "TownPurpleAnimClass", "TownPurpleRazeClass", "MageTowerRedClass", "MageTowerRedRazedClass",
        "MageTowerGreenClass", "MageTowerGreenRazedClass", "MageTowerBlueClass", "MageTowerBlueRazedClass",
        "MageTowerPurpleClass", "MageTowerPurpleRazedClass", "MineRedClass", "MineRedAnimClass",
        "MineGreenClass", "MineGreenAnimClass", "MineBlueClass", "MineBlueAnimClass", "MinePurpleClass",
        "MinePurpleAnimClass", "VillageRedClass", "VillageRedAnimClass", "VillageGreenClass",
        "VillageGreenAnimClass", "VillageBlueClass", "VillageBlueAnimClass", "VillagePurpleClass",
        "VillagePurpleAnimClass", "ObeliskRedClass", "ObeliskRedAnimClass", "ObeliskGreenClass",
        "ObeliskGreenAnimClass", "ObeliskBlueClass", "ObeliskBlueAnimClass", "ObeliskPurpleClass",
        "ObeliskPurpleAnimClass", "ASTChargingClass", "ASTDefendingAndAttackClass", "ASTDefendingClass",
        "ASTMeleeFightingClass", "ASTMovingClass", "ASTRangedFightingClass", "ASTRepairingClass",
        "ASTSleepingClass", "MST_FearModeClass", "MST_HeroicModeClass", "MST_HighMoraleClass",
        "MST_LowMoraleClass", "CatapultMissileClass", "CraterFromExplosionClass", "ExplosionClass",
        "SpellThunderClass", "BannerHealClass", "BannerHeroismClass", "BannerDesecrationClass",
        "BannerBloodlustClass", "BannerAuraHealClass", "BannerAuraHeroismClass", "BannerAuraDesecrationClass",
        "BannerEffectBloodlustClass", "BannerLiberationClass", "BannerLiberationEffectClass",
        "TerrorSpellEffectClass", "TerrorFlyingBatEffectClass", "TerrorSplashEffectClass",
        "FrostEncahntmentEffectClass", "RaiseDeadSpellEffectClass", "StoneWormsEffectClass",
        "StonewormsExplosionEffect", "FireballEffectClass", "FireBallExplosionClass", "IceBlastEffectClass",
        "IceBlastExplosionClass", "GrandTransformationEffectClass", "BuildingRizeEffectClass",
        "SignOfAdventEffectClass", "AvatarFrontLightEffectClass", "AvatarBackLightEffectClass",
        "IconGoPathClass", "IconGoPathOffClass", "IconGoInClass", "IconAttackClass", "CursorOn_HealClass",
        "CursorOff_HealClass", "CursorOn_HeroismClass", "CursorOff_HeroismClass",
        "CursorOn_LordsJudgementClass", "CursorOff_LordsJudgementClass", "CursorOn_StoneWormClass",
        "CursorOn_DisentombClass", "CursorOn_BloodlustClass", "CursorOff_StoneWormClass",
        "CursorOff_DisentombClass", "CursorOff_BloodlustClass", "CursorOn_IceBlastClass",
        "CursorOn_MistyClonesClass", "CursorOn_LiberationClass", "CursorOff_IceBlastClass",
        "CursorOff_MistyClonesClass", "CursorOff_LiberationClass", "GoldStarIconClass", "SilverStarIconClass",
        "BulletClass", "ZealotBulletClass", "AcolytesBulletClass", "GunbotBulletClass", "GiantbotBulletClass"})
        battleClips.push_back({name, "", 1});
}

} // namespace takeover
