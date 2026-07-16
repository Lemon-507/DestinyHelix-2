#include "relic_database.h"



#include <QtCore/QFile>

#include <QtCore/QJsonArray>

#include <QtCore/QJsonDocument>

#include <QtCore/QJsonObject>



#include "utils/rng.h"



namespace {



void parseRelicEffects(const QJsonArray &effects, QVector<EffectDef> &outEffects)

{

    for (const QJsonValue &effectValue : effects) {

        const QJsonObject effectObj = effectValue.toObject();

        EffectDef effect;

        effect.type = effectObj.value(QStringLiteral("type")).toString();

        effect.value = effectObj.value(QStringLiteral("value")).toInt();

        effect.target = effectObj.value(QStringLiteral("target")).toString();

        effect.powerId = effectObj.value(QStringLiteral("power_id")).toString();

        outEffects.append(effect);

    }

}



void parseRelicObject(const QJsonObject &obj, QVector<RelicData> &relics)

{

    RelicData relic;

    relic.id = obj.value(QStringLiteral("id")).toString();

    relic.name = obj.value(QStringLiteral("name")).toString();

    relic.description = obj.value(QStringLiteral("description")).toString();

    relic.trigger = relicTriggerFromString(obj.value(QStringLiteral("trigger")).toString());

    relic.maxStack = obj.value(QStringLiteral("max_stack")).toInt(1);

    relic.characterExclusive = obj.value(QStringLiteral("character_exclusive")).toString();

    parseRelicEffects(obj.value(QStringLiteral("effects")).toArray(), relic.effects);

    relics.append(relic);

}



} // namespace



RelicDatabase &RelicDatabase::instance()

{

    static RelicDatabase database;

    return database;

}



void RelicDatabase::load()

{

    if (m_loaded) {

        return;

    }



    QFile file(QStringLiteral(":/resources/data/relics.json"));

    if (!file.open(QIODevice::ReadOnly)) {

        return;

    }



    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());

    if (document.isArray()) {

        for (const QJsonValue &value : document.array()) {

            parseRelicObject(value.toObject(), m_relics);

        }

    } else {

        const QJsonArray relics = document.object().value(QStringLiteral("relics")).toArray();

        for (const QJsonValue &value : relics) {

            parseRelicObject(value.toObject(), m_relics);

        }

    }



    m_loaded = true;

}



RelicData RelicDatabase::getRelic(const QString &id) const

{

    for (const RelicData &relic : m_relics) {

        if (relic.id == id) {

            return relic;

        }

    }

    return RelicData();

}



QStringList RelicDatabase::allRelicIds() const

{

    QStringList ids;

    for (const RelicData &relic : m_relics) {

        ids.append(relic.id);

    }

    return ids;

}



QString RelicDatabase::randomRelicIdExcluding(const QStringList &ownedIds) const

{

    QStringList candidates;

    for (const RelicData &relic : m_relics) {

        if (!ownedIds.contains(relic.id)) {

            candidates.append(relic.id);

        }

    }



    if (candidates.isEmpty()) {

        return QString();

    }



    return candidates.at(RNG::get(0, candidates.size() - 1));

}


