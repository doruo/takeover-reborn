#include "game/level/Level.h"

#include "game/framework/Symbols.h"
#include "game/framework/core/As3.h"
#include "game/framework/input/MouseDevice.h"
#include "game/framework/raster/ResourceManager.h"
#include "game/framework/ui/HintWindow.h"
#include "game/hud/GamePanel.h"
#include "game/media/SoundConst.h"
#include "game/resources/TOResManager.h"
#include "game/sim/abilities/Abilities.h"
#include "game/sim/abilities/AbilityEffects.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/sim/spells/SpellInfo.h"
#include "game/sim/units/Unit.h"

#include "engine/display/MovieClip.h"
#include "engine/display/Stage.h"
#include "engine/platform/Mouse.h"

#include <cmath>

namespace takeover {

    namespace {

        // Cursor icons, by spell id * 2 (+1 when the spell can't be cast there).
        const char* const ICON_LIST[3][8] = {
            {"CursorOn_LordsJudgementClass", "CursorOff_LordsJudgementClass", "CursorOn_HealClass", "CursorOff_HealClass",
            "CursorOn_HeroismClass", "CursorOff_HeroismClass", "", ""},
            {"CursorOn_StoneWormClass", "CursorOff_StoneWormClass", "CursorOn_DisentombClass", "CursorOff_DisentombClass",
            "CursorOn_BloodlustClass", "CursorOff_BloodlustClass", "", ""},
            {"CursorOn_IceBlastClass", "CursorOff_IceBlastClass", "CursorOn_MistyClonesClass", "CursorOff_MistyClonesClass",
            "CursorOn_LiberationClass", "CursorOff_LiberationClass", "", ""},
        };

        enum BannerKind { HEAL, BLOODLUST, LIBERATION, HEROISM };

    } // namespace

    const SpellInfo* Level::spellInfo(int spellNum, Race race) { 
        return Spell::byId(spellNum + getRaceID(race) * 4); 
    }

    Level& Level::init(const BattleDef& def) {

        auto result = std::make_shared<MissionResult>();
        result->levelNum = def.levelNum;

        LevelLogic::init(result);
        initVisual(def);
        initProcesses();
        
        updater_ = std::make_shared<ObjectUpdater>();
        return *this;
    }

    void Level::customDispose() {
        if (updater_) 
            updater_->dispose();
        
        updater_.reset();
        signalStatsChanged.dispose();
        
        disposeProcesses();
        disposeSignals();
        LevelVisual::customDispose();
    }

    void Level::update(double dt) {
        LevelVisual::update(dt);
        updater_->update(dt);
    }

    void Level::playSpellSound(const SpellInfo& spell) { SoundConst::playSpell(&spell); }

    void Level::createSplashDamadgeEffectFrom(const SplashArea& area, const ArmyStat* stat) {
        LevelVisual::createSplashDamadgeEffectFrom(area, stat);
        if (stat->unitType == UnitType::Siege)
            explosion("ExplosionClass", static_cast<int>(area.position.x), static_cast<int>(area.position.y));
    }

    void Level::explosion(const std::string& key, int x, int y, int repeatCount) {
        auto crater = std::make_shared<RasterSprite>(def_.resources->createRasterObject("CraterFromExplosionClass", x, y));
        addEffect(std::make_shared<TimedEffect>(crater, 7), RasterGraphicLayer::LAYER_GROUND);
        AnimEffectDef anim;
        anim.anim = std::make_shared<RasterAnimation>(def_.resources->getCachedData(key), def_.resources->animTransitions(key)[0],
                                                    x, y, 0, repeatCount);
        anim.time = anim.anim->transition().mscDuration / 1000 * repeatCount;
        addEffect(std::make_shared<AnimEffect>(anim), RasterGraphicLayer::LAYER_UNIT);
    }

    // --- Casting --------------------------------------------------------------------------------------------

    void Level::castSpell(const Spell* spell, Combatant* combatant) {
        LevelVisual::castSpell(spell, combatant);
        statsDirty_ = true;
        const int id = spell->id;
        if (id == SpellInfo::LORDS_JUDGEMENT.id) lordsJudgement(*spell);
        else if (id == SpellInfo::BANNER_OF_CONSECRATION.id) bannerSpell(*spell, *combatant, "BannerAuraHealClass", "BannerHealClass", HEAL);
        else if (id == SpellInfo::BANNER_OF_HEROISM.id) bannerSpell(*spell, *combatant, "BannerAuraHeroismClass", "BannerHeroismClass", HEROISM);
        else if (id == SpellInfo::SIGN_OF_THE_ADVENT.id) spawnSquad(spell->position, *combatant, ArmyStat::AVATAR);
        else if (id == SpellInfo::BANNER_OF_DESECRATION.id) bannerSpell(*spell, *combatant, "BannerAuraDesecrationClass", "BannerDesecrationClass", HEAL);
        else if (id == SpellInfo::RAISE_THE_DEAD.id) raiseDead(*spell, *combatant);
        else if (id == SpellInfo::TERROR.id) terror(*spell, *combatant);
        else if (id == SpellInfo::SUMMON_STONEWORMS.id) stoneWorms(*spell, *combatant);
        else if (id == SpellInfo::DISENTOMB.id) disentomb(*spell, *combatant);
        else if (id == SpellInfo::BANNER_OF_BLOODLUST.id) bannerSpell(*spell, *combatant, "BannerEffectBloodlustClass", "BannerBloodlustClass", BLOODLUST);
        else if (id == SpellInfo::ICE_BLAST.id) iceBlast(*spell);
        else if (id == SpellInfo::FROST_ENCHANTMENT.id) frostEnchantment(*spell, *combatant);
        else if (id == SpellInfo::BANNER_OF_LIBERATION.id) bannerSpell(*spell, *combatant, "BannerLiberationEffectClass", "BannerLiberationClass", LIBERATION);
    }

