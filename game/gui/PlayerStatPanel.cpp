#include "game/gui/PlayerStatPanel.h"

#include "game/framework/Symbols.h"
#include "game/gui/GameScreen.h"
#include "game/progress/Profile.h"

#include <cmath>

namespace takeover {

PlayerStatPanel::PlayerStatPanel(engine::MovieClip* panel, Profile& profile)
    : panel_(sharedClip(panel)), profile_(&profile) {
    refresh();
}

void PlayerStatPanel::refresh() {
    CompanyProgress& campaign = profile_->selectedCompany();
    symbols::child(*panel_, "mcPanel")->gotoAndStop(campaign.info().number() + 1);
    symbols::child<engine::TextField>(*panel_, "txtRankNum")->setText(std::to_string(campaign.rank()));
    symbols::child<engine::TextField>(*panel_, "txtEdictsNum")->setText(std::to_string(campaign.freeSkillPoints()));
    const int current = campaign.xpInRank();
    const int max = campaign.rankSpan();
    auto* xpText = symbols::child<engine::TextField>(*panel_, "txtXP");
    engine::MovieClip* bar = symbols::child(*panel_, "mcXPIndicator");
    if (max == 0) {
        xpText->setHtmlText("<FONT COLOR='#FFFF00'> MAX RANK </FONT>");
        bar->gotoAndStop(bar->totalFrames());
    } else {
        xpText->setHtmlText("<FONT COLOR='#FFFF00'>" + std::to_string(current) + "</FONT><FONT COLOR='#DBDB02'> / " +
                            std::to_string(max) + "</FONT>");
        bar->gotoAndStop(static_cast<int>(std::round(static_cast<double>(current) * bar->totalFrames() / max + 1)));
    }
}

} // namespace takeover
