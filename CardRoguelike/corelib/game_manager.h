#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <memory>
#include <vector>

#include "battle/logic/play_result.h"
#include "economy/run_economy.h"
#include "events/event_database.h"
#include "potions/potion_database.h"
#include "events/event_manager.h"
#include "map/map_manager.h"
#include "save/game_state.h"

class BattleFSM;
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
    int cardDeletionPrice() const;
    bool hasDeletedCardThisShopVisit() const;
    bool canDeleteCardAtShop() const;

    void rollShopStock();
    const ShopStock &shopStock() const;
    TransactionResult purchaseShopOffer(ShopOfferKind kind, int index);
    TransactionResult deleteCardAtShop(int deckIndex);

    void rollRewards();
    const QVector<RewardOfferData> &rewardOffers() const;
    bool claimReward(int index);

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

    void startCombat();
    PlayResult playCard(int handIndex, int enemyIndex);
    void endPlayerTurn();
    PlayResult usePotion(const QString &potionId, int enemyIndex);
    void finishCombat(bool playerWon);
    BattleFSM *battleFsm() const;

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
    void updateRelicBattleContext();
    GameContext potionContext() const;

    MapManager m_mapManager;
    EventManager m_eventManager;
    std::unique_ptr<Player> m_player;
    std::unique_ptr<RelicManager> m_relicManager;
    std::unique_ptr<BattleFSM> m_battleFsm;
    std::vector<std::unique_ptr<Enemy>> m_encounterEnemies;
    ShopStock m_shopStock;
    QVector<RewardOfferData> m_rewardOffers;
    bool m_shopDeletedThisVisit = false;
    int m_mapSeed = 0;
    int m_normalNodesCleared = 0;
    int m_shopDeletionCount = 0;
    QString m_bossEnemyId;
};
