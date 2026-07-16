#include <QtTest>
#include <QTemporaryFile>

#include "battle/entities/player.h"
#include "game_manager.h"

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
};

int runGameManagerTests(int argc, char *argv[])
{
    TestGameManager tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "test_game_manager.moc"
