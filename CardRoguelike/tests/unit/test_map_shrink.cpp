#include <QtTest>
#include <QMetaEnum>
#include <array>
#include <cmath>

#include "battle/entities/actor.h"
#include "map/map_manager.h"
#include "map/node_type.h"

class TestMapShrink : public QObject
{
    Q_OBJECT

private:
    static void makeMap(MapManager &map, int seed)
    {
        map.generateMap(seed);
    }

    static int countNodeType(const MapManager &map, NodeType type)
    {
        int count = 0;
        for (int y = 0; y < 11; ++y) {
            for (int x = 0; x < 11; ++x) {
                if (map.nodeType(x, y) == type) {
                    ++count;
                }
            }
        }
        return count;
    }

    static std::pair<int, int> findFirstNodeOfType(const MapManager &map, NodeType type)
    {
        for (int y = 0; y < 11; ++y) {
            for (int x = 0; x < 11; ++x) {
                if (map.nodeType(x, y) == type) {
                    return {x, y};
                }
            }
        }
        return {-1, -1};
    }

    static std::pair<int, int> findAdjacentStart(const MapManager &map, int startX, int startY)
    {
        const std::array<std::pair<int, int>, 4> neighbors = {{
            {startX + 1, startY},
            {startX - 1, startY},
            {startX, startY + 1},
            {startX, startY - 1},
        }};

        for (const auto &neighbor : neighbors) {
            const int x = neighbor.first;
            const int y = neighbor.second;
            if (x >= 0 && x < 11 && y >= 0 && y < 11 && map.canMoveTo(x, y)) {
                return {x, y};
            }
        }

        return {-1, -1};
    }

private slots:
    void testMapGenerationCountsAndFixedPositions()
    {
        MapManager map;
        makeMap(map, 12345);

        QCOMPARE(countNodeType(map, NodeType::Boss), 1);
        QCOMPARE(countNodeType(map, NodeType::Shop), 10);
        QCOMPARE(countNodeType(map, NodeType::Campfire), 16);
        QCOMPARE(countNodeType(map, NodeType::Elite), 10);
        QCOMPARE(countNodeType(map, NodeType::NormalEnemy), 24);
        QCOMPARE(countNodeType(map, NodeType::Event), 20);
        QCOMPARE(countNodeType(map, NodeType::Empty), 40);

        QCOMPARE(map.nodeType(5, 5), NodeType::Boss);

        const std::array<std::pair<int, int>, 4> bossCampfires = {{
            {5, 4}, {5, 6}, {4, 5}, {6, 5},
        }};
        for (const auto &cell : bossCampfires) {
            QCOMPARE(map.nodeType(cell.first, cell.second), NodeType::Campfire);
        }

        const std::array<std::pair<int, int>, 8> startNodes = {{
            {0, 0}, {0, 5}, {0, 10}, {5, 0}, {5, 10}, {10, 0}, {10, 5}, {10, 10},
        }};
        for (const auto &cell : startNodes) {
            QCOMPARE(map.nodeType(cell.first, cell.second), NodeType::Empty);
        }

        QCOMPARE(map.totalSteps(), 0);
        QCOMPARE(map.currentShrinkCount(), 0);
        QCOMPARE(map.nextShrinkStep(), 12);
    }

    void testInitialPlacementMovementAndStepCounting()
    {
        MapManager map;
        makeMap(map, 100);
        Actor player;
        player.setMaxHp(80);
        player.healToFull();
        map.setPlayer(&player);

        QVERIFY(map.canMoveTo(0, 0));
        QVERIFY(!map.canMoveTo(5, 5));

        map.movePlayer(0, 0);
        QCOMPARE(map.totalSteps(), 1);

        const auto adjacent = findAdjacentStart(map, 0, 0);
        QVERIFY(adjacent.first >= 0);

        const NodeType adjacentType = map.nodeType(adjacent.first, adjacent.second);
        const int stepsBefore = map.totalSteps();
        map.movePlayer(adjacent.first, adjacent.second);

        const int expectedStepGain = adjacentType == NodeType::Empty ? 1 : 3;
        QCOMPARE(map.totalSteps(), stepsBefore + expectedStepGain);
    }

    void testExploredNodeUsesEmptyStepCostAndDamage()
    {
        MapManager map;
        makeMap(map, 777);
        Actor player;
        player.setMaxHp(80);
        player.healToFull();
        map.setPlayer(&player);

        map.movePlayer(0, 0);

        const auto enemyCell = findFirstNodeOfType(map, NodeType::NormalEnemy);
        QVERIFY(enemyCell.first >= 0);

        map.setNodeExplored(enemyCell.first, enemyCell.second);
        QVERIFY(map.isExplored(enemyCell.first, enemyCell.second));

        const int stepsBefore = map.totalSteps();
        map.enterNode(enemyCell.first, enemyCell.second);
        QCOMPARE(map.totalSteps(), stepsBefore + 1);

        MapManager shrinkMap;
        makeMap(shrinkMap, 777);
        shrinkMap.setPlayer(&player);
        shrinkMap.movePlayer(0, 0);
        for (int i = 0; i < 11; ++i) {
            shrinkMap.enterNode(0, 0);
        }
        QCOMPARE(shrinkMap.currentShrinkCount(), 1);

        std::pair<int, int> outerEnemyCell = {-1, -1};
        for (int y = 0; y < 11; ++y) {
            for (int x = 0; x < 11; ++x) {
                if (std::max(std::abs(x - 5), std::abs(y - 5)) == 5
                    && shrinkMap.nodeType(x, y) == NodeType::NormalEnemy) {
                    outerEnemyCell = {x, y};
                    break;
                }
            }
            if (outerEnemyCell.first >= 0) {
                break;
            }
        }
        QVERIFY(outerEnemyCell.first >= 0);

        shrinkMap.setNodeExplored(outerEnemyCell.first, outerEnemyCell.second);
        QCOMPARE(shrinkMap.shrinkDamageForNode(outerEnemyCell.first, outerEnemyCell.second),
                 MapManager::shrinkDamage(NodeType::Empty, shrinkMap.currentShrinkCount()));
    }

