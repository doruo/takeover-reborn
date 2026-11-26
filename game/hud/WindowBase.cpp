#include "game/hud/WindowBase.h"

#include "game/framework/Symbols.h"
#include "game/hud/GamePanel.h"
#include "game/level/Level.h"
#include "game/sim/battle/Combatant.h"
#include "game/squads/SquadIcon.h"

#include "engine/display/MovieClip.h"
#include "engine/display/TextField.h"

namespace takeover {

    namespace {

        std::shared_ptr<engine::MovieClip> shared(engine::DisplayObject* object) {
            return object ?
                std::dynamic_pointer_cast<engine::MovieClip>
                (object->shared_from_this()) : nullptr;
        }

        /// Nothing under the mouse in this part of the ring.
        void ignoreMouse(engine::DisplayObject* object) {

            if (object == nullptr)
                return;

            object->setMouseEnabled(false);

            if (auto* container = dynamic_cast<engine::DisplayObjectContainer*>(object))
                container->setMouseChildren(false);
        }

    } // namespace

    void showCenterPanel(engine::MovieClip& panel, const CenterPanelDef& def) {

        panel.gotoAndStop(def.statsOnRightSide ? "lbRight" : "lbLeft");

        if (auto* ring = symbols::child(panel, "mcRing"))
            ring->gotoAndStop(def.frame);

        if (auto* caption = symbols::child<engine::TextField>(panel, "txtCaption"))
            caption->setText(def.caption);

        const auto stat =
        [&](int value, const char* text, const char* icon, const char* stripe) {

            if (value > 0) {
                symbols::setHtmlText(panel, text, "<FONT>+" + std::to_string(value) + "</FONT>");
            }
            else {
                for (const char* name : {text, icon, stripe}) {
                    if (
                        auto* part =
                        symbols::child<engine::DisplayObject>(panel, name)
                    ) {
                        part->setVisible(false);
                    }
                }
            }

            for (const char* name : {text, icon, stripe})
                ignoreMouse(symbols::child<engine::DisplayObject>(panel, name));
        };

        // Gold income text
        stat(def.goldIncome, "txtGold", "mcGold", "mcStripeGold");

        // Supply income text
        stat(def.supplyIncome, "txtUpkeep", "mcUpkeep", "mcStripeGreen");

        // Mana income text
        stat(def.manaIncome, "txtMana", "mcMana", "mcStripeBlue");
    }

    // /----- WINDOW CONTENT MANAGER -----/

    Combatant* WindowContentManager::owner() { return level_->getLocationOwner(*node_->nodeDescription); }

    std::shared_ptr<engine::DisplayObject> WindowContentManager::unitIcon(int buttonId) {
        Combatant* combatant = owner();
        const UnitType type = unit_types::fromInt(buttonId);

        std::shared_ptr<engine::MovieClip> icon =
            symbols::create(
                SquadIcon::unitIconClass(
                    type,
                    combatant->getRace(),
                    combatant->getTier(type)
                )
            );

        icon->setScaleX(1.5);
        icon->setScaleY(1.5);

        return icon;
    }

    double WindowContentManager::ultimateFactor() {
        auto* node = dynamic_cast<KeyNode*>(node_);
        const bool active = node == nullptr || node->isActive();

        return active ? owner()->getUSpeelFactor() : 0;
    }

    int WindowContentManager::price(int buttonId) {

        Combatant* combatant = owner();

        switch (buttonId) {

            case WindowBase::BUT_ID_UNIT_0:

            case WindowBase::BUT_ID_UNIT_1:

            case WindowBase::BUT_ID_UNIT_2:

            case WindowBase::BUT_ID_UNIT_3:

            case WindowBase::BUT_ID_UNIT_4:
                return combatant->getUnitCost(*combatant->getArmyStat(unit_types::fromInt(buttonId)));

            case WindowBase::BUT_ID_UPGRADE: {
                auto* node = dynamic_cast<KeyNode*>(node_);
                return node != nullptr ?
                    node->getUpgradeCost(node->getLevel()) : -1;
            }

            case WindowBase::BUT_ID_BUILD_FARM:
                return combatant->getSupplyCost(SupplyBuildingType::Village);

            case WindowBase::BUT_ID_BUILD_MINE:
                return combatant->getSupplyCost(SupplyBuildingType::Mine);

            case WindowBase::BUT_ID_BUILD_OBELISK:
                return combatant->getSupplyCost(SupplyBuildingType::Obelisk);

            default:
                return -1;
        }
    }

