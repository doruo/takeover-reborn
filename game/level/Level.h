// Level.h - the battle with the player's hands on it (original: level/Level).
//
// Adds to LevelVisual what the player does: hovering and selecting
// buildings, dragging orders from a squad icon to a destination (with the
// path drawn on the ground), drawing a selection frame around several
// squads, aiming and casting spells, and the four ultimate powers. It also
// carries out every spell, whoever casts it.
//
// Input arrives through three FunctionProcesses the battle chains every
// frame: cursor (what is under the mouse), selection (click on a building),
// spells, and orders.
#pragma once

#include "game/framework/core/ObjectUpdater.h"
#include "game/framework/input/InputProcess.h"
#include "game/framework/raster/ResourceManager.h"
#include "game/level/LevelVisual.h"

#include <memory>
#include <optional>
#include <vector>

namespace takeover {

class SpellInfo;
class USpellButton;

class Level : public LevelVisual {
public:
    /// True while the cursor is over a HUD button, so clicks don't reach
    /// the battlefield (original: isCursorUnderButton).
    static inline bool isCursorUnderButton = false;

    /// The spell of a race by its position in the spell bar (original: _SafeStr_3225).
    static const SpellInfo* spellInfo(int spellNum, Race race);
    /// Where the HUD tooltips go (original: _SafeStr_3041).
    static engine::Rectangle hintRect() { return {480, 560, 310, 30}; }

    Level() = default;
    Level& init(const BattleDef& def); ///< _SafeStr_3283

    Signal<NodeContent*> signalNodeSelected; ///< _SafeStr_3214
    Signal<> signalSpellModeOff;             ///< _SafeStr_3216
    Signal<Level*> signalStatsChanged;       ///< _SafeStr_3213

    // The input processes, for the battle's chain.
    [[nodiscard]] IInputProcess* cursorProcess() const { return cursorProcess_.get(); }       ///< _SafeStr_3295
    [[nodiscard]] IInputProcess* selectionProcess() const { return selectionProcess_.get(); } ///< _SafeStr_3299
    [[nodiscard]] IInputProcess* spellProcess() const { return spellProcess_.get(); }         ///< _SafeStr_3297
    [[nodiscard]] IInputProcess* orderProcess() const { return orderProcess_.get(); }         ///< _SafeStr_1241

    /// Enters aiming mode with the player's spell of a spell bar button (original: _SafeStr_3298).
    void selectSpellButton(int buttonId);
    [[nodiscard]] bool hasSpell(int buttonId); ///< _SafeStr_3030
    void startAiming(std::shared_ptr<Spell> spell); ///< _SafeStr_3245
    void stopAiming();                              ///< _SafeStr_3193
    [[nodiscard]] bool canCast() const { return spellCursorActive_ && hasCursorIcon(); } ///< _SafeStr_3292
    [[nodiscard]] bool isDraggingOrder() const { return draggingOrder_; } ///< _SafeStr_1394
    /// Selecting with a frame on empty ground (original: _SafeStr_1400).
    [[nodiscard]] bool isFraming() const { return framing_ && objectUnderMouse_ == nullptr; }
    void setUltimateIcon(USpellButton& button); ///< _SafeStr_1393
    [[nodiscard]] const SpellInfo* playerSpell(int buttonId); ///< _SafeStr_3021
    void cancelSpell() { stopAiming(); } ///< _SafeStr_3293

    /// One strike of the stone worms (original: _SafeStr_1968).
    void spellStrike(const Spell& spell, Combatant& combatant);