    void Level::castUltimateSpell(Combatant& combatant) {
        const char* messageClass = nullptr;
        switch (combatant.getRace()) {
        case Race::TheKhaganate: messageClass = "UltimateMessage2Class"; break;
        case Race::TheCult: messageClass = "UltimateMessage4Class"; break;
        case Race::TheEmpire: messageClass = "UltimateMessage3Class"; break;
        case Race::Westaria: messageClass = "UltimateMessage1Class"; break;
        default: return;
        }
        // The announcement plays first; the power is released when it ends.
        std::shared_ptr<engine::MovieClip> message = symbols::create(messageClass);
        engine::Stage* stage = view_->stage();
        auto target = std::make_shared<MovieClipTarget>(message);
        auto animation = std::make_shared<SpriteAnimation>(target, message->totalFrames() / stage->frameRate(), 1, 0, true);
        stage->addChild(message);
        updater_->addObject(animation);
        Combatant* caster = &combatant;
        auto released = std::make_shared<bool>(false);
        auto done = [this, caster, released, message](auto*) {
            if (!*released) {
                *released = true;
                releaseUltimate(*caster);
            }
            if (message->parent() != nullptr) message->parent()->removeChild(message.get());
        };
        animation->signalFinish.addListener(done);
        animation->signalDisposed().addListener(done);
        SoundConst::playUltimate(combatant.getRace());
        LevelVisual::castUltimateSpell(combatant);
    }

    void Level::releaseUltimate(Combatant& combatant) {
        const SpellInfo* info = nullptr;
        switch (combatant.getRace()) {
        case Race::TheKhaganate: info = ultimateKhaganate(combatant); break;
        case Race::TheCult: info = ultimateCult(combatant); break;
        case Race::TheEmpire: info = ultimateEmpire(combatant); break;
        default: info = ultimateWestaria(combatant); break;
        }
        signalUltimateCast.dispatch(info);
    }

    const SpellInfo* Level::ultimateEmpire(Combatant& combatant) {
        // Random enemy soldiers rise again in the Empire's ranks.
        const Race race = combatant.getRace();
        const int count = static_cast<int>(SpellInfo::CALL_OF_THE_GRAVE.power);
        KeyNode* home = mostThreatenedNode(combatant);
        for (int i = 0; i < count; ++i) {
            int countdown = static_cast<int>(as3::round(as3::random() * 20)) + 1;
            const int start = countdown;
            while (countdown > 0) {
                bool converted = false;
                forEachObject(squads_, [&](const std::shared_ptr<Squad>& squad) {
                    if (converted || !squad->isAttackableBy(race) || squad->isMechanism()) return;
                    if (--countdown <= 0) {
                        const UnitType type = squad->getUnitType();
                        const Point2D pos = squad->getMainUnit()->getPosition();
                        squad->killAll();
                        std::shared_ptr<FSquad> recruit = spawnSquad(pos, combatant, *combatant.getArmyStat(type));
                        recruit->enterNode(*home);
                        recruit->moveTo(pos.x, pos.y);
                        sendFrom(*home, *recruit);
                        playEffect("TerrorSpellEffectClass", pos.x, pos.y, RasterGraphicLayer::LAYER_UNIT);
                        converted = true;
                    }
                });
                if (start == countdown) break;
            }
        }
        return &SpellInfo::CALL_OF_THE_GRAVE;
    }

    const SpellInfo* Level::ultimateCult(Combatant& combatant) {
        // Every other enemy soldier dies; some become gold or mana.
        const Race race = combatant.getRace();
        bool killUnit = true;
        forEachObject(squads_, [&](const std::shared_ptr<Squad>& squad) {
            if (squad->getRace() == race || squad->getRace() == Race::Bandits) return;
            for (const auto& [index, unit] : squad->getUnits()) {
                if (killUnit) {
                    unit->kill();
                    if (as3::random() > 0.5) combatant.addMana(static_cast<int>(SpellInfo::GRAND_TRANSFORMATION.power));
                    else combatant.addGold(static_cast<int>(SpellInfo::GRAND_TRANSFORMATION.power));
                }
                killUnit = !killUnit;
            }
            const Point2D pos = squad->getMainUnit()->getPosition();
            playEffect("GrandTransformationEffectClass", pos.x, pos.y, RasterGraphicLayer::LAYER_UNIT);
        });
        return &SpellInfo::GRAND_TRANSFORMATION;
    }

    const SpellInfo* Level::ultimateWestaria(Combatant& combatant) {
        // The avatar comes out of the most threatened stronghold.
        Spell spell(SpellInfo::SIGN_OF_THE_ADVENT, &combatant.bonuses);
        KeyNode* node = mostThreatenedNode(combatant);
        spell.position = exitPoint(*node);
        std::shared_ptr<FSquad> avatar = spawnSquad(spell.position, combatant, ArmyStat::AVATAR);
        if (combatant.description != player().description) sendFrom(*node, *avatar);
        playEffect("SignOfAdventEffectClass", spell.position.x, spell.position.y, RasterGraphicLayer::LAYER_UNIT);
        auto* unit = static_cast<FUnit*>(avatar->getMainUnit());
        attachLight(*unit, "AvatarFrontLightEffectClass", true);
        attachLight(*unit, "AvatarBackLightEffectClass", false);
        return &SpellInfo::SIGN_OF_THE_ADVENT;
    }

    void Level::attachLight(FUnit& unit, const std::string& key, bool above) {
        unit.attach(def_.resources->createRasterObject(key, 0, 0), above);
    }

    std::shared_ptr<FSquad> Level::spawnSquad(const Point2D& position, Combatant& combatant, const ArmyStat& stat) {
        std::shared_ptr<FSquad> squad = createSquadFor(combatant.description, stat, combatant.getArmyStatBonus(stat));
        squad->moveTo(position.x, position.y);
        iconLayer_->addChild(squad->iconPtr());
        listenSquad(*squad);
        return squad;
    }

