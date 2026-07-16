#include "relic.h"



RelicTrigger relicTriggerFromString(const QString &trigger)

{

    if (trigger == QStringLiteral("on_battle_start")) {

        return RelicTrigger::OnBattleStart;

    }

    if (trigger == QStringLiteral("on_turn_start")) {

        return RelicTrigger::OnTurnStart;

    }

    if (trigger == QStringLiteral("on_combat_end")) {

        return RelicTrigger::OnCombatEnd;

    }

    if (trigger == QStringLiteral("on_potion_use")) {

        return RelicTrigger::OnPotionUse;

    }

    if (trigger == QStringLiteral("on_attack_played")) {

        return RelicTrigger::OnAttackPlayed;

    }

    if (trigger == QStringLiteral("on_fatal_damage")) {

        return RelicTrigger::OnFatalDamage;

    }

    if (trigger == QStringLiteral("on_pickup")) {

        return RelicTrigger::OnPickup;

    }

    if (trigger == QStringLiteral("on_elite_combat_start")) {

        return RelicTrigger::OnEliteCombatStart;

    }

    if (trigger == QStringLiteral("on_damage_dealt")) {

        return RelicTrigger::OnDamageDealt;

    }

    if (trigger == QStringLiteral("on_damage_received")) {

        return RelicTrigger::OnDamageReceived;

    }

    if (trigger == QStringLiteral("passive")) {

        return RelicTrigger::Passive;

    }

    return RelicTrigger::Unknown;

}



QString relicTriggerToString(RelicTrigger trigger)

{

    switch (trigger) {

    case RelicTrigger::OnBattleStart:

        return QStringLiteral("on_battle_start");

    case RelicTrigger::OnTurnStart:

        return QStringLiteral("on_turn_start");

    case RelicTrigger::OnCombatEnd:

        return QStringLiteral("on_combat_end");

    case RelicTrigger::OnPotionUse:

        return QStringLiteral("on_potion_use");

    case RelicTrigger::OnAttackPlayed:

        return QStringLiteral("on_attack_played");

    case RelicTrigger::OnFatalDamage:

        return QStringLiteral("on_fatal_damage");

    case RelicTrigger::OnPickup:

        return QStringLiteral("on_pickup");

    case RelicTrigger::OnEliteCombatStart:

        return QStringLiteral("on_elite_combat_start");

    case RelicTrigger::OnDamageDealt:

        return QStringLiteral("on_damage_dealt");

    case RelicTrigger::OnDamageReceived:

        return QStringLiteral("on_damage_received");

    case RelicTrigger::Passive:

        return QStringLiteral("passive");

    case RelicTrigger::Unknown:

        break;

    }

    return QStringLiteral("unknown");

}



Relic::Relic(RelicData data)

    : m_data(std::move(data))

{

}



QString Relic::id() const

{

    return m_data.id;

}



RelicTrigger Relic::triggerType() const

{

    return m_data.trigger;

}



const QVector<EffectDef> &Relic::effects() const

{

    return m_data.effects;

}



const RelicData &Relic::data() const

{

    return m_data;

}


