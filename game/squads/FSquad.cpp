#include "game/squads/FSquad.h"

#include "game/framework/Symbols.h"
#include "game/framework/ui/HintWindow.h"
#include "game/hud/WindowController.h"
#include "game/level/LevelVisual.h"
#include "game/media/SoundConst.h"
#include "game/resources/TOResManager.h"
#include "game/resources/XMLData.h"
#include "game/sim/abilities/Abilities.h"
#include "game/sim/abilities/AbilityEffects.h"
#include "game/sim/abilities/UnitParams.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/squads/SquadIcon.h"
#include "game/units/UnitNameModifiers.h"

#include "engine/display/MovieClip.h"
#include "engine/display/TextField.h"
#include "engine/events/Event.h"

#include <algorithm>
#include <cctype>

namespace takeover {

    // /----- HINT TEXTS -----/

    namespace {
        std::string upper(std::string text) {
            std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            return text;
        }
    } // namespace

    FSquad::FSquad() = default;
    FSquad::~FSquad() = default;

    std::string FSquad::attackSpeedText(double attackSpeed) {
        if (attackSpeed <= 1 * 10)
            return "v.fast";

        if (attackSpeed < 1.5 * 10)
            return "fast";

        if (attackSpeed < 2 * 10)
            return "";

        if (attackSpeed <= 3 * 10)
            return "slow";

        return "v.slow";
    }

    std::string FSquad::rangeText(double range) {
        if (range < 70)
            return "SHORT";

        if (range >= 200)
            return "VERY WIDE";

        if (range >= 130)
            return "WIDE";

        return "AVERAGE";
    }

    std::string FSquad::speedText(double speed) {
        if (speed < 25)
            return "VERY SLOW";

        if (speed < 35)
            return "SLOW";

        if (speed < 45)
            return "AVERAGE";

        if (speed < 65)
            return "HIGH";

        return "VERY HIGH";
    }

    std::string FSquad::moraleText(double morale, bool moraleNone) {
        if (moraleNone)
            return "N/A";

        if (morale < 25)
            return "VERY LOW";

        if (morale < 40)
            return "LOW";

        if (morale < 60)
            return "NORMAL";

        if (morale < 75)
            return "HIGH";

        return "VERY HIGH";
    }

    // /----- SETUP -----/

    FUnitDef FSquad::makeUnitDef(TOResManager& resources, const ArmyStat& stat, const ArmyStatBonus& bonus) {
        FUnitDef def;
        def.resources = &resources;
        def.armyStat = &stat;
        def.armyStatBonus = bonus;
        def.phaseNames = stat.animModifier;
        if (def.phaseNames == nullptr) {
            def.phaseNames = std::make_shared<UnitPhaseNameModifier>(XMLData::raceColor(stat.side));
            def.unitID = XMLData::unitTypeName(stat.unitType);
        } else {
            def.unitID = unit_types::toString(stat.unitType) + (stat.techLevel > 1 ? std::to_string(stat.techLevel) : "");
        }

        return def;
    }

    FSquad& FSquad::init(const FSquadDef& def) {
        resources_ = def.resources;
        level_ = def.levelVisual;
        Squad::init(def);
        createIcon();

        return *this;
    }

    void FSquad::customDispose() {
        if (level_ != nullptr)
            for (const auto& [index, unit] : units_)
                level_->rasterLayer().removeRGLObject(static_cast<FUnit*>(unit.get()));
        resources_ = nullptr;
        signalIcon.dispose();
        signalLeftNode.dispose();
        signalAtPosition.dispose();
        disposeIcon();
        Squad::customDispose();
        level_ = nullptr;
    }

    // /----- FRAME -----/

    void FSquad::update(double timeElapsed) {
        Squad::update(timeElapsed);
        hideUnitsUnderBuildings();
        updateIconAndHint(timeElapsed);
    }

    std::shared_ptr<Unit> FSquad::createUnit(int) {
        FUnitDef def = makeUnitDef(*resources_, *stat_, bonus_);
        def.squadId = getId();
        auto unit = std::make_shared<FUnit>();
        unit->init(def);
        unit->signalAnim.addListener(this, &FSquad::onUnitAnim);
        level_->rasterLayer().addRGLObject(unit, RasterGraphicLayer::LAYER_UNIT);

        return unit;
    }

    // /----- UNIT EVENTS -----/

    void FSquad::notifySquadAtPosition() {
        signalAtPosition.dispatch(this);
        Squad::notifySquadAtPosition();
    }

    void FSquad::notifyAllUnitsDead() {
        Squad::notifyAllUnitsDead();
        icon_->icon()->setVisible(false);
    }