    void Level::raiseDead(const Spell& spell, Combatant& combatant) {
        const ArmyStat& stat = as3::random() > 0.5 ? ArmyStat::UNDYING_ARCHERS : ArmyStat::UNDYING_WARRIORS;
        Point2D pos = spell.position;
        if (combatant.description == player().description) {
            std::shared_ptr<FSquad> squad = spawnSquad(spell.position, combatant, stat);
            sendSquad(squad.get(), squad->getMainUnit()->getPosition());
        } else {
            KeyNode* node = mostThreatenedNode(combatant);
            pos = exitPoint(*node);
            std::shared_ptr<FSquad> squad = spawnSquad(pos, combatant, stat);
            sendFrom(*node, *squad);
        }
        playEffect("RaiseDeadSpellEffectClass", pos.x, pos.y, RasterGraphicLayer::LAYER_UNIT);
        playSpellSound(spell);
    }

    const SpellInfo* Level::ultimateKhaganate(Combatant&) {
        // Cataclysm: 90 fireballs on free ground, in waves.
        for (int i = 0; i < 30; ++i) {
            for (int k = 0; k < 3; ++k) {
                Point2D position;
                do {
                    position = Point2D(as3::random() * 800, as3::random() * 600 * Y_KOEFF);
                } while (!isHitColor(static_cast<int>(position.x), static_cast<int>(position.y / Y_KOEFF)));
                const double delay = i * 0.2 + k * 0.05;
                AnimEffectDef anim;
                anim.anim = std::make_shared<RasterAnimation>(def_.resources->getCachedData("FireballEffectClass"),
                                                            def_.resources->animTransitions("FireballEffectClass")[0],
                                                            position.x, position.y, delay);
                anim.time = delay + anim.anim->transition().mscDuration / 1000;
                auto effect = std::make_shared<AnimEffect>(anim);
                effect->signalDisposed().addListener(this, &Level::onFireballLanded);
                addEffect(effect, RasterGraphicLayer::LAYER_UNIT);
            }
        }
        return &SpellInfo::CATACLYSM;
    }

    void Level::onFireballLanded(Disposable* disposed) {
        auto* effect = static_cast<AnimEffect*>(disposed);
        const SpellInfo& spell = SpellInfo::CATACLYSM;
        const Point2D position(effect->rglAnim().getX(), effect->rglAnim().getY());
        const int minDamage = static_cast<int>(spell.power / 2);
        const int maxDamage = static_cast<int>(spell.power);
        const int damage = static_cast<int>(std::ceil(minDamage + as3::random() * (maxDamage - minDamage)));
        SplashArea area;
        area.effects.push_back(std::make_shared<MaxHPPercentDamage>(-1, AbilityEffect::ATK_MAGIC, damage));
        area.attackerId = -1;
        area.radius = spell.radius;
        area.position = position;
        applySplash(area, spell.race, true, true);
        damageNodesInRadius(position, spell.radius, damage / 2, nullptr);
        damageNodesInRadius(position, spell.radius, damage / 2, getCombatantByRace(spell.race)->description.get());
        auto crater = std::make_shared<RasterSprite>(
            def_.resources->createRasterObject("CraterFromExplosionClass", position.x, position.y));
        addEffect(std::make_shared<TimedEffect>(crater, 7), RasterGraphicLayer::LAYER_GROUND);
        playEffect("FireBallExplosionClass", position.x, position.y, RasterGraphicLayer::LAYER_UNIT);
        // The screen shakes.
        view_->setY(view_->y() + as3::random() * 20 + 5);
        view_->setX(view_->x() + as3::random() * 20 - 10);
    }

    void Level::stoneWorms(const Spell& spell, Combatant& combatant) {
        const double spellTime = spell.time + combatant.bonuses.value(Bonus::STONEWORMS_COUNT);
        BannerDef def;
        def.animDef = animDef("StoneWormsEffectClass", spellTime, static_cast<int>(spell.position.x), static_cast<int>(spell.position.y));
        def.spellData = std::make_shared<Spell>(spell);
        def.level = this;
        def.owner = &combatant;
        addEffect(std::make_shared<RepeatingSpellEffect>(def), RasterGraphicLayer::LAYER_UNIT);
    }

    void Level::spellStrike(const Spell& spell, Combatant& combatant) {
        const Point2D pos = spell.position;
        const int minDamage = static_cast<int>(spell.power / 2);
        const int maxDamage = static_cast<int>(spell.power);
        const int damage = static_cast<int>(std::ceil(minDamage + as3::random() * (maxDamage - minDamage)));
        SplashArea area;
        area.effects.push_back(std::make_shared<MaxHPPercentDamage>(-1, AbilityEffect::ATK_MAGIC, damage));
        area.attackerId = -1;
        area.radius = spell.radius;
        area.position = spell.position;
        applySplash(area, spell.race);
        damageNodesInRadius(spell.position, spell.radius, damage, combatant.description.get());
        playEffect("StonewormsExplosionEffect", pos.x, pos.y, RasterGraphicLayer::LAYER_UNIT);
        playSpellSound(spell);
    }

    void Level::disentomb(const Spell& spell, Combatant& combatant) {
        Point2D pos = spell.position;
        const ArmyStat& stat = ArmyStat::IMMORTALS;
        if (combatant.description == player().description) {
            std::shared_ptr<FSquad> squad = spawnSquad(spell.position, combatant, stat);
            sendSquad(squad.get(), squad->getMainUnit()->getPosition());
        } else {
            KeyNode* node = mostThreatenedNode(combatant);
            pos = exitPoint(*node);
            std::shared_ptr<FSquad> squad = spawnSquad(pos, combatant, stat);
            sendFrom(*node, *squad);
        }
        playEffect("RaiseDeadSpellEffectClass", pos.x, pos.y, RasterGraphicLayer::LAYER_UNIT);
        playSpellSound(spell);
    }

