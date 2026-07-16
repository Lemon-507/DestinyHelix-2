#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include "battle/entities/enemy.h"

class EnemyDatabase
{
public:
    static EnemyDatabase &instance();

    void load();
    const EnemyData *getEnemy(const QString &id) const;
    Enemy *createEnemy(const QString &id, QObject *parent = nullptr) const;
    QString randomEnemyIdForType(const QString &type) const;
    QVector<const EnemyData *> enemiesOfType(const QString &type) const;

private:
    EnemyDatabase() = default;

    void parseEnemyObject(const QJsonObject &obj);
    static EnemyIntentDef parseIntent(const QJsonObject &obj);
    static EnemyAbilityDef parseAbility(const QJsonObject &obj);
    static BehaviorDef parseBehavior(const QJsonObject &obj);

    QVector<EnemyData> m_enemies;
    bool m_loaded = false;
};
