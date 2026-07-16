#include "save_manager.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace {

bool validateSaveRoot(const QJsonObject &root)
{
    const QStringList requiredKeys = {
        QStringLiteral("player_x"),
        QStringLiteral("player_y"),
        QStringLiteral("player_hp"),
        QStringLiteral("player_max_hp"),
        QStringLiteral("gold"),
        QStringLiteral("total_steps"),
        QStringLiteral("shrink_count"),
        QStringLiteral("next_shrink_step"),
        QStringLiteral("map_seed"),
        QStringLiteral("deck"),
        QStringLiteral("relics"),
        QStringLiteral("potions"),
        QStringLiteral("explored_nodes"),
        QStringLiteral("completed_events"),
    };

    for (const QString &key : requiredKeys) {
        if (!root.contains(key)) {
            return false;
        }
    }

    if (!root.value(QStringLiteral("deck")).isArray()) {
        return false;
    }
    if (!root.value(QStringLiteral("relics")).isArray()) {
        return false;
    }
    if (!root.value(QStringLiteral("potions")).isObject()) {
        return false;
    }
    if (!root.value(QStringLiteral("explored_nodes")).isArray()) {
        return false;
    }
    if (!root.value(QStringLiteral("completed_events")).isArray()) {
        return false;
    }

    const int maxHp = root.value(QStringLiteral("player_max_hp")).toInt();
    const int hp = root.value(QStringLiteral("player_hp")).toInt();
    if (maxHp <= 0 || hp < 0 || hp > maxHp) {
        return false;
    }

    return true;
}

} // namespace

SaveManager::SaveManager(QObject *parent)
    : QObject(parent)
{
}

void SaveManager::saveGame(const QString &path, const GameState &state)
{
    QJsonObject root;
    root.insert(QStringLiteral("player_x"), state.playerX);
    root.insert(QStringLiteral("player_y"), state.playerY);
    root.insert(QStringLiteral("player_hp"), state.playerHp);
    root.insert(QStringLiteral("player_max_hp"), state.playerMaxHp);
    root.insert(QStringLiteral("gold"), state.gold);
    root.insert(QStringLiteral("total_steps"), state.totalSteps);
    root.insert(QStringLiteral("shrink_count"), state.shrinkCount);
    root.insert(QStringLiteral("next_shrink_step"), state.nextShrinkStep);
    root.insert(QStringLiteral("map_seed"), state.mapSeed);
    root.insert(QStringLiteral("shop_deletion_count"), state.shopDeletionCount);

    QJsonArray deck;
    for (const QString &cardId : state.deckCardIds) {
        deck.append(cardId);
    }
    root.insert(QStringLiteral("deck"), deck);

    QJsonArray relics;
    for (const QString &relicId : state.relicIds) {
        relics.append(relicId);
    }
    root.insert(QStringLiteral("relics"), relics);

    QJsonObject potions;
    for (auto it = state.potionCounts.constBegin(); it != state.potionCounts.constEnd(); ++it) {
        potions.insert(it.key(), it.value());
    }
    root.insert(QStringLiteral("potions"), potions);

    QJsonArray explored;
    for (const QPair<int, int> &node : state.exploredNodes) {
        QJsonObject nodeObj;
        nodeObj.insert(QStringLiteral("x"), node.first);
        nodeObj.insert(QStringLiteral("y"), node.second);
        explored.append(nodeObj);
    }
    root.insert(QStringLiteral("explored_nodes"), explored);

    QJsonArray completedEvents;
    for (const QString &eventId : state.completedEventIds) {
        completedEvents.append(eventId);
    }
    root.insert(QStringLiteral("completed_events"), completedEvents);

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
    }
}

bool SaveManager::loadGame(const QString &path, GameState &state)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    if (root.isEmpty() || !validateSaveRoot(root)) {
        return false;
    }

    state.playerX = root.value(QStringLiteral("player_x")).toInt(-1);
    state.playerY = root.value(QStringLiteral("player_y")).toInt(-1);
    state.playerHp = root.value(QStringLiteral("player_hp")).toInt();
    state.playerMaxHp = root.value(QStringLiteral("player_max_hp")).toInt();
    state.gold = root.value(QStringLiteral("gold")).toInt();
    state.totalSteps = root.value(QStringLiteral("total_steps")).toInt();
    state.shrinkCount = root.value(QStringLiteral("shrink_count")).toInt();
    state.nextShrinkStep = root.value(QStringLiteral("next_shrink_step")).toInt(12);
    state.mapSeed = root.value(QStringLiteral("map_seed")).toInt();
    state.shopDeletionCount = root.value(QStringLiteral("shop_deletion_count")).toInt(0);

    state.deckCardIds.clear();
    const QJsonArray deck = root.value(QStringLiteral("deck")).toArray();
    for (const QJsonValue &value : deck) {
        state.deckCardIds.append(value.toString());
    }

    state.relicIds.clear();
    const QJsonArray relics = root.value(QStringLiteral("relics")).toArray();
    for (const QJsonValue &value : relics) {
        state.relicIds.append(value.toString());
    }

    state.potionCounts.clear();
    const QJsonObject potions = root.value(QStringLiteral("potions")).toObject();
    for (auto it = potions.constBegin(); it != potions.constEnd(); ++it) {
        state.potionCounts.insert(it.key(), it.value().toInt());
    }

    state.exploredNodes.clear();
    const QJsonArray explored = root.value(QStringLiteral("explored_nodes")).toArray();
    for (const QJsonValue &value : explored) {
        const QJsonObject nodeObj = value.toObject();
        state.exploredNodes.append(qMakePair(nodeObj.value(QStringLiteral("x")).toInt(),
                                           nodeObj.value(QStringLiteral("y")).toInt()));
    }

    state.completedEventIds.clear();
    const QJsonArray completedEvents = root.value(QStringLiteral("completed_events")).toArray();
    for (const QJsonValue &value : completedEvents) {
        state.completedEventIds.insert(value.toString());
    }

    return true;
}
