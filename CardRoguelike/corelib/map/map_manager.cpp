#include "map_manager.h"

#include <QtCore/QtGlobal>
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "battle/entities/actor.h"
#include "battle/entities/player.h"
#include "systems/event_bus.h"
#include "utils/rng.h"

namespace {

constexpr int kGridSize = 11;
constexpr int kCenter = 5;

bool isFixedCampfireNode(int x, int y)
{
    return (x == 4 && y == 5) || (x == 6 && y == 5) || (x == 5 && y == 4) || (x == 5 && y == 6);
}

} // namespace

MapManager::MapManager(QObject *parent)
    : QObject(parent)
{
}

void MapManager::generateMap(int seed)
{
    if (seed < 0) {
        seed = RNG::nonDeterministicSeed();
    }
    m_mapSeed = seed;
    RNG::seed(seed);

    for (int y = 0; y < kGridSize; ++y) {
        for (int x = 0; x < kGridSize; ++x) {
            m_grid[y][x] = MapNode{x, y, NodeType::Empty, false, QString()};
        }
    }

    m_grid[kCenter][kCenter].type = NodeType::Boss;

    const std::array<std::pair<int, int>, 4> bossCampfires = {{
        {5, 4}, {5, 6}, {4, 5}, {6, 5},
    }};
    for (const auto &campfire : bossCampfires) {
        m_grid[campfire.second][campfire.first].type = NodeType::Campfire;
    }

    const std::array<std::pair<int, int>, 8> startNodes = {{
        {0, 0}, {0, 5}, {0, 10}, {5, 0}, {5, 10}, {10, 0}, {10, 5}, {10, 10},
    }};

    for (const auto &startNode : startNodes) {
        m_grid[startNode.second][startNode.first].type = NodeType::Empty;
    }

    std::vector<std::pair<int, int>> fillCells;
    for (int y = 0; y < kGridSize; ++y) {
        for (int x = 0; x < kGridSize; ++x) {
            if (x == kCenter && y == kCenter) {
                continue;
            }
            if (isFixedCampfireNode(x, y)) {
                continue;
            }
            if (isFixedStartNode(x, y)) {
                continue;
            }
            fillCells.emplace_back(x, y);
        }
    }

    std::vector<NodeType> pool;
    pool.reserve(fillCells.size());
    for (int i = 0; i < 10; ++i) {
        pool.push_back(NodeType::Shop);
    }
    for (int i = 0; i < 12; ++i) {
        pool.push_back(NodeType::Campfire);
    }
    for (int i = 0; i < 10; ++i) {
        pool.push_back(NodeType::Elite);
    }
    for (int i = 0; i < 24; ++i) {
        pool.push_back(NodeType::NormalEnemy);
    }
    for (int i = 0; i < 20; ++i) {
        pool.push_back(NodeType::Event);
    }
    for (int i = 0; i < 32; ++i) {
        pool.push_back(NodeType::Empty);
    }

    for (int i = static_cast<int>(pool.size()) - 1; i > 0; --i) {
        const int j = RNG::get(0, i);
        std::swap(pool[i], pool[j]);
    }

    for (std::size_t i = 0; i < fillCells.size(); ++i) {
        const auto &cell = fillCells[i];
        m_grid[cell.second][cell.first].type = pool[i];
    }

    m_totalSteps = 0;
    m_currentShrinkCount = 0;
    m_nextShrinkStep = 12;
    m_playerX = -1;
    m_playerY = -1;
    m_playerDead = false;
}

int MapManager::mapSeed() const
{
    return m_mapSeed;
}

bool MapManager::canMoveTo(int x, int y) const
{
    if (x < 0 || x >= kGridSize || y < 0 || y >= kGridSize) {
        return false;
    }

    if (m_playerX == -1 && m_playerY == -1) {
        return isFixedStartNode(x, y);
    }

    const int dx = std::abs(x - m_playerX);
    const int dy = std::abs(y - m_playerY);
    return dx + dy == 1;
}

void MapManager::movePlayer(int x, int y)
{
    Q_ASSERT(canMoveTo(x, y));

    m_playerX = x;
    m_playerY = y;
    enterNode(x, y);
}

void MapManager::enterNode(int x, int y)
{
    Q_ASSERT(x >= 0 && x < kGridSize && y >= 0 && y < kGridSize);

    const MapNode &node = m_grid[y][x];
    const NodeType effectiveType = node.explored ? NodeType::Empty : node.type;

    bool skipStepCost = false;
    if (node.explored) {
        if (auto *player = qobject_cast<Player *>(m_player)) {
            skipStepCost = player->hasRelic(QStringLiteral("hiking_boots"));
        }
    }

    if (!skipStepCost) {
        if (effectiveType == NodeType::Empty) {
            m_totalSteps += 1;
        } else {
            m_totalSteps += 3;
        }
    }

    checkShrink();

    const bool deferDamage = !node.explored && isCombatType(node.type);
    if (!deferDamage && m_player != nullptr) {
        applyShrinkDamageOnEnter(*m_player, effectiveType);
    }
}

void MapManager::checkShrink()
{
    while (m_totalSteps >= m_nextShrinkStep && m_currentShrinkCount < 5) {
        ++m_currentShrinkCount;
        m_nextShrinkStep += 12;
        EventBus::instance().triggerShrinkTriggered(m_currentShrinkCount);
    }
}

int MapManager::totalSteps() const
{
    return m_totalSteps;
}

int MapManager::nextShrinkStep() const
{
    return m_nextShrinkStep;
}

