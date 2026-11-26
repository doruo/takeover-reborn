// BattleScreen.h - the screen hosting a battle (original: gui/BattleScreen,
// gui/BattlePauseScreen and gui/HelpScreen).
//
// It builds the GameBattle for the chosen mission, shows the tutorial panel
// on the first two missions, opens the pause menu (resume, restart, quit
// with a confirmation), and reports the end of the mission after the
// closing transition. It also feeds the badges' metrics as the battle goes.
#pragma once

#include "game/gui/GameScreen.h"

#include <memory>

namespace engine {
    class TextField;
}

namespace takeover {

    class GameBattle;
    class Level;

    /// The pause menu (original: gui/BattlePauseScreen).
    class BattlePauseScreen : public GameScreen {
    public:
        explicit BattlePauseScreen(std::shared_ptr<ScreenDef> def);

        Signal<> signalResume;  ///< Original: _SafeStr_3611.
        Signal<> signalRestart; ///< Original: _SafeStr_750.
        Signal<> signalQuit;    ///< Original: _SafeStr_753.

    protected:
        void customDispose() override;
        void initButtonsBlock() override;
        void disposeButtonsBlock() override;

    private:
        void initQuestion();    ///< _SafeStr_3679
        void disposeQuestion(); ///< _SafeStr_3677

        /// Asks "really?"; yes dispatches `onYes` (original: _SafeStr_3671).
        void ask(const std::string& message, Signal<>* onYes);
        void closeQuestion();   ///< _SafeStr_3678

        std::shared_ptr<engine::MovieClip> question_; ///< scrYesVsNoClass (original: _SafeStr_3664).
        std::shared_ptr<Button> no_;                  ///< Original: _SafeStr_3666.
        std::shared_ptr<Button> yes_;                 ///< Original: _SafeStr_3665.
        Signal<>* pendingYes_ = nullptr;              ///< Original: _SafeStr_3670.
    };

    /// The tutorial panel of missions 1 and 2 (original: gui/HelpScreen).
    class HelpScreen : public GameScreen {
    public:
        static constexpr int BUT_SWITCH = 1;

        HelpScreen(GameBattle& battle, int levelId);

        void update(double timeElapsed) override;

    protected:
        void initVisualBlock() override;
        void disposeVisualBlock() override;
        void initButtonsBlock() override;
        void disposeButtonsBlock() override;
        void onButtonClick(Button* button) override;

    private:
        void showPanel(bool show); ///< _SafeStr_3685
        void onTutorialEvent(const std::string& eventId); ///< _SafeStr_3689
        void onTutorial1Event(const std::string& eventId);
        void onTutorial2Event(const std::string& eventId);

        int levelId_;                                 ///< Original: _SafeStr_3682.
        engine::MovieClip* panel_ = nullptr;          ///< scrTutorialClass (original: _SafeStr_3683).
        std::shared_ptr<engine::MovieClip> text_;     ///< Tutorial<N>TextClass (original: _SafeStr_3681).
        engine::TextField* switchText_ = nullptr;     ///< Original: _SafeStr_3684.
        bool shown_ = false;                          ///< Original: _SafeStr_3686.
    };

    class BattleScreen : public GameScreen {
    public:
        BattleScreen(int levelNum, std::shared_ptr<GameScreenDef> def);
        ~BattleScreen() override;

        Signal<BattleScreen*> signalReady;                         ///< The battle is over (original: _SafeStr_769).
        Signal<std::shared_ptr<MissionResult>> signalMissionOver;  ///< After the delay (original: _SafeStr_767).
        Signal<> signalRestart;                                    ///< Original: _SafeStr_750.
        Signal<> signalQuit;                                       ///< Original: _SafeStr_753.
        Signal<bool> signalEnableLogo;                             ///< Paused or not (original: _SafeStr_760).

        /// Reports the end of the mission after `seconds` (original: _SafeStr_752).
        void setStartDelay(double seconds) { endDelay_ = seconds; }

        void update(double timeElapsed) override;

    protected:
        void customDispose() override;
        void initVisualBlock() override;
        void disposeVisualBlock() override;

    private:
        void createBattle(); ///< _SafeStr_1199
        void destroyBattle(); ///< _SafeStr_1150
        void onMenu();        ///< _SafeStr_3607
        void onRestart() { signalRestart.dispatch(); } ///< _SafeStr_738
        void onQuit();        ///< _SafeStr_740
        void onResume();      ///< _SafeStr_3609
        void closePause();    ///< _SafeStr_3605
        void onStatsChanged(Level* level); ///< _SafeStr_3608
        void onMissionOver(std::shared_ptr<MissionResult> result); ///< _SafeStr_735

        int levelNum_;                               ///< Original: _SafeStr_3475.
        std::shared_ptr<GameScreenDef> gameDef_;     ///< Original: _SafeStr_839.
        double playTime_ = 0;                        ///< Original: _SafeStr_3600.
        std::shared_ptr<engine::Sprite> content_;    ///< Original: _SafeStr_674.
        std::shared_ptr<GameBattle> battle_;         ///< Original: _SafeStr_3596.
        std::shared_ptr<HelpScreen> help_;           ///< Original: _SafeStr_3599.
        std::shared_ptr<BattlePauseScreen> pause_;   ///< Original: _SafeStr_3597.
        std::shared_ptr<MissionResult> result_;      ///< Original: _SafeStr_3601.
        double endDelay_ = 0;                        ///< Original: _SafeStr_3598.
        int enemiesDestroyed_ = 0;                   ///< Original: _SafeStr_3603.
        int strongholdsCaptured_ = 0;                ///< Original: _SafeStr_3604.
    };

} // namespace takeover
