// LevelVisual.h - the battle as seen on screen (original: level/LevelVisual).
//
// On top of the rules (LevelLogic) and the computer players (LevelAi), it
// builds the display: the ground, the battlefield raster layer, the squad
// icons, the HUD layers and the tooltips. It creates the visible squads
// (FSquad) and the timed effects, and announces the battle's events to the
// HUD and the tutorials through signals.
//
// Coordinates: the simulation works in "land" coordinates where y is twice
// the screen y (Y_KOEFF): the map is seen from an angle.
#pragma once

#include "game/framework/core/Signal.h"
#include "game/framework/raster/RasterLayer.h"
#include "game/level/Effects.h"
#include "game/level/LevelLand.h"
#include "game/level/NodeViews.h"
#include "game/sim/base/Flags.h"
#include "game/sim/level/LevelAi.h"
#include "game/squads/FSquad.h"

#include <memory>
#include <string>
#include <vector>

namespace engine {
    class DisplayObject;
    class Sprite;
}

namespace takeover {

    class HintLayer;
    class KeyboardDevice;
    class MouseDevice;
    class TOResManager;

    /// What a battle is made from (original: _SafeCls_2148 and _SafeStr_3129).
    struct BattleDef {
        TOResManager* resources = nullptr;    ///< Original: _SafeStr_755.
        MouseDevice* mouse = nullptr;         ///< Original: _SafeStr_2149.
        KeyboardDevice* keyboard = nullptr;   ///< Original: _SafeStr_2150.
        std::shared_ptr<Combatant> player;
        std::vector<std::shared_ptr<Combatant>> enemyPlayers;
        int levelNum = 0;
        int levelUID = 99;                    ///< Which map ("Level<UID>Class").
        HintLayer* hintLayer = nullptr;       ///< Original: _SafeStr_2151.
    };

    class LevelVisual : public LevelAi {
    public:
        // Debug modes (original: LevelVisual.mode).
        static constexpr int ST_CHEAT_MODE = 2;
        static constexpr int ST_LOCK_AI = 4;
        static constexpr int ST_SHOW_ICON_TEXT = 8;
        static constexpr int ST_SHOW_UNIT_HEALTH = 16;
        static constexpr int ST_SHOW_HIT_LAYER = 32;
        static inline Flags mode{0};

        /// The battlefield is stored twice as tall as it is shown.
        static constexpr double Y_KOEFF = 2;

        [[nodiscard]] static bool isModeOn(int bits) { return mode.test(bits); } ///< _SafeStr_2780
        /// Land position of a marker clip (original: getPosition).
        static Point2D getPosition(const engine::DisplayObject& marker);

        /// Points every `distance` pixels along a polyline (original: _SafeStr_3110).
        static std::vector<Point2D> resamplePath(const std::vector<Point2D>& points, double distance);
        static Point2D convertToLandPoint(const Point2D& position);
        static Point2D convertToScreenPoint(const Point2D& position);

        /// Frame of the race in the HUD icons (original: _SafeStr_3085).
        static int raceIconIndex(Race race);

        /// Race number used in spell ids and assets (original: getRaceID).
        static int getRaceID(Race race);

        LevelVisual() = default;
        LevelVisual& initVisual(const BattleDef& def); ///< _SafeStr_3106

        // /----- SIGNALS -----/

        Signal<FSquad*> signalSquadBought;                       ///< _SafeStr_3074
        Signal<Squad*> signalSquadKilled;                        ///< _SafeStr_3077
        Signal<FUnit*> signalUnitAttack;                         ///< _SafeStr_1307
        Signal<KeyNode*> signalNodeCaptured;                     ///< _SafeStr_3076
        Signal<Combatant*, SupplyBuildingType> signalSupplyBuilt; ///< _SafeStr_3072
        Signal<const SpellInfo*> signalUltimateCast;             ///< _SafeStr_3089
        Signal<> signalHealBanner;                               ///< _SafeStr_3088
        Signal<KeyNode*> signalNodeChecked;                      ///< _SafeStr_3075

        // /----- DISPLAY -----/