    void Level::bannerSpell(const Spell& spell, Combatant& combatant, const std::string& auraKey, const std::string& bannerKey,
                            int bannerKind) {
        const double spellTime = spell.time;
        const int x = static_cast<int>(spell.position.x), y = static_cast<int>(spell.position.y);
        groundAura(auraKey, spellTime, x, y);
        BannerDef def;
        def.animDef = animDef(bannerKey, spellTime, x, y);
        def.spellData = std::make_shared<Spell>(spell);
        def.level = this;
        def.owner = &combatant;
        std::shared_ptr<TimedEffect> banner;
        switch (bannerKind) {
        case BLOODLUST: banner = std::make_shared<BannerBloodlust>(def); break;
        case LIBERATION: banner = std::make_shared<BannerOfLiberation>(def); break;
        case HEROISM: banner = std::make_shared<BannerHeroism>(def); break;
        default: banner = std::make_shared<BannerHeal>(def); break;
        }
        addEffect(banner, RasterGraphicLayer::LAYER_UNIT);
        playSpellSound(spell);
        if (spell.id == SpellInfo::BANNER_OF_CONSECRATION.id) signalHealBanner.dispatch();
    }

    AnimEffectDef Level::animDef(const std::string& key, double spellTime, int x, int y) {
        AnimEffectDef def;
        def.anim = std::make_shared<RasterAnimation>(def_.resources->getCachedData(key), def_.resources->animTransitions(key)[0], x, y, 0);
        def.time = spellTime;
        return def;
    }

    void Level::groundAura(const std::string& key, double spellTime, int x, int y) {
        auto effect = std::make_shared<AnimEffect>(animDef(key, spellTime, x, y));
        addEffect(effect, RasterGraphicLayer::LAYER_GROUND);
        effect->signalDisposed().addListener(this, &Level::onAuraRemoved);
    }

    void Level::onAuraRemoved(Disposable*) { SoundConst::play(SoundId::BANNER_REMOVE); }

    void Level::lordsJudgement(const Spell& spell) {
        damageUnitsInRadius(static_cast<int>(spell.time), static_cast<int>(spell.power), spell.radius, spell.position, spell.race);
        explosion("SpellThunderClass", static_cast<int>(spell.position.x), static_cast<int>(spell.position.y));
        playSpellSound(spell);
    }

    void Level::frostEnchantment(const Spell& spell, Combatant& combatant) {
        const Race race = combatant.getRace();
        const Point2D pos = spell.position;
        forEachObject(squads_, [&](const std::shared_ptr<Squad>& squad) {
            if (squad->getRace() == race) squad->enchant(pos, spell.radius, spell.time, static_cast<int>(spell.power));
        });
        playEffect("FrostEncahntmentEffectClass", pos.x, pos.y, RasterGraphicLayer::LAYER_UNIT);
        playSpellSound(spell);
    }

    void Level::iceBlast(const Spell& spell) {
        const Point2D position = spell.position;
        AnimEffectDef anim;
        anim.anim = std::make_shared<RasterAnimation>(def_.resources->getCachedData("IceBlastEffectClass"),
                                                    def_.resources->animTransitions("IceBlastEffectClass")[0], position.x,
                                                    position.y, 0);
        anim.time = anim.anim->transition().mscDuration / 1000;
        auto effect = std::make_shared<AnimEffect>(anim);
        effect->signalDisposed().addListener(this, &Level::onIceBlastLanded);
        addEffect(effect, RasterGraphicLayer::LAYER_UNIT);
        effect->data = std::make_shared<Spell>(spell);
        SoundConst::play(SoundId::SPELL_ICE_BLAST_LAUNCH);
    }

    void Level::onIceBlastLanded(Disposable* disposed) {
        auto* effect = static_cast<AnimEffect*>(disposed);
        const auto spell = std::any_cast<std::shared_ptr<Spell>>(effect->data);
        SplashArea area;
        Abilities::addFreezeEffects(area.effects, static_cast<int>(spell->time * 1000));
        area.attackerId = -1;
        area.radius = spell->radius;
        area.position = spell->position;
        applySplashToSquads(area, spell->race);
        playEffect("IceBlastExplosionClass", area.position.x, area.position.y, RasterGraphicLayer::LAYER_UNIT);
        playSpellSound(*spell);
    }

    void Level::terror(const Spell& spell, Combatant& combatant) {
        const Race race = combatant.getRace();
        const Point2D pos = spell.position;
        forEachObject(squads_, [&](const std::shared_ptr<Squad>& squad) {
            if (squad->getRace() != race && squad->isNear(pos, spell.radius)) squad->terrify();
        });
        playEffect("TerrorSpellEffectClass", pos.x, pos.y, RasterGraphicLayer::LAYER_UNIT);
        playEffect("TerrorFlyingBatEffectClass", pos.x, pos.y, RasterGraphicLayer::LAYER_UNIT);
        playEffect("TerrorSplashEffectClass", pos.x, pos.y, RasterGraphicLayer::LAYER_GROUND);
        playSpellSound(spell);
    }

    // --- Input processes -------------------------------------------------------------------------------------

    void Level::initProcesses() {
        initCursor();
        initSelection();
        initSpells();
        initOrders();
    }

    void Level::disposeProcesses() {
        disposeOrders();
        disposeSpells();
        disposeSelection();
        disposeCursor();
    }

    // Cursor ------------------------------------------------------------------------------------------------

    void Level::initCursor() {
        cursorHitColor_ = 0;
        cursorProcess_ = std::make_shared<FunctionProcess>([this](double dt, bool locked) { return processCursor(dt, locked); });
        cursorIconId_ = -1;
        cursorIcons_.clear();
    }

    void Level::disposeCursor() {
        showCursorIcon(-1);
        for (auto& icon : cursorIcons_)
            if (icon) icon->dispose();
        cursorIcons_.clear();
        if (cursorProcess_) cursorProcess_->dispose();
        cursorProcess_.reset();
    }

    bool Level::processCursor(double dt, bool locked) {
        if (locked) return locked;
        MouseDevice& mouse = *def_.mouse;
        if (mouse.wasMoved()) {
            objectUnderMouse_ = rasterLayer_->findFirstHitTest(mouse.x(), mouse.y() * Y_KOEFF, RasterGraphicLayer::LAYER_UNIT);
            cursorHitColor_ = hitColorAt(static_cast<int>(mouse.x()), static_cast<int>(mouse.y()));
        }
        if (hasCursorIcon()) {
            cursorIcon().update(dt);
            if (mouse.wasMoved()) moveCursorIconToMouse();
        }
        return locked;
    }

