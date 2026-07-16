#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include "events/event_database.h"

class MapViewModel;
struct EffectDef;

class EventViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString eventName READ eventName NOTIFY eventDataChanged)
    Q_PROPERTY(QString eventDescription READ eventDescription NOTIFY eventDataChanged)
    Q_PROPERTY(bool optionSelected READ optionSelected NOTIFY optionSelectedChanged)

public:
    explicit EventViewModel(MapViewModel *mapViewModel, QObject *parent = nullptr);

    void setEvent(const EventData &eventData);
    const EventData &eventData() const;

    QString eventName() const;
    QString eventDescription() const;
    int optionCount() const;
    QString optionButtonText(int index) const;
    bool isOptionAvailable(int index) const;
    bool optionSelected() const;
    bool hasPendingCardSelection() const;

    Q_INVOKABLE void selectOption(int index);
    Q_INVOKABLE void completePendingOption(int selectedDeckIndex);
    Q_INVOKABLE void cancelPendingOption();
    Q_INVOKABLE void requestExit();

    static QString describeEffects(const QVector<EffectDef> &effects);
    static QString cardSelectionActionForOption(const EventOption &option);

signals:
    void eventDataChanged();
    void optionSelectedChanged();
    void exitRequested();
    void cardSelectionNeeded(const QString &actionType);

private:
    static QString describeEffect(const EffectDef &effect);
    bool meetsCondition(const EventOption &option) const;
    void finishOption(int optionIndex, int selectedDeckIndex);

    MapViewModel *m_mapViewModel = nullptr;
    EventData m_eventData;
    bool m_optionSelected = false;
    int m_selectedOptionIndex = -1;
    int m_pendingOptionIndex = -1;
};
