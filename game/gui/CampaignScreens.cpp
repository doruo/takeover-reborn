#include "game/gui/CampaignScreens.h"

#include "game/data/LevelInfo.h"
#include "game/framework/Symbols.h"
#include "game/framework/core/Trash.h"
#include "game/progress/Profile.h"
#include "game/sim/battle/Combatant.h"

namespace takeover {

    // /----- CHOOSE COMPAIGN SCREEN -----/

    ChooseCompaignScreen::ChooseCompaignScreen(std::shared_ptr<GameScreenDef> def) {
        def->viewClass = "scrChooseCampaignClass";
        initScreen(std::move(def));
    }

    void ChooseCompaignScreen::initButtonsBlock() {
        GameScreen::initButtonsBlock();

        // Campaign buttons: _-lS Westaria, _-1k Cult, _-bD Khaganate.
        addCampaignButton(clip("_-lS"), CompanyInfo::WESTARIA);

        // The other campaigns open once the third Westaria mission is reached.
        const bool unlocked = def()->profile->company(CompanyInfo::WESTARIA).firstOpenLevelNumber() >= 3;
        addCampaignButton(clip("_-1k"), CompanyInfo::CULT)->setEnabled(unlocked);
        addCampaignButton(clip("_-bD"), CompanyInfo::KHAGANATE)->setEnabled(unlocked);
        addButton("butQuit", "QUIT", BUT_QUIT);
    }

    Button* ChooseCompaignScreen::addCampaignButton(engine::MovieClip* button, int companyId) {
        const CompanyProgress& campaign = def()->profile->company(companyId);
        symbols::setHtmlText(*button, "mcRankBlock.mcRank.txtField", std::to_string(campaign.rank()));
        symbols::setHtmlText(*button, "mcRankBlock.mcProgress.txtField",
                             std::to_string(campaign.completedCount()) + "/" + std::to_string(campaign.levelCount()));
        symbols::child(*button, "mcNewGameMessage")->setVisible(!campaign.isStarted());
        symbols::child(*button, "mcRankBlock")->setVisible(campaign.isStarted());

        return addButton(button, std::nullopt, companyId);
    }

    // /----- GLOBAL MAP SCREEN -----/

    GlobalMapScreen::GlobalMapScreen(std::shared_ptr<GameScreenDef> def) : profile_(def->profile) {
        def->viewClass = "scrCompaignMapClass";
        initScreen(std::move(def));
        createLevelButtons();
    }

    void GlobalMapScreen::customDispose() {
        disposeLevelButtons();
        hideLevelInfo();
        signalLevel.dispose();
        GameScreen::customDispose();
    }

    void GlobalMapScreen::initVisualBlock() {
        GameScreen::initVisualBlock();
        statPanel_ = std::make_unique<PlayerStatPanel>(clip("_-dj"), *profile_);
    }

    void GlobalMapScreen::disposeVisualBlock() {
        statPanel_.reset();
        GameScreen::disposeVisualBlock();
    }

    void GlobalMapScreen::createLevelButtons() {
        CompanyProgress& campaign = profile_->selectedCompany();
        mapClip()->gotoAndStop(campaign.info().number() + 1);
        engine::MovieClip* iconPlace = symbols::child(*mapClip(), "mcLevels");
        const int openLevelCount = campaign.firstOpenLevelNumber();

        for (int i = 0; i < campaign.levelCount(); ++i) {
            engine::MovieClip* icon = symbols::child(*iconPlace, "level" + std::to_string(i));
            auto button = std::make_shared<Button>(ButtonDef::make(sharedClip(icon), std::to_string(i + 1), i));
            symbols::child(*button->image(), "mcStatus")->gotoAndStop(campaign.levels()[static_cast<std::size_t>(i)].completeStatus + 1);
            button->signalClick.addListener(this, &GlobalMapScreen::onLevelClick);

            if (i >= openLevelCount)
                button->setEnabled(false);

            levelButtons_.push_back(button);
        }

        // The road drawn on the map: one frame per progress step and campaign.
        clip("_-0D")->gotoAndStop(openLevelCount + campaign.info().id() * 9 + (campaign.isCompleted() ? 1 : 0));
    }

    void GlobalMapScreen::disposeLevelButtons() {
        for (auto& button : levelButtons_)
            button->dispose();

        levelButtons_.clear();
    }

    void GlobalMapScreen::onLevelClick(Button* button) {
        showLevelInfo(button->image()->x(), button->image()->y() - 15, button->id());
    }