    void Level::showCursorIcon(int id) {
        if (hasCursorIcon() && cursorIcons_[static_cast<std::size_t>(cursorIconId_)] != nullptr) {
            const auto& sprite = cursorIcon().image().sprite();
            if (sprite->parent() != nullptr) sprite->parent()->removeChild(sprite.get());
        }
        cursorIconId_ = id;
        if (cursorIconId_ >= 0 && cursorIcons_[static_cast<std::size_t>(cursorIconId_)] != nullptr) {
            cursorLayer_->addChild(cursorIcon().image().sprite());
            moveCursorIconToMouse();
        }
    }

    int Level::addCursorIcon(const std::string& key) {
        cursorIcons_.push_back(key.empty() ? nullptr : def_.resources->createAnimation(key));
        return static_cast<int>(cursorIcons_.size()) - 1;
    }

    void Level::moveCursorIconToMouse() { moveCursorIcon(def_.mouse->x(), def_.mouse->y()); }

    void Level::moveCursorIcon(double x, double y) {
        if (!hasCursorIcon() || cursorIcons_[static_cast<std::size_t>(cursorIconId_)] == nullptr) return;
        cursorIcon().image().setX(x);
        cursorIcon().image().setY(y);
    }

    // Selection ---------------------------------------------------------------------------------------------

    void Level::initSelection() {
        objectUnderMouse_ = nullptr;
        selectedObject_ = nullptr;
        selectionProcess_ = std::make_shared<FunctionProcess>([this](double dt, bool locked) { return processSelection(dt, locked); },
                                                            [this] { resetSelection(); });
    }

    void Level::disposeSelection() {
        if (selectionProcess_) selectionProcess_->dispose();
        selectionProcess_.reset();
        objectUnderMouse_ = nullptr;
        selectedObject_ = nullptr;
        signalNodeSelected.dispose();
    }

    bool Level::processSelection(double, bool locked) {
        if (locked) {
            objectUnderMouse_ = nullptr;
            selectedObject_ = nullptr;
            return locked;
        }
        MouseDevice& mouse = *def_.mouse;
        if (mouse.wasMoved()) {
            rasterLayer_->allHighLightsOff(RasterGraphicLayer::LAYER_UNIT);
            if (objectUnderMouse_ != nullptr) {
                if (auto* chest = dynamic_cast<ChestView*>(objectUnderMouse_)) {
                    showChestHint(*chest);
                } else {
                    hideChestHint();
                    engine::Mouse::cursor = engine::Mouse::Cursor::Button;
                }
                objectUnderMouse_->setHighlighted(true);
            } else {
                hideChestHint();
                engine::Mouse::cursor = highlightedIcon_ != nullptr ? engine::Mouse::Cursor::Hand : engine::Mouse::Cursor::Auto;
            }
            if (selectedObject_ != nullptr) selectedObject_->setHighlighted(true);
        }
        if (mouse.buttonWasClicked()) {
            if (selectedObject_ != nullptr) selectedObject_->setHighlighted(false);
            selectedObject_ = dynamic_cast<NodeContent*>(objectUnderMouse_) != nullptr ? objectUnderMouse_ : nullptr;
            if (selectedObject_ != nullptr && isModeOn(ST_LOCK_AI)) {
                // Debug: play the owner of whatever is clicked.
                if (Combatant* owner = getLocationOwner(*dynamic_cast<NodeContent*>(selectedObject_)->nodeDescription))
                    def_.player = combatants_.getShared(owner->description.get());
            }
            signalNodeSelected.dispatch(dynamic_cast<NodeContent*>(selectedObject_));
        }
        return locked;
    }

    void Level::resetSelection() {
        if (selectedObject_ != nullptr) selectedObject_->setHighlighted(false);
        selectedObject_ = nullptr;
        engine::Mouse::cursor = engine::Mouse::Cursor::Auto;
    }

    // Spells ---------------------------------------------------------------------------------------------

    void Level::initSpells() {
        stopAiming();
        spellProcess_ = std::make_shared<FunctionProcess>([this](double dt, bool locked) { return processSpells(dt, locked); },
                                                        [this] { stopAiming(); }, aiming_,
                                                        [this](bool value) { aiming_ = value; }, [this] { return aiming_; });
        for (const auto& row : ICON_LIST)
            for (const char* key : row) addCursorIcon(key);
        isCursorUnderButton = false;
    }

    void Level::disposeSpells() {
        signalSpellModeOff.dispose();
        currentSpell_.reset();
        if (spellProcess_) spellProcess_->dispose();
        spellProcess_.reset();
    }

    bool Level::processSpells(double, bool locked) {
        if (locked) {
            showCursorIcon(-1);
            return locked;
        }
        if (!aiming_) return false;
        MouseDevice& mouse = *def_.mouse;
        if (mouse.wasMoved()) aimAtMouse();
        if (mouse.buttonWasClicked() && canCast()) {
            castSpell(currentSpell_.get(), &player());
            stopAiming();
        }
        if (isCursorUnderButton) showCursorIcon(-1);
        return true;
    }

    void Level::selectSpellButton(int buttonId) {
        if (!hasSpell(buttonId)) return;
        const std::shared_ptr<Spell> spell = player().getSpells()[static_cast<std::size_t>(buttonId)];
        if (spell != nullptr) startAiming(std::make_shared<Spell>(*spell, &player().bonuses));
    }

    bool Level::hasSpell(int buttonId) {
        // AS3 arrays answer undefined beyond their end.
        const auto& spells = player().getSpells();
        return buttonId >= 0 && buttonId < static_cast<int>(spells.size()) && spells[static_cast<std::size_t>(buttonId)] != nullptr;
    }

