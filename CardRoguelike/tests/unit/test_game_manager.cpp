#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>

#include "battle/entities/card.h"
#include "battle/entities/player.h"
#include "battle/logic/battlefsm.h"
#include "events/event_database.h"
#include "game_manager.h"
#include "map/map_manager.h"
#include "map/node_type.h"
#include "utils/rng.h"

class TestGameManager : public QObject
{
    Q_OBJECT

private slots:
    void test_save_load_roundtrip()
    {
        GameManager gameManager;
        gameManager.startNewGame(QStringLiteral("ironclad"), 555);
        gameManager.placePlayerAtStart(0, 0);
        gameManager.player()->takeDirectDamage(10);
        gameManager.mapManager()->addSteps(3);

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
        QCOMPARE(loadedManager.mapManager()->playerX(), gameManager.mapManager()->playerX());
        QCOMPARE(loadedManager.mapManager()->playerY(), gameManager.mapManager()->playerY());
        QCOMPARE(loadedManager.player()->deck().size(), gameManager.player()->deck().size());
    }

    void test_shrink_signal_fires_once_after_two_new_games()
    {
        GameManager gameManager;
        gameManager.startNewGame(QStringLiteral("ironclad"), 1);
        gameManager.startNewGame(QStringLiteral("ironclad"), 2);

        QSignalSpy spy(&gameManager, &GameManager::shrinkChanged);
        gameManager.mapManager()->addSteps(12);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 1);
    }

    void test_shop_stock_stable_for_seed()
    {
        GameManager gameManager;
        gameManager.startNewGame(QStringLiteral("ironclad"), 7);

        RNG::seed(42);
        gameManager.rollShopStock();
        const ShopStock first = gameManager.shopStock();

        RNG::seed(42);
        gameManager.rollShopStock();
        const ShopStock second = gameManager.shopStock();

        QCOMPARE(first.cards.size(), second.cards.size());
        for (int i = 0; i < first.cards.size(); ++i) {
            QCOMPARE(first.cards.at(i).itemId, second.cards.at(i).itemId);
            QCOMPARE(first.cards.at(i).price, second.cards.at(i).price);
        }
        QCOMPARE(first.relics.size(), second.relics.size());
        for (int i = 0; i < first.relics.size(); ++i) {
            QCOMPARE(first.relics.at(i).itemId, second.relics.at(i).itemId);
            QCOMPARE(first.relics.at(i).price, second.relics.at(i).price);
        }
    }

    void test_shop_purchase_rejects_when_broke()
    {
        GameManager gameManager;
        gameManager.startNewGame(QStringLiteral("ironclad"), 3);
        RNG::seed(1);
        gameManager.rollShopStock();
        gameManager.player()->setGold(0);

        QVERIFY(!gameManager.shopStock().cards.isEmpty());
        QVERIFY(gameManager.shopStock().cards.at(0).price > 0);

        const TransactionResult result = gameManager.purchaseShopOffer(ShopOfferKind::Card, 0);
        QVERIFY(!result.ok);
        QCOMPARE(result.reason, QStringLiteral("Not enough gold"));
        QCOMPARE(gameManager.player()->gold(), 0);
        QVERIFY(!gameManager.shopStock().cards.at(0).sold);
    }

    void test_shop_delete_increments_count_once_per_visit()
    {
        GameManager gameManager;
        gameManager.startNewGame(QStringLiteral("ironclad"), 4);
        gameManager.rollShopStock();
        gameManager.player()->setGold(500);

        const int before = gameManager.shopDeletionCount();
        const int deckBefore = gameManager.player()->deck().size();
        const TransactionResult deleted = gameManager.deleteCardAtShop(0);
        QVERIFY(deleted.ok);
        QCOMPARE(gameManager.shopDeletionCount(), before + 1);
        QCOMPARE(gameManager.player()->deck().size(), deckBefore - 1);
        QVERIFY(gameManager.hasDeletedCardThisShopVisit());

        const TransactionResult again = gameManager.deleteCardAtShop(0);
        QVERIFY(!again.ok);
        QCOMPARE(gameManager.shopDeletionCount(), before + 1);
    }

    void test_reward_offers_stable_for_seed()
    {
        GameManager gameManager;
        gameManager.startNewGame(QStringLiteral("ironclad"), 8);

        RNG::seed(9);
        gameManager.rollRewards();
        const QVector<RewardOfferData> first = gameManager.rewardOffers();

        RNG::seed(9);
        gameManager.rollRewards();
        const QVector<RewardOfferData> second = gameManager.rewardOffers();

        QCOMPARE(first.size(), second.size());
        for (int i = 0; i < first.size(); ++i) {
            QCOMPARE(first.at(i).granted, second.at(i).granted);
            QCOMPARE(first.at(i).goldAmount, second.at(i).goldAmount);
            QCOMPARE(first.at(i).itemId, second.at(i).itemId);
        }
    }

    void test_event_death_stops_later_effects()
    {
        GameManager gameManager;
        gameManager.startNewGame(QStringLiteral("ironclad"), 1);
        gameManager.player()->setGold(0);

        EventOption option;
        EffectDef loseHp;
        loseHp.type = QStringLiteral("lose_hp");
        loseHp.value = 999;
        EffectDef gainGold;
        gainGold.type = QStringLiteral("gain_gold");
        gainGold.value = 50;
        option.effects = {loseHp, gainGold};

        const bool survived = gameManager.eventManager()->executeOption(
            option, gameManager.player(), gameManager.mapManager());
        QVERIFY(!survived);
        QVERIFY(gameManager.player()->currentHp() <= 0);
        QCOMPARE(gameManager.player()->gold(), 0);
    }

    void test_event_gain_random_relic_adds_id()
    {
        GameManager gameManager;
        gameManager.startNewGame(QStringLiteral("ironclad"), 1);
        const int before = gameManager.player()->relicIds().size();

        EventOption option;
        EffectDef gainRelic;
        gainRelic.type = QStringLiteral("gain_random_relic");
        gainRelic.value = 1;
        option.effects = {gainRelic};

        QVERIFY(gameManager.eventManager()->executeOption(
            option, gameManager.player(), gameManager.mapManager()));
        QCOMPARE(gameManager.player()->relicIds().size(), before + 1);
    }

    void test_game_manager_runs_combat_session()
    {
        const int starts[8][2] = {
            {0, 0}, {0, 5}, {0, 10}, {5, 0}, {5, 10}, {10, 0}, {10, 5}, {10, 10},
        };
        const int dx[4] = {1, -1, 0, 0};
        const int dy[4] = {0, 0, 1, -1};

        bool fought = false;
        for (int seed = 1; seed <= 40 && !fought; ++seed) {
            for (const auto &start : starts) {
                GameManager gameManager;
                gameManager.startNewGame(QStringLiteral("ironclad"), seed);
                gameManager.placePlayerAtStart(start[0], start[1]);
                if (gameManager.mapManager()->playerX() != start[0]
                    || gameManager.mapManager()->playerY() != start[1]) {
                    continue;
                }

                for (int dir = 0; dir < 4; ++dir) {
                    const int nx = start[0] + dx[dir];
                    const int ny = start[1] + dy[dir];
                    if (nx < 0 || ny < 0 || nx > 10 || ny > 10) {
                        continue;
                    }
                    const NodeType type = gameManager.mapManager()->nodeType(nx, ny);
                    if (type != NodeType::NormalEnemy && type != NodeType::Elite) {
                        continue;
                    }

                    gameManager.movePlayer(nx, ny);
                    if (gameManager.player()->currentHp() <= 0) {
                        continue;
                    }
                    QVERIFY(!gameManager.currentEncounterEnemies().isEmpty());

                    gameManager.startCombat();
                    QVERIFY(gameManager.battleFsm() != nullptr);
                    QCOMPARE(gameManager.battleFsm()->battleState(), BattleState::PlayerTurn);
                    QVERIFY(!gameManager.currentEncounterEnemies().isEmpty());

                    const QVector<Card> hand = gameManager.battleFsm()->hand();
                    int handIndex = -1;
                    for (int i = 0; i < hand.size(); ++i) {
                        const Card &card = hand.at(i);
                        if (card.isPlayable() && card.cost() <= gameManager.battleFsm()->currentEnergy()) {
                            handIndex = i;
                            break;
                        }
                    }
                    QVERIFY(handIndex >= 0);

                    const int energyBefore = gameManager.battleFsm()->currentEnergy();
                    gameManager.playCard(handIndex, 0);
                    if (gameManager.battleFsm()->battleState() == BattleState::PlayerTurn) {
                        QVERIFY(gameManager.battleFsm()->currentEnergy() <= energyBefore);
                        gameManager.endPlayerTurn();
                    }
                    const BattleState state = gameManager.battleFsm()->battleState();
                    QVERIFY(state == BattleState::PlayerTurn || state == BattleState::Ended);
                    fought = true;
                    break;
                }
                if (fought) {
                    break;
                }
            }
        }
        QVERIFY(fought);
    }

    void test_play_card_rejects_invalid_index()
    {
        GameManager gameManager;
        gameManager.startNewGame(QStringLiteral("ironclad"), 1);
        const int deckSize = gameManager.player()->deck().size();

        const PlayResult result = gameManager.playCard(0, 0);
        QVERIFY(!result.ok);
        QVERIFY(!result.reason.isEmpty());
        QCOMPARE(gameManager.player()->deck().size(), deckSize);
    }

    void test_use_potion_rejects_missing_potion()
    {
        GameManager gameManager;
        gameManager.startNewGame(QStringLiteral("ironclad"), 1);
        const QString potionId = QStringLiteral("health_potion");
        const int before = gameManager.player()->potionCount(potionId);

        const PlayResult result = gameManager.usePotion(potionId, -1);
        QVERIFY(!result.ok);
        QVERIFY(!result.reason.isEmpty());
        QCOMPARE(gameManager.player()->potionCount(potionId), before);
    }
};

int runGameManagerTests(int argc, char *argv[])
{
    TestGameManager tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "test_game_manager.moc"
