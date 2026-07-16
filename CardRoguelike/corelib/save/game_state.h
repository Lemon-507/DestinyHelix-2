#pragma once

#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

struct GameState {
    int playerX = -1;
    int playerY = -1;
    int playerHp = 0;
    int playerMaxHp = 0;
    int gold = 0;
    QStringList deckCardIds;
    QStringList relicIds;
    QHash<QString, int> potionCounts;
    QVector<QPair<int, int>> exploredNodes;
    int totalSteps = 0;
    int shrinkCount = 0;
    int nextShrinkStep = 12;
    QSet<QString> completedEventIds;
    int mapSeed = 0;
    int shopDeletionCount = 0;
};
