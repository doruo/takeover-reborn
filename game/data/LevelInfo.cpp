#include "game/data/LevelInfo.h"

#include "game/data/CompanyInfo.h"
#include "game/progress/LevelProgress.h"
#include "game/sim/battle/Combatant.h"
#include "game/sim/battle/MissionStats.h"

#include <cmath>

namespace takeover {

std::optional<LevelInfo> LevelInfo::make(int companyId, int levelNum) {
    if (companyId < 0 || companyId >= CompanyInfo::COMPANY_COUNT) return std::nullopt;
    return LevelInfo(companyId, levelNum);
}

std::string LevelInfo::caption() const {
    static const char* const kCaptions[3][8] = {
        {"Enemy At The Gate", "Havoc", "A Swift Thrust", "A New Rival", "Storm Of Westport", "It's Our Land",
         "Chaos of Battle", "Hail the Empire!"},
        {"It's Not Over Yet!", "Revenge", "To the East", "Tame The Undead", "Divide And Conquer", "The Rich North",
         "Dark Secrets", "Revival"},
        {"The World's Edge", "The Empire's End", "Friend Or Foe?", "New Plague", "Light And Magic", "Remedy",
         "Close The Circle", "The Final Battle"},
    };
    if (levelNum_ >= 0 && levelNum_ < 8) return kCaptions[companyId_][levelNum_];
    return "> CAPTION <";
}

std::string LevelInfo::description() const {
    static const char* const kTexts[3][8] = {
        {"   The Duchy of Westaria is located on the western islands. It was always safe from the troubles of the continent. But here they come.",
         "   The Crimson Horde was conquered by the Empire ages ago but they never were fully bent. Havoc reigns in Rivadis. And these barbarians are trying to seize our land.",
         "   We wage a crusade against the filth in the name of The Lord. City of Westport is well fortified against a direct attack. But we can use limited forces to create a springboard for our advancement on the continent.",
         "   We must cut Wesport off from the Capital. But the recons report that the rebels from the North were seen there and they are practicing forbidden magic. Our duty is to destroy them along with the necromancers!",
         "   Our forces on the continent have to fight on two fronts these days. Moreover we have to deal with the Horde too! The only way out is to attack Westport and seize it to secure the transfer of the reinforcement.",
         "   The Horde still threatens us from the west coast. Let's wipe them out from the lands granted to us by The Lord!",
         "   Before we attack the Capital we have to complete the outflanking of the left wing. Necromancers are fighting against the northern Cult in these snowy lands. Let's annihilate both of them with one strike!",
         "   The Empire of Rivadis must get rid of necromancers and regenerate. Hail to the new Empire, hail to the new Emperor! For the Lord's sake!"},
        {"   The Horde was defeated and fled in disgrace. But the new khan will lead our warriors to the new victories. We shall requite and our rivals will shed their blood. If they have blood...",
         "   The prophets foresee a great victory. Westarian cowards won't hide behind the thick walls. We'll pillage their towns and temples and enslave their wives.",
         "   Once the lands on the east were our forefathers' homeland. We can't let the Empire possess it any longer. We'll get it back as we are stronger!",
         "   The necromancers built their magical laboratories on the eastern cape. We must seize them all before the northern wizards do that.",
         "   The remnants of Undead flee from The Horde to the lands of these silly wizards. Let's strike the weakened northerners with what our shamans have created.",
         "   These lands are famous for the riches hidden here. We will seize the mines of The Cult and force the wizards to mine gold and gems for us.",
         "   Our shamans conceived the secrets of the Empire's necromancers and our forefathers are now fighting on our side. The Empire must take these secrets to the grave – these secrets now belong to us!",
         "   The Capital of The Empire. The very Empire which have been killing us for ages. This is the time for the Great Khan to rule over all the lands and all the creatures, both alive and dead."},
        {"   The Empire enslaved us, the church persecuted our wizards, the steppe barbarians plundered our towns. And when the time came to fight for our freedom we were defeated. We hadn't enough experience, but the Cult evolves.",
         "   Alarming reports came from our spies in the Capital: Khan and his shamans are corrupted by necromancy. They sacrifice their enemies and resurrect them. We have to get rid of the Imperial forces' remnants in our homelands.",
         "   Westaria could be our ally but the fanatic templars don't let our envoy reach the Duke, who is now the Emperor. They hate us even more than they hate the undeads. Let's fight for a free life!",
         "   Dreadful news comes from The Capital. Undeads crawl from the city. Resurrected Horde warriors rose against their masters. We must stop the undead invasion on our borders.",
         "   We have finally allied with the Westarians. Now we can send all the forces to the east and the Westarian army will strike from the south. We'd better hurry up – our forces on the Capital outskirts are exhausted.",
         "   No one should play with the dark forces. The shamans of Horde believed that these forces obeyed them but they didn't have enough strength. The plague devours the Horde from within. We'll bring the remedy to them.",
         "   The lands which are not controlled by the Alliance are abundant in undeads. We will strike together with the Westarians and encircle The Capital.",
         "   We'll meet the Westarians in the battlefield again but now we'll fight shoulder to shoulder. Let the combined light of The Lord and Northern Magic wipe the undeads out from our world forever!"},
    };
    if (levelNum_ >= 0 && levelNum_ < 8) return kTexts[companyId_][levelNum_];
    return "Level Text " + std::to_string(levelNum_);
}

int LevelInfo::xpReward() const {
    switch (levelNum_) {
    case 1: return 1500;
    case 2: return 1750;
    case 3: return 2000;
    case 4: return 3000;
    default: return 1000;
    }
}

int LevelInfo::rankFor(const MissionResult& result) const {
    const double time = std::floor(result.missionTime);
    if (result.missionOverStatus == MissionResult::VICTORY) {
        if (time <= goldTime()) return LevelProgress::RANK_GOLD;
        if (time <= silverTime()) return LevelProgress::RANK_SILVER;
        return LevelProgress::RANK_BRONZE;
    }
    return LevelProgress::RANK_NEW;
}

namespace {

/// Original: _SafeStr_3474.
std::shared_ptr<Combatant> makeEnemy(Race race, const EdictBonuses* bonuses = nullptr) {
    const EdictBonuses edicts = bonuses ? *bonuses : EdictTree::forRace(race).buildBonuses();
    auto combatant = std::make_shared<Combatant>(std::make_shared<CombatantKey>(), race, edicts);
    combatant->addGold(0);
    combatant->addMana(0);
    return combatant;
}

} // namespace

std::vector<std::shared_ptr<Combatant>> LevelInfo::enemyPlayers() const {
    using namespace Bonus;
    std::vector<std::shared_ptr<Combatant>> enemies;
    EdictBonuses westaria = EdictTree::forRace(Race::Westaria).buildBonuses();
    EdictBonuses icedale = EdictTree::forRace(Race::TheCult).buildBonuses();
    EdictBonuses horde = EdictTree::forRace(Race::TheKhaganate).buildBonuses();
    EdictBonuses empire = EdictTree::forRace(Race::TheEmpire).buildBonuses();
    const auto set = [](EdictBonuses& b, std::initializer_list<int> bonuses, int value = 1) {
        for (const int bonus : bonuses) b.set(bonus, value);
    };

    switch (companyId_) {
    case CompanyInfo::WESTARIA:
        switch (levelNum_) {
        case 0: enemies.push_back(makeEnemy(Race::TheEmpire)); break;
        case 1:
            horde.set(BANNER_OF_BLOODLUST, 0);
            enemies.push_back(makeEnemy(Race::TheKhaganate));
            break;
        case 2:
            empire.set(TERROR, 1);
            enemies.push_back(makeEnemy(Race::TheEmpire, &empire));
            break;
        case 3:
            set(empire, {BANNER_OF_DESECRATION, TERROR, ARCHERS_TIER});
            enemies.push_back(makeEnemy(Race::TheEmpire, &empire));
            set(icedale, {ICE_BLAST, INFANTRY_TIER, ARCHERS_TIER});
            enemies.push_back(makeEnemy(Race::TheCult, &icedale));
            break;
        case 4:
            set(empire, {BANNER_OF_DESECRATION, TERROR, ARCHERS_TIER, INFANTRY_TIER});
            enemies.push_back(makeEnemy(Race::TheEmpire, &empire));
            set(horde, {BANNER_OF_BLOODLUST, CAVALRY_TIER, SUMMON_STONEWORMS});
            enemies.push_back(makeEnemy(Race::TheKhaganate, &horde));
            break;
        case 5:
            set(horde, {BANNER_OF_BLOODLUST, CAVALRY_TIER, SUMMON_STONEWORMS, INFANTRY_TIER});
            enemies.push_back(makeEnemy(Race::TheKhaganate, &horde));
            break;
        case 6:
            set(empire, {BANNER_OF_DESECRATION, TERROR, ARCHERS_TIER, INFANTRY_TIER, DISENTOMB, MAGIC_TIER, SIEGE_TIER});
            enemies.push_back(makeEnemy(Race::TheEmpire, &empire));
            set(icedale, {ICE_BLAST, INFANTRY_TIER, ARCHERS_TIER, FROST_ENCHANTMENT, MAGIC_TIER});
            enemies.push_back(makeEnemy(Race::TheCult, &icedale));
            break;
        case 7:
            set(empire, {BANNER_OF_DESECRATION, TERROR, ARCHERS_TIER, INFANTRY_TIER, DISENTOMB, MAGIC_TIER, SIEGE_TIER,
                         CAVALRY_TIER});
            enemies.push_back(makeEnemy(Race::TheEmpire, &empire));
            break;
        }
        break;
    case CompanyInfo::KHAGANATE:
        switch (levelNum_) {
        case 0: enemies.push_back(makeEnemy(Race::TheEmpire)); break;
        case 1:
            westaria.set(BANNER_OF_CONSECRATION, 1);
            enemies.push_back(makeEnemy(Race::Westaria, &westaria));
            break;
        case 2:
        case 3:
            set(empire, {BANNER_OF_DESECRATION, TERROR});
            enemies.push_back(makeEnemy(Race::TheEmpire, &empire));
            break;
        case 4:
            set(empire, {BANNER_OF_DESECRATION, TERROR, ARCHERS_TIER, INFANTRY_TIER});
            enemies.push_back(makeEnemy(Race::TheEmpire, &empire));
            set(icedale, {ICE_BLAST, INFANTRY_TIER, ARCHERS_TIER});
            enemies.push_back(makeEnemy(Race::TheCult, &icedale));
            break;
        case 5:
            set(icedale, {ICE_BLAST, INFANTRY_TIER, ARCHERS_TIER, FROST_ENCHANTMENT});
            enemies.push_back(makeEnemy(Race::TheCult, &icedale));
            break;
        case 6:
            set(empire, {BANNER_OF_DESECRATION, TERROR, ARCHERS_TIER, INFANTRY_TIER, DISENTOMB, MAGIC_TIER, SIEGE_TIER});
            enemies.push_back(makeEnemy(Race::TheEmpire, &empire));
            set(icedale, {ICE_BLAST, INFANTRY_TIER, ARCHERS_TIER, FROST_ENCHANTMENT, MAGIC_TIER});
            enemies.push_back(makeEnemy(Race::TheCult, &icedale));
            break;
        case 7:
            westaria.set(BANNER_OF_CONSECRATION, 1);
            westaria.set(LORDS_JUDGEMENT, 1);
            westaria.set(INFANTRY_TIER, 2);
            westaria.set(ARCHERS_TIER, 1);
            westaria.set(CAVALRY_TIER, 1);
            enemies.push_back(makeEnemy(Race::Westaria, &westaria));
            break;
        }
        break;
    case CompanyInfo::CULT:
        switch (levelNum_) {
        case 0: enemies.push_back(makeEnemy(Race::TheEmpire)); break;
        case 1: enemies.push_back(makeEnemy(Race::TheEmpire, &empire)); break;
        case 2:
            set(westaria, {BANNER_OF_CONSECRATION, INFANTRY_TIER});
            enemies.push_back(makeEnemy(Race::Westaria, &westaria));
            break;
        case 3:
            set(empire, {BANNER_OF_DESECRATION, TERROR, ARCHERS_TIER});
            enemies.push_back(makeEnemy(Race::TheEmpire, &empire));
            set(horde, {BANNER_OF_BLOODLUST, CAVALRY_TIER, SUMMON_STONEWORMS});
            enemies.push_back(makeEnemy(Race::TheKhaganate, &horde));
            break;
        case 4:
            set(empire, {BANNER_OF_DESECRATION, TERROR, ARCHERS_TIER, INFANTRY_TIER});
            enemies.push_back(makeEnemy(Race::TheEmpire, &empire));
            set(horde, {BANNER_OF_BLOODLUST, CAVALRY_TIER, SUMMON_STONEWORMS});
            enemies.push_back(makeEnemy(Race::TheKhaganate, &horde));
            break;
        case 5:
            set(horde, {BANNER_OF_BLOODLUST, INFANTRY_TIER, CAVALRY_TIER, SUMMON_STONEWORMS});
            enemies.push_back(makeEnemy(Race::TheKhaganate, &horde));
            break;
        case 6:
            set(empire, {BANNER_OF_DESECRATION, TERROR, ARCHERS_TIER, INFANTRY_TIER, DISENTOMB, MAGIC_TIER, SIEGE_TIER});
            enemies.push_back(makeEnemy(Race::TheEmpire, &empire));
            break;
        case 7:
            set(empire, {BANNER_OF_DESECRATION, TERROR, ARCHERS_TIER, INFANTRY_TIER, DISENTOMB, MAGIC_TIER, SIEGE_TIER,
                         CAVALRY_TIER});
            enemies.push_back(makeEnemy(Race::TheEmpire, &empire));
            break;
        }
        break;
    }
    return enemies;
}

} // namespace takeover
