#include "enemy_pool.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "battle/data/enemy_database.h"
#include "battle/entities/enemy.h"
#include "utils/rng.h"

EnemyPool &EnemyPool::instance()
{
    static EnemyPool pool;
    return pool;
}

void EnemyPool::load()
{
    if (m_loaded) {
        return;
    }

    Q_INIT_RESOURCE(resources);

    QFile file(QStringLiteral(":/resources/data/enemypool.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    const QJsonObject pools = document.object().value(QStringLiteral("pools")).toObject();
    for (auto it = pools.constBegin(); it != pools.constEnd(); ++it) {
        parsePoolArray(it.key(), it.value().toArray());
    }

    m_loaded = true;
}

QVector<Enemy *> EnemyPool::rollEncounter(const QString &poolType, int normalNodesCleared, QObject *parent)
{
    load();
    EnemyDatabase::instance().load();

    QString poolName = poolType;
    if (poolType == QStringLiteral("normal")) {
        poolName = normalNodesCleared < 3 ? QStringLiteral("normal_early") : QStringLiteral("normal_late");
    }

    const QVector<EnemyEncounterEntry> *entries = poolEntries(poolName);
    if (entries == nullptr || entries->isEmpty()) {
        const QString fallbackId = EnemyDatabase::instance().randomEnemyIdForType(
            poolType == QStringLiteral("boss")   ? QStringLiteral("boss")
            : poolType == QStringLiteral("elite") ? QStringLiteral("elite")
                                                  : QStringLiteral("normal"));
        Enemy *enemy = EnemyDatabase::instance().createEnemy(fallbackId, parent);
        return enemy != nullptr ? QVector<Enemy *>{enemy} : QVector<Enemy *>{};
    }

    double totalWeight = 0.0;
    for (const EnemyEncounterEntry &entry : *entries) {
        totalWeight += entry.weight;
    }

    double roll = RNG::get(0, 10000) / 10000.0 * totalWeight;
    const EnemyEncounterEntry *chosenEntry = &entries->first();
    for (const EnemyEncounterEntry &entry : *entries) {
        roll -= entry.weight;
        if (roll <= 0.0) {
            chosenEntry = &entry;
            break;
        }
    }

    QVector<Enemy *> enemies;
    for (const EnemySpawnSpec &spec : chosenEntry->spawns) {
        const QVector<QString> ids = resolveSpawnIds(spec);
        for (const QString &id : ids) {
            Enemy *enemy = EnemyDatabase::instance().createEnemy(id, parent);
            if (enemy != nullptr) {
                enemies.append(enemy);
            }
        }
    }

    return enemies;
}

void EnemyPool::parsePoolArray(const QString &poolName, const QJsonArray &entries)
{
    QVector<EnemyEncounterEntry> parsedEntries;
    for (const QJsonValue &value : entries) {
        const QJsonObject obj = value.toObject();
        EnemyEncounterEntry entry;
        entry.weight = obj.value(QStringLiteral("weight")).toDouble(1.0);

        const QJsonArray enemies = obj.value(QStringLiteral("enemies")).toArray();
        for (const QJsonValue &enemyValue : enemies) {
            entry.spawns.append(parseSpawnSpec(enemyValue.toObject()));
        }

        parsedEntries.append(entry);
    }

    m_pools.insert(poolName, parsedEntries);
}

EnemySpawnSpec EnemyPool::parseSpawnSpec(const QJsonObject &obj)
{
    EnemySpawnSpec spec;
    spec.id = obj.value(QStringLiteral("id")).toString();
    spec.count = obj.value(QStringLiteral("count")).toInt(1);
    spec.allowDuplicates = obj.value(QStringLiteral("allow_duplicates")).toBool(true);

    const QJsonArray randomFrom = obj.value(QStringLiteral("random_from")).toArray();
    for (const QJsonValue &value : randomFrom) {
        spec.randomFrom.append(value.toString());
    }

    if (obj.value(QStringLiteral("type")).toString() == QStringLiteral("gremlin_group")) {
        const QJsonArray pool = obj.value(QStringLiteral("pool")).toArray();
        for (const QJsonValue &value : pool) {
            spec.gremlinPool.append(value.toString());
        }
        spec.count = obj.value(QStringLiteral("count")).toInt(4);

        const QJsonObject constraints = obj.value(QStringLiteral("constraints")).toObject();
        for (auto it = constraints.constBegin(); it != constraints.constEnd(); ++it) {
            spec.gremlinMaxCounts.insert(it.key(), it.value().toObject().value(QStringLiteral("max")).toInt(99));
        }
    }

    return spec;
}

QVector<QString> EnemyPool::resolveSpawnIds(const EnemySpawnSpec &spec)
{
    if (!spec.gremlinPool.isEmpty()) {
        return rollGremlinGroup(spec);
    }

    if (!spec.randomFrom.isEmpty()) {
        QVector<QString> ids;
        for (int i = 0; i < spec.count; ++i) {
            const int index = RNG::get(0, spec.randomFrom.size() - 1);
            ids.append(spec.randomFrom.at(index));
        }
        return ids;
    }

    QVector<QString> ids;
    for (int i = 0; i < spec.count; ++i) {
        ids.append(spec.id);
    }
    return ids;
}

QVector<QString> EnemyPool::rollGremlinGroup(const EnemySpawnSpec &spec)
{
    QVector<QString> result;
    QMap<QString, int> used;

    for (int i = 0; i < spec.count; ++i) {
        QStringList candidates;
        for (const QString &id : spec.gremlinPool) {
            const int maxCount = spec.gremlinMaxCounts.value(id, 99);
            if (used.value(id, 0) < maxCount) {
                candidates.append(id);
            }
        }

        if (candidates.isEmpty()) {
            break;
        }

        const QString picked = candidates.at(RNG::get(0, candidates.size() - 1));
        result.append(picked);
        used[picked] = used.value(picked, 0) + 1;
    }

    return result;
}

const QVector<EnemyEncounterEntry> *EnemyPool::poolEntries(const QString &poolName) const
{
    const auto it = m_pools.constFind(poolName);
    if (it == m_pools.constEnd()) {
        return nullptr;
    }
    return &(*it);
}
