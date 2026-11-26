#include "game/gui/BattleScreen.h"

#include "game/GameBattle.h"
#include "game/data/LevelInfo.h"
#include "game/framework/Symbols.h"
#include "game/level/Level.h"
#include "game/media/MusicConst.h"
#include "game/progress/Badges.h"
#include "game/progress/CompanyProgress.h"
#include "game/progress/Profile.h"
#include "game/resources/TOResManager.h"

#include "engine/display/MovieClip.h"
#include "engine/display/TextField.h"

namespace takeover {

    // /----- BATTLE PAUSE SCREEN -----/

    BattlePauseScreen::BattlePauseScreen(std::shared_ptr<ScreenDef> def) {
        def->viewClass = "scrPauseClass";
        initScreen(std::move(def));
        initQuestion();
    }

    void BattlePauseScreen::customDispose() {
        disposeQuestion();
        GameScreen::customDispose();
    }

    void BattlePauseScreen::initButtonsBlock() {
        GameScreen::initButtonsBlock();
        addButton("butResume", "RESUME", 0)->signalClick.addListener([this](Button*) { signalResume.dispatch(); });
        addButton("butRestart", "RESTART", 1)->signalClick.addListener(
            [this](Button*) { ask("DO YOU REALLY WANT TO RESTART THE MISSION?", &signalRestart); });
        addButton("butQuit", "QUIT", 2)->signalClick.addListener(
            [this](Button*) { ask("DO YOU REALLY WANT TO QUIT THE MISSION?", &signalQuit); });
    }

    void BattlePauseScreen::disposeButtonsBlock() {
        signalResume.dispose();
        signalRestart.dispose();
        signalQuit.dispose();
        GameScreen::disposeButtonsBlock();
    }

    void BattlePauseScreen::initQuestion() {
        question_ = symbols::create("scrYesVsNoClass");
        no_ = std::make_shared<Button>(ButtonDef::make(sharedClip(symbols::child(*question_, "butNo")), "NO"));
        yes_ = std::make_shared<Button>(ButtonDef::make(sharedClip(symbols::child(*question_, "butYes")), "YES"));
    }

    void BattlePauseScreen::disposeQuestion() {
        pendingYes_ = nullptr;

        if (no_)
            no_->dispose();

        if (yes_)
            yes_->dispose();

        no_.reset(), yes_.reset();

        if (question_ && question_->parent() != nullptr)
            question_->parent()->removeChild(question_.get());

        question_.reset();
    }

    void BattlePauseScreen::ask(const std::string& message, Signal<>* onYes) {
        // The message field of the question window (_SafeStr_3168 is "_-Mq").
        for (int i = 0; i < question_->numChildren(); ++i)
            if (auto* field = symbols::child<engine::TextField>(question_->getChildAt(i), "txtField"))
                field->setHtmlText(message);
        pendingYes_ = onYes;

        yes_->signalClick.addListener([this](Button*) {
            if (pendingYes_ != nullptr)
                pendingYes_->dispatch();
        });

        no_->signalClick.addListener([this](Button*) { closeQuestion(); });
        image_->addChild(question_);
    }

    void BattlePauseScreen::closeQuestion() {
        pendingYes_ = nullptr;
        yes_->signalClick.removeAllListeners();
        no_->signalClick.removeAllListeners();

        if (question_->parent() != nullptr)
            question_->parent()->removeChild(question_.get());
    }

    // /----- HELP SCREEN -----/

    HelpScreen::HelpScreen(GameBattle& battle, int levelId) : levelId_(levelId) {
        if (levelId_ >= 3) {
            // No tutorial: an empty sprite.
            image_ = std::make_shared<engine::Sprite>();
            return;
        }

        auto def = std::make_shared<ScreenDef>();
        def->viewClass = "scrTutorialClass";
        initScreen(def);
        battle.signalTutorialEvent.addListener(this, &HelpScreen::onTutorialEvent);
        showPanel(true);
    }

