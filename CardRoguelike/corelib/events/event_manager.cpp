#include "event_manager.h"

#include "battle/entities/player.h"
#include "battle/logic/effectresolver.h"
#include "map/map_manager.h"

EventManager::EventManager(QObject *parent)
    : QObject(parent)
{
}

bool EventManager::executeOption(const EventOption &option, Player *player, MapManager *mapManager,
                                 int selectedDeckIndex)
{
    if (player == nullptr) {
        return false;
    }

    return EffectResolver::instance().execute(option.effects, player, player, {}, nullptr, QString(),
                                              mapManager, selectedDeckIndex, true);
}

void EventManager::markEventCompleted(const QString &eventId)
{
    m_completedEvents.insert(eventId);
}

QSet<QString> EventManager::completedEvents() const
{
    return m_completedEvents;
}

void EventManager::setCompletedEvents(const QSet<QString> &completedEvents)
{
    m_completedEvents = completedEvents;
}
