#include "game/gui/ThroneRoomScreen.h"

#include "game/framework/Symbols.h"
#include "game/framework/ui/HintWindow.h"
#include "game/hud/GamePanel.h"
#include "game/hud/WindowController.h"
#include "game/level/Level.h"
#include "game/progress/Profile.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/sim/battle/Combatant.h"
#include "game/sim/spells/SpellInfo.h"
#include "game/squads/SquadIcon.h"

namespace takeover {

    ThroneRoomScreen::ThroneRoomScreen(std::shared_ptr<GameScreenDef> def) : profile_(def->profile), hintLayer_(def->hintLayer) {
        def->viewClass = "scrThroneRoomClass";
        initScreen(std::move(def));
        createIcons();
    }

    void ThroneRoomScreen::customDispose() {
        disposeIcons();
        GameScreen::customDispose();
    }

    void ThroneRoomScreen::initVisualBlock() {
        GameScreen::initVisualBlock();
        static_cast<engine::MovieClip*>(image_.get())->gotoAndStop(profile_->selectedCompany().info().number() + 1);
        statPanel_ = std::make_unique<PlayerStatPanel>(clip("_-dj"), *profile_);
    }

    void ThroneRoomScreen::disposeVisualBlock() {
        statPanel_.reset();
        GameScreen::disposeVisualBlock();
    }

    void ThroneRoomScreen::initButtonsBlock() {
        GameScreen::initButtonsBlock();
        addButton("butOk", "OK", BUT_OK);
        resetButton_ = addButton("_-gu", "RESET", BUT_UNDO); // _-gu: the reset button (_SafeStr_2941)
    }

    void ThroneRoomScreen::disposeButtonsBlock() {
        resetButton_ = nullptr;
        GameScreen::disposeButtonsBlock();
    }

    void ThroneRoomScreen::onButtonClick(Button* button) {
        if (button->id() == BUT_UNDO) {
            edicts().reset();
            refresh();
        }

        GameScreen::onButtonClick(button);
    }

    EdictTree& ThroneRoomScreen::edicts() const { return profile_->selectedCompany().edicts(); }

    void ThroneRoomScreen::createIcons() {
        const Race race = profile_->selectedCompany().info().getRace();
        int j = 0;

        for (auto& row : edicts().getRows()) {
            for (auto& edict : row) {
                auto icon = std::make_shared<EdictIcon>(edict, clip("mcIcon" + std::to_string(j)),
                                                        j + 1 + profile_->selectedCompany().info().id() * 9);
                icon->signalClick.addListener(this, &ThroneRoomScreen::onEdictClick);
                edictIcons_.push_back(icon);
                ++j;
            }
        }

        for (int type = 0; type < unit_types::COUNT; ++type) {
            unitButtons_.emplace_back();

            for (int tier = 0; tier < 3; ++tier) {
                engine::MovieClip* icon = clip("mcUnitIcon_" + std::to_string(type) + "T" + std::to_string(tier));

                if (const ArmyStat* stat = ArmyStat::find(race, unit_types::fromInt(type), tier + 1))
                    unitButtons_.back().push_back(createUnitButton(icon, *stat, type * 10 + tier));
                else
                    icon->setVisible(false);
            }
        }

        const auto spells = Combatant::raceSpells(race);

        for (int i = 0; i < 3; ++i) {
            auto iconSet = symbols::create("SpellIconSetClass");
            iconSet->gotoAndStop(spells[static_cast<std::size_t>(i)]->id + 1);
            auto button = std::make_shared<Button>(ButtonDef::make(sharedClip(clip("mcSpellIcon" + std::to_string(i))), std::nullopt, i));
            symbols::find(*button->image(), "mcIconSet.mcColor")->addChild(iconSet);
            button->setReadOnly(true);
            button->signalRollOut.addListener(this, &ThroneRoomScreen::hideHint);
            button->signalRollOver.addListener(this, &ThroneRoomScreen::onSpellRollOver);
            spellButtons_.push_back(button);
        }

        refresh();

        auto unitHint = std::make_shared<HintWindow2>(symbols::create("HintUnit"));
        unitHint->dockSide = DockSide::Bottom;
        unitHint->dockAlign = DockAlign::LeftTop;
        unitHint->setAutoWidth(false);
        unitHintId_ = hintLayer_->addHintWindow(unitHint);
        auto spellHint = std::make_shared<HintWindow2>(symbols::create("HintSpell"));
        spellHint->dockSide = DockSide::Bottom;
        spellHint->dockAlign = DockAlign::LeftTop;
        spellHint->setAutoWidth(false);
        spellHintId_ = hintLayer_->addHintWindow(spellHint);
    }