    void HelpScreen::update(double timeElapsed) {
        if (levelId_ >= 3)
            return;

        GameScreen::update(timeElapsed);
    }

    void HelpScreen::initVisualBlock() {
        GameScreen::initVisualBlock();
        panel_ = dynamic_cast<engine::MovieClip*>(image_.get());
        text_ = symbols::create(levelId_ == 1 ? "Tutorial1TextClass" : "Tutorial2TextClass");
        text_->gotoAndStop(1);

        if (auto* holder = symbols::child(*panel_, "mcPanel"))
            holder->addChild(text_);
    }

    void HelpScreen::disposeVisualBlock() {
        panel_ = nullptr;
        GameScreen::disposeVisualBlock();
    }

    void HelpScreen::initButtonsBlock() {
        GameScreen::initButtonsBlock();
        engine::MovieClip* button = clip("butAvatar");
        switchText_ = symbols::find<engine::TextField>(*button, "butShowHide.mcText.txt");

        if (auto* showHide = symbols::child(*button, "butShowHide"))
            showHide->gotoAndStop(1);

        addButton(button, std::nullopt, BUT_SWITCH);
    }

    void HelpScreen::disposeButtonsBlock() {
        switchText_ = nullptr;

        if (text_ && text_->parent() != nullptr)
            text_->parent()->removeChild(text_.get());

        text_.reset();
        GameScreen::disposeButtonsBlock();
    }

    void HelpScreen::onButtonClick(Button* button) {
        if (button->id() == BUT_SWITCH)
            showPanel(!shown_);

        GameScreen::onButtonClick(button);
    }

    void HelpScreen::showPanel(bool show) {
        panel_->gotoAndStop(show ? "lbShow" : "lbHide");

        if (switchText_ != nullptr)
            switchText_->setText(show ? "HIDE" : "SHOW");

        shown_ = show;
    }

    void HelpScreen::onTutorialEvent(const std::string& eventId) {
        switch (levelId_) {
        case 1: onTutorial1Event(eventId); break; // and then the mission 2 handler, like the original
        case 2: break;
        default: return;
        }

        onTutorial2Event(eventId);
    }

    void HelpScreen::onTutorial1Event(const std::string& id) {
        const int frame = text_->currentFrame();
        if (id == GameBattle::EVENT_LVL1_CASTLE_CLICKED && frame == 1)
            text_->gotoAndStop(2);
        else if (id == GameBattle::EVENT_LVL1_CASTLE_MENU_CLOSES && frame == 2)
            text_->gotoAndStop(1);
        else if (id == GameBattle::EVENT_LVL1_GUARDS_PURCHASING_STARTED && frame == 2)
            text_->gotoAndStop(3);
        else if (id == GameBattle::EVENT_LVL1_GUARDS_PURCHASING_FINISHED && frame == 3)
            text_->gotoAndStop(4);
        else if (id == GameBattle::EVENT_LVL1_ENEMY_SQUAD_DESTROYED && frame == 4)
            text_->gotoAndStop(5);
        else if (id == GameBattle::EVENT_LVL1_TOWN_SIEZED && frame == 5)
            text_->gotoAndStop(6);
    }

    void HelpScreen::onTutorial2Event(const std::string& id) {
        const int frame = text_->currentFrame();

        if (id == GameBattle::EVENT_LVL2_FARM_CONSTRUCTION_STARTED) {
            if (frame == 1)
                text_->gotoAndStop(2);
        } else if (id == GameBattle::EVENT_LVL2_FARM_CONSTRUCTION_FINISHED) {
            if (frame == 2)
                text_->gotoAndStop(3);
        } else if (id == GameBattle::EVENT_LVL2_SQUAD_PURCHASING_STARTED) {
            if (frame == 3 || frame == 5)
                text_->gotoAndStop(4);
        } else if (id == GameBattle::EVENT_LVL2_FIGHT_WITH_OGRE_STARTED) {
            if (frame == 4)
                text_->gotoAndStop(5);
        } else if (id == GameBattle::EVENT_LVL2_FIGHT_WITH_OGRE_FINISHED || id == GameBattle::EVENT_LVL2_BANNER_SET) {
            if (frame == 5)
                text_->gotoAndStop(6);
        } else if (id == GameBattle::EVENT_LVL2_MAGIC_TOWER_SIEZED) {
            if (frame == 6)
                text_->gotoAndStop(7);
        } else if (id == GameBattle::EVENT_LVL2_ULTIMATE_CAST) {
            if (frame == 7)
                text_->gotoAndStop(8);
        } else if (id == GameBattle::EVENT_LVL2_CASTLE_SIEZED) {
            if (frame == 6)
                text_->gotoAndStop(9);
        }
    }

