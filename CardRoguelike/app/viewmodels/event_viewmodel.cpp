#include "event_viewmodel.h"

#include "battle/entities/card.h"
#include "viewmodels/map_viewmodel.h"

EventViewModel::EventViewModel(MapViewModel *mapViewModel, QObject *parent)
    : QObject(parent)
    , m_mapViewModel(mapViewModel)
{
}

void EventViewModel::setEvent(const EventData &eventData)
{
    m_eventData = eventData;
    m_optionSelected = false;
    m_selectedOptionIndex = -1;
    m_pendingOptionIndex = -1;
    emit eventDataChanged();
    emit optionSelectedChanged();
}

const EventData &EventViewModel::eventData() const
{
    return m_eventData;
}

QString EventViewModel::eventName() const
{
    return m_eventData.name;
}

QString EventViewModel::eventDescription() const
{
    if (m_optionSelected && m_selectedOptionIndex >= 0
        && m_selectedOptionIndex < m_eventData.options.size()) {
        const QString &optionDescription = m_eventData.options.at(m_selectedOptionIndex).description;
        if (!optionDescription.isEmpty()) {
            return optionDescription;
        }
    }
    return m_eventData.description;
}

int EventViewModel::optionCount() const
{
    return m_eventData.options.size();
}

QString EventViewModel::optionButtonText(int index) const
{
    if (index < 0 || index >= m_eventData.options.size()) {
        return {};
    }
    return m_eventData.options.at(index).text;
}

bool EventViewModel::isOptionAvailable(int index) const
{
    if (index < 0 || index >= m_eventData.options.size()) {
        return false;
    }
    return meetsCondition(m_eventData.options.at(index));
}

bool EventViewModel::optionSelected() const
{
    return m_optionSelected;
}

bool EventViewModel::hasPendingCardSelection() const
{
    return m_pendingOptionIndex >= 0;
}

QString EventViewModel::cardSelectionActionForOption(const EventOption &option)
{
    for (const EffectDef &effect : option.effects) {
        if (effect.type == QStringLiteral("remove_card")) {
            return QStringLiteral("remove");
        }
        if (effect.type == QStringLiteral("upgrade_card")) {
            return QStringLiteral("upgrade");
        }
        if (effect.type == QStringLiteral("transform_card")) {
            return QStringLiteral("transform");
        }
    }
    return {};
}

void EventViewModel::selectOption(int index)
{
    if (m_optionSelected || m_pendingOptionIndex >= 0 || m_mapViewModel == nullptr) {
        return;
    }
    if (index < 0 || index >= m_eventData.options.size()) {
        return;
    }
    if (!isOptionAvailable(index)) {
        return;
    }

    const EventOption &option = m_eventData.options.at(index);
    const QString action = cardSelectionActionForOption(option);
    if (!action.isEmpty()) {
        m_pendingOptionIndex = index;
        emit cardSelectionNeeded(action);
        return;
    }

    finishOption(index, -1);
}

void EventViewModel::completePendingOption(int selectedDeckIndex)
{
    if (m_pendingOptionIndex < 0) {
        return;
    }
    const int optionIndex = m_pendingOptionIndex;
    m_pendingOptionIndex = -1;
    finishOption(optionIndex, selectedDeckIndex);
}

void EventViewModel::cancelPendingOption()
{
    m_pendingOptionIndex = -1;
}

void EventViewModel::finishOption(int optionIndex, int selectedDeckIndex)
{
    if (m_mapViewModel == nullptr) {
        return;
    }

    m_mapViewModel->executeEventOption(m_eventData, optionIndex, selectedDeckIndex);

    m_selectedOptionIndex = optionIndex;
    m_optionSelected = true;
    emit eventDataChanged();
    emit optionSelectedChanged();
}

void EventViewModel::requestExit()
{
    if (!m_optionSelected) {
        return;
    }
    emit exitRequested();
}