    void Level::startAiming(std::shared_ptr<Spell> spell) {
        if (aiming_) {
            stopAiming();
            if (currentSpell_->id == spell->id) return;
        }
        spellCursorActive_ = false;
        currentSpell_ = std::move(spell);
        showSpellCursor(*currentSpell_);
        aimAtMouse();
        aiming_ = true;
    }

   // --- Casting Aim --------------------------------------------------------------------------------------------

    void Level::stopAiming() {
        showCursorIcon(-1);
        aiming_ = false;
        spellCursorActive_ = false;
        signalSpellModeOff.dispatch();
    }

    void Level::aimAtMouse() {
        currentSpell_->position.x = def_.mouse->x();
        currentSpell_->position.y = def_.mouse->y() * 2;
        setSpellCursor(!cursorOnForbidden());
    }

    void Level::setSpellCursor(bool enable) {
        showSpellCursor(*currentSpell_, enable);
        spellCursorActive_ = enable;
    }

    void Level::showSpellCursor(const Spell& spell, bool active) { showCursorIcon(spell.id * 2 + (active ? 0 : 1)); }

    void Level::setUltimateIcon(USpellButton& button) {
        if (auto* icons = symbols::find(*button.image(), "mcIconBack.mcIconSet")) icons->gotoAndStop(playerSpell(3)->id + 1);
    }

    const SpellInfo* Level::playerSpell(int buttonId) { return spellInfo(buttonId, player().getRace()); }

    void Level::invalidateStatistics() {
        LevelVisual::invalidateStatistics();
        signalStatsChanged.dispatch(this);
    }

    void Level::killSquad(Squad& squad) {
        if (hintSquad_ == &squad) hideSquadHint();
        if (draggedSquad_ == &squad.getKey()) draggedSquad_ = nullptr;
        if (iconEventSquad_ == &squad.getKey()) clearIconEvent();
        LevelVisual::killSquad(squad);
    }

    // Orders ---------------------------------------------------------------------------------------------

    void Level::initOrders() {
        clearIconEvent();
        highlightedIcon_ = nullptr;
        savedIconFilters_.clear();
        draggedSquad_ = nullptr;
        ordersEnabled_ = true;
        orderProcess_ = std::make_shared<FunctionProcess>([this](double dt, bool locked) { return processOrders(dt, locked); },
                                                        [this] { resetOrders(); }, ordersEnabled_,
                                                        [this](bool value) { ordersEnabled_ = value; },
                                                        [this] { return ordersEnabled_; });
        iconGoPath_ = addCursorIcon("IconGoPathClass");
        iconGoPathOff_ = addCursorIcon("IconGoPathOffClass");
        iconGoPathOff_ = addCursorIcon("IconGoPathOffClass"); // (registered twice in the original)
        iconGoIn_ = addCursorIcon("IconGoInClass");
        iconAttack_ = addCursorIcon("IconAttackClass");
        initPathPoints();
        // The glows of selected and hovered squad icons are taken from a sample clip.
        std::shared_ptr<engine::MovieClip> sample = symbols::create("IconsHighligtSetClass");
        const auto filtersOf = [&](const char* name) {
            auto* clip = symbols::child<engine::DisplayObject>(*sample, name);
            return clip ? clip->filters() : std::vector<swf::Filter>{};
        };
        switch (player().getRace()) {
        case Race::Westaria: selectionFilters_ = filtersOf("_-bS"); break;
        case Race::TheCult: selectionFilters_ = filtersOf("_-Zg"); break;
        case Race::TheKhaganate: selectionFilters_ = filtersOf("_-t-"); break;
        default: break;
        }
        overFilters_ = filtersOf("_-F");
        draggingOrder_ = false;
        stopFrame();
    }

    void Level::disposeOrders() {
        disposePathPoints();
        clearIconEvent();
        draggedSquad_ = nullptr;
        highlightIcon(false);
        highlightedIcon_ = nullptr;
        if (orderProcess_) orderProcess_->dispose();
        orderProcess_.reset();
    }

    bool Level::processOrders(double, bool locked) {
        MouseDevice& mouse = *def_.mouse;
        if (locked) {
            if (isDragging()) {
                hidePathPoints();
                stopDrag();
            }
            stopFrame();
            highlightIcon(false);
            return locked;
        }
        if (!ordersEnabled_) return false;
        if (hasIconEvent()) {
            if (auto* squad = dynamic_cast<FSquad*>(squads_.getObject(iconEventSquad_))) handleIconEvent(*squad);
            clearIconEvent();
        }
        FSquad* squad = isDragging() ? draggedSquad() : nullptr;
        if (mouse.buttonWasReleased()) {
            if (squad != nullptr && cursorIconId_ != iconGoPathOff_) {
                const Point2D position = convertToLandPoint(targetPosition_);
                sendSquad(squad, position);
                sendSelected(position, *squad);
                SoundConst::playSend(squad->getArmyStat());
                unselectAll();
            }
            hidePathPoints();
            stopDrag();
            return false;
        }
        if (mouse.wasMoved() && squad != nullptr) {
            frontLayer_->graphics().clear();
            const NodeKey* nodeKey = nullptr;
            if (auto* node = dynamic_cast<FKeyNodeContent*>(objectUnderMouse_)) nodeKey = node->nodeDescription.get();
            if (auto* chest = dynamic_cast<ChestView*>(objectUnderMouse_)) nodeKey = chest->nodeDescription.get();
            Point2D goTarget;
            if (nodeKey != nullptr) {
                goTarget = graphNodeOf(nodeKey)->pos;
                targetPosition_ = convertToScreenPoint(goTarget);
            } else {
                goTarget = mouseLandPosition();
                targetPosition_ = Point2D(mouse.x(), mouse.y());
            }
            const std::optional<std::vector<Point2D>> path = showPath(*squad, goTarget);
            if (path) {
                if (nodeKey == nullptr) targetPosition_ = convertToScreenPoint(path->back());
                if (cursorIconId_ != iconGoPath_) showCursorIcon(iconGoPath_);
            } else {
                if (cursorIconId_ != iconGoPathOff_) showCursorIcon(iconGoPathOff_);
                hidePathPoints();
            }
            if (auto* node = dynamic_cast<KeyNode*>(objectUnderMouse_)) {
                if (Combatant* owner = getLocationOwner(*node->nodeDescription))
                    showCursorIcon(isPlayer(owner->description.get()) ? iconGoIn_ : iconAttack_);
            }
        }
        draggingOrder_ = squad != nullptr && squad->icon() != nullptr;
        if (draggingOrder_) {
            engine::Graphics& g = frontLayer_->graphics();
            g.clear();
            aimPosition_.x += (targetPosition_.x - aimPosition_.x) * 0.4;
            aimPosition_.y += (targetPosition_.y - aimPosition_.y) * 0.4;
            drawArrow(g, Point2D(squad->icon()->x(), squad->icon()->y()), aimPosition_);
            drawOrderArrows(g, *squad, aimPosition_);
            moveCursorIcon(aimPosition_.x, aimPosition_.y);
        }
        if (mouse.buttonIsDown()) {
            if (draggedSquad_ == nullptr && !framing_) startFrame();
        } else if (framing_) {
            stopFrame();
        }
        if (mouse.buttonWasPressed() && draggedSquad_ == nullptr) unselectAll();
        if (framing_) updateFrame();
        return locked;
    }