    // /----- BATTLE SCREEN -----/

    BattleScreen::BattleScreen(int levelNum, std::shared_ptr<GameScreenDef> def) : levelNum_(levelNum), gameDef_(def) {
        def->viewClass = "scrBattleClass";
        initScreen(std::move(def));
        createBattle();
        playTime_ = 0;
        enemiesDestroyed_ = 0;
        strongholdsCaptured_ = 0;
    }

    BattleScreen::~BattleScreen() = default;

    void BattleScreen::customDispose() {
        destroyBattle();
        GameScreen::customDispose();
        gameDef_.reset();
    }

    void BattleScreen::update(double timeElapsed) {
        GameScreen::update(timeElapsed);

        if (pause_) {
            pause_->update(timeElapsed);
            return;
        }

        help_->update(timeElapsed);

        if (battle_)
            battle_->update(timeElapsed);

        if (endDelay_ > 0) {
            endDelay_ -= timeElapsed;

            if (endDelay_ <= 0)
                signalMissionOver.dispatch(result_);
        } else {
            playTime_ += timeElapsed;
        }
    }

    void BattleScreen::initVisualBlock() {
        GameScreen::initVisualBlock();
        content_ = std::make_shared<engine::Sprite>();
        image_->addChildAt(content_, 0);
    }

    void BattleScreen::disposeVisualBlock() {
        if (content_ && content_->parent() != nullptr)
            content_->parent()->removeChild(content_.get());

        content_.reset();
        GameScreen::disposeVisualBlock();
    }

    void BattleScreen::createBattle() {
        pause_.reset();
        CompanyProgress& company = gameDef_->profile->selectedCompany();
        const std::optional<LevelInfo> info = LevelInfo::make(company.info().id(), levelNum_);
        BattleDef def;
        def.resources = gameDef_->resources;
        def.mouse = gameDef_->mouse;
        def.keyboard = gameDef_->keyboard;
        def.levelNum = levelNum_;
        def.levelUID = info->globalId();
        def.player = company.createPlayer(*info);
        def.enemyPlayers = info->enemyPlayers();
        def.hintLayer = gameDef_->hintLayer;
        battle_ = std::make_shared<GameBattle>(def);
        battle_->signalMenu.addListener(this, &BattleScreen::onMenu);
        battle_->signalStatsChanged().addListener(this, &BattleScreen::onStatsChanged);
        battle_->signalMissionOver.addListener(this, &BattleScreen::onMissionOver);
        content_->addChild(battle_->image());
        help_ = std::make_shared<HelpScreen>(*battle_, def.levelUID);
        content_->addChild(help_->imagePtr());
        MusicConst::instance().playGameplayMusic();
        endDelay_ = 0;
        result_.reset();
    }

    void BattleScreen::destroyBattle() {
        result_.reset();

        // (The original sends the play time to its online statistics here.)
        closePause();

        if (help_)
            help_->dispose();

        help_.reset();

        if (battle_)
            battle_->dispose();

        battle_.reset();
        signalQuit.dispose();
        signalRestart.dispose();
        signalEnableLogo.dispose();
        signalMissionOver.dispose();
        signalReady.dispose();
    }