    void update(double dt) override;
    void castSpell(const Spell* spell, Combatant* combatant) override;
    void castUltimateSpell(Combatant& combatant) override;
    void killSquad(Squad& squad) override;

protected:
    void customDispose() override;
    void createSplashDamadgeEffectFrom(const SplashArea& area, const ArmyStat* stat) override;
    void invalidateStatistics() override;
    void onSquadIconEvent(const SquadKey* squad, int type) override;

private:
    void playSpellSound(const SpellInfo& spell); ///< _SafeStr_1928
    /// An explosion with its crater (original: _SafeStr_3227).
    void explosion(const std::string& key, int x, int y, int repeatCount = 1);
    void onUltimateMessageDone(Disposable* animation); ///< _SafeStr_3222
    void releaseUltimate(Combatant& combatant);         ///< _SafeStr_3278
    const SpellInfo* ultimateEmpire(Combatant& combatant);   ///< _SafeStr_3270: enemies join the Empire
    const SpellInfo* ultimateCult(Combatant& combatant);     ///< _SafeStr_3241: half the enemies die
    const SpellInfo* ultimateWestaria(Combatant& combatant); ///< _SafeStr_3285: the avatar
    const SpellInfo* ultimateKhaganate(Combatant& combatant); ///< _SafeStr_3257: cataclysm
    void onFireballLanded(Disposable* effect);          ///< _SafeStr_3288
    void attachLight(FUnit& unit, const std::string& key, bool above = true); ///< _SafeStr_3232
    std::shared_ptr<FSquad> spawnSquad(const Point2D& position, Combatant& combatant, const ArmyStat& stat); ///< _SafeStr_3186
    // Spells.
    void raiseDead(const Spell& spell, Combatant& combatant);      ///< _SafeStr_3264
    void stoneWorms(const Spell& spell, Combatant& combatant);     ///< _SafeStr_3246
    void disentomb(const Spell& spell, Combatant& combatant);      ///< _SafeStr_3235
    void bannerSpell(const Spell& spell, Combatant& combatant, const std::string& auraKey, const std::string& bannerKey,
                     int bannerKind); ///< _SafeStr_3253, 3273, 3269, 3276, 3277
    AnimEffectDef animDef(const std::string& key, double spellTime, int x, int y); ///< _SafeStr_3187
    void groundAura(const std::string& key, double spellTime, int x, int y);   ///< _SafeStr_3191
    void onAuraRemoved(Disposable*);                                           ///< _SafeStr_3242
    void lordsJudgement(const Spell& spell);                                   ///< _SafeStr_3256
    void frostEnchantment(const Spell& spell, Combatant& combatant);           ///< _SafeStr_3267
    void iceBlast(const Spell& spell);                                         ///< _SafeStr_3274
    void onIceBlastLanded(Disposable* effect);                                 ///< _SafeStr_3252
    void terror(const Spell& spell, Combatant& combatant);                     ///< _SafeStr_3266

    void initProcesses();    ///< _SafeStr_3240
    void disposeProcesses(); ///< _SafeStr_3250
    // Cursor.
    void initCursor();       ///< _SafeStr_3243
    void disposeCursor();    ///< _SafeStr_3239
    bool processCursor(double dt, bool locked); ///< _SafeStr_3258
    [[nodiscard]] bool cursorOnFreeGround() const { return cursorHitColor_ == 0xFFFFFF; } ///< _SafeStr_3296
    [[nodiscard]] bool cursorOnForbidden() const { return cursorHitColor_ == 0x0000FF; }  ///< _SafeStr_3236
    [[nodiscard]] SpriteAnimation& cursorIcon() const { return *cursorIcons_[static_cast<std::size_t>(cursorIconId_)]; } ///< _SafeStr_3185
    [[nodiscard]] bool hasCursorIcon() const { return cursorIconId_ >= 0; } ///< _SafeStr_3217
    void showCursorIcon(int id);                  ///< _SafeStr_3176
    int addCursorIcon(const std::string& key);    ///< _SafeStr_3190
    void moveCursorIconToMouse();                 ///< _SafeStr_3230
    void moveCursorIcon(double x, double y);      ///< _SafeStr_3234
    // Selection.
    void initSelection();    ///< _SafeStr_3282
    void disposeSelection(); ///< _SafeStr_3281
    bool processSelection(double dt, bool locked); ///< _SafeStr_3259
    void resetSelection();   ///< _SafeStr_3237
    // Spells.
    void initSpells();       ///< _SafeStr_3268
    void disposeSpells();    ///< _SafeStr_3287
    bool processSpells(double dt, bool locked); ///< _SafeStr_3265
    void aimAtMouse();       ///< _SafeStr_3219
    void setSpellCursor(bool enable); ///< _SafeStr_3244
    void showSpellCursor(const Spell& spell, bool active = true); ///< _SafeStr_3223
    // Orders.
    void initOrders();       ///< _SafeStr_3249
    void disposeOrders();    ///< _SafeStr_3271
    bool processOrders(double dt, bool locked); ///< _SafeStr_3279
    void resetOrders();      ///< _SafeStr_3289
    void startFrame();       ///< _SafeStr_3280
    void stopFrame();        ///< _SafeStr_3211
    void updateFrame();      ///< _SafeStr_3291
    void selectInFrame(const engine::Rectangle& rect); ///< _SafeStr_3263
    void unselectAll();      ///< _SafeStr_3228
    /// Sends the squad and the selected ones, staggered (original: _SafeStr_3255).
    void sendSelected(const Point2D& position, FSquad& squad);
    void drawOrderArrows(engine::Graphics& graphics, FSquad& squad, const Point2D& aim); ///< _SafeStr_3251
    void clearIconEvent();   ///< _SafeStr_3215
    [[nodiscard]] bool hasIconEvent() const { return iconEventSquad_ != nullptr; } ///< _SafeStr_3261
    void handleIconEvent(FSquad& squad); ///< _SafeStr_3284
    void highlightIcon(bool enable, FSquad* squad = nullptr); ///< _SafeStr_3198
    void startDrag(FSquad* squad); ///< _SafeStr_3262
    void stopDrag();         ///< _SafeStr_3210
    [[nodiscard]] bool isDragging() const { return draggedSquad_ != nullptr; } ///< _SafeStr_3231
    [[nodiscard]] FSquad* draggedSquad(); ///< _SafeStr_3233
    void drawArrow(engine::Graphics& graphics, const Point2D& from, const Point2D& to); ///< _SafeStr_3229
    bool sendSquad(FSquad* squad, const Point2D& target); ///< _SafeStr_3208
    void initPathPoints();   ///< _SafeStr_3238
    void addPathPoint(const Point2D& point); ///< _SafeStr_3218
    void hidePathPoints();   ///< _SafeStr_3205
    void disposePathPoints(); ///< _SafeStr_3286
    /// Path from a squad to a point, drawn as dots (original: _SafeStr_3275).
    std::optional<std::vector<Point2D>> showPath(FSquad& squad, const Point2D& target);
    void showChestHint(ChestView& chest); ///< _SafeStr_3290
    void hideChestHint();                 ///< _SafeStr_3220
    void showSquadHint(FSquad& squad);    ///< _SafeStr_3254
    void hideSquadHint();                 ///< _SafeStr_3212

