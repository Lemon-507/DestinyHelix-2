#include "effectresolver.h"

#include <QtCore/QDebug>

#include "battle/data/card_database.h"
#include "battle/entities/actor.h"
#include "battle/entities/card.h"
#include "battle/entities/player.h"
#include "battle/entities/power.h"
#include "battle/logic/battlefsm.h"
#include "battle/logic/power_factory.h"

DamageCommand::DamageCommand(Actor *source, Actor *target, int baseDamage)
    : m_source(source)
    , m_target(target)
    , m_baseDamage(baseDamage)
{
}

void DamageCommand::execute()
{
    if (m_source == nullptr || m_target == nullptr) {
        return;
    }

    int baseDamage = m_baseDamage;
    if (auto *player = qobject_cast<Player *>(m_source)) {
        if (player->consumePenNibDoubleDamage()) {
            baseDamage *= 2;
        }
    }

    const int finalDamage = m_source->calculateOutgoingDamage(baseDamage);
    m_target->takeDamage(finalDamage, m_source);
}

GainBlockCommand::GainBlockCommand(Actor *target, int amount)
    : m_target(target)
    , m_amount(amount)
{
}

void GainBlockCommand::execute()
{
    if (m_target == nullptr || m_amount <= 0) {
        return;
    }

    m_target->gainBlock(m_amount);
}

HealCommand::HealCommand(Actor *target, int amount)
    : m_target(target)
    , m_amount(amount)
{
}

void HealCommand::execute()
{
    if (m_target == nullptr || m_amount <= 0) {
        return;
    }

    m_target->heal(m_amount);
}

ApplyPowerCommand::ApplyPowerCommand(Actor *target, std::unique_ptr<Power> power)
    : m_target(target)
    , m_power(std::move(power))
{
}

void ApplyPowerCommand::execute()
{
    if (m_target == nullptr || !m_power) {
        return;
    }

    m_target->addPower(std::move(m_power));
}

namespace {

void queueDamage(std::vector<std::unique_ptr<ICommand>> &commands, Actor *caster, Actor *target,
                 int amount, const QVector<Actor *> &allEnemies, const QString &effectTarget)
{
    if (effectTarget == QStringLiteral("all_enemies")) {
        for (Actor *enemy : allEnemies) {
            if (enemy != nullptr) {
                commands.push_back(std::make_unique<DamageCommand>(caster, enemy, amount));
            }
        }
        if (allEnemies.isEmpty() && target != nullptr) {
            commands.push_back(std::make_unique<DamageCommand>(caster, target, amount));
        }
        return;
    }

    Actor *damageTarget = target;
    if (effectTarget == QStringLiteral("self")) {
        damageTarget = caster;
    } else if (effectTarget == QStringLiteral("attacker")) {
        damageTarget = target;
    }

    if (damageTarget != nullptr) {
        commands.push_back(std::make_unique<DamageCommand>(caster, damageTarget, amount));
    }
}

void resolveApplyPower(std::vector<std::unique_ptr<ICommand>> &commands, const EffectDef &effect,
                       Actor *caster, Actor *target, const QVector<Actor *> &allEnemies)
{
    QString powerId = effect.powerId;
    if (powerId.isEmpty() && effect.target != QStringLiteral("self")
        && effect.target != QStringLiteral("attacker")
        && effect.target != QStringLiteral("all_enemies")
        && effect.target != QStringLiteral("selected_enemy")) {
        powerId = effect.target;
    }

    if (powerId.isEmpty()) {
        qWarning() << "EffectResolver: apply_power missing power id";
        return;
    }

    // Buff definitions must be loaded before stacking-mode remapping; otherwise cards that only
    // set "value" (e.g. Bash Vulnerable 2) keep the EffectDef default duration of 1.
    PowerFactory::instance().load();

    int powerValue = effect.value > 0 ? effect.value : 1;
    int duration = effect.duration;
    const BuffDefinition *definition = PowerFactory::instance().definition(powerId);
    if (definition != nullptr) {
        if (definition->stacking == QStringLiteral("duration")) {
            // cards.json uses value as the turn count; intensity is always 1 for these powers.
            // Explicit duration > 1 (from EffectDef::applyPower) wins over value.
            if (effect.duration > 1) {
                duration = effect.duration;
            } else if (effect.value > 0) {
                duration = effect.value;
            } else {
                duration = definition->durationDefault;
            }
            powerValue = 1;
        } else if (definition->stacking == QStringLiteral("intensity")) {
            // value = stack amount (Strength, Metallicize, ...); permanent unless duration > 1.
            powerValue = effect.value > 0 ? effect.value : definition->valueDefault;
            duration = effect.duration > 1 ? effect.duration : -1;
        }
    }

    auto queueFor = [&](Actor *powerTarget) {
        if (powerTarget == nullptr) {
            return;
        }
        commands.push_back(std::make_unique<ApplyPowerCommand>(
            powerTarget, PowerFactory::createPower(powerId, powerValue, duration)));
    };

    if (effect.target == QStringLiteral("all_enemies")) {
        for (Actor *enemy : allEnemies) {
            queueFor(enemy);
        }
        if (allEnemies.isEmpty()) {
            queueFor(target);
        }
        return;
    }

    Actor *powerTarget = target;
    if (effect.target == QStringLiteral("self") || effect.target == QStringLiteral("attacker")) {
        powerTarget = caster;
    }
    queueFor(powerTarget);
}

} // namespace

EffectResolver &EffectResolver::instance()
{
    static EffectResolver resolver;
    return resolver;
}

