#include "power.h"

#include "actor.h"
#include "player.h"

Power::Power(Actor *owner, const QString &id, int value, int duration, QObject *parent)
    : QObject(parent)
    , m_value(value)
    , m_owner(owner)
    , m_id(id)
    , m_duration(duration)
{
}

Actor *Power::owner() const
{
    return m_owner;
}

QString Power::powerId() const
{
    return m_id;
}

int Power::value() const
{
    return m_value;
}

int Power::duration() const
{
    return m_duration;
}

void Power::addValue(int amount)
{
    m_value += amount;
}

void Power::addDuration(int amount)
{
    if (m_duration >= 0) {
        m_duration += amount;
    }
}

bool Power::isExpired() const
{
    return m_duration == 0;
}

void Power::bindOwner(Actor *owner)
{
    m_owner = owner;
    if (owner != nullptr) {
        setParent(owner);
    }
}

void Power::onTurnStart()
{
}

void Power::onTurnEnd()
{
    tickDuration();
}

void Power::onDamageReceived(int damage, Actor *source)
{
    Q_UNUSED(damage);
    Q_UNUSED(source);
}

void Power::onPlayerPlayedAttack(Player *player)
{
    Q_UNUSED(player);
}

void Power::onOwnerBound()
{
}

QString Power::name() const
{
    return m_id;
}

bool Power::isDispellable() const
{
    return true;
}

int Power::modifyOutgoingDamage(int damage) const
{
    return damage;
}

int Power::modifyIncomingDamage(int damage) const
{
    return damage;
}

int Power::modifyBlockGained(int block) const
{
    return block;
}

void Power::tickDuration()
{
    if (m_duration > 0) {
        --m_duration;
    }
}