    void Level::resetOrders() {
        hidePathPoints();
        stopDrag();
    }

    // --- Frames --------------------------------------------------------------------------------------------


    void Level::startFrame() {
        framing_ = true;
        frame_.x = def_.mouse->x();
        frame_.y = def_.mouse->y();
    }

    void Level::stopFrame() {
        framing_ = false;
        topLayer_->graphics().clear();
    }

    void Level::updateFrame() {
        frame_.width = def_.mouse->x() - frame_.x;
        frame_.height = def_.mouse->y() - frame_.y;
        engine::Graphics& g = topLayer_->graphics();
        g.clear();
        g.lineStyle(2, 0x00FF00);
        g.drawRect(frame_.x, frame_.y, frame_.width, frame_.height);
        selectInFrame(frame_);
    }

   // --- Selection --------------------------------------------------------------------------------------------

    void Level::selectInFrame(const engine::Rectangle& rect) {
        const engine::Rectangle frame{std::min(rect.x, rect.x + rect.width), std::min(rect.y, rect.y + rect.height),
                                    std::abs(rect.width), std::abs(rect.height)};
        forEachObject(squads_, [&](const std::shared_ptr<Squad>& squad) {
            auto& fsquad = static_cast<FSquad&>(*squad);
            if (fsquad.getRace() != player().getRace() || fsquad.icon() == nullptr) return;
            const double x = fsquad.icon()->x(), y = fsquad.icon()->y();
            const bool inside = x >= frame.x && x < frame.x + frame.width && y >= frame.y && y < frame.y + frame.height;
            fsquad.setIconHighlight(inside, inside ? selectionFilters_ : std::vector<swf::Filter>{});
        });
    }

    void Level::unselectAll() {
        forEachObject(squads_, [&](const std::shared_ptr<Squad>& squad) {
            auto& fsquad = static_cast<FSquad&>(*squad);
            if (fsquad.getRace() == player().getRace()) fsquad.setIconHighlight(false, {});
        });
    }

    void Level::sendSelected(const Point2D& position, FSquad& squad) {
        std::vector<FSquad*> selected{&squad};
        forEachObject(squads_, [&](const std::shared_ptr<Squad>& other) {
            auto* fsquad = static_cast<FSquad*>(other.get());
            if (fsquad->getRace() == player().getRace() && fsquad->isIconHighlighted() && fsquad != &squad) selected.push_back(fsquad);
        });
        // Squads standing at the same place leave one after the other.
        std::vector<std::vector<FSquad*>> groups;
        for (FSquad* candidate : selected) {
            std::size_t k = 0;
            for (; k < groups.size(); ++k) {
                const double d2 = groups[k][0]->getMainUnit()->getPosition().getDistanceTo(candidate->getMainUnit()->getPosition(), true);
                if (d2 < 3) {
                    groups[k].push_back(candidate);
                    break;
                }
            }
            if (k == groups.size()) groups.push_back({candidate});
        }
        for (auto& group : groups) {
            for (std::size_t k = 0; k < group.size(); ++k) {
                group[k]->commandGotoPoint(position);
                group[k]->increaseSpeed(-1, static_cast<double>(k) * 4);
            }
        }
    }

    void Level::drawOrderArrows(engine::Graphics& graphics, FSquad& squad, const Point2D& aim) {
        forEachObject(squads_, [&](const std::shared_ptr<Squad>& other) {
            auto* fsquad = static_cast<FSquad*>(other.get());
            if (fsquad->getRace() == player().getRace() && fsquad->isIconHighlighted() && fsquad != &squad)
                drawArrow(graphics, Point2D(fsquad->icon()->x(), fsquad->icon()->y()), aim);
        });
    }

    void Level::onSquadIconEvent(const SquadKey* squad, int type) {
        iconEventSquad_ = squad;
        iconEventType_ = type;
    }

    void Level::clearIconEvent() {
        iconEventSquad_ = nullptr;
        iconEventType_ = -1;
    }

    void Level::handleIconEvent(FSquad& squad) {
        switch (iconEventType_) {
        case FSquad::ICON_EVENT_DOWN:
            if (isPlayerRace(squad.getRace())) startDrag(&squad);
            break;
        case FSquad::ICON_EVENT_OUT:
            highlightIcon(false);
            hideSquadHint();
            break;
        case FSquad::ICON_EVENT_OVER:
            highlightIcon(true, &squad);
            showSquadHint(squad);
            break;
        default:
            break;
        }
    }

    // --- Highlight --------------------------------------------------------------------------------------------


