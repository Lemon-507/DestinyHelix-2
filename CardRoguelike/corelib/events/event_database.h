#pragma once

#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QVector>

#include "battle/entities/card.h"

struct EventOption {
    QString text;
    QString description;
    QString conditionType;
    QVector<EffectDef> effects;
};

struct EventData {
    QString id;
    QString name;
    QString description;
    QVector<EventOption> options;
};

Q_DECLARE_METATYPE(EventData)

class EventDatabase
{
public:
    static EventDatabase &instance();

    void load();
    const EventData *randomEvent(const QSet<QString> &completedEventIds) const;
    const EventData *getEvent(const QString &id) const;

private:
    EventDatabase() = default;

    QVector<EventData> m_events;
    bool m_loaded = false;
};
