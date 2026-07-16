#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

class Enemy;
class QObject;

struct EnemySpawnSpec {
    QString id;
    QStringList randomFrom;
    int count = 1;
    bool allowDuplicates = true;
    QStringList gremlinPool;
    QMap<QString, int> gremlinMaxCounts;
};

struct EnemyEncounterEntry {
    double weight = 1.0;
    QVector<EnemySpawnSpec> spawns;
};

class EnemyPool
{
public:
    static EnemyPool &instance();

    void load();
    QVector<Enemy *> rollEncounter(const QString &poolType, int normalNodesCleared, QObject *parent = nullptr);

private:
    EnemyPool() = default;

    void parsePoolArray(const QString &poolName, const QJsonArray &entries);
    static EnemySpawnSpec parseSpawnSpec(const QJsonObject &obj);
    static QVector<QString> resolveSpawnIds(const EnemySpawnSpec &spec);
    static QVector<QString> rollGremlinGroup(const EnemySpawnSpec &spec);
    const QVector<EnemyEncounterEntry> *poolEntries(const QString &poolName) const;

    QMap<QString, QVector<EnemyEncounterEntry>> m_pools;
    bool m_loaded = false;
};
