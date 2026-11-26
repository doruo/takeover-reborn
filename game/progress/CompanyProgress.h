// CompanyProgress.h - a campaign in a profile: XP and rank, the medal of
// each mission, the edicts bought (original: progress/CompanyProgress).
#pragma once

#include "game/data/CompanyInfo.h"
#include "game/framework/storage/ByteArray.h"
#include "game/progress/LevelProgress.h"
#include "game/sim/battle/Edicts.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace takeover {

class Badges;
class Combatant;
class LevelInfo;

class CompanyProgress {
public:
    static constexpr int LEVEL_COUNT = 8;

    explicit CompanyProgress(int companyId = 0);

    [[nodiscard]] const CompanyInfo& info() const { return info_; }

    // --- Experience ---------------------------------------------------------------------
    [[nodiscard]] std::uint32_t xp() const { return xp_; }
    void setXp(std::uint32_t xp) { xp_ = XPConverter::clampXp(xp); }
    [[nodiscard]] int rank() const { return static_cast<int>(XPConverter::rankOf(xp_)); }    ///< _SafeStr_3489
    [[nodiscard]] int xpInRank() const { return static_cast<int>(XPConverter::xpInRank(xp_)); } ///< _SafeStr_3626
    [[nodiscard]] int rankSpan() const { return static_cast<int>(XPConverter::rankSpan(xp_)); } ///< _SafeStr_3627

    // --- Missions -------------------------------------------------------------------------
    [[nodiscard]] std::vector<LevelProgress>& levels() { return levels_; }
    [[nodiscard]] const std::vector<LevelProgress>& levels() const { return levels_; }
    /// Mission chosen on the map, -1 before the first one (original: _SafeStr_689).
    [[nodiscard]] int currentLevel() const { return currentLevel_; }
    void setCurrentLevel(int level) { currentLevel_ = level; }
    /// Has this campaign been started (original: _SafeStr_3488)?
    [[nodiscard]] bool isStarted() const { return currentLevel_ >= 0; }
    [[nodiscard]] std::string curLevelGlobalID() const;
    [[nodiscard]] int levelCount() const { return static_cast<int>(levels_.size()); }     ///< _SafeStr_3490
    /// Number of the first mission not yet won, 1-based (original: _SafeStr_3491).
    [[nodiscard]] int firstOpenLevelNumber() const;
    /// Missions won in a row from the first (original: _SafeStr_3492).
    [[nodiscard]] int completedCount() const;
    [[nodiscard]] int goldMedalCount() const;                                             ///< _SafeStr_3640
    [[nodiscard]] bool isCompleted() const; ///< Last mission won (original: _SafeStr_3507).
    /// The victory screen hasn't been shown yet for a completed campaign (original: _SafeStr_759).
    [[nodiscard]] bool shouldShowVictory() const { return !victoryShown_ && isCompleted(); }
    void markVictoryShown() { victoryShown_ = true; } ///< _SafeStr_768

    // --- Edicts ---------------------------------------------------------------------------
    [[nodiscard]] EdictTree& edicts() { return edicts_; } ///< _SafeStr_3457
    [[nodiscard]] const EdictTree& edicts() const { return edicts_; }
    /// Points the rank gives to spend on edicts (original: _SafeStr_3638).
    [[nodiscard]] int skillPoints() const;
    /// Points not spent yet (original: native, i.e. a method the obfuscator named "native").
    [[nodiscard]] int freeSkillPoints() const { return skillPoints() - edicts_.totalPoints(); }

    /// The player for a mission of this campaign (original: _SafeStr_3610).
    [[nodiscard]] std::shared_ptr<Combatant> createPlayer(const LevelInfo& level) const;

    // --- Badges ------------------------------------------------------------------------------
    /// Updates the mission metrics; returns the number of gold medals.
    int checkLevelBadges(Badges& badges) const;
    void updateEdictsBadge(Badges& badges) const; ///< _SafeStr_3471

    // --- Save ------------------------------------------------------------------------------------
    void reset();
    void serialize(ByteArray& data) const;
    void deserialize(ByteArray& data);

private:
    static constexpr int SAVE_VER = 1;
    static constexpr int SKILL_POINTS_PER_RANK = 2;
    static constexpr int MAX_SKILL_POINTS = 27;
    static constexpr int EDICTS_FORMAT_ID = 1;

    void resetEdicts(); ///< _SafeStr_3642
    void serializeEdicts(ByteArray& data) const;
    void deserializeEdicts(ByteArray& data);

    CompanyInfo info_;
    std::uint32_t xp_ = 0;
    bool victoryShown_ = false;
    int currentLevel_ = -1;
    EdictTree edicts_;
    std::vector<LevelProgress> levels_;
};

} // namespace takeover