    void GlobalMapScreen::showLevelInfo(double x, double y, int levelNum) {
        hideLevelInfo();
        const LevelInfo level = *LevelInfo::make(profile_->selectedCompany().info().number(), levelNum);
        infoPanel_ = symbols::create("LevelInfoPanelClass");
        infoPanel_->setX(x);
        infoPanel_->setY(y);

        if (infoPanel_->x() < 200)
            infoPanel_->gotoAndStop("lbLeftBorder");
        else if (infoPanel_->x() < 360)
            infoPanel_->gotoAndStop("lbLeft");
        else if (infoPanel_->x() < 550)
            infoPanel_->gotoAndStop("lbRight");
        else
            infoPanel_->gotoAndStop("lbRightBorder");

        const int colorFrame = profile_->selectedCompany().info().number() + 1;
        symbols::child(*infoPanel_, "mcColor")->gotoAndStop(colorFrame);
        symbols::child(*infoPanel_, "mcRing")->gotoAndStop(colorFrame);
        symbols::child(*infoPanel_, "mcBack")->addEventListener(engine::MouseEvent::CLICK, this, &GlobalMapScreen::onInfoBackClick);

        // Obfuscated children: _-eo reward block, _-5M caption, _-oc info
        // block, _-Xi player crest, _-yX "to battle" button.
        symbols::setHtmlText(*infoPanel_, "_-eo.txtReward", std::to_string(profile_->missionXp(level)));
        symbols::child<engine::TextField>(*infoPanel_, "_-5M")->setText(level.caption());
        symbols::setHtmlText(*infoPanel_, "_-oc.txtInfo", level.description());
        symbols::child(*infoPanel_, "_-Xi")->gotoAndStop(colorFrame);

        const auto enemies = level.enemyPlayers();
        symbols::child(*infoPanel_, "mcEnemyCrest1")->gotoAndStop(CompanyInfo::raceIconFrame(enemies[0]->getRace()));

        if (enemies.size() > 1)
            symbols::child(*infoPanel_, "mcEnemyCrest2")->gotoAndStop(CompanyInfo::raceIconFrame(enemies[1]->getRace()));
        else
            symbols::child(*infoPanel_, "mcEnemyCrest2")->setVisible(false);

        if (enemies.size() > 2)
            symbols::child(*infoPanel_, "mcEnemyCrest3")->gotoAndStop(CompanyInfo::raceIconFrame(enemies[2]->getRace()));
        else
            symbols::child(*infoPanel_, "mcEnemyCrest3")->setVisible(false);

        toBattle_ = std::make_shared<Button>(ButtonDef::make(sharedClip(symbols::child(*infoPanel_, "_-yX")), "TO BATTLE", levelNum));
        toBattle_->signalClick.addListener(this, &GlobalMapScreen::onToBattle);
        image_->addChild(infoPanel_);
    }

    void GlobalMapScreen::onToBattle(Button* button) { signalLevel.dispatch(button->id()); }

    void GlobalMapScreen::onInfoBackClick(engine::MouseEvent&) { hideLevelInfo(); }

    void GlobalMapScreen::hideLevelInfo() {
        if (!infoPanel_)
            return;

        symbols::child(*infoPanel_, "mcBack")->removeEventListener(engine::MouseEvent::CLICK, this, &GlobalMapScreen::onInfoBackClick);

        // The original never disposes the "to battle" button (a local variable
        // shadows the member); dropping it changes nothing visible.
        if (toBattle_)
            releaseLater(toBattle_);

        toBattle_.reset();

        if (infoPanel_->parent())
            infoPanel_->parent()->removeChild(infoPanel_.get());

        releaseLater(infoPanel_);
        infoPanel_.reset();
    }

    void GlobalMapScreen::initButtonsBlock() {
        GameScreen::initButtonsBlock();
        addButton("butQuit", "QUIT", BUT_QUIT);
        addButton("_-e2", "EDICTS", BUT_EDICTS);
        Button* badges = addButton("_-NV", "BADGES", BUT_BADGES);
        int unlocked = 0;

        for (const std::string& id : Badges::badgeIds())
            if (profile_->badges().badge(id)->condition()->fulfilled())
                ++unlocked;
        symbols::setHtmlText(*badges->image(), "txtProgress",
                             std::to_string(unlocked) + " / " + std::to_string(Badges::badgeIds().size()));
        // The original computes the frame from `bar - 1` (a clip minus one: NaN),
        // so the bar always shows its first frame.
        symbols::child(*badges->image(), "bar")->gotoAndStop(1);
    }

} // namespace takeover
