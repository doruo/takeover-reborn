#include "game/sim/level/KeyNode.h"

#include "game/sim/abilities/AbilityEffects.h"
#include "game/sim/battle/Combatant.h"
#include "game/sim/units/Unit.h"

#include <cmath>
#include <cstdio>

namespace takeover {

    const PriorityEvent KeyNode::EV_REPAIR{1000};
    const PriorityEvent KeyNode::EV_DAMAGE{1000};
    const PriorityEvent KeyNode::EV_FULL_HP{9999};
    const PriorityEvent KeyNode::EV_DESTROYED{9999};

    KeyNode::KeyNode(std::shared_ptr<NodeContentKey> contentKey, std::shared_ptr<NodeKey> nodeKey,
                     const Combatant& ownerCombatant, KeyNodeType type, const Point2D& position)
        : NodeContent(std::move(contentKey), std::move(nodeKey)), owner(ownerCombatant.description), type_(type),
          position_(position) {
        initByType(type_, ownerCombatant);
        initFSM();
        initLinks();
        lastAttackerSquadId_ = -1;
        level_ = 0;
    }

    void KeyNode::onTargetedBy(Unit& attacker) { lastAttackerSquadId_ = attacker.getSquadId(); }

    void KeyNode::onMeleeHitBy(Unit& attacker) {
        addDamage(attacker.getDamageForFortress());
        lastAttackerSquadId_ = attacker.getSquadId();
    }

    void KeyNode::onMissileHitBy(Unit& attacker) {
        addDamage(attacker.getDamageForFortress());
        lastAttackerSquadId_ = attacker.getSquadId();
    }

    void KeyNode::updateFSM(double dt) {
        fsm_->getState()->update(dt);

        if (pendingEvent_ != nullptr) {
            const PriorityEvent* event = pendingEvent_;
            pendingEvent_ = nullptr;
            fsm_->handleEvent(*event);
        }

        pendingDamage_ = 0;
        pendingRepair_ = 0;
    }

    void KeyNode::setLevel(int level) {
        const int oldMax = getMaxHP();
        level_ = level;
        const int newMax = getMaxHP();
        setHP(getHP() + newMax - oldMax);
    }

    void KeyNode::setHP(int hp) {
        if (hp_ == hp)
            return;

        if (hp <= 0) {
            hp_ = 0;
            postEvent(EV_DESTROYED);
        } else if (hp >= getMaxHP()) {
            hp_ = getMaxHP();
            postEvent(EV_FULL_HP);
        } else {
            hp_ = hp;
        }
    }

    void KeyNode::initLinks() {
        links_ = {};
        attackers.clear();
        defenders.clear();
        aiDefendersWanted = 1;
    }

    void KeyNode::initFSM() {
        stateActive_ = std::make_shared<KeyNodeState>(this);
        stateRuined_ = std::make_shared<KeyNodeState>(this);
        stateRebuilding_ = std::make_shared<KeyNodeState>(this);
        pendingEvent_ = nullptr;
        pendingDamage_ = 0;
        pendingRepair_ = 0;
        const auto call = [](std::function<void()> f) { return std::make_shared<CallbackAction>(std::move(f)); };
        const auto nothing = std::make_shared<NoAction>();
        fsm_ = std::make_unique<StateMachine>(stateActive_);
        fsm_->addTransition({stateActive_, &EV_REPAIR, stateActive_, call([this] { applyRepair(); })});
        fsm_->addTransition({stateActive_, &EV_DAMAGE, stateActive_, call([this] { applyDamage(); })});
        fsm_->addTransition({stateActive_, &EV_DESTROYED, stateRuined_, call([this] { onRuined(); })});
        fsm_->addTransition({stateRuined_, &EV_REPAIR, stateRebuilding_, nothing});
        fsm_->addTransition({stateRebuilding_, &EV_REPAIR, stateRebuilding_, call([this] { applyRepair(); })});
        fsm_->addTransition({stateRebuilding_, &EV_DAMAGE, stateRebuilding_, call([this] { applyDamage(); })});
        fsm_->addTransition({stateRebuilding_, &EV_DESTROYED, stateRuined_, nothing});
        fsm_->addTransition({stateRebuilding_, &EV_FULL_HP, stateActive_, call([this] { onRebuilt(); })});
    }