    void testShrinkTriggersAtStepThresholds()
    {
        MapManager map;
        makeMap(map, 42);
        Actor player;
        player.setMaxHp(200);
        player.healToFull();
        map.setPlayer(&player);

        map.movePlayer(0, 0);
        QCOMPARE(map.totalSteps(), 1);

        for (int i = 0; i < 11; ++i) {
            map.enterNode(0, 0);
        }

        QCOMPARE(map.totalSteps(), 12);
        QCOMPARE(map.currentShrinkCount(), 1);
        QCOMPARE(map.nextShrinkStep(), 24);

        for (int i = 0; i < 12; ++i) {
            map.enterNode(0, 0);
        }

        QCOMPARE(map.totalSteps(), 24);
        QCOMPARE(map.currentShrinkCount(), 2);
        QCOMPARE(map.nextShrinkStep(), 36);
    }

    void testShrinkDamageValues()
    {
        QCOMPARE(MapManager::shrinkDamage(NodeType::Empty, 0), 0);
        QCOMPARE(MapManager::shrinkDamage(NodeType::Empty, 1), 1);
        QCOMPARE(MapManager::shrinkDamage(NodeType::Empty, 3), 3);
        QCOMPARE(MapManager::shrinkDamage(NodeType::Shop, 2), 6);
        QCOMPARE(MapManager::shrinkDamage(NodeType::Event, 4), 12);
        QCOMPARE(MapManager::shrinkDamage(NodeType::NormalEnemy, 5), 15);

        MapManager map;
        makeMap(map, 55);
        Actor player;
        player.setMaxHp(200);
        player.healToFull();
        map.setPlayer(&player);
        map.movePlayer(0, 0);

        for (int i = 0; i < 11; ++i) {
            map.enterNode(0, 0);
        }
        QCOMPARE(map.currentShrinkCount(), 1);

        const int outerLayerDamage = map.shrinkDamageForNode(0, 0);
        QCOMPARE(outerLayerDamage, MapManager::shrinkDamage(NodeType::Empty, 1));

        const auto shopCell = findFirstNodeOfType(map, NodeType::Shop);
        if (shopCell.first >= 0 && map.shrinkDamageForNode(shopCell.first, shopCell.second) > 0) {
            QCOMPARE(map.shrinkDamageForNode(shopCell.first, shopCell.second),
                     MapManager::shrinkDamage(NodeType::Shop, 1));
        }
    }

    void testShrinkZoneEmptyNodeCanKillPlayer()
    {
        MapManager map;
        makeMap(map, 99);
        Actor player;
        player.setMaxHp(1);
        player.healToFull();
        map.setPlayer(&player);

        map.movePlayer(0, 0);

        for (int i = 0; i < 11; ++i) {
            map.enterNode(0, 0);
        }

        QCOMPARE(map.currentShrinkCount(), 1);
        map.enterNode(0, 0);

        QVERIFY(player.currentHp() <= 0);
        QVERIFY(map.isPlayerDead());
    }

    void testBossNodeNeverInShrinkZone()
    {
        MapManager map;
        makeMap(map, 1);
        Actor player;
        player.setMaxHp(500);
        player.healToFull();
        map.setPlayer(&player);

        map.movePlayer(0, 0);

        for (int i = 0; i < 59; ++i) {
            map.enterNode(0, 0);
        }

        QCOMPARE(map.currentShrinkCount(), 5);
        QCOMPARE(map.shrinkDamageForNode(5, 5), 0);
    }

    void testCombatShrinkCountOnlyWhenPlayerInZone()
    {
        MapManager map;
        makeMap(map, 55);
        Actor player;
        player.setMaxHp(200);
        player.healToFull();
        map.setPlayer(&player);

        map.movePlayer(0, 0);
        QCOMPARE(map.combatShrinkCount(), 0);

        for (int i = 0; i < 11; ++i) {
            map.enterNode(0, 0);
        }
        QCOMPARE(map.currentShrinkCount(), 1);
        QCOMPARE(map.combatShrinkCount(), 1);

        // Inner safe cell (layer 3) is outside shrink zone when n == 1.
        map.setPlayerPosition(5, 2);
        QVERIFY(!map.isInShrinkZone(5, 2, 1));
        QCOMPARE(map.combatShrinkCount(), 0);

        map.setPlayerPosition(0, 0);
        QCOMPARE(map.combatShrinkCount(), 1);
    }
};

int runMapShrinkTests(int argc, char *argv[])
{
    TestMapShrink tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "test_map_shrink.moc"
