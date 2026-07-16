#include "event_bus.h"



EventBus::EventBus(QObject *parent)

    : QObject(parent)

{

}



EventBus &EventBus::instance()

{

    static EventBus bus;

    return bus;

}



void EventBus::triggerShrinkTriggered(int newShrinkCount)

{

    emit shrinkTriggered(newShrinkCount);

}



void EventBus::triggerBattleStarted()

{

    emit battleStarted();

}



void EventBus::triggerPlayerTurnStarted()

{

    emit playerTurnStarted();

}



void EventBus::triggerPlayerTurnEnded()

{

    emit playerTurnEnded();

}



void EventBus::triggerCombatEnded(bool playerWon)

{

    emit combatEnded(playerWon);

}



void EventBus::triggerPlayerReceivedDamage(int damage, Actor *source, Actor *player)

{

    emit playerReceivedDamage(damage, source, player);

}



void EventBus::triggerPotionUsed()

{

    emit potionUsed();

}



void EventBus::triggerPlayerPlayedAttack(int attackCount)

{

    emit playerPlayedAttack(attackCount);

}



void EventBus::triggerFatalDamageReceived(Actor *victim)

{

    emit fatalDamageReceived(victim);

}


