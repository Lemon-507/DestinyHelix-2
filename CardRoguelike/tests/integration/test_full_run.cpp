#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>

#include "battle/data/card_database.h"
#include "battle/data/enemy_database.h"
#include "battle/data/relic_database.h"
#include "battle/entities/enemy.h"
#include "battle/entities/player.h"
#include "battle/logic/battlefsm.h"
#include "character_factory.h"
#include "events/event_database.h"
#include "events/event_manager.h"
#include "game_manager.h"
#include "map/map_manager.h"
#include "potions/potion_database.h"
#include "save/save_manager.h"

namespace {

void prepareTestEnemy(Enemy &enemy, const QString &id, int hpOverride = -1)
{
    EnemyDatabase::instance().load();
    const EnemyData *data = EnemyDatabase::instance().getEnemy(id);
    if (data != nullptr) {
        enemy.loadFromData(*data);
    }
    if (hpOverride >= 0) {
        enemy.setMaxHp(hpOverride);
        enemy.healToFull();
    }
}

QVector<Enemy *> enemyList(Enemy *enemy)
{
    return QVector<Enemy *>{enemy};
}

} // namespace

class TestFullRun : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        CharacterFactory::initialize();
    }

    void test_shrink_death_on_map_movement()
    {
        GameManager gameManager;
        gameManager.startNewGame(QStringLiteral("ironclad"), 99);
        gameManager.placePlayerAtStart(0, 0);

        Player *player = gameManager.player();
        player->setMaxHp(1);
        player->healToFull();

        MapManager *map = gameManager.mapManager();
        for (int i = 0; i < 11; ++i) {
            map->enterNode(0, 0);
        }
        QCOMPARE(map->currentShrinkCount(), 1);

        QSignalSpy diedSpy(&gameManager, &GameManager::playerDied);
        map->enterNode(0, 0);

        QVERIFY(player->currentHp() <= 0);
        QVERIFY(map->isPlayerDead());
        QCOMPARE(diedSpy.count(), 0);
    }

    void test_shrink_death_in_combat()
    {
        Player player;
        player.setMaxHp(1);
        player.healToFull();

        Enemy enemy;
        prepareTestEnemy(enemy, QStringLiteral("cultist"));

        BattleFSM fsm;
        QSignalSpy endedSpy(&fsm, &BattleFSM::combatEnded);
        fsm.startCombat(&player, enemyList(&enemy), 5);

        QCOMPARE(player.currentHp(), 0);
        QCOMPARE(endedSpy.count(), 1);
        QCOMPARE(endedSpy.at(0).at(0).toBool(), false);
    }

    void test_save_load_mid_run()
    {
        GameManager gameManager;
        gameManager.startNewGame(QStringLiteral("ironclad"), 777);
        gameManager.placePlayerAtStart(0, 0);
        gameManager.addRelic(QStringLiteral("vajra"));
        gameManager.mapManager()->addSteps(5);

        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        tempFile.close();
        gameManager.saveGame(tempFile.fileName());

        GameManager loadedManager;
        loadedManager.startNewGame(QStringLiteral("ironclad"), 0);
        QVERIFY(loadedManager.loadGame(tempFile.fileName()));

        QCOMPARE(loadedManager.player()->currentHp(), gameManager.player()->currentHp());
        QCOMPARE(loadedManager.player()->gold(), gameManager.player()->gold());
        QCOMPARE(loadedManager.mapManager()->totalSteps(), gameManager.mapManager()->totalSteps());
        QCOMPARE(loadedManager.player()->relicIds(), gameManager.player()->relicIds());
        QCOMPARE(loadedManager.player()->deck().size(), gameManager.player()->deck().size());
        QVERIFY(loadedManager.player()->relicIds().contains(QStringLiteral("vajra")));
    }

    void test_event_old_chest_and_merchant()
    {
        Player *player = CharacterFactory::createCharacter(QStringLiteral("ironclad"));
        MapManager mapManager;
        mapManager.generateMap(100);
        mapManager.setPlayer(player);

        EventManager eventManager;
        const EventData *chest = EventDatabase::instance().getEvent(QStringLiteral("suspicious_chest"));
        QVERIFY(chest != nullptr);

        const int hpBefore = player->currentHp();
        const int relicCountBefore = player->relicIds().size();
        QVERIFY(eventManager.executeOption(chest->options.at(0), player, &mapManager));
        QCOMPARE(player->currentHp(), hpBefore - 20);
        QVERIFY(player->relicIds().size() >= relicCountBefore);

        const EventData *merchant = EventDatabase::instance().getEvent(QStringLiteral("wandering_merchant"));
        QVERIFY(merchant != nullptr);
        const int goldBefore = player->gold();
        QVERIFY(eventManager.executeOption(merchant->options.at(0), player, &mapManager));
        QCOMPARE(player->gold(), goldBefore - 20);
        int potionTotal = 0;
        for (int count : player->potionCounts()) {
            potionTotal += count;
        }
        QVERIFY(potionTotal >= 1);

        delete player;
    }

    void test_boss_combat_victory()
    {
        GameManager gameManager;
        gameManager.startNewGame(QStringLiteral("ironclad"), 1);

        MapManager *map = gameManager.mapManager();
        map->setPlayer(gameManager.player());
        map->setPlayerPosition(5, 4);
        map->setNodeExplored(5, 4);

        Player *player = gameManager.player();
        Enemy enemy;
        prepareTestEnemy(enemy, QStringLiteral("slime_boss"), 8);

        BattleFSM fsm;
        QSignalSpy endedSpy(&fsm, &BattleFSM::combatEnded);
        fsm.startCombat(player, enemyList(&enemy));

        CardDatabase::instance().load();
        Card strikePlus = CardDatabase::instance().getCard(QStringLiteral("strike+"));
        if (strikePlus.id().isEmpty()) {
            strikePlus = CardDatabase::instance().getCard(QStringLiteral("strike"));
        }

        for (int turn = 0; turn < 20 && enemy.currentHp() > 0; ++turn) {
            const QVector<Card> hand = fsm.hand();
            bool played = false;
            for (const Card &card : hand) {
                if (card.cost() <= fsm.currentEnergy()) {
                    fsm.playCard(card, &enemy);
                    played = true;
                    break;
                }
            }
            if (enemy.currentHp() <= 0) {
                break;
            }
            if (fsm.battleState() == BattleState::PlayerTurn && !played) {
                fsm.playerEndTurn();
            }
            if (fsm.battleState() == BattleState::Ended) {
                break;
            }
        }

        QCOMPARE(enemy.currentHp(), 0);
        QVERIFY(endedSpy.count() >= 1);
        QCOMPARE(endedSpy.at(0).at(0).toBool(), true);
    }

    void test_explosive_and_block_potions()
    {
        CharacterFactory::initialize();
        Player *player = CharacterFactory::createCharacter(QStringLiteral("ironclad"));
        Enemy enemy;
        prepareTestEnemy(enemy, QStringLiteral("cultist"));

        const PotionData explosive = PotionDatabase::instance().getPotion(QStringLiteral("explosive_potion"));
        const PotionData blockPotion = PotionDatabase::instance().getPotion(QStringLiteral("block_potion"));
        QVERIFY(!explosive.id.isEmpty());
        QVERIFY(!blockPotion.id.isEmpty());

        const int hpBefore = enemy.currentHp();
        PotionDatabase::instance().usePotion(explosive, player, &enemy, {&enemy});
        QCOMPARE(enemy.currentHp(), hpBefore - 10);

        PotionDatabase::instance().usePotion(blockPotion, player, &enemy);
        QCOMPARE(player->currentBlock(), 12);

        delete player;
    }

    void test_invalid_save_rejected()
    {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        tempFile.write("{ \"player_x\": 0 }");
        tempFile.close();

        GameState state;
        SaveManager saveManager;
        QVERIFY(!saveManager.loadGame(tempFile.fileName(), state));
    }
};

int runIntegrationTests(int argc, char *argv[])
{
    TestFullRun tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "test_full_run.moc"
