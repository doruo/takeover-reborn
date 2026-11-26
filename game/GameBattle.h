// GameBattle.h - a battle in progress (original: GameBattle).
//
// Owns the Level and the HUD (GamePanel, WindowController). Every frame it
// hands the input to the processes in order - panel, building windows,
// cursor, selection, spells, orders - where the first that uses it locks
// it for the others; Space or Escape reset them all. It also scripts the
// two tutorial missions, announcing their steps to the HelpScreen.
#pragma once

#include "game/framework/core/Signal.h"
#include "game/framework/states/StateManager.h"
#include "game/hud/GamePanel.h"
#include "game/level/LevelVisual.h"

#include <memory>
#include <string>
#include <vector>

namespace takeover {

    class FKeyNodeContent;
    class Level;
    class WindowController;
    struct WindowInfo;

    class GameBattle : public GameState, public IResourceSource {
    public:
        // Tutorial steps (dispatched by signalTutorialEvent).
        static constexpr const char* EVENT_LVL1_CASTLE_CLICKED = "evnt1_castleClicked";
        static constexpr const char* EVENT_LVL1_CASTLE_MENU_CLOSES = "evnt1_castleMenuClosed";
        static constexpr const char* EVENT_LVL1_GUARDS_PURCHASING_STARTED = "evnt1_guardsPurchasingStarted";
        static constexpr const char* EVENT_LVL1_GUARDS_PURCHASING_FINISHED = "evnt1_guardsPurchasingFinished";
        static constexpr const char* EVENT_LVL1_ENEMY_SQUAD_DESTROYED = "evnt1_enemySquadDestroyed";
        static constexpr const char* EVENT_LVL1_TOWN_SIEZED = "evnt1_townSiezed";
        static constexpr const char* EVENT_LVL2_FARM_CONSTRUCTION_STARTED = "evnt2_farmConstructionStarted";
        static constexpr const char* EVENT_LVL2_FARM_CONSTRUCTION_FINISHED = "evnt2_farmConstructionFinished";
        static constexpr const char* EVENT_LVL2_SQUAD_PURCHASING_STARTED = "evnt2_squadPurchasingStarted";
        static constexpr const char* EVENT_LVL2_FIGHT_WITH_OGRE_STARTED = "evnt2_fightWithOgreStarted";
        static constexpr const char* EVENT_LVL2_FIGHT_WITH_OGRE_FINISHED = "evnt2_fightWithOgreFinished";
        static constexpr const char* EVENT_LVL2_BANNER_SET = "evnt2_bannerSet";
        static constexpr const char* EVENT_LVL2_MAGIC_TOWER_SIEZED = "evnt2_magicTowerSiezed";
        static constexpr const char* EVENT_LVL2_ULTIMATE_CAST = "evnt2_ultimateCast";
        static constexpr const char* EVENT_LVL2_CASTLE_SIEZED = "evnt2_castleSiezed";

        explicit GameBattle(const BattleDef& def);
        ~GameBattle() override;

        Signal<std::shared_ptr<MissionResult>> signalMissionOver; ///< Original: _SafeStr_769.
        Signal<> signalMenu;                                      ///< Original: _SafeStr_3354.
        Signal<> signalHelp;                                      ///< Original: _SafeStr_3357.
        Signal<std::string> signalTutorialEvent;                  ///< Original: _SafeStr_3345.

        [[nodiscard]] const std::shared_ptr<engine::Sprite>& image() const { return image_; }
        [[nodiscard]] std::shared_ptr<MissionResult> missionStats() const;
        [[nodiscard]] Signal<Level*>& signalStatsChanged(); ///< _SafeStr_3213
        void update(double timeElapsed) override;

        // IResourceSource
        [[nodiscard]] int getGold() const override;
        [[nodiscard]] int getGoldSpeed() const override;
        [[nodiscard]] int getArmyPower() const override;
        [[nodiscard]] int getSupplyPower() const override;
        [[nodiscard]] int getGoldForSupply() const override;
        [[nodiscard]] int getMana() const override;
        [[nodiscard]] int getManaSpeed() const override;
        [[nodiscard]] double getUltimateFactor() const override;

    protected:
        void customDispose() override;

    private:
        void initVisualBlock();
        void disposeVisualBlock();
        void initProcesses();   ///< _SafeStr_3377
        void processInput(double dt); ///< _SafeStr_3382
        [[nodiscard]] bool isMissionOver() const; ///< _SafeStr_3380
        void initHud();         ///< _SafeStr_3374
        void disposeHud();      ///< _SafeStr_3367
        void updateHud(double dt); ///< _SafeStr_3369
        void onMenuButton(int id);     ///< _SafeStr_3362
        void onSpellButton(int id);    ///< _SafeStr_3366
        void onUltimateButton(int id); ///< _SafeStr_3381
        void onSpellModeOff();         ///< _SafeStr_3372
        void onNodeSelected(NodeContent* node); ///< _SafeStr_3373

        // Tutorials.
        void startTutorials();  ///< _SafeStr_3375
        void stopTutorials();   ///< _SafeStr_3370
        void enableSpells(bool enable); ///< _SafeStr_3356
        [[nodiscard]] bool isOwnCastle(NodeContent* node) const; ///< _SafeStr_3359
        void onWindowOpened_Tutorial1(WindowInfo* info); ///< _SafeStr_3360
        void onWindowClosed_Tutorial1(WindowInfo* info); ///< _SafeStr_3361
        void onBuySquad_Tutorial1(FSquad* squad);
        void onKillSquad_Tutorial1(Squad* squad);
        void onKeyNodeCaptured_Tutorial1(KeyNode* node);
        void onSupplyBuilt_Tutorial2(Combatant* owner, SupplyBuildingType type); ///< _SafeStr_3355
        void onBuySquad_Tutorial2(FSquad* squad);
        void onUnitAttack_Tutorial2(FUnit* unit); ///< _SafeStr_3351
        void onKillSquad_Tutorial2(Squad* squad);
        void onHealBanner_Tutorial2();            ///< _SafeStr_3350
        void onKeyNodeCaptured_Tutorial2(KeyNode* node);
        void onTowerChecked_Tutorial2(KeyNode* node); ///< _SafeStr_3358
        void onUltimateCast_Tutorial2(const SpellInfo* spell); ///< _SafeStr_3353

        BattleDef def_;                       ///< Original: _SafeStr_839.
        std::shared_ptr<engine::Sprite> image_; ///< Original: _SafeStr_37.
        std::shared_ptr<Level> level_;        ///< Original: _SafeStr_1502.
        std::vector<IInputProcess*> processes_; ///< Original: _SafeStr_3347.
        int speed_ = 1;                       ///< Time factor (original: _SafeStr_3352).
        std::shared_ptr<engine::Sprite> hud_; ///< Original: _SafeStr_3349.
        std::shared_ptr<GamePanel> panel_;    ///< Original: _SafeStr_3346.
        std::shared_ptr<WindowController> windows_; ///< Original: _SafeStr_3348.
        Squad* tutor1EnemySquad_ = nullptr;   ///< Original: ref_tutor1EnemySquad.
        KeyNode* tutor1EnemyTown_ = nullptr;  ///< Original: ref_tutor1EnemyTown.
        std::vector<std::shared_ptr<Spell>> savedSpells_; ///< Original: _SafeStr_2477.
    };

} // namespace takeover
