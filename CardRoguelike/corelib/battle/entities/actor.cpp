#include "actor.h"



#include <algorithm>



#include "battle/logic/power_factory.h"

#include "player.h"
#include "power.h"
#include "systems/event_bus.h"



namespace {



bool isStatPowerId(const QString &id)

{

    return id == QStringLiteral("strength") || id == QStringLiteral("dexterity");

}



} // namespace



Actor::Actor(QObject *parent)

    : QObject(parent)

{

}



int Actor::currentHp() const

{

    return m_stats.hp;

}



int Actor::currentBlock() const

{

    return m_stats.block;

}



int Actor::maxHp() const

{

    return m_stats.maxHp;

}



int Actor::strength() const

{

    return m_stats.strength + getPowerValue(QStringLiteral("strength"));

}



int Actor::dexterity() const

{

    return m_stats.dexterity + getPowerValue(QStringLiteral("dexterity"));

}



void Actor::setStrength(int value)

{

    m_stats.strength = value;

}



void Actor::gainBlock(int amount)

{

    if (amount <= 0) {

        return;

    }



    addBlockRaw(calculateBlockGained(amount));

}



void Actor::addBlockRaw(int amount)

{

    if (amount <= 0) {

        return;

    }



    m_stats.block += amount;

    emit blockChanged(m_stats.block);

}



void Actor::clearBlock()

{

    if (m_stats.block == 0) {

        return;

    }



    m_stats.block = 0;

    emit blockChanged(m_stats.block);

}



void Actor::takeDamage(int damage, Actor *source)

{

    if (damage <= 0) {

        return;

    }



    const int modifiedDamage = calculateIncomingDamage(damage, source);



    const int blockAbsorbed = qMin(m_stats.block, modifiedDamage);

    m_stats.block -= blockAbsorbed;

    emit blockChanged(m_stats.block);



    const int remainingDamage = modifiedDamage - blockAbsorbed;

    if (remainingDamage > 0) {

        m_stats.hp = qBound(0, m_stats.hp - remainingDamage, m_stats.maxHp);

        emit hpChanged(m_stats.hp);



        notifyPowersDamageReceived(remainingDamage, source);



        if (auto *player = qobject_cast<Player *>(this)) {

            EventBus::instance().triggerPlayerReceivedDamage(remainingDamage, source, player);

        }



        if (m_stats.hp <= 0) {

            if (auto *player = qobject_cast<Player *>(this)) {

                EventBus::instance().triggerFatalDamageReceived(player);

            }

            if (m_stats.hp <= 0) {

                emit died();

            }

        }

    }

}



void Actor::takeDirectDamage(int damage)

{

    if (damage <= 0) {

        return;

    }



    m_stats.hp = qBound(0, m_stats.hp - damage, m_stats.maxHp);

    emit hpChanged(m_stats.hp);



    if (m_stats.hp <= 0) {

        if (auto *player = qobject_cast<Player *>(this)) {

            EventBus::instance().triggerFatalDamageReceived(player);

        }

        if (m_stats.hp <= 0) {

            emit died();

        }

    }

}



void Actor::setMaxHp(int value)

{

    m_stats.maxHp = value;

    m_stats.hp = qBound(0, m_stats.hp, m_stats.maxHp);

    emit hpChanged(m_stats.hp);

}



void Actor::healToFull()

{

    m_stats.hp = m_stats.maxHp;

    emit hpChanged(m_stats.hp);

}



void Actor::heal(int amount)

{

    if (amount <= 0) {

        return;

    }



    m_stats.hp = qBound(0, m_stats.hp + amount, m_stats.maxHp);

    emit hpChanged(m_stats.hp);

}



void Actor::addPower(std::unique_ptr<Power> power)

{

    if (!power) {

        return;

    }



    power->bindOwner(this);



    const BuffDefinition *definition = PowerFactory::instance().definition(power->powerId());

    if (definition != nullptr) {

        if (definition->stacking == QStringLiteral("duration")) {

            for (const auto &existing : m_stats.powers) {

                if (existing && existing->powerId() == power->powerId()) {

                    existing->addDuration(power->duration());

                    return;

                }

            }

        } else if (definition->stacking == QStringLiteral("intensity")) {

            for (const auto &existing : m_stats.powers) {

                if (existing && existing->powerId() == power->powerId()) {

                    existing->addValue(power->value());

                    return;

                }

            }

        }

    }



    m_stats.powers.push_back(std::move(power));
    if (m_stats.powers.back()) {
        m_stats.powers.back()->onOwnerBound();
    }
}



