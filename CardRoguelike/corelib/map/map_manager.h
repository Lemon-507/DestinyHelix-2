#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <array>

#include "map/node_type.h"

class Actor;

struct MapNode {
    int x = 0;
    int y = 0;
    NodeType type = NodeType::Empty;
    bool explored = false;
    QString enemyId;
};

class MapManager : public QObject
{
    Q_OBJECT

public:
    explicit MapManager(QObject *parent = nullptr);

    void generateMap(int seed = -1);
    int mapSeed() const;
    bool canMoveTo(int x, int y) const;
    void movePlayer(int x, int y);
    void enterNode(int x, int y);

    int totalSteps() const;
    int nextShrinkStep() const;
    int currentShrinkCount() const;

    MapNode nodeAt(int x, int y) const;
    NodeType nodeType(int x, int y) const;
    QString enemyIdAt(int x, int y) const;
    bool isExplored(int x, int y) const;

    void setNodeExplored(int x, int y);
    void setEnemyIdAt(int x, int y, const QString &enemyId);
    int shrinkDamageForNode(int x, int y) const;
    void applyShrinkDamageOnEnter(Actor &player, NodeType type);
    bool isPlayerDead() const;

    void setPlayer(Actor *player);

    void addSteps(int steps);
    int playerX() const;
    int playerY() const;
    void setPlayerPosition(int x, int y);
    void setTotalSteps(int steps);
    void setCurrentShrinkCount(int count);
    void setNextShrinkStep(int step);
    QVector<QPair<int, int>> exploredNodes() const;
    void restoreExploredNodes(const QVector<QPair<int, int>> &nodes);

    static int shrinkDamage(NodeType type, int shrinkCount);

    void checkShrink();
    bool isInShrinkZone(int x, int y, int shrinkCount) const;

    // n if the player is currently inside the shrink zone; otherwise 0.
    // Used when starting combat so ShrinkDamagePower is only applied in-zone.
    int combatShrinkCount() const;

private:
    static int nodeLayer(int x, int y);
    static bool isFixedStartNode(int x, int y);
    static bool isCombatType(NodeType type);

    std::array<std::array<MapNode, 11>, 11> m_grid;
    Actor *m_player = nullptr;
    int m_playerX = -1;
    int m_playerY = -1;
    int m_totalSteps = 0;
    int m_currentShrinkCount = 0;
    int m_nextShrinkStep = 12;
    bool m_playerDead = false;
    int m_mapSeed = 0;
};