    void WindowContentManager::refreshButtons(WindowBase& window) {

        Combatant* combatant = owner();
        auto* node = dynamic_cast<KeyNode*>(node_);
        const bool active = node == nullptr || node->isActive();

        for (const auto& button : window.buttons()) {

            const int id = button->id();

            switch (id) {

                case WindowBase::BUT_ID_UNIT_0:

                case WindowBase::BUT_ID_UNIT_1:

                case WindowBase::BUT_ID_UNIT_2:

                case WindowBase::BUT_ID_UNIT_3:

                case WindowBase::BUT_ID_UNIT_4:
                    button->setEnabled(
                        id <= LOCK_UNIT_ABOVE
                        && combatant->canBuy(
                            *combatant->getArmyStat(unit_types::fromInt(id))
                        ) && active
                    );
                    break;

                case WindowBase::BUT_ID_UPGRADE: {

                    const int cost =
                        node != nullptr ?
                        node->getUpgradeCost(node->getLevel()) : -1;

                    button->setEnabled(
                        combatant->getGold() >= cost && cost > 0 && active
                    );

                    window.showLocked(*button, node->isMaxLevel());
                    break;
                }

                case WindowBase::BUT_ID_BUILD_FARM:
                    button->setEnabled(
                        combatant->getGold() >=
                        combatant->getSupplyCost(SupplyBuildingType::Village)
                        && UNLOCK_VILAGE
                    );
                    break;

                case WindowBase::BUT_ID_BUILD_MINE:
                    button->setEnabled(
                        combatant->getGold() >=
                        combatant->getSupplyCost(SupplyBuildingType::Mine)
                        && UNLOCK_MINE
                    );
                    break;

                case WindowBase::BUT_ID_BUILD_OBELISK:
                    button->setEnabled(
                        combatant->getGold() >=
                        combatant->getSupplyCost(SupplyBuildingType::Obelisk)
                        && UNLOCK_OBELISK
                    );
                    break;

                default:
                    break;
            }
        }
    }

    // /----- WINDOW BASE -----/

    WindowBase::WindowBase(WindowDef def) : def_(std::move(def)) {
        window_ = symbols::create(def_.windowClass);
        window_->setX(def_.position.x);
        window_->setY(def_.position.y);
        placeWindow();
        decorate();
    }

    void WindowBase::customDispose() {

        for (auto& button : buttons_) {
            button->dispose();
        }

        buttons_.clear();

        if (ultimate_)
            ultimate_->dispose();

        ultimate_.reset();

        signalAction.dispose();

        if (window_ && window_->parent() != nullptr)
            window_->parent()->removeChild(window_.get());

        window_.reset();

        if (def_.content)
            def_.content->dispose();

        def_.content.reset();

        UpdatableObject::customDispose();
    }

    void WindowBase::showGold(engine::MovieClip* clip, int gold) {
        if (clip != nullptr) {
            symbols::setHtmlText(
        *clip,
            "txtGold",
            std::to_string(gold)
            );
        }
    }

    void WindowBase::placeWindow() {
        std::size_t i = 0;

        for (; i + 1 < def_.positions.size(); ++i) {

            if (window_->x() < def_.positions[i].second) {

                window_->gotoAndStop(
                    def_.positions[i].first
                );
                return;
            }
        }

        window_->gotoAndStop(def_.positions[i].first);
    }