void Actor::removePower(const QString &id)

{

    m_stats.powers.erase(

        std::remove_if(

            m_stats.powers.begin(),

            m_stats.powers.end(),

            [&id](const std::unique_ptr<Power> &power) {

                return power && power->powerId() == id;

            }),

        m_stats.powers.end());

}



int Actor::getPowerValue(const QString &id) const

{

    int total = 0;

    for (const auto &power : m_stats.powers) {

        if (power && power->powerId() == id) {

            total += power->value();

        }

    }

    return total;

}



int Actor::getPowerDuration(const QString &id) const

{

    for (const auto &power : m_stats.powers) {

        if (power && power->powerId() == id) {

            return power->duration();

        }

    }

    return 0;

}



bool Actor::hasPower(const QString &id) const

{

    for (const auto &power : m_stats.powers) {

        if (power && power->powerId() == id) {

            return true;

        }

    }

    return false;

}



int Actor::calculateOutgoingDamage(int baseDamage) const

{

    int damage = baseDamage + getPowerValue(QStringLiteral("strength"));

    for (const auto &power : m_stats.powers) {

        if (power && !isStatPowerId(power->powerId())) {

            damage = power->modifyOutgoingDamage(damage);

        }

    }

    return qMax(0, damage);

}



int Actor::calculateIncomingDamage(int baseDamage, Actor *source) const

{

    int damage = baseDamage;



    const auto *attacker = qobject_cast<const Player *>(source);

    const bool paperFrogActive = attacker != nullptr

                                 && attacker->hasRelic(QStringLiteral("paper_frog"))

                                 && hasPower(QStringLiteral("vulnerable"));



    if (paperFrogActive) {

        damage = static_cast<int>(damage * 1.75);

        for (const auto &power : m_stats.powers) {

            if (power && power->powerId() != QStringLiteral("vulnerable")) {

                damage = power->modifyIncomingDamage(damage);

            }

        }

        return qMax(0, damage);

    }



    for (const auto &power : m_stats.powers) {

        if (power) {

            damage = power->modifyIncomingDamage(damage);

        }

    }

    return qMax(0, damage);

}



int Actor::calculateBlockGained(int baseBlock) const

{

    int block = baseBlock + getPowerValue(QStringLiteral("dexterity"));

    for (const auto &power : m_stats.powers) {

        if (power && !isStatPowerId(power->powerId())) {

            block = power->modifyBlockGained(block);

        }

    }

    return qMax(0, block);

}



int Actor::applyDexterityToBlock(int block) const

{

    return calculateBlockGained(block);

}



void Actor::beginTurn()

{

    // Block expires at the start of the owner's turn so end-of-turn block
    // (e.g. Metallicize) remains through the opponent's actions.
    clearBlock();



    const size_t powerCount = m_stats.powers.size();

    for (size_t i = 0; i < powerCount; ++i) {

        if (m_stats.powers[i]) {

            m_stats.powers[i]->onTurnStart();

        }

    }

}



void Actor::endTurn()

{

    // Index-bounded loop: onTurnEnd may add powers (Ritual → Strength).
    const size_t powerCount = m_stats.powers.size();

    for (size_t i = 0; i < powerCount; ++i) {

        if (m_stats.powers[i]) {

            m_stats.powers[i]->onTurnEnd();

        }

    }



    removeExpiredPowers();

}



void Actor::notifyPlayerPlayedAttack(Player *player)

{

    const size_t powerCount = m_stats.powers.size();

    for (size_t i = 0; i < powerCount; ++i) {

        if (m_stats.powers[i]) {

            m_stats.powers[i]->onPlayerPlayedAttack(player);

        }

    }

}



void Actor::notifyPowersDamageReceived(int damage, Actor *source)

{

    for (const auto &power : m_stats.powers) {

        if (power) {

            power->onDamageReceived(damage, source);

        }

    }

}



void Actor::removeExpiredPowers()

{

    m_stats.powers.erase(

        std::remove_if(

            m_stats.powers.begin(),

            m_stats.powers.end(),

            [](const std::unique_ptr<Power> &power) { return power && power->isExpired(); }),

        m_stats.powers.end());

}