    void FSquad::checkDeath() {
        aliveCount_ = 0;
        int deadAndVisible = 0;

        for (const auto& [index, unit] : units_) {
            auto* soldier = static_cast<FUnit*>(unit.get());
            if (soldier->isDead()) {
                if (soldier->isVisible())
                    ++deadAndVisible;
            } else {
                ++aliveCount_;
            }
        }

        if (aliveCount_ == 0) {
            postEvent(EV_ALL_DEAD);

            if (deadAndVisible == 0)
                readyToRemove_ = true;
        }
    }

    engine::Sprite* FSquad::icon() const { return icon_ ? icon_->icon().get() : nullptr; }
    std::shared_ptr<engine::Sprite> FSquad::iconPtr() const { return icon_ ? icon_->icon() : nullptr; }

    // /----- HINT -----/

    void FSquad::fillHint(HintWindow2* hint, engine::MovieClip* hintWindow) {
        hintWindow_ = hintWindow;
        if (hintWindow_ == nullptr)
            return;

        const ArmyStat& stat = *stat_;
        const int unitsCount = stat.unitsCount + bonus_.addUnit;
        const int penalty = stat.meleePenalty;
        const int range = stat.range;
        const std::vector<std::string> abilities = Abilities::splitAbilities(stat.abilities);
        std::string hintText = "<br><br><br><br><br>";
        symbols::setHtmlText(*hintWindow_, "_-y3", upper(stat.name));

        if (auto* number = symbols::child(*hintWindow_, "mcUnitNumber"))
            number->gotoAndStop(unitsCount);

        if (auto* ranged = symbols::child(*hintWindow_, "mcRangedBlock")) {
            ranged->setVisible(range != 0);

            if (range != 0) {
                symbols::setHtmlText(*ranged, "textRange", rangeText(range));
                symbols::setHtmlText(*ranged, "textMeleePenalty", "- " + std::to_string(penalty) + "%");
            }
        }

        bool hasAbility = false;
        const int level = getLevel();
        if (level > 0) {
            hintText += WindowController::levelColored(level, upper(WindowController::levelName(level)));
            hasAbility = true;
        }

        if (!abilities.empty() && !abilities[0].empty()) {
            for (const std::string& ability : abilities) {
                const std::vector<std::string> parts = Abilities::splitAbility(ability);
                const std::string param = parts.size() > 1 ? parts[1] : "";
                const std::string name = AbilityEffect::displayName(parts[0], param, bonus_);

                if (!hasAbility)
                    hasAbility = true;
                else
                    hintText += ", ";

                hintText += name;
            }
        }

        if (auto* abilitiesTitle = symbols::child<engine::DisplayObject>(*hintWindow_, "_-ig"))
            abilitiesTitle->setVisible(hasAbility);

        if (hint != nullptr)
            hint->setText(hintText);
    }

    // /----- ICON -----/

    void FSquad::createIcon() {
        icon_ = std::make_unique<SquadIcon>(*this, *resources_);
        listenIcon(*icon_->body());
        hintWindow_ = nullptr;
    }

    void FSquad::disposeIcon() {
        fillHint(nullptr, nullptr);

        if (icon_) {
            if (icon_->body())
                unlistenIcon(*icon_->body());

            icon_->dispose();
            icon_.reset();
        }
    }

    void FSquad::updateIconAndHint(double timeElapsed) {
        icon_->update(timeElapsed, *this);

        if (hintWindow_ == nullptr)
            return;

        Unit* main = getMainUnit();
        const int unitsCount = stat_->unitsCount + bonus_.addUnit;
        const int hp = main->getMaxHP();
        const int level = getLevel();
        const int minDamage = UnitParams::levelDamageMin(*stat_, bonus_, level);
        const int maxDamage = UnitParams::levelDamageMax(*stat_, bonus_, level);
        std::string hpText = WindowController::levelColored(level, std::to_string(hp));

        if (unitsCount > 1)
            hpText += "<FONT COLOR='#999999'>  (" + std::to_string(currentHP_) + ")</FONT>";

        std::string damageText = WindowController::levelColored(level, std::to_string(minDamage) + " - " + std::to_string(maxDamage));

        if (unitsCount > 1)
            damageText += "<FONT COLOR='#999999'>  (" + std::to_string(minDamage * aliveCount_) + " - " +
                          std::to_string(maxDamage * aliveCount_) + ")</FONT>";
        symbols::setHtmlText(*hintWindow_, "textUnitsNum", std::to_string(aliveCount_) + " / " + std::to_string(unitsCount));
        symbols::setHtmlText(*hintWindow_, "textHp", hpText);
        symbols::setHtmlText(*hintWindow_, "textDamage", damageText);
    }

    // /----- STATUS EVENTS -----/

    void FSquad::onActionStatusUpdated() {
        Squad::onActionStatusUpdated();
        icon_->setActionStatus(getActionState());
    }