    std::shared_ptr<ObjectUpdater> updater_; ///< Original: _SafeStr_1871.
    // Cursor.
    std::shared_ptr<FunctionProcess> cursorProcess_;  ///< Original: _SafeStr_3197.
    std::uint32_t cursorHitColor_ = 0;                ///< Original: _SafeStr_3202.
    int cursorIconId_ = -1;                           ///< Original: _SafeStr_3177.
    std::vector<std::shared_ptr<SpriteAnimation>> cursorIcons_; ///< Original: _SafeStr_3174.
    // Selection.
    std::shared_ptr<FunctionProcess> selectionProcess_; ///< Original: _SafeStr_3196.
    ILayerObject* selectedObject_ = nullptr;   ///< Original: _SafeStr_3172.
    ILayerObject* objectUnderMouse_ = nullptr; ///< Original: _SafeStr_3170.
    // Spells.
    std::shared_ptr<FunctionProcess> spellProcess_; ///< Original: _SafeStr_3194.
    bool aiming_ = false;                      ///< Original: _SafeStr_3183.
    std::shared_ptr<Spell> currentSpell_;      ///< Original: _SafeStr_3182.
    bool spellCursorActive_ = false;           ///< Original: _SafeStr_3199.
    // Orders.
    std::shared_ptr<FunctionProcess> orderProcess_; ///< Original: _SafeStr_3192.
    bool ordersEnabled_ = true;                ///< Original: _SafeStr_3195.
    const SquadKey* iconEventSquad_ = nullptr; ///< Original: _SafeStr_3207.
    int iconEventType_ = -1;                   ///< Original: _SafeStr_3209.
    engine::DisplayObject* highlightedIcon_ = nullptr; ///< Original: _SafeStr_3180.
    std::vector<swf::Filter> savedIconFilters_; ///< Original: _SafeStr_3206.
    const SquadKey* draggedSquad_ = nullptr;   ///< Original: _SafeStr_3181.
    int iconGoPath_ = -1;    ///< Original: _SafeStr_3201.
    int iconGoPathOff_ = -1; ///< Original: _SafeStr_3189.
    int iconGoIn_ = -1;      ///< Original: _SafeStr_3221.
    int iconAttack_ = -1;    ///< Original: _SafeStr_3224.
    Point2D aimPosition_;    ///< Original: _SafeStr_3175.
    Point2D targetPosition_; ///< Original: _SafeStr_3184.
    std::vector<swf::Filter> selectionFilters_; ///< Original: _SafeStr_3204.
    std::vector<swf::Filter> overFilters_;      ///< Original: _SafeStr_3226.
    bool draggingOrder_ = false; ///< Original: _SafeStr_3203.
    bool framing_ = false;       ///< Original: _SafeStr_3188.
    engine::Rectangle frame_;    ///< Original: _SafeStr_3171.
    std::vector<std::shared_ptr<RasterSprite>> pathPoints_; ///< Original: _SafeStr_3173.
    int pathPointCount_ = 0;     ///< Original: _SafeStr_3178.
    bool chestHintShown_ = false; ///< Original: _SafeStr_3200.
    FSquad* hintSquad_ = nullptr; ///< Original: _SafeStr_3179.
};

} // namespace takeover