        [[nodiscard]] const std::shared_ptr<engine::Sprite>& view() const { return view_; }
        [[nodiscard]] engine::Graphics& graphics() { return debugSprite_->graphics(); }
        [[nodiscard]] LevelLand& land() { return *land_; }                       ///< _SafeStr_3116
        [[nodiscard]] RasterGraphicLayer& rasterLayer() { return *rasterLayer_; } ///< _SafeStr_1254
        [[nodiscard]] const std::shared_ptr<engine::Sprite>& iconLayer() const { return iconLayer_; }     ///< _SafeStr_3102
        [[nodiscard]] const std::shared_ptr<engine::Sprite>& cursorLayer() const { return cursorLayer_; } ///< _SafeStr_3121
        [[nodiscard]] const std::shared_ptr<engine::Sprite>& frontLayer() const { return frontLayer_; }   ///< _SafeStr_3108
        [[nodiscard]] const std::shared_ptr<engine::Sprite>& topLayer() const { return topLayer_; }       ///< _SafeStr_3114
        [[nodiscard]] HintLayer& hintLayer() { return *hintLayer_; }             ///< _SafeStr_751
        // Tooltip windows of the hint layer.
        [[nodiscard]] int hintBottomId() const { return hintBottom_; }     ///< _SafeStr_1383
        [[nodiscard]] int hintOverPanelId() const { return hintOverPanel_; } ///< _SafeStr_3045
        [[nodiscard]] int hintUnitId() const { return hintUnit_; }         ///< _SafeStr_1381
        [[nodiscard]] int hintSpellId() const { return hintSpell_; }       ///< _SafeStr_1399
        [[nodiscard]] int hintUnit2Id() const { return hintUnit2_; }       ///< _SafeStr_3123
        /// Puts the ground, the battlefield and the icons in the view (original: _SafeStr_3113).
        void assembleView();

        /// Changes the color of a location (original: _SafeStr_3105).
        void setLocationView(int location, int locationType);

        /// Is the hit mask pixel at (x, y) of that color? White = free ground,
        /// red = under a building (original: _SafeStr_1298).
        [[nodiscard]] bool isHitColor(int x, int y, std::uint32_t color = 0xFFFFFF) const;
        [[nodiscard]] std::uint32_t hitColorAt(int x, int y) const; ///< _SafeStr_3112

        /// Draws roads and crossings, for debugging (original: _SafeStr_3115).
        void drawGraph();

        // /----- PLAYERS -----/

        [[nodiscard]] Combatant& player() { return *def_.player; }
        [[nodiscard]] int playerRaceIndex() const { return playerRaceIndex_; } ///< _SafeStr_3039
        /// Something changed that the HUD should show (original: _SafeStr_1392).
        [[nodiscard]] bool isStatsDirty() const { return statsDirty_; }
        void setStatsDirty(bool dirty) { statsDirty_ = dirty; }
        /// Can the player give orders to this race? (original: _SafeStr_3117).
        [[nodiscard]] bool isPlayerRace(Race race) const;
        [[nodiscard]] bool isPlayer(const CombatantKey* key) const; ///< _SafeStr_1367

        /// The mouse in land coordinates (original: _SafeStr_3111).
        [[nodiscard]] Point2D mouseLandPosition() const;
        void setPlayer(const std::shared_ptr<CombatantKey>& player); ///< _SafeStr_3092

        // /----- SQUADS AND EFFECTS -----/

        /// Buys a squad for a combatant (original: _SafeStr_3091).
        std::shared_ptr<FSquad> buySquadFor(const ArmyStat& stat, Combatant& combatant);

        /// Creates a squad for free (original: _SafeStr_3109).
        std::shared_ptr<FSquad> createSquadFor(const std::shared_ptr<CombatantKey>& owner, const ArmyStat& stat,
                                               const ArmyStatBonus& bonus);
        std::shared_ptr<Squad> buyUnit(const ArmyStat* stat, KeyNode& node) override;

        /// Shows a timed effect (original: _SafeStr_3101).
        void addEffect(const std::shared_ptr<TimedEffect>& effect, int layer);

