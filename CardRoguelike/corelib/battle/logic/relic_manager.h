#pragma once

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include "battle/entities/relic.h"

class Actor;
class BattleFSM;

class RelicManager : public QObject
{
    Q_OBJECT

public:
    explicit RelicManager(QObject *parent = nullptr);

    void clear();
    void unregisterAll();
    void registerRelic(const RelicData &relic, Actor *owner, BattleFSM *fsm = nullptr);
    bool isRegistered(const QString &relicId) const;
    QStringList registeredRelicIds() const;

    void setBattleContext(BattleFSM *fsm, const QVector<Actor *> &enemies);
    void clearBattleContext();

private:
    struct RegisteredRelic {
        RelicData relic;
        Actor *owner = nullptr;
    };

    void applyRelicEffects(const RelicData &relic, Actor *caster, Actor *target);
    void consumeRelic(const QString &relicId, Actor *owner);

    QVector<RegisteredRelic> m_registeredRelics;
    QVector<QMetaObject::Connection> m_connections;
    QSet<QString> m_registeredIds;
    BattleFSM *m_battleFsm = nullptr;
    QVector<Actor *> m_battleEnemies;
    int m_penNibAttackCount = 0;
};
