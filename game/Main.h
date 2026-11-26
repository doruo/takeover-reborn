// Main.h - the game's root sprite: boot sequence, screen flow, music and
// achievement popups (original: Main).
//
//   loading screen -> Armor Games intro -> IriySoft intro -> GameIntro
//   -> Title -> ChooseProfile -> ChooseCampaign -> GlobalMap
//   GlobalMap -> ThroneRoom / BadgesRoom / Battle -> MissionOver
//   -> GlobalMap / Victory / ThroneRoom
#pragma once

#include "engine/display/DisplayObjectContainer.h"
#include "game/GameStorage.h"
#include "game/framework/input/KeyboardDevice.h"
#include "game/framework/input/MouseDevice.h"
#include "game/framework/states/StateManager.h"
#include "game/framework/ui/Button.h"
#include "game/framework/ui/HintWindow.h"
#include "game/gui/ScreenDefs.h"

#include <deque>
#include <memory>

namespace takeover {

class Badge;
class BattleScreen;
class GameScreen;
class IntroScreen;
class PreloaderScreen;
class TOResManager;

class Main : public engine::Sprite {
public:
    Main();
    ~Main() override;

    /// Builds the scene once Main is on the stage (original: init).
    void init();

private:
    [[nodiscard]] Profile& profile(); ///< _SafeStr_660
    void showVersion(bool enable);    ///< _SafeStr_668
    void createLogoPanel();
    void showPreloader();             ///< _SafeStr_729
    void onPreloaderStart();          ///< _SafeStr_718
    void onLoadingFrame(engine::Event& event); ///< _SafeStr_687
    void onFinishCaching();           ///< onFinishCahing
    std::shared_ptr<IntroScreen> playIntro(const std::string& introClass, void (Main::*then)(), const std::string& url); ///< _SafeStr_670
    void onSponsorIntroDone();        ///< _SafeStr_728
    double playTransition(bool out);  ///< _SafeStr_707
    void start();
    void onEnterFrame(engine::Event& event);
    void onBadgeUnlocked(Badge* badge, const std::string& key); ///< _SafeStr_696

    std::shared_ptr<ScreenDef> screenDef();         ///< _SafeStr_693
    std::shared_ptr<GameScreenDef> gameScreenDef(std::shared_ptr<GameScreenDef> def = nullptr); ///< _SafeStr_669
    void showScreen(std::shared_ptr<GameScreen> screen, bool lockSound = false); ///< _SafeStr_663

    void showGameIntro();               ///< _SafeStr_734
    void onGameIntroButton(int id);     ///< _SafeStr_719
    void onIntroSkipped();              ///< _SafeStr_710
    void onLogoClick(Button*);          ///< _SafeStr_700
    void onLogoRollOut(Button*);        ///< _SafeStr_699
    void onLogoRollOver(Button*);       ///< _SafeStr_698
    void showTitle();                   ///< _SafeStr_692
    void onStorageReady();              ///< _SafeStr_715
    void onTitleButton(int id);         ///< _SafeStr_737
    void showChooseProfile();           ///< _SafeStr_724
    void onChooseProfileButton(int id); ///< _SafeStr_712
    void showChooseCampaign();          ///< _SafeStr_688
    void onChooseCampaignButton(int id);///< _SafeStr_739
    void showGlobalMap();               ///< _SafeStr_672
    void onGlobalMapButton(int id);     ///< _SafeStr_721
    void onLevelChosen(int levelNum);   ///< _SafeStr_716
    void onMissionOver(std::shared_ptr<MissionResult> result); ///< _SafeStr_717
    void onMissionOverButton(int id);   ///< _SafeStr_711
    std::shared_ptr<MissionOverDef> missionOverDef(const MissionResult& result); ///< _SafeStr_727
    void showVictory();                 ///< _SafeStr_731
    void onVictoryButton(int id);       ///< _SafeStr_744
    void showThroneRoom();              ///< _SafeStr_705
    void onThroneRoomButton(int id);    ///< _SafeStr_732
    void enableLogo(bool enable);       ///< _SafeStr_662
    void startLevel(int levelNum);      ///< _SafeStr_706
    void createBattle();                ///< _SafeStr_726
    void onBattleReady(BattleScreen* battle); ///< _SafeStr_735
    void onBattleEnableLogo(bool enable);     ///< _SafeStr_736
    void onBattleRestart();             ///< _SafeStr_738
    void onBattleQuit();                ///< _SafeStr_740
    void showBadgesRoom();              ///< _SafeStr_741
    void onBadgesRoomButton(int id);    ///< _SafeStr_709

    static constexpr int FRAME_RATE = 30;   ///< _SafeStr_684
    static constexpr double FRAME_TIME = 1.0 / FRAME_RATE; ///< _SafeStr_681: dt of every update

    std::shared_ptr<engine::Sprite> screens_;    ///< Original: _SafeStr_674.
    std::shared_ptr<HintLayer> hintLayer_;       ///< Original: _SafeStr_677.
    std::shared_ptr<GameStorage> storage_;       ///< Original: _SafeStr_665.
    std::shared_ptr<MouseDevice> mouse_;         ///< Original: _SafeStr_673.
    std::shared_ptr<KeyboardDevice> keyboard_;   ///< Original: _SafeStr_685.
    std::shared_ptr<StateManager> states_;       ///< Original: _SafeStr_682.
    std::shared_ptr<TOResManager> resources_; ///< Original: _SafeStr_680.
    std::shared_ptr<engine::DisplayObject> version_; ///< Original: _SafeStr_676.
    std::shared_ptr<Button> logoButton_;         ///< Original: _SafeStr_664.
    std::shared_ptr<PreloaderScreen> preloader_; ///< Original: _SafeStr_666.
    std::deque<Badge*> badgeQueue_;              ///< Original: _SafeStr_671.
    std::shared_ptr<engine::MovieClip> achievementPopup_; ///< Original: _SafeStr_661.
    bool storageReady_ = false;                  ///< Original: _SafeStr_703.
    int pendingLevel_ = 0;                       ///< Original: _SafeStr_697.
};

} // namespace takeover