    void WindowBase::decorate() {

        if (auto* color = symbols::find(*window_, "mcPanel.mcColor"))
            color->gotoAndStop(def_.panelColorFrame);

        if (auto* center = symbols::child(*window_, "mcCenter"))
            showCenterPanel(*center, def_.center);

        if (auto* caption =
            symbols::child<engine::DisplayObjectContainer>(*window_, "txtCaption")) {

            if (
                auto* text = symbols::child<engine::TextField>(*caption, "text")
            ) {
                text->setHtmlText(def_.text);
                text->setMouseEnabled(false);
            }

            caption->setMouseEnabled(false);
        }

        const std::pair<const char*, const char*> titles[] = {
            {"txtUnits", "UNITS"},
            {"txtUpgrade", "UPGRADE"},
            {"txtMagic", "MAGIC"},
            {"txtBuild", "BUILD"},
            {"txtDestroy", "DESTROY"}
        };

        for (const auto& [name, title] : titles) {
            if (auto* block =
                symbols::child<engine::DisplayObject>(*window_, name)) {

                symbols::setHtmlText(
                    *block, "text", title
                );
            }
        }
    }

    Button& WindowBase::addButton(engine::MovieClip* movie, int id) {

        auto button = std::make_shared<Button>(
            ButtonDef::make(
                shared(movie), std::nullopt, id
            )
        );

        button->signalClick.addListener(this, &WindowBase::onButtonClick);

        buttons_.push_back(button);
        return *button;
    }

    void WindowBase::onButtonClick(Button* button) { signalAction.dispatch(button->id()); }

    void WindowBase::refresh() {

        if (ultimate_)
            ultimate_->setReadyFactor(def_.content->ultimateFactor());

        if (def_.content)
            def_.content->refreshButtons(*this);
    }

    Button& WindowBase::addUnitButton(int slot, int typeId) {

        engine::MovieClip* image = symbols::child(*window_, "butUnit" + std::to_string(slot));

        Button& button = addButton(image, typeId);

        showGold(
            symbols::child(*image, "mcGold"),
            def_.content->price(typeId)
        );

        if (auto* back = symbols::child(*image, "mcBack")) {
            back->gotoAndStop(def_.panelColorFrame);
            back->addChild(def_.content->unitIcon(typeId));
        }

        return button;
    }

    void WindowBase::addUpgradeButton() {
        engine::MovieClip* image = symbols::child(*window_, "butUpgrade");

        addButton(image, BUT_ID_UPGRADE);
        showGold(
            symbols::child(
                *image,
                "mcGoldBlock"
            ),
            def_.content->price(BUT_ID_UPGRADE)
        );
    }

    void WindowBase::showLocked(Button& button, bool locked) {
        if (auto* message =
            symbols::child<engine::DisplayObject>(*button.image(), "mcLockMessage")) {
            message->setVisible(locked);
        }

        if (auto* gold =
            symbols::child<engine::DisplayObject>(*button.image(), "mcGoldBlock")) {
            gold->setVisible(!locked);
        }
    }

    USpellButton& WindowBase::addUltimateButton() {

        ultimate_ = std::make_shared<USpellButton>(
            ButtonDef::make(
                shared(symbols::child(*window_, "butSpell")),
                std::nullopt, BUT_ID_ULTIMATE_SPELL
            )
        );

        ultimate_->signalClick.addListener(
            [this](Button*) { signalAction.dispatch(BUT_ID_ULTIMATE_SPELL); }
        );

        return *ultimate_;
    }

    Button& WindowBase::addBuildButton(int slot, int typeId) {

        engine::MovieClip* image = symbols::child(
            *window_,
            "butBuild" + std::to_string(slot)
        );

        Button& button = addButton(image, typeId);

        showGold(
            symbols::child(*image, "mcGoldBlock"),
            def_.content->price(typeId)
        );

        return button;
    }

    Button& WindowBase::addDestroyButton() {
        return addButton(
            symbols::child(
                *window_,
                "butDestroy"
            ),
            BUT_ID_DESTROY
        );
    }

} // namespace takeover
