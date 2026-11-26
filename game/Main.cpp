#include "game/Main.h"

#include "engine/display/Stage.h"
#include "engine/platform/Application.h"
#include "game/data/LevelInfo.h"
#include "game/framework/Symbols.h"
#include "game/framework/core/Trash.h"
#include "game/gui/BadgesRoomScreen.h"
#include "game/gui/BattleScreen.h"
#include "game/gui/CampaignScreens.h"
#include "game/gui/ChooseProfileScreen.h"
#include "game/gui/IntroScreen.h"
#include "game/gui/MissionOverScreen.h"
#include "game/gui/PreloaderScreen.h"
#include "game/gui/ThroneRoomScreen.h"
#include "game/gui/TitleScreen.h"
#include "game/level/Level.h"
#include "game/media/MusicConst.h"
#include "game/progress/LevelProgress.h"
#include "game/resources/TOResManager.h"

namespace takeover {

    Main::Main() = default;

    Main::~Main() {
        removeEventListener(engine::Event::ENTER_FRAME, this, &Main::onEnterFrame);

        if (states_)
            states_->dispose();
    }

    Profile& Main::profile() { return storage_->currentProfile(); }

    void Main::showVersion(bool enable) {
        if (enable)
            addChild(version_);

        else if (version_->parent() != nullptr)
            version_->parent()->removeChild(version_.get());
    }

    // /----- LOGO PANEL -----/

    void Main::createLogoPanel() {

        // The sponsor logo in the corner of the screens (Armor Games branding).
        auto panel =
            symbols::create(
                Branding::A10_COM ? "SmallLogoPanelA10Class" : "SmallLogoPanelClass"
            );

        logoButton_ =
            std::make_shared<Button>(
                ButtonDef::make(
                    sharedClip(symbols::child(*panel, "butLogo")), std::nullopt
                )
            );

        logoButton_->signalClick.addListener(this, &Main::onLogoClick);
        logoButton_->signalRollOut.addListener(this, &Main::onLogoRollOut);
        logoButton_->signalRollOver.addListener(this, &Main::onLogoRollOver);

        addChild(panel);
    }

    // /----- INIT -----/

    void Main::init() {
        screens_ = std::make_shared<engine::Sprite>();
        addChild(screens_);

        hintLayer_ = std::make_shared<HintLayer>();
        addChild(hintLayer_->imagePtr());

        createLogoPanel();
        logoButton_->image()->setVisible(false);
        logoButton_->setEnabled(false);

        version_ = createVersionLabel();
        showVersion(true);

        resources_ = std::make_shared<TOResManager>(FRAME_RATE, false);
        showPreloader();
    }

    void Main::showPreloader() {

        preloader_ = std::make_shared<PreloaderScreen>(2, true);
        preloader_->signalStart.addListener(this, &Main::onPreloaderStart);
        screens_->addChild(preloader_->image());

        // The preloader shows the progress of the startup rendering.
        addEventListener(engine::Event::ENTER_FRAME, this, &Main::onLoadingFrame);

        resources_->startLoadRes().addListener(this, &Main::onFinishCaching);
    }

    void Main::onLoadingFrame(engine::Event&) { preloader_->setProgress(resources_->maker().getProgress()); }

    void Main::onFinishCaching() {
        if (preloader_ == nullptr)
            return;

        removeEventListener(engine::Event::ENTER_FRAME, this, &Main::onLoadingFrame);
        preloader_->complete();
    }

    // /----- SPONSOR INTRO -----/

    std::shared_ptr<IntroScreen> Main::playIntro(const std::string& introClass, void (Main::*then)(), const std::string& url) {
        auto intro = IntroScreen::play(symbols::create(introClass), url);

        if (then != nullptr)
            intro->signalFinish.addListener(this, then);

        screens_->addChild(intro->clip());
        return intro;
    }