int MapManager::currentShrinkCount() const
{
    return m_currentShrinkCount;
}

MapNode MapManager::nodeAt(int x, int y) const
{
    Q_ASSERT(x >= 0 && x < kGridSize && y >= 0 && y < kGridSize);
    return m_grid[y][x];
}

NodeType MapManager::nodeType(int x, int y) const
{
    return nodeAt(x, y).type;
}

QString MapManager::enemyIdAt(int x, int y) const
{
    return nodeAt(x, y).enemyId;
}

bool MapManager::isExplored(int x, int y) const
{
    return nodeAt(x, y).explored;
}

void MapManager::setNodeExplored(int x, int y)
{
    Q_ASSERT(x >= 0 && x < kGridSize && y >= 0 && y < kGridSize);
    m_grid[y][x].explored = true;
}

void MapManager::setEnemyIdAt(int x, int y, const QString &enemyId)
{
    Q_ASSERT(x >= 0 && x < kGridSize && y >= 0 && y < kGridSize);
    m_grid[y][x].enemyId = enemyId;
}

int MapManager::shrinkDamage(NodeType type, int shrinkCount)
{
    if (shrinkCount <= 0) {
        return 0;
    }

    if (type == NodeType::Empty) {
        return shrinkCount;
    }

    return 3 * shrinkCount;
}

int MapManager::shrinkDamageForNode(int x, int y) const
{
    Q_ASSERT(x >= 0 && x < kGridSize && y >= 0 && y < kGridSize);

    const MapNode &node = m_grid[y][x];
    const NodeType effectiveType = node.explored ? NodeType::Empty : node.type;

    if (!isInShrinkZone(x, y, m_currentShrinkCount)) {
        return 0;
    }

    return shrinkDamage(effectiveType, m_currentShrinkCount);
}

void MapManager::applyShrinkDamageOnEnter(Actor &player, NodeType effectiveType)
{
    if (!isInShrinkZone(m_playerX, m_playerY, m_currentShrinkCount)) {
        return;
    }

    const int damage = shrinkDamage(effectiveType, m_currentShrinkCount);
    if (damage <= 0) {
        return;
    }

    if (auto *ownedPlayer = qobject_cast<Player *>(&player)) {
        if (ownedPlayer->hasRelic(QStringLiteral("fate"))) {
            const int newHp = qMax(1, ownedPlayer->currentHp() - damage);
            const int actualDamage = ownedPlayer->currentHp() - newHp;
            if (actualDamage > 0) {
                ownedPlayer->takeDirectDamage(actualDamage);
            }
        } else {
            player.takeDamage(damage);
        }
    } else {
        player.takeDamage(damage);
    }

    if (player.currentHp() <= 0) {
        m_playerDead = true;
    }
}

bool MapManager::isPlayerDead() const
{
    return m_playerDead;
}

void MapManager::setPlayer(Actor *player)
{
    m_player = player;
}

int MapManager::nodeLayer(int x, int y)
{
    return std::max(std::abs(x - kCenter), std::abs(y - kCenter));
}

bool MapManager::isFixedStartNode(int x, int y)
{
    return (x == 0 && y == 0) || (x == 0 && y == 5) || (x == 0 && y == 10) || (x == 5 && y == 0)
           || (x == 5 && y == 10) || (x == 10 && y == 0) || (x == 10 && y == 5) || (x == 10 && y == 10);
}

bool MapManager::isCombatType(NodeType type)
{
    return type == NodeType::Elite || type == NodeType::NormalEnemy || type == NodeType::Boss;
}

bool MapManager::isInShrinkZone(int x, int y, int shrinkCount) const
{
    if (x == kCenter && y == kCenter) {
        return false;
    }

    if (shrinkCount <= 0) {
        return false;
    }

    return nodeLayer(x, y) > (5 - shrinkCount);
}

int MapManager::combatShrinkCount() const
{
    if (!isInShrinkZone(m_playerX, m_playerY, m_currentShrinkCount)) {
        return 0;
    }
    return m_currentShrinkCount;
}

void MapManager::addSteps(int steps)
{
    if (steps <= 0) {
        return;
    }

    m_totalSteps += steps;
    checkShrink();
}

int MapManager::playerX() const
{
    return m_playerX;
}

int MapManager::playerY() const
{
    return m_playerY;
}

void MapManager::setPlayerPosition(int x, int y)
{
    m_playerX = x;
    m_playerY = y;
}

void MapManager::setTotalSteps(int steps)
{
    m_totalSteps = steps;
}

void MapManager::setCurrentShrinkCount(int count)
{
    m_currentShrinkCount = count;
}

void MapManager::setNextShrinkStep(int step)
{
    m_nextShrinkStep = step;
}

QVector<QPair<int, int>> MapManager::exploredNodes() const
{
    QVector<QPair<int, int>> nodes;
    for (int y = 0; y < kGridSize; ++y) {
        for (int x = 0; x < kGridSize; ++x) {
            if (m_grid[y][x].explored) {
                nodes.append(qMakePair(x, y));
            }
        }
    }
    return nodes;
}

void MapManager::restoreExploredNodes(const QVector<QPair<int, int>> &nodes)
{
    for (int y = 0; y < kGridSize; ++y) {
        for (int x = 0; x < kGridSize; ++x) {
            m_grid[y][x].explored = false;
        }
    }

    for (const QPair<int, int> &node : nodes) {
        if (node.first >= 0 && node.first < kGridSize && node.second >= 0 && node.second < kGridSize) {
            m_grid[node.second][node.first].explored = true;
        }
    }
}
