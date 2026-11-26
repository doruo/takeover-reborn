#include "game/progress/CompanyProgress.h"

#include "game/data/LevelInfo.h"
#include "game/framework/storage/SerializableObject.h"
#include "game/progress/Badges.h"
#include "game/sim/battle/Combatant.h"

#include <algorithm>

namespace takeover {

CompanyProgress::CompanyProgress(int companyId)
    : info_(*CompanyInfo::make(companyId)), edicts_(EdictTree::forRace(info_.getRace())),
      levels_(LEVEL_COUNT) {
    reset();
}

std::string CompanyProgress::curLevelGlobalID() const {
    std::string prefix = "err";
    switch (info_.id()) {
    case CompanyInfo::WESTARIA: prefix = "w"; break;
    case CompanyInfo::KHAGANATE: prefix = "h"; break;
    case CompanyInfo::CULT: prefix = "i"; break;
    }
    return prefix + std::to_string(currentLevel_ + 1);
}

int CompanyProgress::firstOpenLevelNumber() const {
    for (std::size_t i = 0; i < levels_.size() && i < LEVEL_COUNT; ++i)
        if (levels_[i].completeStatus == LevelProgress::RANK_NEW) return static_cast<int>(i) + 1;
    return std::min(static_cast<int>(levels_.size()), LEVEL_COUNT);
}

int CompanyProgress::completedCount() const {
    for (std::size_t i = 0; i < levels_.size(); ++i)
        if (levels_[i].completeStatus == LevelProgress::RANK_NEW) return static_cast<int>(i);
    return static_cast<int>(levels_.size());
}

int CompanyProgress::goldMedalCount() const {
    return static_cast<int>(std::count_if(levels_.begin(), levels_.end(), [](const LevelProgress& l) {
        return l.completeStatus == LevelProgress::RANK_GOLD;
    }));
}

bool CompanyProgress::isCompleted() const {
    return levels_[LEVEL_COUNT - 1].completeStatus != LevelProgress::RANK_NEW;
}

int CompanyProgress::skillPoints() const {
    return std::min(2 + (rank() - 1) * SKILL_POINTS_PER_RANK, MAX_SKILL_POINTS);
}

std::shared_ptr<Combatant> CompanyProgress::createPlayer(const LevelInfo& level) const {
    auto player = std::make_shared<Combatant>(std::make_shared<CombatantKey>(), info_.getRace(), edicts_.buildBonuses());
    player->addGold(250);
    player->addMana(100);
    if (level.companyId() == CompanyInfo::WESTARIA && level.levelNum() == 1) player->addGold(300);
    return player;
}

int CompanyProgress::checkLevelBadges(Badges& badges) const {
    const int completed = completedCount();
    const int perfect = goldMedalCount();
    switch (info_.id()) {
    case CompanyInfo::WESTARIA:
        badges.metric(Badges::METRIC_WESTARIA_MISSIONS_COMPLETED).setValue(completed);
        badges.metric(Badges::METRIC_WESTARIA_MISSIONS_PERFECT).setValue(perfect);
        break;
    case CompanyInfo::KHAGANATE:
        badges.metric(Badges::METRIC_HORDE_MISSIONS_COMPLETED).setValue(completed);
        badges.metric(Badges::METRIC_HORDE_MISSIONS_PERFECT).setValue(perfect);
        break;
    case CompanyInfo::CULT:
        badges.metric(Badges::METRIC_CULT_MISSIONS_COMPLETED).setValue(completed);
        badges.metric(Badges::METRIC_CULT_MISSIONS_PERFECT).setValue(perfect);
        break;
    }
    return perfect;
}

void CompanyProgress::updateEdictsBadge(Badges& badges) const {
    const int issued = edicts_.countIssued();
    switch (info_.id()) {
    case CompanyInfo::WESTARIA: badges.metric(Badges::METRIC_WESTARIA_MAIN_EDICTS_ISSUED).setValue(issued); break;
    case CompanyInfo::KHAGANATE: badges.metric(Badges::METRIC_HORDE_MAIN_EDICTS_ISSUED).setValue(issued); break;
    case CompanyInfo::CULT: badges.metric(Badges::METRIC_CULT_MAIN_EDICTS_ISSUED).setValue(issued); break;
    }
}

void CompanyProgress::reset() {
    xp_ = 0;
    currentLevel_ = -1;
    victoryShown_ = false;
    for (LevelProgress& level : levels_) level.reset();
    resetEdicts();
}

void CompanyProgress::resetEdicts() {
    // The two edicts of the first row start with one point: "basic troops"
    // and the race's first banner or spell.
    edicts_ = EdictTree::forRace(info_.getRace());
    auto& rows = edicts_.getRows();
    rows[0][0].setStartPoints(1);
    rows[0][0].reset();
    rows[0][1].setStartPoints(1);
    rows[0][1].reset();
}

void CompanyProgress::serialize(ByteArray& data) const {
    SerializableObject object;
    object.add("svr", SerializableObject::SHORT, static_cast<double>(SAVE_VER));
    object.add("xp", SerializableObject::DOUBLE, static_cast<double>(xp_));
    object.add("curlvl", SerializableObject::SHORT, static_cast<double>(currentLevel_));
    object.add("win", SerializableObject::BOOLEAN, victoryShown_);
    object.save(data);
    for (const LevelProgress& level : levels_) level.serialize(data);
    serializeEdicts(data);
}

void CompanyProgress::deserialize(ByteArray& data) {
    SerializableObject object;
    object.load(data);
    const int saveVersion = object.has("svr") ? static_cast<int>(object.getNumber("svr")) : -1;
    // As the original: an unknown version stops here, leaving the rest of
    // the stream unread.
    if (saveVersion != SAVE_VER) return;
    if (object.has("xp")) xp_ = static_cast<std::uint32_t>(object.getNumber("xp"));
    if (object.has("curlvl")) currentLevel_ = static_cast<int>(object.getNumber("curlvl"));
    if (object.has("win")) victoryShown_ = object.getBool("win");
    for (LevelProgress& level : levels_) level.deserialize(data);
    deserializeEdicts(data);
}

void CompanyProgress::serializeEdicts(ByteArray& data) const {
    SerializableObject object;
    object.add("edfrm", SerializableObject::SHORT, static_cast<double>(EDICTS_FORMAT_ID));
    ByteArray values;
    for (const auto& row : edicts_.getRows())
        for (const auto& edict : row) values.writeByte(edict.curPoints);
    object.add("vals", SerializableObject::BYTEARRAY, values);
    object.save(data);
}

void CompanyProgress::deserializeEdicts(ByteArray& data) {
    SerializableObject object;
    object.load(data);
    const int format = object.has("edfrm") ? static_cast<int>(object.getNumber("edfrm")) : 0;
    if (format != EDICTS_FORMAT_ID || !object.has("vals")) return;
    ByteArray values = object.getBytes("vals");
    values.setPosition(0);
    for (auto& row : edicts_.getRows())
        for (auto& edict : row) edict.curPoints = values.readByte();
}

} // namespace takeover