    void ThroneRoomScreen::disposeIcons() {
        for (auto& icon : edictIcons_)
            icon->dispose();

        edictIcons_.clear();

        for (auto& row : unitButtons_)
            for (auto& button : row)
                button->dispose();
        unitButtons_.clear();

        for (auto& button : spellButtons_)
            button->dispose();

        spellButtons_.clear();
    }

    void ThroneRoomScreen::refresh() {
        const int edictCount = edicts().totalPoints();
        const int freeCount = profile_->selectedCompany().freeSkillPoints();

        for (auto& icon : edictIcons_)
            icon->updateIconView(edictCount);

        symbols::setHtmlText(*image_, "txtEdictsNum", std::to_string(edictCount));
        symbols::setHtmlText(*image_, "_-Xg", std::to_string(freeCount)); // _-Xg: free points (_SafeStr_2940)
        resetButton_->setEnabled(edictCount != 0);
        statPanel_->refresh();
        refreshUnlocks();
    }

    void ThroneRoomScreen::onEdictClick(EdictIcon* icon) {
        if (profile_->selectedCompany().freeSkillPoints() > 0) {
            icon->edict().addPoint();
            refresh();
            profile_->selectedCompany().updateEdictsBadge(profile_->badges());
        }
    }

    std::shared_ptr<Button> ThroneRoomScreen::createUnitButton(engine::MovieClip* icon, const ArmyStat& stat, int id) {
        auto button = std::make_shared<Button>(ButtonDef::make(sharedClip(icon), std::nullopt, id));
        button->signalRollOver.addListener(this, &ThroneRoomScreen::onUnitRollOver);
        button->signalRollOut.addListener(this, &ThroneRoomScreen::hideHint);
        button->setReadOnly(true);
        engine::MovieClip* colorSet = symbols::find(*icon, "mcColor.mcColorSet");
        colorSet->gotoAndStop(SquadIcon::raceColorFrame(stat.side));
        colorSet->addChild(symbols::create(SquadIcon::unitIconClass(stat.unitType, stat.side, stat.techLevel)));

        return button;
    }

    void ThroneRoomScreen::setGrayed(bool enabled, engine::DisplayObject* target) {
        std::vector<swf::Filter> filters;

        if (!enabled) {
            // Desaturate: every channel becomes the average of the three.
            const float third = 1.0f / 3.0f;
            swf::ColorMatrixFilter gray;
            gray.matrix = {third, third, third, 0, 0, third, third, third, 0, 0, third, third, third, 0, 0, 0, 0, 0, 1, 0};
            filters.emplace_back(gray);
        }

        target->setFilters(std::move(filters));
    }

    void ThroneRoomScreen::refreshUnlocks() {
        const EdictBonuses bonuses = edicts().buildBonuses();

        // Tier bonus of each unit type, in unit type order.
        const int tierBonus[] = {Bonus::INFANTRY_TIER, Bonus::ARCHERS_TIER, Bonus::CAVALRY_TIER, Bonus::SIEGE_TIER,
                                 Bonus::MAGIC_TIER};
        for (std::size_t type = 0; type < unitButtons_.size(); ++type)
            for (std::size_t tier = 0; tier < unitButtons_[type].size(); ++tier)
                setGrayed(bonuses.value(tierBonus[type]) >= static_cast<int>(tier),
                          symbols::find(*unitButtons_[type][tier]->image(), "mcColor.mcColorSet"));
        const auto spells = Combatant::raceSpells(profile_->selectedCompany().info().getRace());

        for (std::size_t i = 0; i < spellButtons_.size(); ++i)
            setGrayed(Combatant::spellUnlockValue(spells[i], bonuses) > 0,
                      symbols::find(*spellButtons_[i]->image(), "mcIconSet.mcColor"));
    }

    void ThroneRoomScreen::onUnitRollOver(Button* button) {
        const int type = button->id() / 10;
        const int tier = button->id() % 10 + 1;
        const Race race = profile_->selectedCompany().info().getRace();
        const ArmyStat* stat = ArmyStat::find(race, unit_types::fromInt(type), tier);
        const EdictBonuses bonuses = edicts().buildBonuses();
        WindowController::fillUnitHint(*stat, Combatant::makeArmyStatBonus(*stat, bonuses),
                                       *hintLayer_->getHintWindow(unitHintId_));
        hintLayer_->showHintAtObject(button->image(), unitHintId_);
    }

    void ThroneRoomScreen::onSpellRollOver(Button* button) {
        const Race race = profile_->selectedCompany().info().getRace();
        const EdictBonuses bonuses = edicts().buildBonuses();
        const Spell spell(*Level::spellInfo(button->id(), race), &bonuses);
        GamePanel::fillSpellHint(spell, *hintLayer_->getHintWindow(spellHintId_));
        hintLayer_->showHintAtObject(button->image(), spellHintId_);
    }

    void ThroneRoomScreen::hideHint(Button*) { hintLayer_->hideHint(); }

} // namespace takeover