    void KeyNode::initByType(KeyNodeType type, const Combatant& combatant) {
        const int towerBonus = combatant.bonuses.value(Bonus::MAGE_TOWERS_BONUS);

        switch (type) {
        case KeyNodeType::Town:
            goldIncome = 2;
            manaIncome = 0;
            supplyIncome = 1;
            baseHP_ = 500;
            name = "TOWN";
            recruitableTypes = {0, 1};
            break;
        case KeyNodeType::Castle:
            goldIncome = 3;
            manaIncome = 1;
            supplyIncome = 2;
            baseHP_ = 1000;
            name = "CASTLE";
            recruitableTypes = {0, 1, 2, 3};
            break;
        case KeyNodeType::Camp:
            goldIncome = 1;
            manaIncome = 0;
            supplyIncome = 0;
            baseHP_ = 500;
            name = "CAMP";
            recruitableTypes = {0, 1};
            break;
        case KeyNodeType::MagicTower:
            goldIncome = 1 + towerBonus;
            manaIncome = 2 + towerBonus;
            supplyIncome = 1 + towerBonus;
            baseHP_ = 500;
            name = "MAGIC TOWER";
            ultimateIncome = 1;
            recruitableTypes = {0, 4};
            break;
        }

        baseHP_ = static_cast<int>(std::round(baseHP_ * combatant.bonuses.factor(Bonus::STRONGHOLDS_HP)));
        hp_ = baseHP_;
    }

    int KeyNode::getUpgradeCost(int level) const {
        switch (level) {
        case 0: return 350;
        case 1: return 500;
        default: return -1;
        }
    }

    int KeyNode::getMaxHP() const { return static_cast<int>(std::round(baseHP_ + baseHP_ * level_ * 0.5)); }

    void KeyNode::disposeLinks() {
        links_.dispose();
        attackers.clear();
        defenders.clear();
        recruitableTypes.clear();
    }

    void KeyNode::disposeFSM() {
        pendingEvent_ = nullptr;

        for (auto* state : {&stateActive_, &stateRuined_, &stateRebuilding_}) {
            if (*state) {
                (*state)->dispose();
                state->reset();
            }
        }

        if (fsm_)
            fsm_->dispose();
    }

    void KeyNode::damagePercent(int percent) { addDamage(PercentModifier::percentOf(getMaxHP(), percent)); }

    void KeyNode::addDamage(int damage) {
        if (postEvent(EV_DAMAGE))
            pendingDamage_ += damage;
    }

    void KeyNode::customUpdate(double dt) {
        stateChanged_ = false;
        updateFSM(dt);

        // The AI likes strongholds under attack, close enemy strongholds and
        // enemies with fewer defenders.
        threat_ = static_cast<double>(attackers.size()) * 20;

        for (const auto& [index, link] : links_.objects()) {
            if (!owner->compare(*link->keyNode->owner))
                threat_ += 10000 / link->dist +
                           10 * (static_cast<int>(link->keyNode->defenders.size()) - static_cast<int>(defenders.size()));
        }

        lastAttackerSquadId_ = -1;
        NodeContent::customUpdate(dt);
    }

    void KeyNode::customDispose() {
        disposeLinks();
        disposeFSM();
        NodeContent::customDispose();
    }

    void KeyNode::onRuined() {
        stateChanged_ = true;
        changeViewToRize();
    }

    void KeyNode::onRebuilt() {
        stateChanged_ = true;
        changeViewToActive();
    }

    bool KeyNode::postEvent(const PriorityEvent& event) {
        if (pendingEvent_ == nullptr || event.comparePriority(*pendingEvent_) > 0) {
            pendingEvent_ = &event;
            return true;
        }

        return false;
    }

    void KeyNode::activate(int repair) {
        if (postEvent(EV_REPAIR))
            pendingRepair_ += repair;
    }

} // namespace takeover
