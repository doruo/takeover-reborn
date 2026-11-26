#include "game/sim/level/LevelAi.h"

#include "game/framework/core/As3.h"
#include "game/sim/battle/ArmyStat.h"
#include "game/sim/spells/SpellInfo.h"
#include "game/sim/units/Unit.h"

#include <cmath>

namespace takeover {

    void LevelAi::customUpdate(double dt) {
        // getActiveTime() already includes dt: this runs on the frame before a
        // new second starts, once per second.
        if (std::floor(getActiveTime() + dt) > getActiveTime()) {
            updateBaseCombotantLogic(dt);

            if (!aiDisabled)
                updateCpuCombotantLogic(dt);

            LevelLogic::customUpdate(dt);
        }
    }

    bool LevelAi::updateCpuCombotantLogic(double dt) {
        forEachObject(combatants_, [&](const std::shared_ptr<Combatant>& combatant) {
            if (combatant->description->compare(*playerKey_))
                return;

            sendSurplusDefenders(*combatant);

            if (!aiBuildDisabled)
                buildSupply(*combatant);

            if (combatant->getFreeSupply() > 0 && combatant->getGold() >= 180)
                recruit(*combatant);

            castSpells(*combatant, dt);
            tryUltimate(*combatant);
        });

        return true;
    }

    bool LevelAi::updateBaseCombotantLogic(double /*dt*/) {
        forEachObject(combatants_, [](const std::shared_ptr<Combatant>& combatant) { combatant->tick(); });
        return true;
    }

    // /----- SPELLS -----/

    std::shared_ptr<Spell> LevelAi::targetEnemyNodeOfFightingSquad(const Spell& spell, Combatant& combatant) {
        for (const auto& [index, squad] : squads_.objects()) {
            if (!combatant.isOwnSquad(*squad) || !squad->isFighting())
                continue;

            if (KeyNode* node = squad->nearestEnemyNode()) {
                auto cast = std::make_shared<Spell>();
                cast->copyFrom(spell);
                cast->position = node->getPosition();
                cast->position.y += 15;

                return cast;
            }
        }

        return nullptr;
    }

    std::shared_ptr<Spell> LevelAi::targetNowhere(const Spell& spell, Combatant& /*combatant*/) {
        auto cast = std::make_shared<Spell>();
        cast->SpellInfo::copyFrom(spell);

        return cast;
    }

    std::shared_ptr<Spell> LevelAi::targetOwnFightingSquad(const Spell& spell, Combatant& combatant) {
        Squad* chosen = nullptr;

        for (const auto& [index, squad] : squads_.objects()) {
            if (!combatant.isOwnSquad(*squad) || !squad->isFighting())
                continue;

            double weight = 0;

            for (Squad* enemy : squad->nearbyEnemySquads())
                weight += squadWeight(*enemy);

            if (0 < weight)
                chosen = squad.get();
        }

        if (chosen == nullptr)
            return nullptr;

        auto cast = std::make_shared<Spell>();
        cast->copyFrom(spell);
        cast->position = squadCentre(*chosen, cast->type == SpellInfo::TYPE_BANNER);

        return cast;
    }

    std::shared_ptr<Spell> LevelAi::targetEnemySquad(const Spell& spell, Combatant& combatant) {
        Squad* chosen = nullptr;

        for (const auto& [index, squad] : squads_.objects())
            if (squad->isAttackableBy(combatant.getRace()) && 0 < squadWeight(*squad))
                chosen = squad.get();
        if (chosen == nullptr)
            return nullptr;

        auto cast = std::make_shared<Spell>();
        cast->copyFrom(spell);
        cast->position = squadCentre(*chosen, cast->type == SpellInfo::TYPE_BANNER);

        return cast;
    }

    std::shared_ptr<Spell> LevelAi::targetEnemyFightingOwn(const Spell& spell, Combatant& combatant) {
        Squad* chosen = nullptr;

        for (const auto& [index, squad] : squads_.objects()) {
            if (!combatant.isOwnSquad(*squad) || !squad->isFighting())
                continue;

            for (Squad* enemy : squad->nearbyEnemySquads())
                if (0 < squadWeight(*enemy))
                    chosen = enemy;
        }

        if (chosen == nullptr)
            return nullptr;

        auto cast = std::make_shared<Spell>();
        cast->copyFrom(spell);
        cast->position = squadCentre(*chosen, cast->type == SpellInfo::TYPE_BANNER);

        return cast;
    }

