#pragma once

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>

#include "events/event_database.h"

class MapManager;
class Player;

class EventManager : public QObject
{
    Q_OBJECT

public:
    explicit EventManager(QObject *parent = nullptr);

    bool executeOption(const EventOption &option, Player *player, MapManager *mapManager,
                       int selectedDeckIndex = -1);
    void markEventCompleted(const QString &eventId);
    QSet<QString> completedEvents() const;
    void setCompletedEvents(const QSet<QString> &completedEvents);

private:
    QSet<QString> m_completedEvents;
};