    void Level::highlightIcon(bool enable, FSquad* squad) {
        if (highlightedIcon_ != nullptr) {
            highlightedIcon_->setFilters(savedIconFilters_);
            highlightedIcon_ = nullptr;
        }
        if (enable) {
            highlightedIcon_ = squad->icon();
            savedIconFilters_ = highlightedIcon_->filters();
            highlightedIcon_->setFilters(overFilters_);
        }
    }
       
    // --- Drag --------------------------------------------------------------------------------------------


    void Level::startDrag(FSquad* squad) {
        if (squad == nullptr) return;
        draggedSquad_ = &squad->getKey();
        showCursorIcon(iconGoPath_);
        aimPosition_ = Point2D(squad->icon()->x(), squad->icon()->y());
        targetPosition_ = aimPosition_;
        SoundConst::play(-1); // the original plays the squad's "selected" sound: none
    }

    void Level::stopDrag() {
        frontLayer_->graphics().clear();
        draggedSquad_ = nullptr;
        showCursorIcon(-1);
    }

    FSquad* Level::draggedSquad() { return dynamic_cast<FSquad*>(squads_.getObject(draggedSquad_)); }

    void Level::drawArrow(engine::Graphics& graphics, const Point2D& from, const Point2D& to) {
        std::uint32_t color = player().getColor();
        if (iconGoPathOff_ == cursorIconId_) color = 0xFF0000;
        const double length = from.getDistanceTo(to);
        const Point2D control((from.x + to.x) / 2, (from.y + to.y) / 2 - length * 0.7);
        graphics.lineStyle(6, 0, 0.8);
        graphics.moveTo(from.x, from.y);
        graphics.curveTo(control.x, control.y, to.x, to.y);
        graphics.lineStyle(5, color, 0.8);
        graphics.moveTo(from.x, from.y);
        graphics.curveTo(control.x, control.y, to.x, to.y);
    }

    bool Level::sendSquad(FSquad* squad, const Point2D& target) {
        if (squad == nullptr) return false;
        squad->commandGotoPoint(target);
        return true;
    }

   // --- Path --------------------------------------------------------------------------------------------

    void Level::initPathPoints() {
        pathPoints_.clear();
        pathPointCount_ = 0;
    }

    void Level::addPathPoint(const Point2D& point) {
        if (pathPointCount_ == static_cast<int>(pathPoints_.size())) {
            auto dot = std::make_shared<RasterSprite>(def_.resources->createRasterObject("PathPointGreenClass"));
            pathPoints_.push_back(dot);
            rasterLayer_->addRGLObject(dot, RasterGraphicLayer::LAYER_PATH);
        }
        RasterSprite& dot = *pathPoints_[static_cast<std::size_t>(pathPointCount_)];
        dot.setX(point.x);
        dot.setY(point.y);
        dot.setVisible(true);
        ++pathPointCount_;
    }

    void Level::hidePathPoints() {
        for (int i = 0; i < pathPointCount_; ++i) {
            if (!pathPoints_[static_cast<std::size_t>(i)]->isVisible()) break;
            pathPoints_[static_cast<std::size_t>(i)]->setVisible(false);
        }
        pathPointCount_ = 0;
    }

    void Level::disposePathPoints() {
        for (auto& dot : pathPoints_) dot->dispose();
        pathPoints_.clear();
    }

    std::optional<std::vector<Point2D>> Level::showPath(FSquad& squad, const Point2D& target) {
        std::optional<std::vector<Point2D>> path = findRoadPath(squad.getMainUnit()->getPosition(), target, 60 * 60);
        if (path) {
            *path = resamplePath(*path, 50);
            pathPointCount_ = 0;
            for (const Point2D& p : *path) addPathPoint(p);
            // The selected squads' paths too.
            forEachObject(squads_, [&](const std::shared_ptr<Squad>& other) {
                auto* fsquad = static_cast<FSquad*>(other.get());
                if (fsquad->getRace() == player().getRace() && fsquad->isIconHighlighted() && fsquad != &squad) {
                    // Like the original, a failed path of a selected squad is not checked.
                    std::optional<std::vector<Point2D>> otherPath = findRoadPath(fsquad->getMainUnit()->getPosition(), target, 60 * 60);
                    if (!otherPath) return;
                    for (const Point2D& p : resamplePath(*otherPath, 50)) addPathPoint(p);
                }
            });
            for (std::size_t i = static_cast<std::size_t>(pathPointCount_); i < pathPoints_.size(); ++i) {
                if (!pathPoints_[i]->isVisible()) break;
                pathPoints_[i]->setVisible(false);
            }
        }
        return path;
    }

    // --- Hints --------------------------------------------------------------------------------------------

    // Chests

    void Level::showChestHint(ChestView&) {

        if (hintSquad_ != nullptr) 
            return;
        
        hintLayer_->getHintWindow(hintOverPanel_)
            ->setText("This <FONT COLOR='#FFFF00'>CHEST</FONT> contains <FONT COLOR='#FFFF00'>GOLD</FONT>!<br>"
                    "<FONT COLOR='#65FF00'>Bring your squad here </FONT> and it will be yours!");
        
        hintLayer_->showHintAtRect(hintRect(), hintOverPanel_);
        
        chestHintShown_ = true;
    }

    void Level::hideChestHint() {
        if (chestHintShown_) {
            hintLayer_->hideHint();
            chestHintShown_ = false;
        }
    }

    // Squads

    void Level::showSquadHint(FSquad& squad) {

        if (hintSquad_ == &squad) 
            return;
        
        if (hintSquad_ != nullptr) 
            hideSquadHint();
        
        hintSquad_ = &squad;
        HintWindow2* hint = hintLayer_->getHintWindow(hintUnit2_);
        
        hintSquad_->fillHint(hint, dynamic_cast<engine::MovieClip*>(hint->image()));
        hintLayer_->showHintWindowAtRect(hintRect(), hint);
        
        chestHintShown_ = false;
    }

    void Level::hideSquadHint() {
        if (hintSquad_ != nullptr) {
            hintLayer_->hideHint();
            hintSquad_->fillHint(nullptr, nullptr);
            hintSquad_ = nullptr;
        }
    }

} // namespace takeover
