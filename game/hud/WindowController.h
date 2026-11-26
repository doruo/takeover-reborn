// WindowController.h - opens the window of the building the player clicks
// (original: WindowController and hud/_SafeCls_824).
//
// Own strongholds get a recruiting window (castle, town, camp, magic tower),
// own build places a building or destroying window, enemy places an
// information window. The controller turns the window's buttons into
// actions (buy a squad, upgrade, build...) and fills their tooltips.
#pragma once

#include "game/framework/core/Signal.h"
#include "game/framework/input/InputProcess.h"
#include "game/hud/WindowBase.h"

#include <memory>
#include <string>

namespace engine {
    class MouseEvent;
    class SoundChannel;
    class Sprite;
}

namespace takeover {

    class ArmyStat;
    class CombatantKey;
    class KeyNode;
    class NodeKey;
    class BuildPlace;
    class FKeyNodeContent;
    class FsmState;
    class HintWindow2;
    class Level;
    class NodeContent;
    struct ArmyStatBonus;
    enum class SupplyBuildingType;

    /// An open window and what it is about (original: hud/_SafeCls_824).
    struct WindowInfo {
        explicit WindowInfo(NodeContent* content) : node(content) { stateChanged(); }
        void dispose();

        /// Did the stronghold change state since last time? (original: _SafeStr_829).
        bool stateChanged();

        std::shared_ptr<WindowBase> window;     ///< Original: _SafeStr_827.
        NodeContent* node;                      ///< Original: _SafeStr_826.
        std::shared_ptr<CombatantKey> owner;    ///< Original: _SafeStr_831.
        int openSound = -1;                     ///< Original: _SafeStr_830.
        int level = 0;

    private:
        FsmState* lastState_ = nullptr;         ///< Original: _SafeStr_825.
    };

    class WindowController : public UpdatableObject, public IInputProcess {
    public:
        /// Colors a text by squad level: blue for veterans, yellow for elites
        /// (original: _SafeStr_1261).
        static std::string levelColored(int squadLevel, const std::string& text) {
            if (squadLevel == 1)
                return "<FONT COLOR='#00CBFF'>" + text + "</FONT>";

            if (squadLevel == 2)
                return "<FONT COLOR='#FFFF00'>" + text + "</FONT>";

            return text;
        }

        /// "Regular", "Veteran" or "Elite" (original: _SafeStr_1309).
        static std::string levelName(int squadLevel) {
            if (squadLevel == 1)
                return "Veteran";

            if (squadLevel >= 2)
                return "Elite";

            return "Regular";
        }

        /// What the rank gives (original: _SafeStr_1361).
        static std::string levelBonusText(int squadLevel);

        /// Fills a unit hint window: name, HP, damage, abilities... (original: _SafeStr_1362).
        static void fillUnitHint(const ArmyStat& stat, const ArmyStatBonus& bonus, HintWindow2& hint, int squadLevel = 0);

        explicit WindowController(Level& level);

        Signal<WindowInfo*> signalWindowOpened; ///< Original: _SafeStr_1347.
        Signal<WindowInfo*> signalWindowClosed; ///< Original: _SafeStr_1349.

        [[nodiscard]] const std::shared_ptr<engine::Sprite>& image() const { return image_; }
        void update(double timeElapsed) override;

        /// Opens (or closes) the window of a building (original: _SafeStr_1365).
        void openWindow(NodeContent* node, Combatant* player);
        void closeWindow(); ///< _SafeStr_1326

        // IInputProcess
        [[nodiscard]] bool isEnabled() const override { return enabled_; }
        void setEnabled(bool enabled) override { enabled_ = enabled; }
        bool process(double timeElapsed, bool locked) override;
        void reset() override { closeWindow(); }

    protected:
        void customDispose() override;

    private:
        void stopSound(); ///< _SafeStr_1345
        void listenWindow(WindowBase* window, bool enable); ///< _SafeStr_1356
        void onWindowRollOut(engine::MouseEvent&);  ///< _SafeStr_1357
        void onWindowRollOver(engine::MouseEvent&); ///< _SafeStr_1353
        void show(std::unique_ptr<WindowInfo> info); ///< _SafeStr_1374
        [[nodiscard]] int frameColorOf(const NodeKey& node); ///< _SafeStr_1332
        static bool statsOnRight(double x) { return x < 700; } ///< _SafeStr_1331
        std::unique_ptr<WindowInfo> keyNodeWindow(FKeyNodeContent& node, Combatant& owner); ///< _SafeStr_1366
        std::unique_ptr<WindowInfo> buildPlaceWindow(BuildPlace& place, Combatant& owner);  ///< _SafeStr_1378
        void build(SupplyBuildingType type); ///< _SafeStr_1343
        void buyUnit(int unitId);
        void upgrade(); ///< _SafeStr_1382
        void doAction(); ///< _SafeStr_1364
        std::shared_ptr<WindowBase> enemyWindow(NodeContent& node, const Point2D& position, const std::string& text); ///< _SafeStr_1358
        std::shared_ptr<WindowBase> castleWindow(KeyNode& node, const Point2D& position);  ///< _SafeStr_1376
        std::shared_ptr<WindowBase> townWindow(KeyNode& node, const Point2D& position);    ///< _SafeStr_1355
        std::shared_ptr<WindowBase> towerWindow(KeyNode& node, const Point2D& position);   ///< _SafeStr_1388
        std::shared_ptr<WindowBase> buildWindow(NodeContent& node, const Point2D& position); ///< _SafeStr_1375
        std::shared_ptr<WindowBase> destroyWindow(NodeContent& node, const Point2D& position); ///< _SafeStr_1391
        CenterPanelDef centerOf(NodeContent& node, bool active, int frame, const Point2D& position);
        void listenButtons(WindowBase& window); ///< _SafeStr_1336
        void onButtonOver(Button* button); ///< _SafeStr_1359
        void onButtonOut(Button* button);  ///< _SafeStr_1351
        void showHintNear(int hintId, engine::DisplayObject* buttonImage); ///< _SafeStr_1344
        static std::string upgradeText(int toLevel);                      ///< _SafeStr_1370
        static std::string buildingText(const Combatant& combatant, SupplyBuildingType type); ///< _SafeStr_1348
        void showUnitHint(Combatant& combatant, int unitId, int level);  ///< _SafeStr_1379

        Level* level_;                                 ///< Original: _SafeStr_859.
        std::shared_ptr<engine::Sprite> image_;        ///< Original: _SafeStr_37.
        std::unique_ptr<WindowInfo> current_;          ///< Original: _SafeStr_1321.
        std::shared_ptr<engine::SoundChannel> sound_;  ///< Original: _SafeStr_1340.
        bool mouseOverWindow_ = true;                  ///< Original: _SafeStr_1342.
        bool enabled_ = false;                         ///< Original: _SafeStr_1335.
        int pendingAction_ = -1;                       ///< Original: _SafeStr_1337.
    };

} // namespace takeover