    std::shared_ptr<Spell> LevelAi::targetBraveEnemy(const Spell& spell, Combatant& combatant) {
        Squad* chosen = nullptr;
        double bestMorale = 1.2;

        for (const auto& [index, squad] : squads_.objects()) {
            if (!combatant.isOwnSquad(*squad) || !squad->isFighting())
                continue;

            for (Squad* enemy : squad->nearbyEnemySquads()) {
                if (enemy->hpFactor() > 0.5 && enemy->isFighting()) {
                    const double morale = enemy->moraleRatio();
                    if (morale > bestMorale)
                        bestMorale = morale, chosen = enemy;
                }
            }
        }

        if (chosen == nullptr)
            return nullptr;

        auto cast = std::make_shared<Spell>();
        cast->copyFrom(spell);
        cast->position = squadCentre(*chosen, cast->type == SpellInfo::TYPE_BANNER);

        return cast;
    }

    std::shared_ptr<Spell> LevelAi::targetShakenOwn(const Spell& spell, Combatant& combatant) {
        Squad* chosen = nullptr;
        double worstMorale = 0.8;

        for (const auto& [index, squad] : squads_.objects()) {
            if (combatant.isOwnSquad(*squad) && squad->isFighting() && squad->hpFactor() > 0.5) {
                const double morale = squad->moraleRatio();
                if (morale < worstMorale)
                    worstMorale = morale, chosen = squad.get();
            }
        }

        if (chosen == nullptr)
            return nullptr;

        auto cast = std::make_shared<Spell>();
        cast->copyFrom(spell);
        cast->position = squadCentre(*chosen, cast->type == SpellInfo::TYPE_BANNER);

        return cast;
    }

    std::shared_ptr<Spell> LevelAi::targetWoundedOwn(const Spell& spell, Combatant& combatant) {
        Squad* chosen = nullptr;
        double lowest = 0.5;

        for (const auto& [index, squad] : squads_.objects()) {
            if (combatant.isOwnSquad(*squad) && squad->isFighting()) {
                const double hp = squad->hpFactor();
                if (hp < lowest && hp > 0.3)
                    lowest = hp, chosen = squad.get();
            }
        }

        if (chosen == nullptr)
            return nullptr;

        auto cast = std::make_shared<Spell>();
        cast->copyFrom(spell);
        cast->position = squadCentre(*chosen, cast->type == SpellInfo::TYPE_BANNER);

        return cast;
    }

    std::shared_ptr<Spell> LevelAi::chooseSpellTarget(const Spell& spell, Combatant& combatant) {
        const int id = spell.id;
        if (id == SpellInfo::TERROR.id)
            return targetBraveEnemy(spell, combatant);

        if (id == SpellInfo::RAISE_THE_DEAD.id)
            return targetNowhere(spell, combatant);

        if (id == SpellInfo::BANNER_OF_DESECRATION.id)
            return targetWoundedOwn(spell, combatant);

        if (id == SpellInfo::LORDS_JUDGEMENT.id)
            return targetEnemySquad(spell, combatant);

        if (id == SpellInfo::BANNER_OF_CONSECRATION.id)
            return targetWoundedOwn(spell, combatant);

        if (id == SpellInfo::BANNER_OF_HEROISM.id)
            return targetShakenOwn(spell, combatant);

        if (id == SpellInfo::ICE_BLAST.id)
            return targetEnemyFightingOwn(spell, combatant);

        if (id == SpellInfo::FROST_ENCHANTMENT.id)
            return targetOwnFightingSquad(spell, combatant);

        if (id == SpellInfo::BANNER_OF_LIBERATION.id)
            return targetOwnFightingSquad(spell, combatant);

        if (id == SpellInfo::SUMMON_STONEWORMS.id)
            return targetEnemyNodeOfFightingSquad(spell, combatant);

        if (id == SpellInfo::DISENTOMB.id)
            return targetNowhere(spell, combatant);

        if (id == SpellInfo::BANNER_OF_BLOODLUST.id)
            return targetOwnFightingSquad(spell, combatant);

        return nullptr;
    }

    void LevelAi::castSpells(Combatant& combatant, double dt) {
        combatant.aiCooldown -= dt;

        if (combatant.aiCooldown > 0)
            return;

        for (const auto& spell : combatant.getSpells()) {
            if (spell != nullptr && spell->cost <= combatant.getMana()) {
                if (const std::shared_ptr<Spell> cast = chooseSpellTarget(*spell, combatant)) {
                    castSpell(cast.get(), &combatant);
                    combatant.aiCooldown = 2;
                    break;
                }
            }
        }
    }

