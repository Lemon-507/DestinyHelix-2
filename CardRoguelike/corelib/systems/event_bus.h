#pragma once



#include <QtCore/QObject>



class Actor;



class EventBus : public QObject

{

    Q_OBJECT



public:

    static EventBus &instance();



    EventBus(const EventBus &) = delete;

    EventBus &operator=(const EventBus &) = delete;

    EventBus(EventBus &&) = delete;

    EventBus &operator=(EventBus &&) = delete;



    void triggerShrinkTriggered(int newShrinkCount);

    void triggerBattleStarted();

    void triggerPlayerTurnStarted();

    void triggerPlayerTurnEnded();

    void triggerCombatEnded(bool playerWon);

    void triggerPlayerReceivedDamage(int damage, Actor *source, Actor *player);

    void triggerPotionUsed();

    void triggerPlayerPlayedAttack(int attackCount);

    void triggerFatalDamageReceived(Actor *victim);



signals:

    void battleStarted();

    void playerTurnStarted();

    void playerTurnEnded();

    void combatEnded(bool playerWon);

    void shrinkTriggered(int newShrinkCount);

    void playerReceivedDamage(int damage, Actor *source, Actor *player);

    void potionUsed();

    void playerPlayedAttack(int attackCount);

    void fatalDamageReceived(Actor *victim);



private:

    explicit EventBus(QObject *parent = nullptr);

};


