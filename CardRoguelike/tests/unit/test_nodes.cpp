#include <QtTest>
#include <QTemporaryFile>

#include "battle/data/enemy_database.h"
#include "battle/data/card_database.h"
#include "battle/entities/enemy.h"
#include "battle/entities/player.h"
#include "character_factory.h"
#include "events/event_database.h"
#include "events/event_manager.h"
#include "map/map_manager.h"
#include "potions/potion_database.h"
#include "save/game_state.h"
#include "save/save_manager.h"

namespace {

void prepareTestEnemy(Enemy &enemy, int hp = 40)
{
    EnemyDatabase::instance().load();
    const EnemyData *data = EnemyDatabase::instance().getEnemy(QStringLiteral("generic_normal"));
    if (data != nullptr) {
        enemy.loadFromData(*data);
    }
    enemy.setMaxHp(hp);
    enemy.healToFull();
}

} // namespace

class TestNodes : public QObject
{
    Q_OBJECT

private slots:
    void test_trash_heap_event_option()
    {
        CharacterFactory::initialize();
        Player *player = CharacterFactory::createCharacter(QStringLiteral("ironclad"));
        QVERIFY(player != nullptr);

        MapManager mapManager;
        mapManager.generateMap(100);
        mapManager.setPlayer(player);

        EventManager eventManager;
        const EventData *eventData = EventDatabase::instance().getEvent(QStringLiteral("slime_world"));
        QVERIFY(eventData != nullptr);
        QVERIFY(eventData->options.size() >= 1);

        const int stepsBefore = mapManager.totalSteps();
        const int hpBefore = player->currentHp();

        const bool survived = eventManager.executeOption(eventData->options.at(0), player, &mapManager);

        QVERIFY(survived);
        QCOMPARE(player->currentHp(), hpBefore - 11);
        QCOMPARE(mapManager.totalSteps(), stepsBefore + 2);
        QVERIFY(player->relicIds().contains(QStringLiteral("burning_blood")));

        delete player;
    }

    void test_health_potion_restores_hp()
    {
        CharacterFactory::initialize();
        Player *player = CharacterFactory::createCharacter(QStringLiteral("ironclad"));
        player->takeDirectDamage(40);

        const PotionData potion = PotionDatabase::instance().getPotion(QStringLiteral("blood_potion"));
        QVERIFY(PotionDatabase::instance().canUse(potion, GameContext::NonCombat));

        PotionDatabase::instance().usePotion(potion, player);
        QCOMPARE(player->currentHp(), 56);

        delete player;
    }

    void test_fire_potion_deals_damage_in_combat()
    {
        CharacterFactory::initialize();
        Player *player = CharacterFactory::createCharacter(QStringLiteral("ironclad"));
        Enemy enemy;
        prepareTestEnemy(enemy, 30);

        const PotionData potion = PotionDatabase::instance().getPotion(QStringLiteral("fire_potion"));
        QVERIFY(PotionDatabase::instance().canUse(potion, GameContext::CombatPlayerTurn));

        PotionDatabase::instance().usePotion(potion, player, &enemy);
        QCOMPARE(enemy.currentHp(), 10);

        delete player;
    }

    void test_campfire_heal_and_upgrade()
    {
        CharacterFactory::initialize();
        Player *player = CharacterFactory::createCharacter(QStringLiteral("ironclad"));
        player->takeDirectDamage(40);
        const int hpBefore = player->currentHp();

        player->heal((player->maxHp() * 30) / 100);
        QCOMPARE(player->currentHp(), hpBefore + 24);

        CardDatabase::instance().load();
        int strikeIndex = -1;
        for (int i = 0; i < player->deck().size(); ++i) {
            if (player->deck().at(i).id() == QStringLiteral("strike")) {
                strikeIndex = i;
                break;
            }
        }
        QVERIFY(strikeIndex >= 0);
        QCOMPARE(player->deck().at(strikeIndex).data().effects.at(0).value, 6);

        QVERIFY(player->upgradeCard(strikeIndex));
        QCOMPARE(player->deck().at(strikeIndex).id(), QStringLiteral("strike+"));
        QCOMPARE(player->deck().at(strikeIndex).data().effects.at(0).value, 9);

        delete player;
    }

    void test_save_load_roundtrip()
    {
        CharacterFactory::initialize();

        GameState state;
        state.playerX = 0;
        state.playerY = 0;
        state.playerHp = 70;
        state.playerMaxHp = 80;
        state.gold = 99;
        state.totalSteps = 3;
        state.shrinkCount = 0;
        state.nextShrinkStep = 12;
        state.mapSeed = 555;
        state.relicIds = QStringList{QStringLiteral("burning_blood")};
        state.deckCardIds = QStringList{QStringLiteral("strike"), QStringLiteral("strike"),
                                        QStringLiteral("defend")};
        state.exploredNodes.append(qMakePair(0, 0));

        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        tempFile.close();

        SaveManager saveManager;
        saveManager.saveGame(tempFile.fileName(), state);

        GameState loaded;
        QVERIFY(saveManager.loadGame(tempFile.fileName(), loaded));

        QCOMPARE(loaded.playerHp, state.playerHp);
        QCOMPARE(loaded.gold, state.gold);
        QCOMPARE(loaded.totalSteps, state.totalSteps);
        QCOMPARE(loaded.playerX, state.playerX);
        QCOMPARE(loaded.playerY, state.playerY);
        QCOMPARE(loaded.deckCardIds.size(), state.deckCardIds.size());
        QCOMPARE(loaded.relicIds, state.relicIds);
    }

    void test_character_factory_ironclad()
    {
        CharacterFactory::initialize();
        Player *player = CharacterFactory::createCharacter(QStringLiteral("ironclad"));
        QVERIFY(player != nullptr);

        QCOMPARE(player->maxHp(), 80);
        QCOMPARE(player->currentHp(), 80);
        QCOMPARE(player->gold(), 99);
        QCOMPARE(player->deck().size(), 10);
        QCOMPARE(player->relicIds().size(), 1);
        QCOMPARE(player->relicIds().at(0), QStringLiteral("burning_blood"));

        delete player;
    }
};

int runNodeTests(int argc, char *argv[])
{
    TestNodes tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "test_nodes.moc"