    // /----- ULTIMATE POWERS -----/

    void LevelAi::tryUltimate(Combatant& combatant) {
        if (combatant.getUSpeelFactor() < 1)
            return;

        switch (combatant.getRace()) {
        case Race::TheEmpire: ultimateEmpire(combatant); break;
        case Race::Westaria: ultimateWestaria(combatant); break;
        case Race::TheKhaganate: ultimateKhaganate(combatant); break;
        case Race::TheCult: ultimateCult(combatant); break;
        default: break;
        }
    }

    void LevelAi::ultimateKhaganate(Combatant& combatant) {
        int own = 0, enemies = 0;

        for (const auto& [index, squad] : squads_.objects()) {
            if (squad->getRace() == Race::Bandits)
                continue;

            if (combatant.isOwnSquad(*squad))
                ++own;
            else
                ++enemies;
        }

        if (enemies > 3 && static_cast<double>(own) / enemies < 1)
            castUltimateSpell(combatant);
    }

    void LevelAi::ultimateEmpire(Combatant& combatant) {
        int count = 0;

        for (const auto& [index, squad] : squads_.objects())
            if (squad->isAttackableBy(combatant.getRace()) && !squad->isMechanism())
                ++count;
        if (count >= 3)
            castUltimateSpell(combatant);
    }

    void LevelAi::ultimateCult(Combatant& combatant) {
        int count = 0;

        for (const auto& [index, squad] : squads_.objects())
            if (isRealEnemy(combatant, *squad))
                ++count;
        if (count > 5)
            castUltimateSpell(combatant);
    }

    bool LevelAi::isRealEnemy(const Combatant& combatant, const Squad& squad) {
        return !combatant.isOwnSquad(squad) && squad.getRace() != Race::Bandits;
    }

    // /----- TROOPS -----/

    void LevelAi::sendSurplusDefenders(Combatant& combatant) {
        // A copy: sending a squad may change the combatant's list.
        const std::vector<std::shared_ptr<KeyNode>> nodes = combatant.getKeyNodes();
        for (const auto& node : nodes) {
            if (static_cast<int>(node->defenders.size()) > node->aiDefendersWanted)
                sendFrom(*node, *node->defenders[0]);
        }
    }

    KeyNode* LevelAi::sendFrom(KeyNode& node, Squad& squad) {
        KeyNode* target = nearestTarget(node);
        if (target != nullptr)
            squad.commandGotoPoint(target->getPosition());

        return target;
    }

    KeyNode* LevelAi::nearestTarget(KeyNode& node) {
        double best = 4000000;
        KeyNode* nearest = nullptr;

        for (const auto& [index, link] : node.getLinks().objects()) {
            if (!link->keyNode->owner->compare(*node.owner) || link->keyNode->isRuined()) {
                if (link->dist < best)
                    nearest = link->keyNode, best = link->dist;
            }
        }

        return nearest;
    }

    KeyNode* LevelAi::mostThreatenedNode(Combatant& combatant) {
        double best = 0;
        KeyNode* chosen = nullptr;

        for (const auto& node : combatant.getKeyNodes()) {
            double score = 0;

            for (const auto& [index, link] : node->getLinks().objects()) {
                if (!combatant.ownsKeyNode(*link->keyNode)) {
                    score += 100000 / link->dist;
                    score += defendersHP(*link->keyNode);
                }
            }

            if (score > best)
                best = score, chosen = node.get();
        }

        return chosen;
    }

    void LevelAi::recruit(Combatant& combatant) {
        std::vector<int> wanted;
        const int count = combatant.getSquadsCount() + 1;
        for (int i = 0; i < count; ++i)
            wanted.push_back(i);

        const std::vector<int> affordable = affordableTypes(combatant, wanted);
        if (affordable.empty())
            return;

        // The stronghold with the highest (slightly random) threat that can
        // recruit one of the affordable types. Like the original, the list of
        // all wanted types is matched, not the affordable one.
        KeyNode* chosen = nullptr;
        double bestThreat = -9999;
        std::vector<int> choices;

        for (const auto& node : combatant.getKeyNodes()) {
            double threat = node->getThreat();
            threat += (0.5 - as3::random()) * threat / 3;

            if (node->isActive() && threat >= bestThreat) {
                std::vector<int> types = intersect(wanted, node->recruitableTypes);
                if (!types.empty())
                    chosen = node.get(), bestThreat = threat, choices = std::move(types);
            }
        }

        if (chosen != nullptr) {
            const int pick = static_cast<int>(std::floor(as3::random() * static_cast<double>(choices.size())));
            const UnitType type = unit_types::fromInt(choices[static_cast<std::size_t>(pick)]);
            buyUnit(combatant.getArmyStat(type), *chosen);
        }
    }