    void Main::onPreloaderStart() {
        showVersion(false);
        playIntro("ArmorGamesIntro", &Main::onSponsorIntroDone, Branding::URL_ARMOR_GAMES);
    }

    void Main::onSponsorIntroDone() { playIntro("IriySoftIntro", &Main::start, Branding::URL_IRIYSOFT); }

    double Main::playTransition(bool out) {
        auto intro = playIntro(out ? "scrProxy2Class" : "scrProxy3Class", nullptr, "");
        return static_cast<double>(intro->clip()->totalFrames()) / FRAME_RATE;
    }

    void Main::start() {
        if (preloader_) {
            preloader_->dispose();
            releaseLater(preloader_);
            preloader_.reset();
        }

        // Setup storage
        storage_ = std::make_shared<GameStorage>();
        storage_->signalReady.addListener(this, &Main::onStorageReady);
        storage_->init();
        storage_->load();

        // Setup stage
        engine::Stage* stage = this->stage();

        // Setup controls
        mouse_ = std::make_shared<MouseDevice>(stage);
        keyboard_ = std::make_shared<KeyboardDevice>(stage);

        states_ = std::make_shared<StateManager>();
        addEventListener(engine::Event::ENTER_FRAME, this, &Main::onEnterFrame);

        showGameIntro();
    }

    void Main::onEnterFrame(engine::Event&) {
        emptyTrash();

        // Update controls and states
        mouse_->update(FRAME_TIME);
        keyboard_->update(FRAME_TIME);
        states_->update(FRAME_TIME);

        // Achievement popups, one at a time.
        if (achievementPopup_ && achievementPopup_->currentFrame() == achievementPopup_->totalFrames()) {
            removeChild(achievementPopup_.get());
            achievementPopup_.reset();
        }

        if (!badgeQueue_.empty() && !achievementPopup_) {

            achievementPopup_ = symbols::create("AchievementMessageClass");
            achievementPopup_->setX(400);
            achievementPopup_->setY(500);

            addChild(achievementPopup_);

            symbols::setHtmlText(*achievementPopup_, "message.txtName", badgeQueue_.front()->name());
            symbols::find(*achievementPopup_, "message.icon")->gotoAndStop(static_cast<int>(badgeQueue_.front()->iconNum()));

            badgeQueue_.pop_front();
        }
    }

    void Main::onBadgeUnlocked(Badge* badge, const std::string&) {
        badgeQueue_.push_back(badge);
        storage_->save();
    }

    std::shared_ptr<ScreenDef> Main::screenDef() { return std::make_shared<ScreenDef>(); }

    std::shared_ptr<GameScreenDef> Main::gameScreenDef(std::shared_ptr<GameScreenDef> def) {
        if (!def)
            def = std::make_shared<GameScreenDef>();

        def->keyboard = keyboard_.get();
        def->mouse = mouse_.get();
        def->resources = resources_.get();
        def->profile = &profile();
        def->hintLayer = hintLayer_.get();

        return def;
    }

    // /----- SCREEN -----/

    void Main::showScreen(std::shared_ptr<GameScreen> screen, bool lockSound) {
        mouse_->reset();
        screens_->addChild(screen->imagePtr());
        states_->setState(screen);
        mouse_->reset();
        hintLayer_->hideHint();

        // Play Menu Music
        if (!lockSound)
            MusicConst::instance().playMenuMusic();
    }

    // /----- GAME INTRO -----/

    void Main::showGameIntro() {
        showVersion(false);

        // Show screen
        auto screen = std::make_shared<GameIntro>(screenDef());
        showScreen(screen, true);

        // Enable music
        MusicConst::instance().playGameplayMusic();

        // Listen intro button
        screen->signalButton.addListener(this, &Main::onGameIntroButton);
    }

    void Main::onGameIntroButton(int id) {
        if (id == GameIntro::BUT_SKIP)
            onIntroSkipped();
    }

    void Main::onIntroSkipped() {
        // As the original: a second logo panel is created here.
        createLogoPanel();
        showVersion(true);
        showTitle();
    }

