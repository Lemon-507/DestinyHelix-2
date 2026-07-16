#include "shrink_damage_power.h"



#include "actor.h"

#include "player.h"



ShrinkDamagePower::ShrinkDamagePower(Actor *owner, int initialDamage, QObject *parent)

    : Power(owner, QStringLiteral("shrink_damage"), initialDamage, -1, parent)

    , m_damagePerTurn(initialDamage)

{

}



void ShrinkDamagePower::setDamage(int newDamage)

{

    m_damagePerTurn = newDamage;

}



int ShrinkDamagePower::damagePerTurn() const

{

    return m_damagePerTurn;

}



void ShrinkDamagePower::onTurnStart()

{

    if (owner() == nullptr || m_damagePerTurn <= 0) {

        return;

    }



    if (auto *player = qobject_cast<Player *>(owner())) {

        if (player->hasRelic(QStringLiteral("fate"))) {

            const int newHp = qMax(1, player->currentHp() - m_damagePerTurn);

            const int actualDamage = player->currentHp() - newHp;

            if (actualDamage > 0) {

                player->takeDirectDamage(actualDamage);

            }

            return;

        }

    }



    owner()->takeDirectDamage(m_damagePerTurn);

}



QString ShrinkDamagePower::name() const

{

    return QStringLiteral("ShrinkDamage");

}



bool ShrinkDamagePower::isDispellable() const

{

    return false;

}