    void FSquad::onChangeMorale(int eventType, BaseObject* sender, const std::any& status) {
        icon_->removeMorale();
        Squad::onChangeMorale(eventType, sender, status);
        const MoraleStatus morale = getMoraleStatus();
        icon_->showMorale(morale);
        SoundConst::playMorale(stat_, morale);
    }

    void FSquad::makelevelUP(int level) {
        Squad::makelevelUP(level);

        if (icon_)
            icon_->makelevelUP(level);

        if (level > 0)
            SoundConst::play(SoundId::SQUAD_LEVEL_UP);
    }

    // /----- ICON LISTENERS -----/

    void FSquad::listenIcon(engine::Sprite& icon) {
        icon.addEventListener(engine::MouseEvent::MOUSE_OVER, this, &FSquad::onIconOver);
        icon.addEventListener(engine::MouseEvent::MOUSE_OUT, this, &FSquad::onIconOut);
        icon.addEventListener(engine::MouseEvent::MOUSE_DOWN, this, &FSquad::onIconDown);
        icon.addEventListener(engine::MouseEvent::MOUSE_UP, this, &FSquad::onIconUp);
        icon.addEventListener(engine::MouseEvent::CLICK, this, &FSquad::onIconClick);
    }

    void FSquad::unlistenIcon(engine::Sprite& icon) {
        icon.removeEventListener(engine::MouseEvent::MOUSE_OVER, this, &FSquad::onIconOver);
        icon.removeEventListener(engine::MouseEvent::MOUSE_OUT, this, &FSquad::onIconOut);
        icon.removeEventListener(engine::MouseEvent::MOUSE_DOWN, this, &FSquad::onIconDown);
        icon.removeEventListener(engine::MouseEvent::MOUSE_UP, this, &FSquad::onIconUp);
        icon.removeEventListener(engine::MouseEvent::CLICK, this, &FSquad::onIconClick);
    }

    void FSquad::onIconClick(engine::MouseEvent&) { signalIcon.dispatch(&getKey(), ICON_EVENT_CLICK); }
    void FSquad::onIconUp(engine::MouseEvent&) { signalIcon.dispatch(&getKey(), ICON_EVENT_UP); }
    void FSquad::onIconDown(engine::MouseEvent&) { signalIcon.dispatch(&getKey(), ICON_EVENT_DOWN); }
    void FSquad::onIconOut(engine::MouseEvent&) { signalIcon.dispatch(&getKey(), ICON_EVENT_OUT); }
    void FSquad::onIconOver(engine::MouseEvent&) { signalIcon.dispatch(&getKey(), ICON_EVENT_OVER); }

    // /----- UNITS -----/

    void FSquad::hideUnitsUnderBuildings() {
        const bool stateVisible = state_ != STATE_INSIDE;

        for (const auto& [index, unit] : units_) {
            auto* soldier = static_cast<FUnit*>(unit.get());
            soldier->hidden = !(stateVisible && !level_->isHitColor(static_cast<int>(soldier->getX()),
                                                                  static_cast<int>(soldier->getY() / LevelVisual::Y_KOEFF),
                                                                  0xFF0000));
        }
    }

    void FSquad::onUnitAnim(FUnit* unit, int animId) {
        switch (animId) {
        case FUnit::ANIM_FALL_DOWN:
            SoundConst::playDeath(stat_);
            break;
        case FUnit::ANIM_FALL_DOWN_FINISH: {
            auto shared = std::dynamic_pointer_cast<FUnit>(unit->shared_from_this());
            level_->rasterLayer().removeRGLObject(unit);
            level_->rasterLayer().addRGLObject(shared, RasterGraphicLayer::LAYER_CORPSE);
            break;
        }
        case FUnit::ANIM_ATTACK:
            level_->signalUnitAttack.dispatch(unit);
            break;
        default:
            break;
        }
    }

    // /----- BATTLE STATE -----/

    void FSquad::notifyStartBattle() {
        Squad::notifyStartBattle();
        SoundConst::playBattleCry(stat_);
    }

    void FSquad::changeFSMState(int newState) {
        if (auto* node = dynamic_cast<FKeyNodeContent*>(keyNode_)) {
            if (newState == STATE_INSIDE && state_ != STATE_INSIDE) {
                node->addIcon(icon_->icon());
            } else if (newState != STATE_INSIDE && state_ == STATE_INSIDE) {
                signalLeftNode.dispatch(this);
                node->updateIconView();
            }
        }

        Squad::changeFSMState(newState);
    }

    void FSquad::setIconHighlight(bool enable, std::vector<swf::Filter> filters) {
        if (icon() == nullptr)
            return;

        icon()->setFilters(enable ? std::move(filters) : std::vector<swf::Filter>{});
        iconHighlighted_ = enable;
    }

} // namespace takeover
