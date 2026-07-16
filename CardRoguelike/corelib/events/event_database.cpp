#include "event_database.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "utils/rng.h"

EventDatabase &EventDatabase::instance()
{
    static EventDatabase database;
    return database;
}

void EventDatabase::load()
{
    if (m_loaded) {
        return;
    }

    QFile file(QStringLiteral(":/resources/data/events.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QJsonArray events;
    if (document.isArray()) {
        events = document.array();
    } else {
        events = document.object().value(QStringLiteral("events")).toArray();
    }

    for (const QJsonValue &value : events) {
        const QJsonObject obj = value.toObject();
        EventData eventData;
        eventData.id = obj.value(QStringLiteral("id")).toString();
        eventData.name = obj.value(QStringLiteral("name")).toString();
        eventData.description = obj.value(QStringLiteral("description")).toString();

        const QJsonArray options = obj.value(QStringLiteral("options")).toArray();
        for (const QJsonValue &optionValue : options) {
            const QJsonObject optionObj = optionValue.toObject();
            EventOption option;
            option.text = optionObj.value(QStringLiteral("text")).toString();
            option.description = optionObj.value(QStringLiteral("description")).toString();

            const QJsonObject conditionObj = optionObj.value(QStringLiteral("condition")).toObject();
            if (!conditionObj.isEmpty()) {
                option.conditionType = conditionObj.value(QStringLiteral("type")).toString();
            }

            const QJsonArray effects = optionObj.value(QStringLiteral("effects")).toArray();
            for (const QJsonValue &effectValue : effects) {
                const QJsonObject effectObj = effectValue.toObject();
                EffectDef effect;
                effect.type = effectObj.value(QStringLiteral("type")).toString();
                effect.value = effectObj.value(QStringLiteral("value")).toInt();
                effect.target = effectObj.value(QStringLiteral("target")).toString();
                if (effect.target.isEmpty() && effectObj.contains(QStringLiteral("card_id"))) {
                    effect.target = effectObj.value(QStringLiteral("card_id")).toString();
                }
                if (effectObj.contains(QStringLiteral("min"))) {
                    effect.value = effectObj.value(QStringLiteral("min")).toInt();
                }
                if (effectObj.contains(QStringLiteral("max"))) {
                    effect.duration = effectObj.value(QStringLiteral("max")).toInt();
                }
                option.effects.append(effect);
            }

            eventData.options.append(option);
        }

        m_events.append(eventData);
    }

    m_loaded = true;
}

const EventData *EventDatabase::randomEvent(const QSet<QString> &completedEventIds) const
{
    QVector<const EventData *> available;
    for (const EventData &eventData : m_events) {
        if (!completedEventIds.contains(eventData.id)) {
            available.append(&eventData);
        }
    }

    // All events completed this run: allow repeats with replacement.
    if (available.isEmpty()) {
        for (const EventData &eventData : m_events) {
            available.append(&eventData);
        }
    }

    if (available.isEmpty()) {
        return nullptr;
    }

    const int index = RNG::get(0, available.size() - 1);
    return available.at(index);
}

const EventData *EventDatabase::getEvent(const QString &id) const
{
    for (const EventData &eventData : m_events) {
        if (eventData.id == id) {
            return &eventData;
        }
    }
    return nullptr;
}
