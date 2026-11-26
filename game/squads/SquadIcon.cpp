#include "game/squads/SquadIcon.h"

#include "game/framework/Symbols.h"
#include "game/framework/raster/ResourceManager.h"
#include "game/level/Level.h"
#include "game/resources/TOResManager.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/squads/FSquad.h"

#include "engine/display/BitmapData.h"
#include "engine/display/MovieClip.h"
#include "engine/display/Shape.h"

#include <cmath>

namespace takeover {

    int SquadIcon::raceColorFrame(Race race) {
        switch (race) {
        case Race::Westaria: return 1;
        case Race::TheKhaganate: return 2;
        case Race::TheCult: return 3;
        case Race::TheEmpire: return 4;
        case Race::Bandits: return 5;
        }

        return 1;
    }

    std::string SquadIcon::unitIconClass(UnitType type, Race race, int techLevel) {
        const char* raceChar = "";

        switch (race) {
        case Race::TheEmpire: raceChar = "_E_"; break;
        case Race::Westaria: raceChar = "_W_"; break;
        case Race::TheKhaganate: raceChar = "_K_"; break;
        case Race::TheCult: raceChar = "_B_"; break;
        case Race::Bandits: raceChar = "_N_"; break;
        }

        return "IconT" + std::to_string(techLevel) + raceChar + unit_types::toString(type) + "Class";
    }

    void SquadIcon::rasterizeContents(engine::DisplayObjectContainer& container) {
        const engine::Rectangle rect = container.getBounds(&container);
        auto pixels = std::make_shared<engine::BitmapData>(static_cast<int>(rect.width), static_cast<int>(rect.height), true, 0);
        pixels->draw(container, engine::Matrix::translation(-rect.x, -rect.y));
        auto bitmap = std::make_shared<engine::Bitmap>(pixels);
        bitmap->setX(rect.x);
        bitmap->setY(rect.y);

        while (container.numChildren() > 0)
            container.removeChildAt(0);

        container.addChild(bitmap);
    }

    SquadIcon::SquadIcon(FSquad& squad, TOResManager& resources) : stat_(squad.getArmyStat()), resources_(&resources) {
        icon_ = std::make_shared<engine::Sprite>();
        view_ = std::make_shared<View>();
        body_ = createBody(squad);
        body_->addChild(view_->sprite());
        icon_->addChild(body_);
    }

    SquadIcon::~SquadIcon() { dispose(); }

    void SquadIcon::dispose() {
        resources_ = nullptr;

        if (icon_ == nullptr)
            return;

        removeAll();
        round_.reset();
        body_.reset();

        if (icon_->parent() != nullptr)
            icon_->parent()->removeChild(icon_.get());

        icon_.reset();
    }

    void SquadIcon::update(double timeElapsed, FSquad& squad) {
        if (icon_ == nullptr)
            return;

        const int frames = round_->totalFrames();
        round_->gotoAndStop(frames - static_cast<int>(std::floor(frames * squad.hpFactor())) + 1);

        if (!squad.isInside()) {
            if (Unit* unit = squad.getMainUnit()) {
                icon_->setX(unit->getX());
                icon_->setY(unit->getY() / Level::Y_KOEFF - 60);
            }
        }

        if (action_)
            action_->update(timeElapsed);

        if (morale_)
            morale_->update(timeElapsed);

        // The star stays on its first frame.
        if (star_) {
            star_->dispose();
            star_.reset();
        }
    }

    std::shared_ptr<engine::Sprite> SquadIcon::createBody(FSquad& squad) {
        const ArmyStat& stat = *squad.getArmyStat();
        auto body = std::make_shared<engine::Sprite>();
        round_ = symbols::create("SquadRoundIconClass");
        if (auto* color = symbols::child(*round_, "mcColor"))
            color->gotoAndStop(raceColorFrame(stat.side));

        body->addChild(round_);
        showLevel(squad.getLevel());
        setActionStatus(squad.getActionState());
        showMorale(squad.getMoraleStatus());
        const std::string iconClass = unitIconClass(stat.unitType, stat.side, stat.techLevel);
        if (symbols::exists(iconClass)) {
            std::shared_ptr<engine::MovieClip> portrait = symbols::create(iconClass);
            rasterizeContents(*portrait);
            body->addChild(portrait);
        }

        return body;
    }

    std::shared_ptr<SpriteAnimation> SquadIcon::addAnimation(const std::string& key, double delay) {
        std::shared_ptr<SpriteAnimation> animation = resources_->createAnimation(key);
        animation->setDelay(delay);
        animation->update(delay);
        view_->addImage(*animation);

        return animation;
    }

    void SquadIcon::showAction(std::optional<SquadState> status) {
        if (view_ == nullptr || !status)
            return;

        const char* key = nullptr;
        double delay = 0;

        switch (*status) {
        case SquadState::Charging: key = "ASTChargingClass"; break;
        case SquadState::DefendingAndAttack: key = "ASTDefendingAndAttackClass", delay = 1.5; break;
        case SquadState::Defending: key = "ASTDefendingClass"; break;
        case SquadState::MeleeFighting: key = "ASTMeleeFightingClass"; break;
        case SquadState::Moving: key = "ASTMovingClass"; break;
        case SquadState::RangedFighting: key = "ASTRangedFightingClass", delay = 1.5; break;
        case SquadState::Repairing: key = "ASTRepairingClass"; break;
        case SquadState::Sleeping: key = "ASTSleepingClass"; break;
        default: return;
        }

        action_ = addAnimation(key, delay);
    }

    void SquadIcon::showMorale(MoraleStatus status) {
        if (view_ == nullptr)
            return;

        const char* key = nullptr;
        double delay = 0;

        switch (status) {
        case MoraleStatus::Heroism: key = "MST_HeroicModeClass", delay = 1; break;
        case MoraleStatus::HightMorale: key = "MST_HighMoraleClass", delay = 1; break;
        case MoraleStatus::LowMorale: key = "MST_LowMoraleClass", delay = 1; break;
        case MoraleStatus::Feared: key = "MST_FearModeClass"; break;
        case MoraleStatus::Neutral: return;
        }

        morale_ = addAnimation(key, delay);
    }

    void SquadIcon::showLevel(int level) {
        if (view_ == nullptr)
            return;

        if (auto* stars = symbols::child(*round_, "_-Rk"))
            stars->gotoAndStop(level + 1);

        const char* key = nullptr;

        switch (level) {
        case 1: key = "SilverStarIconClass"; break;
        case 2: key = "GoldStarIconClass"; break;
        default: return;
        }

        star_ = addAnimation(key, 0);
    }

    void SquadIcon::removeAction() {
        if (view_ && action_) {
            view_->removeImage(*action_);
            action_->dispose();
            action_.reset();
        }
    }

    void SquadIcon::removeMorale() {
        if (view_ && morale_) {
            view_->removeImage(*morale_);
            morale_->dispose();
            morale_.reset();
        }
    }

    void SquadIcon::removeLevel() {
        if (view_ && star_) {
            view_->removeImage(*star_);
            star_->dispose();
            star_.reset();
        }
    }

    void SquadIcon::removeAll() {
        removeAction();
        removeMorale();
        removeLevel();

        if (view_) {
            view_->dispose();
            view_.reset();
        }
    }

    void SquadIcon::setActionStatus(std::optional<SquadState> status) {
        removeAction();
        showAction(status);
    }

    void SquadIcon::makelevelUP(int level) {
        removeLevel();
        showLevel(level);
    }

} // namespace takeover