    std::vector<int> LevelAi::affordableTypes(Combatant& combatant, const std::vector<int>& wanted) {
        std::vector<int> result;

        for (const int value : wanted) {
            const ArmyStat* stat = combatant.getArmyStat(unit_types::fromInt(value));
            if (combatant.canBuy(*stat))
                result.push_back(value);
        }

        return result;
    }

    std::vector<int> LevelAi::intersect(const std::vector<int>& a, const std::vector<int>& b) {
        std::vector<int> result;

        for (const int x : a) {
            for (const int y : b) {
                if (x == y) {
                    result.push_back(x);
                    break;
                }
            }
        }

        return result;
    }

    void LevelAi::sendScoutsHome() {
        forEachObject(squads_, [&](const std::shared_ptr<Squad>& squad) {
            if (squad->getOwner() != nullptr && playerKey_ != squad->getOwner() && squad->getRole() == SquadRole::Scout &&
                squad->isIdle()) {
                KeyNode* node = nearestOwnNode(*combatants_.getObject(squad->getOwner().get()), squad->getMainUnit()->getPosition());
                if (node != nullptr)
                    squad->commandGotoPoint(node->getPosition());
            }
        });
    }

    // /----- BUILDINGS -----/

    void LevelAi::buildSupply(Combatant& combatant) {
        if (combatant.getFreeSupply() <= 0 && combatant.getGold() >= combatant.getSupplyCost(SupplyBuildingType::Village))
            buildSupplyOfType(combatant, SupplyBuildingType::Village);

        const int goldSpeed = combatant.getGoldSpeed();
        const int nodes = static_cast<int>(combatant.getKeyNodes().size());
        if (combatant.getManaSpeed() < static_cast<int>(std::round(nodes / 3.0)))
            buildSupplyOfType(combatant, SupplyBuildingType::Obelisk);

        if (goldSpeed < 2 && nodes > goldSpeed)
            buildSupplyOfType(combatant, SupplyBuildingType::Mine);
    }

    void LevelAi::buildSupplyOfType(Combatant& combatant, SupplyBuildingType type) {
        const std::vector<std::shared_ptr<KeyNode>> nodes = combatant.getKeyNodes();
        for (const auto& node : nodes) {
            const int location = graphNodes_.getObject(node->nodeDescription.get())->locationNum;
            for (NodeContent* place : buildPlacesAt(location))
                if (tryBuySupply(*place, combatant, type))
                    return;
        }
    }

    // /----- HELPERS -----/

    Point2D LevelAi::squadCentre(Squad& squad, bool findEmptySpot) {
        // The main soldier counts twice, as in the original.
        Point2D centre = squad.getMainUnit()->getPosition();
        int count = 1;

        for (const auto& [index, unit] : squad.getUnits()) {
            if (!unit->isDestroyed()) {
                centre.add(unit->getPosition());
                ++count;
            }
        }

        centre.x /= count;
        centre.y /= count;

        if (findEmptySpot && !checkEmptyPoinPos(centre)) {
            // Spiral around the centre (the angle formula is the original's).
            const Point2D origin = centre.getCopy();

            for (int ring = 1; ring < 25; ++ring) {
                for (int step = 0; step < 7; ++step) {
                    centre.x = origin.x + static_cast<int>(std::cos(45 * step / M_PI) * ring * 2);
                    centre.y = origin.y + static_cast<int>(std::sin(45 * step / M_PI) * ring * 2);

                    if (checkEmptyPoinPos(centre))
                        return centre;
                }
            }

            centre = squad.getMainUnit()->getPosition();
        }

        return centre;
    }

    Point2D LevelAi::exitPoint(KeyNode& node) {
        const auto& links = node.getLinks().objects();
        if (!links.empty())
            return (*links.begin()->second->path)[3].getCopy();

        return node.getPosition();
    }

    double LevelAi::squadWeight(const Squad& squad) {
        double weight = squad.getHP();

        if (squad.isFighting())
            weight *= 10;

        return weight;
    }

    int LevelAi::defendersHP(const KeyNode& node) {
        int total = 0;

        for (const Squad* squad : node.defenders)
            total += squad->getHP();

        return total;
    }

} // namespace takeover
