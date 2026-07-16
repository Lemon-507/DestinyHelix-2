#include <QtTest>

#include <QTemporaryFile>



#include "battle/data/enemy_database.h"

#include "battle/data/relic_database.h"

#include "battle/entities/card.h"
#include "battle/entities/actor.h"
#include "battle/entities/enemy.h"

#include "battle/entities/player.h"

#include "battle/logic/battlefsm.h"

#include "battle/logic/relic_manager.h"

#include "character_factory.h"

#include "game_manager.h"

#include "systems/event_bus.h"



namespace {



void prepareTestEnemy(Enemy &enemy, int hp = 40)

{

    EnemyDatabase::instance().load();

    const EnemyData *data = EnemyDatabase::instance().getEnemy(QStringLiteral("cultist"));

    if (data != nullptr) {

        enemy.loadFromData(*data);

    }

    enemy.setMaxHp(hp);

    enemy.healToFull();

}



QVector<Enemy *> enemyList(Enemy *enemy)

{

    return QVector<Enemy *>{enemy};

}



QVector<Actor *> actorEnemyList(Enemy *enemy)

{

    return QVector<Actor *>{enemy};

}



} // namespace



class TestRelics : public QObject

{

    Q_OBJECT



private:

    void populateStrikeDeck(Player &player) const

    {

        const Card strike = CardBuilder(QStringLiteral("strike"), CardType::Attack)

                                .setCost(1)

                                .addEffect(EffectDef::damage(6))

                                .build();

        for (int i = 0; i < 10; ++i) {

            player.addCardToDeck(strike);

        }

    }



    Card findStrikeInHand(const BattleFSM &fsm) const

    {

        for (const Card &card : fsm.hand()) {

            if (card.id() == QStringLiteral("strike")) {

                return card;

            }

        }

        return Card();

    }



private slots:

    void initTestCase()

    {

        CharacterFactory::initialize();

        RelicDatabase::instance().load();

    }



    void test_burning_blood_on_combat_end()

    {

        Player player;

        populateStrikeDeck(player);

        player.setMaxHp(80);

        player.healToFull();

        player.takeDirectDamage(20);



        Enemy enemy;

        prepareTestEnemy(enemy, 6);



        RelicManager relicManager;

        const RelicData burningBlood = RelicDatabase::instance().getRelic(QStringLiteral("burning_blood"));

        QVERIFY(!burningBlood.id.isEmpty());



        relicManager.registerRelic(burningBlood, &player, nullptr);

        relicManager.setBattleContext(nullptr, actorEnemyList(&enemy));



        BattleFSM fsm;

        fsm.startCombat(&player, enemyList(&enemy));



        const int hpBeforeWin = player.currentHp();



        const Card strike = findStrikeInHand(fsm);

        QVERIFY(!strike.id().isEmpty());

        fsm.playCard(strike, &enemy);



        QCOMPARE(enemy.currentHp(), 0);

        QCOMPARE(player.currentHp(), hpBeforeWin + 6);

    }



    void test_burning_blood_heal_does_not_stack_across_combats()

    {

        Player player;

        populateStrikeDeck(player);

        player.setMaxHp(80);

        player.healToFull();

        player.takeDirectDamage(30);



        RelicManager relicManager;

        const RelicData burningBlood = RelicDatabase::instance().getRelic(QStringLiteral("burning_blood"));

        QVERIFY(!burningBlood.id.isEmpty());

        relicManager.registerRelic(burningBlood, &player, nullptr);



        for (int fight = 0; fight < 3; ++fight) {

            Enemy enemy;

            prepareTestEnemy(enemy, 6);

            relicManager.setBattleContext(nullptr, actorEnemyList(&enemy));



            BattleFSM fsm;

            fsm.startCombat(&player, enemyList(&enemy));



            const int hpBeforeWin = player.currentHp();

            const Card strike = findStrikeInHand(fsm);

            QVERIFY(!strike.id().isEmpty());

            fsm.playCard(strike, &enemy);



            QCOMPARE(enemy.currentHp(), 0);

            QCOMPARE(player.currentHp(), hpBeforeWin + 6);

        }

    }



    void test_marble_bag_applies_vulnerable_on_battle_start()

    {

        Player player;

        player.setMaxHp(80);

        player.healToFull();



        Enemy enemy;

        prepareTestEnemy(enemy);



        RelicManager relicManager;

        const RelicData marbleBag = RelicDatabase::instance().getRelic(QStringLiteral("marble_bag"));

        QVERIFY(!marbleBag.id.isEmpty());



        relicManager.registerRelic(marbleBag, &player, nullptr);

        relicManager.setBattleContext(nullptr, actorEnemyList(&enemy));



        BattleFSM fsm;

        fsm.startCombat(&player, enemyList(&enemy));



        QVERIFY(enemy.hasPower(QStringLiteral("vulnerable")));

        QCOMPARE(enemy.getPowerValue(QStringLiteral("vulnerable")), 1);

    }



    void test_lizard_tail_revives_once()

    {

        Player player;

        player.setMaxHp(80);

        player.healToFull();



        RelicManager relicManager;

        const RelicData lizardTail = RelicDatabase::instance().getRelic(QStringLiteral("lizard_tail"));

        QVERIFY(!lizardTail.id.isEmpty());



        player.addRelicId(QStringLiteral("lizard_tail"));

        relicManager.registerRelic(lizardTail, &player, nullptr);



        Enemy enemy;

        prepareTestEnemy(enemy);



        player.takeDirectDamage(80);

        QCOMPARE(player.currentHp(), 40);

        QVERIFY(!player.hasRelic(QStringLiteral("lizard_tail")));

        QVERIFY(!relicManager.isRegistered(QStringLiteral("lizard_tail")));



        player.takeDirectDamage(80);

        QCOMPARE(player.currentHp(), 0);

    }



    void test_old_coin_grants_gold_on_pickup()

    {

        Player player;

        player.setMaxHp(80);

        player.healToFull();

        player.setGold(99);



        RelicManager relicManager;

        const RelicData oldCoin = RelicDatabase::instance().getRelic(QStringLiteral("old_coin"));

        QVERIFY(!oldCoin.id.isEmpty());



        relicManager.registerRelic(oldCoin, &player, nullptr);



        QCOMPARE(player.gold(), 399);

    }



    void test_relic_save_load()

    {

        GameManager gameManager;

        gameManager.startNewGame(QStringLiteral("ironclad"), 123);

        gameManager.addRelic(QStringLiteral("marble_bag"));



        QVERIFY(gameManager.relicManager()->isRegistered(QStringLiteral("burning_blood")));

        QVERIFY(gameManager.relicManager()->isRegistered(QStringLiteral("marble_bag")));



        QTemporaryFile tempFile;

        QVERIFY(tempFile.open());

        tempFile.close();

        gameManager.saveGame(tempFile.fileName());



        GameManager loadedManager;

        loadedManager.startNewGame(QStringLiteral("ironclad"), 0);

        QVERIFY(loadedManager.loadGame(tempFile.fileName()));



        QCOMPARE(loadedManager.player()->relicIds().size(), 2);

        QVERIFY(loadedManager.relicManager()->isRegistered(QStringLiteral("burning_blood")));

        QVERIFY(loadedManager.relicManager()->isRegistered(QStringLiteral("marble_bag")));

    }

};



int runRelicTests(int argc, char *argv[])

{

    TestRelics tests;

    return QTest::qExec(&tests, argc, argv);

}



#include "test_relics.moc"