        /// Plays an animation once at a place (original: _SafeStr_3096).
        void playEffect(const std::string& key, double x, double y, int layer, int repeatCounter = -1, double delay = 0);

        /// Renders the animations a battle needs, beforehand (original: _SafeStr_3122).
        void precache(int levelNum);

        // /----- OVERRIDES -----/

        void update(double dt) override;
        void captureNode(const std::shared_ptr<CombatantKey>& newOwner, KeyNode& node) override;
        void buildSupplyBuilding(NodeContent& place, SupplyBuildingType type) override;
        void killSquad(Squad& squad) override;
        bool updateBaseCombotantLogic(double dt) override;
        bool updateCpuCombotantLogic(double dt) override;
        bool tryBuySupply(NodeContent& place, Combatant& combatant, SupplyBuildingType type) override;
        bool checkEmptyPoinPos(const Point2D& point) override { return isHitColor(static_cast<int>(point.x), static_cast<int>(point.y)); }
        SupplyBuildingType getBuildPlaceType(NodeContent& place) override;

    protected:
        void customDispose() override;
        void resetBuildPlace(NodeContent& place) override;
        bool checkNodeActivState(KeyNode& node) override;
        void initVisualBlock(HintLayer* hintLayer);
        void disposeVisualBlock();
        void renderLayer(double dt);         ///< _SafeStr_3098
        virtual void afterRender(double) {}  ///< _SafeStr_3094
        virtual void afterInit() {}          ///< _SafeStr_3100
        virtual void onSquadIconEvent(const SquadKey* /*squad*/, int /*type*/) {}
        virtual void onSquadAtPosition(FSquad*) {} ///< _SafeStr_3103
        void onSquadLeftNode(FSquad* squad);       ///< _SafeStr_3107
        void listenSquad(FSquad& squad);           ///< _SafeStr_3097
        void disposeSignals();                     ///< _SafeStr_3120
        void addPlayers();                         ///< _SafeStr_3095
        std::shared_ptr<FSquad> makeSquad(const std::shared_ptr<CombatantKey>& owner, const ArmyStat& stat,
                                          const ArmyStatBonus& bonus); ///< _SafeStr_3071
        void showRizeEffect(const Point2D& position); ///< _SafeStr_3081
        [[nodiscard]] GraphNode* graphNodeOf(const NodeKey* node) { return node ? graphNodes_.getObject(node) : nullptr; } ///< _SafeStr_3119
        static std::vector<std::string> spellEffectKeys(int spellId); ///< _SafeStr_3104
        void drawWay(const std::vector<Point2D>& wayPoints); ///< _SafeStr_3082

        BattleDef def_;                               ///< Original: _SafeStr_839.
        std::shared_ptr<engine::Sprite> view_;        ///< Original: _SafeStr_37.
        std::shared_ptr<LevelLand> land_;             ///< Original: _SafeStr_3064.
        std::shared_ptr<RasterGraphicLayer> rasterLayer_; ///< Original: _SafeStr_3067.
        std::shared_ptr<engine::Sprite> iconLayer_;   ///< Original: _SafeStr_3065.
        std::shared_ptr<engine::Sprite> cursorLayer_; ///< Original: _SafeStr_3086.
        std::shared_ptr<engine::Sprite> debugSprite_; ///< Original: _SafeStr_3063.
        std::shared_ptr<engine::Sprite> frontLayer_;  ///< Original: _SafeStr_3062.
        std::shared_ptr<engine::Sprite> topLayer_;    ///< Original: _SafeStr_3066.
        std::shared_ptr<engine::BitmapData> hitMask_; ///< Original: _SafeStr_3069.
        HintLayer* hintLayer_ = nullptr;              ///< Original: _SafeStr_3073.
        int hintBottom_ = 0, hintOverPanel_ = 0, hintUnit_ = 0, hintSpell_ = 0, hintUnit2_ = 0;
        int playerRaceIndex_ = 0;                     ///< Original: _SafeStr_3090.
        bool statsDirty_ = false;                     ///< Original: _SafeStr_3068.
    };

} // namespace takeover
