#pragma once



#include <QtCore/QString>

#include <QtCore/QVector>



#include "battle/entities/card.h"



enum class RelicTrigger {

    OnBattleStart,

    OnTurnStart,

    OnCombatEnd,

    OnPotionUse,

    OnAttackPlayed,

    OnFatalDamage,

    OnPickup,

    OnEliteCombatStart,

    OnDamageDealt,

    OnDamageReceived,

    Passive,

    Unknown

};



struct RelicData {

    QString id;

    QString name;

    QString description;

    RelicTrigger trigger = RelicTrigger::Unknown;

    QVector<EffectDef> effects;

    int maxStack = 1;

    QString characterExclusive;

};



RelicTrigger relicTriggerFromString(const QString &trigger);

QString relicTriggerToString(RelicTrigger trigger);



class Relic

{

public:

    explicit Relic(RelicData data = RelicData());



    QString id() const;

    RelicTrigger triggerType() const;

    const QVector<EffectDef> &effects() const;

    const RelicData &data() const;



private:

    RelicData m_data;

};