bool EventViewModel::meetsCondition(const EventOption &option) const
{
    if (option.conditionType.isEmpty()) {
        return true;
    }

    if (option.conditionType == QStringLiteral("has_upgradable_card")) {
        return m_mapViewModel != nullptr && m_mapViewModel->canUpgradeAtCampfire();
    }

    return true;
}

QString EventViewModel::describeEffects(const QVector<EffectDef> &effects)
{
    QStringList parts;
    parts.reserve(effects.size());
    for (const EffectDef &effect : effects) {
        const QString part = describeEffect(effect);
        if (!part.isEmpty()) {
            parts.append(part);
        }
    }
    return parts.join(QStringLiteral(", "));
}

QString EventViewModel::describeEffect(const EffectDef &effect)
{
    if (effect.type == QStringLiteral("lose_hp")) {
        return QStringLiteral("Lose %1 HP").arg(effect.value);
    }
    if (effect.type == QStringLiteral("lose_hp_percent")) {
        return QStringLiteral("Lose %1% max HP").arg(effect.value);
    }
    if (effect.type == QStringLiteral("gain_random_relic")) {
        return QStringLiteral("Gain random relic");
    }
    if (effect.type == QStringLiteral("add_steps")) {
        return QStringLiteral("Add %1 steps").arg(effect.value);
    }
    if (effect.type == QStringLiteral("restore_hp_percent")) {
        return QStringLiteral("Restore %1% max HP").arg(effect.value);
    }
    if (effect.type == QStringLiteral("restore_hp")) {
        return QStringLiteral("Restore %1 HP").arg(effect.value);
    }
    if (effect.type == QStringLiteral("deal_damage")) {
        return QStringLiteral("Deal %1 damage").arg(effect.value);
    }
    if (effect.type == QStringLiteral("gain_block")) {
        return QStringLiteral("Gain %1 Block").arg(effect.value);
    }
    if (effect.type == QStringLiteral("gain_gold")) {
        return QStringLiteral("Gain %1 gold").arg(effect.value);
    }
    if (effect.type == QStringLiteral("lose_gold")) {
        return QStringLiteral("Lose %1 gold").arg(effect.value);
    }
    if (effect.type == QStringLiteral("lose_gold_random")) {
        return QStringLiteral("Lose %1~%2 gold").arg(effect.value).arg(effect.duration);
    }
    if (effect.type == QStringLiteral("gain_potion")
        || effect.type == QStringLiteral("gain_random_potion")) {
        if (!effect.target.isEmpty()) {
            return QStringLiteral("Gain potion (%1)").arg(effect.target);
        }
        return QStringLiteral("Gain potion");
    }
    if (effect.type == QStringLiteral("draw_cards")) {
        return QStringLiteral("Draw %1 cards").arg(effect.value);
    }
    if (effect.type == QStringLiteral("gain_strength")) {
        return QStringLiteral("Gain %1 Strength").arg(effect.value);
    }
    if (effect.type == QStringLiteral("upgrade_random_cards")) {
        return QStringLiteral("Upgrade %1 random cards").arg(effect.value);
    }
    if (effect.type == QStringLiteral("upgrade_card")) {
        return QStringLiteral("Upgrade a card");
    }
    if (effect.type == QStringLiteral("remove_card")) {
        return QStringLiteral("Remove a card");
    }
    if (effect.type == QStringLiteral("transform_card")) {
        return QStringLiteral("Transform a card");
    }
    if (effect.type == QStringLiteral("max_hp_increase")) {
        return QStringLiteral("Max HP +%1").arg(effect.value);
    }
    if (effect.type == QStringLiteral("add_card")) {
        if (!effect.target.isEmpty()) {
            return QStringLiteral("Add card (%1)").arg(effect.target);
        }
        return QStringLiteral("Add a card");
    }
    if (!effect.type.isEmpty()) {
        return effect.type;
    }
    return {};
}