    void BattleScreen::onMenu() {
        pause_ = std::make_shared<BattlePauseScreen>(std::make_shared<ScreenDef>());
        pause_->signalResume.addListener(this, &BattleScreen::onResume);
        pause_->signalRestart.addListener(this, &BattleScreen::onRestart);
        pause_->signalQuit.addListener(this, &BattleScreen::onQuit);
        content_->addChild(pause_->imagePtr());
        signalEnableLogo.dispatch(true);
    }

    void BattleScreen::onQuit() {
        if (Level::isModeOn(Level::ST_CHEAT_MODE)) {
            // Cheat: quitting wins the mission.
            battle_->missionStats()->missionOverStatus = MissionResult::VICTORY;
            signalMissionOver.dispatch(battle_->missionStats());
        } else {
            signalQuit.dispatch();
        }
    }

    void BattleScreen::onResume() {
        closePause();
        signalEnableLogo.dispatch(false);
    }

    void BattleScreen::closePause() {
        if (pause_)
            pause_->dispose();

        pause_.reset();
    }

    void BattleScreen::onStatsChanged(Level* level) {
        Badges& badges = gameDef_->profile->badges();
        const MissionStats& stats = *level->player().missionStats;
        const auto set = [&](const std::string& metric, double value) { badges.metric(metric).setValue(value); };
        const auto get = [&](const std::string& metric) { return badges.metric(metric).value(); };
        set(Badges::METRIC_STRONGHOLDS_CAPTURED, stats.get(MissionStats::STRONGHOLDS_SEIZED));
        const int newStrongholds = stats.get(MissionStats::STRONGHOLDS_SEIZED) - strongholdsCaptured_;
        strongholdsCaptured_ = stats.get(MissionStats::STRONGHOLDS_SEIZED);
        set(Badges::METRIC_STRONGHOLDS_CAPTURED_TOTAL, get(Badges::METRIC_STRONGHOLDS_CAPTURED_TOTAL) + newStrongholds);
        set(Badges::METRIC_SQUADS_PURCHASED, stats.get(MissionStats::SQUADS_PURCHASED));
        set(Badges::METRIC_GOLD_COLLECTED, level->player().getGold());
        set(Badges::METRIC_CHESTS_OPENED, stats.get(MissionStats::CHESTS_OPENED));
        set(Badges::METRIC_BANNERS_SET, stats.get(MissionStats::BANNERS_SET));
        set(Badges::METRIC_ENEMY_SQUADS_DESTROYED, stats.get(MissionStats::ENEMY_SQUADS_DESTROYED));
        const int newEnemies = stats.get(MissionStats::ENEMY_SQUADS_DESTROYED) - enemiesDestroyed_;
        enemiesDestroyed_ = stats.get(MissionStats::ENEMY_SQUADS_DESTROYED);
        set(Badges::METRIC_ENEMY_SQUADS_DESTROYED_TOTAL, get(Badges::METRIC_ENEMY_SQUADS_DESTROYED_TOTAL) + newEnemies);
        set(Badges::METRIC_HEROIC_MODE_ENGAGED, stats.get(MissionStats::HEROIC_MODE_ENGAGED));
        set(Badges::METRIC_VILLAGES_OWNED, stats.get(MissionStats::VILLAGES_OWNED));
        set(Badges::METRIC_MINES_OWNED, stats.get(MissionStats::MINES_OWNED));
        set(Badges::METRIC_OBELISKS_OWNED, stats.get(MissionStats::OBELISKS_OWNED));
        set(Badges::METRIC_SQUADS_OWNED, stats.get(MissionStats::SQUADS_OWNED));
        set(Badges::METRIC_ULITIMATE_USED, stats.get(MissionStats::ULTIMATES_USED));
    }

    void BattleScreen::onMissionOver(std::shared_ptr<MissionResult> result) {
        battle_->signalMissionOver.removeListener(this, &BattleScreen::onMissionOver);
        result_ = std::move(result);
        signalReady.dispatch(this);
    }

} // namespace takeover
