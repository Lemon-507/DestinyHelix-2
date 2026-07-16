#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <memory>
#include <vector>

#include "events/event_database.h"
#include "events/event_manager.h"
#include "map/map_manager.h"
#include "save/game_state.h"

class Enemy;
class Player;
class RelicManager;

class GameManager : public QObject
{
    Q_OBJECT

public:
    explicit GameManager(QObject *parent = nullptr);
    ~GameManager() override;

    void startNewGame(const QString &characterId, int mapSeed = -1);
    void placePlayerAtStart(int x, int y);
    void movePlayer(int x, int y);

    void campfireHeal();
    void campfireUpgradeCard(int deckIndex);
    bool canUpgradeAtCampfire() const;

    void executeEventOption(const EventData &eventData, int optionIndex, int selectedDeckIndex = -1);
    void addRelic(const QString &relicId);

    int shopDeletionCount() const;
    void incrementShopDeletionCount();

    MapManager *mapManager();
    Player *player();
    EventManager *eventManager();
    RelicManager *relicManager();

    QVector<Enemy *> currentEncounterEnemies() const;
    Enemy *activeEncounterEnemy() const;
    int normalNodesCleared() const;
    QString bossEnemyId() const;
    void recordCombatVictory();
    void clearCombatEncounter();

    GameState buildGameState() const;
    void applyGameState(const GameState &state);

    void saveGame(const QString &path) const;
    bool loadGame(const QString &path);

signals:
    void combatRequested();
    void eventRequested(const EventData &eventData);
    void shopRequested();
    void campfireRequested();
    void gameOver(bool victory);
    void playerDied();
    void stepsChanged(int totalSteps);
    void shrinkChanged(int shrinkCount);

private:
    void handleNodeInteraction(int x, int y);
    void prepareCombatEncounter(int x, int y);
    void assignRunBoss();
    void connectMapSignals();
    void registerPlayerRelics();
    void syncPlayerRelics();

    MapManager m_mapManager;
    EventManager m_eventManager;
    std::unique_ptr<Player> m_player;
    std::unique_ptr<RelicManager> m_relicManager;
    std::vector<std::unique_ptr<Enemy>> m_encounterEnemies;
    int m_mapSeed = 0;
    int m_normalNodesCleared = 0;
    int m_shopDeletionCount = 0;
    QString m_bossEnemyId;
};