    // /----- LOGO -----/

    void Main::onLogoClick(Button*) {
        engine::navigateToURL(
            Branding::A10_COM ? Branding::URL_A10COM : Branding::URL_ARMOR_GAMES
        );
    }

    void Main::onLogoRollOut(Button*) {
        Level::isCursorUnderButton = false;
    }

    void Main::onLogoRollOver(Button*) {
        Level::isCursorUnderButton = true;
    }

    // /----- TITLE -----/

    void Main::showTitle() {

        enableLogo(false);
        auto screen = std::make_shared<TitleScreen>(screenDef());

        showScreen(screen);
        screen->signalButton.addListener(this, &Main::onTitleButton);

        showVersion(true);

        profile()
            .badges()
            .signalBadgeUnlocked
            .removeListener(this, &Main::onBadgeUnlocked);
    }

    void Main::onStorageReady() {
        storageReady_ = true;
    }

    void Main::onTitleButton(int id) {

        if (!storageReady_)
            return;

        if (id == TitleScreen::BUT_START)
            showChooseProfile();
    }

    // /----- CHOOSE PROFILE -----/

    void Main::showChooseProfile() {

        enableLogo(false);
        auto screen = std::make_shared<ChooseProfileScreen>(screenDef(), *storage_);

        showScreen(screen);
        screen->
            signalButton.addListener(this, &Main::onChooseProfileButton);

        showVersion(true);
    }

    void Main::onChooseProfileButton(int id) {
        showVersion(false);

        switch (id) {
        case ChooseProfileScreen::BUT_SELECT: showChooseCampaign(); break;
        case ChooseProfileScreen::BUT_QUIT: showTitle(); break;
        }

        profile().badges().signalBadgeUnlocked.addListener(this, &Main::onBadgeUnlocked);
    }

    // /----- CHOOSE CAMPAIGN -----/

    void Main::showChooseCampaign() {
        enableLogo(true);
        auto screen = std::make_shared<ChooseCompaignScreen>(gameScreenDef());
        screen->signalButton.addListener(this, &Main::onChooseCampaignButton);
        showScreen(screen);
    }

    void Main::onChooseCampaignButton(int id) {
        switch (id) {
        case ChooseCompaignScreen::BUT_QUIT: showTitle(); break;
        case ChooseCompaignScreen::BUT_DEEDS:
        case ChooseCompaignScreen::BUT_TALES: break;
        default:
            profile().selectCompany(id);
            showGlobalMap();
            break;
        }
    }

    // /----- GLOBAL MAP -----/

    void Main::showGlobalMap() {
        auto screen = std::make_shared<GlobalMapScreen>(gameScreenDef());
        screen->signalButton.addListener(this, &Main::onGlobalMapButton);

        if (profile().selectedCompany().currentLevel() < 0)
            profile().selectedCompany().setCurrentLevel(0);

        showScreen(screen);
        screen->signalLevel.addListener(this, &Main::onLevelChosen);
        profile().checkLevelBadges();
    }

    void Main::onGlobalMapButton(int id) {
        switch (id) {
        case GlobalMapScreen::BUT_QUIT: showChooseCampaign(); break;
        case GlobalMapScreen::BUT_BADGES: showBadgesRoom(); break;
        case GlobalMapScreen::BUT_EDICTS: showThroneRoom(); break;
        default: break;
        }
    }

    void Main::onLevelChosen(int levelNum) { startLevel(levelNum); }

    // /----- MISSION -----/

    void Main::onMissionOver(std::shared_ptr<MissionResult> result) {
        enableLogo(true);
        auto screen = std::make_shared<MissionOverScreen>(missionOverDef(*result));
        screen->signalButton.addListener(this, &Main::onMissionOverButton);
        showScreen(screen);
    }

