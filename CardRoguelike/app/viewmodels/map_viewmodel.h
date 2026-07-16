#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QVector>

#include "events/event_database.h"

class GameManager;

class MapViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariant gridData READ gridData NOTIFY gridDataChanged)
    Q_PROPERTY(int playerX READ playerX NOTIFY playerXChanged)
    Q_PROPERTY(int playerY READ playerY NOTIFY playerYChanged)
    Q_PROPERTY(int totalSteps READ totalSteps NOTIFY totalStepsChanged)
    Q_PROPERTY(int nextShrinkStep READ nextShrinkStep NOTIFY nextShrinkStepChanged)
    Q_PROPERTY(int shrinkCount READ shrinkCount NOTIFY shrinkCountChanged)
    Q_PROPERTY(bool playerDead READ playerDead NOTIFY playerDeadChanged)
    Q_PROPERTY(bool gameStarted READ gameStarted NOTIFY gameStartedChanged)
    Q_PROPERTY(int playerHp READ playerHp NOTIFY playerStatsChanged)
    Q_PROPERTY(int playerMaxHp READ playerMaxHp NOTIFY playerStatsChanged)
    Q_PROPERTY(int gold READ gold NOTIFY playerStatsChanged)
    Q_PROPERTY(QVariantList relics READ relics NOTIFY playerStatsChanged)

public:
    explicit MapViewModel(GameManager *gameManager, QObject *parent = nullptr);

    QVariant gridData() const;
    int playerX() const;
    int playerY() const;
    int totalSteps() const;
    int nextShrinkStep() const;
    int shrinkCount() const;
    bool playerDead() const;
    bool gameStarted() const;

    int playerHp() const;
    int playerMaxHp() const;
    int gold() const;
    QVariantList relics() const;

    GameManager *gameManager() const;

    Q_INVOKABLE void startNewGame();
    Q_INVOKABLE void selectStartNode(int x, int y);
    Q_INVOKABLE void movePlayer(int x, int y);
    Q_INVOKABLE void handleCampfireChoice(int choice, int deckIndex = -1);
    Q_INVOKABLE void executeEventOption(const EventData &eventData, int optionIndex,
                                        int selectedDeckIndex = -1);
    Q_INVOKABLE void markCurrentNodeExplored();
    Q_INVOKABLE bool isInShrinkZone(int x, int y) const;
    Q_INVOKABLE bool isStartNode(int x, int y) const;
    Q_INVOKABLE bool canMoveTo(int x, int y) const;
    Q_INVOKABLE QStringList deckCardIds() const;
    Q_INVOKABLE QVariantList upgradeableDeckCards() const;
    Q_INVOKABLE bool canUpgradeAtCampfire() const;
    Q_INVOKABLE QString cellLetter(int x, int y) const;
    Q_INVOKABLE bool isNodeExplored(int x, int y) const;
    Q_INVOKABLE QString bossEnemyId() const;

public slots:
    void refreshFromGameManager();

signals:
    void gridDataChanged();
    void exploredNodeChanged(int x, int y);
    void playerXChanged();
    void playerYChanged();
    void totalStepsChanged();
    void nextShrinkStepChanged();
    void shrinkCountChanged();
    void playerDeadChanged();
    void gameStartedChanged();

    void mapReady();
    void playerPlaced();
    void eventTriggered(const EventData &eventData);
    void combatTriggered();
    void shopTriggered();
    void campfireReached();
    void gameOver(bool victory);
    void playerStatsChanged();

private:
    void connectGameManager();
    void emitAllChanged();
    void emitPositionChanged();

    GameManager *m_gameManager = nullptr;
    bool m_gameStarted = false;
};