void EffectResolver::execute(const QVector<EffectDef> &effects, Actor *caster, Actor *target,
                             const QVector<Actor *> &allEnemies, BattleFSM *battleFsm,
                             const QString &sourceCardId)
{
    std::vector<std::unique_ptr<ICommand>> commands;

    for (const EffectDef &effect : effects) {
        if (effect.type == QStringLiteral("deal_damage")) {
            int amount = effect.value;
            if (battleFsm != nullptr && !sourceCardId.isEmpty()) {
                amount += battleFsm->cardDamageBonus(sourceCardId);
            }
            queueDamage(commands, caster, target, amount, allEnemies, effect.target);
        } else if (effect.type == QStringLiteral("deal_damage_per_block")) {
            const int amount = caster != nullptr ? caster->currentBlock() : 0;
            queueDamage(commands, caster, target, amount, allEnemies,
                        effect.target.isEmpty() ? QStringLiteral("selected_enemy") : effect.target);
        } else if (effect.type == QStringLiteral("gain_block")) {
            Actor *blockTarget = caster;
            if (effect.target != QStringLiteral("self") && !effect.target.isEmpty()
                && effect.target != QStringLiteral("selected_enemy")) {
                blockTarget = target;
            }
            commands.push_back(std::make_unique<GainBlockCommand>(blockTarget, effect.value));
        } else if (effect.type == QStringLiteral("restore_hp")
                   || effect.type == QStringLiteral("restore_hp_percent")) {
            Actor *healTarget = caster;
            int healAmount = effect.value;
            if (effect.type == QStringLiteral("restore_hp_percent") && caster != nullptr) {
                healAmount = (caster->maxHp() * effect.value) / 100;
            }
            commands.push_back(std::make_unique<HealCommand>(healTarget, healAmount));
        } else if (effect.type == QStringLiteral("revive_to_percent_hp")) {
            if (caster != nullptr && effect.value > 0) {
                const int reviveHp = (caster->maxHp() * effect.value) / 100;
                caster->heal(reviveHp);
            }
        } else if (effect.type == QStringLiteral("gain_gold")) {
            if (auto *player = qobject_cast<Player *>(caster)) {
                player->addGold(effect.value);
            }
        } else if (effect.type == QStringLiteral("lose_hp")) {
            if (caster != nullptr && effect.value > 0) {
                caster->takeDirectDamage(effect.value);
            }
        } else if (effect.type == QStringLiteral("draw_cards")
                   || effect.type == QStringLiteral("draw_card")) {
            if (battleFsm != nullptr && effect.value > 0) {
                battleFsm->drawExtraCards(effect.value);
            }
        } else if (effect.type == QStringLiteral("gain_energy")) {
            if (battleFsm != nullptr && effect.value != 0) {
                battleFsm->gainEnergy(effect.value);
            }
        } else if (effect.type == QStringLiteral("exhaust")) {
            // Routing to exhaust pile is handled by BattleFSM after execute.
        } else if (effect.type == QStringLiteral("unplayable")
                   || effect.type == QStringLiteral("ethereal")) {
            // Keyword flags handled by BattleFSM play/turn logic.
        } else if (effect.type == QStringLiteral("end_of_turn_damage")) {
            // Handled at end of player turn for status cards remaining in hand.
        } else if (effect.type == QStringLiteral("increase_damage_per_play")) {
            if (battleFsm != nullptr && !sourceCardId.isEmpty() && effect.value > 0) {
                battleFsm->increaseCardDamageBonus(sourceCardId, effect.value);
            }
        } else if (effect.type == QStringLiteral("add_card_to_hand")) {
            if (battleFsm != nullptr && !effect.cardId.isEmpty()) {
                CardDatabase::instance().load();
                const Card templateCard = CardDatabase::instance().getCard(effect.cardId);
                if (!templateCard.id().isEmpty()) {
                    const int copies = effect.value > 0 ? effect.value : 1;
                    for (int i = 0; i < copies; ++i) {
                        battleFsm->addCardToHand(templateCard);
                    }
                } else {
                    qWarning() << "EffectResolver: add_card_to_hand unknown card" << effect.cardId;
                }
            }
        } else if (effect.type == QStringLiteral("exhaust_non_attack_gain_block")) {
            if (battleFsm != nullptr) {
                battleFsm->exhaustNonAttackCardsGainBlock(effect.value);
            }
        } else if (effect.type == QStringLiteral("apply_power_to_all_enemies")) {
            EffectDef aoe = effect;
            aoe.target = QStringLiteral("all_enemies");
            resolveApplyPower(commands, aoe, caster, target, allEnemies);
        } else if (effect.type == QStringLiteral("gain_strength")) {
            if (caster != nullptr) {
                caster->setStrength(caster->strength() + effect.value);
            }
        } else if (effect.type == QStringLiteral("max_hp_increase")) {
            if (auto *player = qobject_cast<Player *>(caster)) {
                player->setMaxHp(player->maxHp() + effect.value);
                player->heal(effect.value);
            }
        } else if (effect.type == QStringLiteral("apply_power")) {
            resolveApplyPower(commands, effect, caster, target, allEnemies);
        } else if (effect.type == QStringLiteral("double_damage_every_10th_attack")
                   || effect.type == QStringLiteral("increase_vulnerable_multiplier")
                   || effect.type == QStringLiteral("explored_nodes_no_step_cost")
                   || effect.type == QStringLiteral("shrink_damage_cannot_kill")
                   || effect.type == QStringLiteral("elite_hp_multiplier")) {
            // Passive or externally handled relic effects.
        } else {
            qWarning() << "EffectResolver: unsupported effect type" << effect.type;
        }
    }

    for (const auto &command : commands) {
        if (command) {
            command->execute();
        }
    }
}