    void Main::onMissionOverButton(int id) {
        if (id != MissionOverScreen::BUT_OK)
            return;

        CompanyProgress& campaign = profile().selectedCompany();
        if (campaign.shouldShowVictory()) {
            showVictory();
            campaign.markVictoryShown();
        } else if (campaign.freeSkillPoints() > 0) {
            showThroneRoom();
        } else {
            showGlobalMap();
        }
    }

    std::shared_ptr<MissionOverDef> Main::missionOverDef(const MissionResult& result) {
        const LevelInfo level = *LevelInfo::make(profile().selectedCompany().info().id(), result.levelNum);
        const LevelProgress progress = LevelProgress::fromMission(result, level);
        const double xp = profile().addMissionResult(progress, level);
        auto def = std::make_shared<MissionOverDef>();
        gameScreenDef(def);
        def->xpEarned = xp;
        def->missionResult = std::make_shared<MissionResult>(result);
        def->completeStatus = progress.completeStatus + 1;
        storage_->save();

        return def;
    }

    // /----- VICTORY MENU -----/

    void Main::showVictory() {
        auto screen = std::make_shared<VictoryScreen>(gameScreenDef());
        screen->signalButton.addListener(this, &Main::onVictoryButton);
        showScreen(screen);
        enableLogo(false);
    }

    void Main::onVictoryButton(int id) {
        enableLogo(true);

        switch (id) {
        case VictoryScreen::BUT_YES: showChooseCampaign(); break;
        case VictoryScreen::BUT_NO: showGlobalMap(); break;
        }
    }

    // /----- THRONE ROOM -----/

    void Main::showThroneRoom() {
        auto screen = std::make_shared<ThroneRoomScreen>(gameScreenDef());
        screen->signalButton.addListener(this, &Main::onThroneRoomButton);
        showScreen(screen);
    }

    void Main::onThroneRoomButton(int id) {
        if (id == ThroneRoomScreen::BUT_OK) {
            storage_->save();
            showGlobalMap();
        }
    }

    void Main::enableLogo(bool enable) {
        logoButton_->image()->setVisible(enable);
        logoButton_->setEnabled(enable);
    }

    void Main::startLevel(int levelNum) {
        pendingLevel_ = levelNum;

        playIntro("scrProxyClass", &Main::createBattle, "");
        MusicConst::instance().stopMusic();

        enableLogo(false);
    }

    // /----- BATTLE -----/

    void Main::createBattle() {

        const int levelNum = pendingLevel_;
        enableLogo(false);

        auto screen = std::make_shared<BattleScreen>(levelNum, gameScreenDef());

        screen->signalReady.addListener(this, &Main::onBattleReady);
        screen->signalMissionOver.addListener(this, &Main::onMissionOver);
        screen->signalRestart.addListener(this, &Main::onBattleRestart);
        screen->signalQuit.addListener(this, &Main::onBattleQuit);
        screen->signalEnableLogo.addListener(this, &Main::onBattleEnableLogo);

        showScreen(screen, true);
        profile().selectedCompany().setCurrentLevel(levelNum);
        playTransition(true);
    }

    void Main::onBattleReady(BattleScreen* battle) {
        battle->setStartDelay(playTransition(false));
    }

    void Main::onBattleEnableLogo(bool enable) {
        enableLogo(enable);
    }

    void Main::onBattleRestart() {
        states_->setState(std::make_shared<GameState>());
        startLevel(profile().selectedCompany().currentLevel());
    }

    void Main::onBattleQuit() {
        enableLogo(true);
        showGlobalMap();
    }

    // /----- BADGES ROOM -----/

    void Main::showBadgesRoom() {
        auto screen = std::make_shared<BadgesRoomScreen>(gameScreenDef());
        screen->signalButton.addListener(this, &Main::onBadgesRoomButton);
        showScreen(screen);
    }

    void Main::onBadgesRoomButton(int id) {
        if (id == BadgesRoomScreen::BUT_OK)
            showGlobalMap();
    }

} // namespace takeover
