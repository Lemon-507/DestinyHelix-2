#include "game_manager.h"

#include <QtCore/QMetaType>

#include "battle/data/card_database.h"
#include "battle/data/enemy_database.h"
#include "battle/data/enemy_pool.h"
#include "battle/data/relic_database.h"
#include "battle/entities/card.h"
#include "battle/entities/enemy.h"
#include "battle/entities/player.h"
#include "battle/logic/relic_manager.h"
#include "character_factory.h"
#include "events/event_database.h"
#include "events/event_manager.h"
#include "map/map_manager.h"
#include "save/save_manager.h"
#include "systems/event_bus.h"
#include "utils/rng.h"

GameManager::GameManager(QObject *parent)
    : QObject(parent)
    , m_eventManager(this)
    , m_relicManager(std::make_unique<RelicManager>(this))
{
    qRegisterMetaType<EventData>("EventData");
    connectMapSignals();
}

GameManager::~GameManager() = default;

void GameManager::startNewGame(const QString &characterId, int mapSeed)
{
    CharacterFactory::initialize();
    m_player.reset(CharacterFactory::createCharacter(characterId));
    m_mapSeed = mapSeed;
    m_normalNodesCleared = 0;
    m_shopDeletionCount = 0;
    clearCombatEncounter();

    m_mapManager.generateMap(mapSeed);
    m_mapSeed = m_mapManager.mapSeed();
    m_mapManager.setPlayer(m_player.get());
    m_eventManager.setCompletedEvents({});
    assignRunBoss();

    m_relicManager->clear();
    registerPlayerRelics();

    connectMapSignals();
}

void GameManager::addRelic(const QString &relicId)
{
    if (m_player == nullptr || m_relicManager == nullptr) {
        return;
    }

    RelicDatabase::instance().load();
    const RelicData relicData = RelicDatabase::instance().getRelic(relicId);
    if (relicData.id.isEmpty()) {
        return;
    }

    if (!m_player->relicIds().contains(relicId)) {
        m_player->addRelicId(relicId);
    }

    if (!m_relicManager->isRegistered(relicId)) {
        m_relicManager->registerRelic(relicData, m_player.get(), nullptr);
    }
}

void GameManager::registerPlayerRelics()
{
    if (m_player == nullptr || m_relicManager == nullptr) {
        return;
    }

    RelicDatabase::instance().load();
    for (const QString &relicId : m_player->relicIds()) {
        addRelic(relicId);
    }
}

void GameManager::syncPlayerRelics()
{
    if (m_player == nullptr || m_relicManager == nullptr) {
        return;
    }

    RelicDatabase::instance().load();
    for (const QString &relicId : m_player->relicIds()) {
        if (!m_relicManager->isRegistered(relicId)) {
            addRelic(relicId);
        }
    }
}

void GameManager::placePlayerAtStart(int x, int y)
{
    if (!m_mapManager.canMoveTo(x, y)) {
        return;
    }

    m_mapManager.movePlayer(x, y);
}

void GameManager::movePlayer(int x, int y)
{
    if (!m_mapManager.canMoveTo(x, y)) {
        return;
    }

    m_mapManager.movePlayer(x, y);

    if (m_mapManager.isPlayerDead()) {
        emit playerDied();
        emit gameOver(false);
        return;
    }

    handleNodeInteraction(x, y);
    emit stepsChanged(m_mapManager.totalSteps());
}

void GameManager::campfireHeal()
{
    if (m_player == nullptr) {
        return;
    }

    m_player->heal((m_player->maxHp() * 30) / 100);
}

void GameManager::campfireUpgradeCard(int deckIndex)
{
    if (m_player == nullptr) {
        return;
    }

    m_player->upgradeCard(deckIndex);
}

bool GameManager::canUpgradeAtCampfire() const
{
    return m_player != nullptr && m_player->canUpgrade();
}

void GameManager::executeEventOption(const EventData &eventData, int optionIndex,
                                     int selectedDeckIndex)
{
    if (m_player == nullptr || optionIndex < 0 || optionIndex >= eventData.options.size()) {
        return;
    }

    const EventOption &option = eventData.options.at(optionIndex);
    const bool survived =
        m_eventManager.executeOption(option, m_player.get(), &m_mapManager, selectedDeckIndex);
    m_eventManager.markEventCompleted(eventData.id);
    syncPlayerRelics();

    emit stepsChanged(m_mapManager.totalSteps());
    emit shrinkChanged(m_mapManager.currentShrinkCount());

    if (!survived || m_player->currentHp() <= 0) {
        emit playerDied();
        emit gameOver(false);
    }
}

int GameManager::shopDeletionCount() const
{
    return m_shopDeletionCount;
}

void GameManager::incrementShopDeletionCount()
{
    ++m_shopDeletionCount;
}

MapManager *GameManager::mapManager()
{
    return &m_mapManager;
}

Player *GameManager::player()
{
    return m_player.get();
}

EventManager *GameManager::eventManager()
{
    return &m_eventManager;
}

RelicManager *GameManager::relicManager()
{
    return m_relicManager.get();
}

GameState GameManager::buildGameState() const
{
    GameState state;
    if (m_player == nullptr) {
        return state;
    }

    state.playerX = m_mapManager.playerX();
    state.playerY = m_mapManager.playerY();
    state.playerHp = m_player->currentHp();
    state.playerMaxHp = m_player->maxHp();
    state.gold = m_player->gold();
    state.totalSteps = m_mapManager.totalSteps();
    state.shrinkCount = m_mapManager.currentShrinkCount();
    state.nextShrinkStep = m_mapManager.nextShrinkStep();
    state.exploredNodes = m_mapManager.exploredNodes();
    state.completedEventIds = m_eventManager.completedEvents();
    state.mapSeed = m_mapSeed;
    state.shopDeletionCount = m_shopDeletionCount;
    state.relicIds = m_player->relicIds();
    state.potionCounts = m_player->potionCounts();

    for (const Card &card : m_player->deck()) {
        state.deckCardIds.append(card.id());
    }

    return state;
}

void GameManager::applyGameState(const GameState &state)
{
    if (m_player == nullptr) {
        return;
    }

    m_mapSeed = state.mapSeed;
    m_mapManager.generateMap(state.mapSeed);
    m_mapManager.setPlayer(m_player.get());
    assignRunBoss();
    m_mapManager.setPlayerPosition(state.playerX, state.playerY);
    m_mapManager.setTotalSteps(state.totalSteps);
    m_mapManager.setCurrentShrinkCount(state.shrinkCount);
    m_mapManager.setNextShrinkStep(state.nextShrinkStep);
    m_mapManager.restoreExploredNodes(state.exploredNodes);
    m_eventManager.setCompletedEvents(state.completedEventIds);
    m_shopDeletionCount = state.shopDeletionCount;

    m_player->setMaxHp(state.playerMaxHp);
    m_player->healToFull();
    if (state.playerHp < state.playerMaxHp) {
        m_player->takeDirectDamage(state.playerMaxHp - state.playerHp);
    }
    m_player->setGold(state.gold);
    m_player->setRelicIds(state.relicIds);
    m_player->setPotionCounts(state.potionCounts);

    m_player->clearDeck();
    CardDatabase::instance().load();
    for (const QString &cardId : state.deckCardIds) {
        m_player->addCardToDeck(CardDatabase::instance().getCard(cardId));
    }

    m_relicManager->clear();
    registerPlayerRelics();
}

void GameManager::saveGame(const QString &path) const
{
    SaveManager saveManager;
    saveManager.saveGame(path, buildGameState());
}

bool GameManager::loadGame(const QString &path)
{
    GameState state;
    SaveManager saveManager;
    if (!saveManager.loadGame(path, state)) {
        return false;
    }

    if (m_player == nullptr) {
        m_player.reset(new Player());
    }

    applyGameState(state);
    return true;
}

void GameManager::handleNodeInteraction(int x, int y)
{
    const NodeType type = m_mapManager.isExplored(x, y) ? NodeType::Empty : m_mapManager.nodeType(x, y);

    switch (type) {
    case NodeType::Boss:
    case NodeType::Elite:
    case NodeType::NormalEnemy:
        prepareCombatEncounter(x, y);
        emit combatRequested();
        break;
    case NodeType::Event:
        // 30% surprise combat from the Normal Enemy pool; otherwise a normal event.
        if (RNG::get(0.0, 1.0) < 0.3) {
            prepareCombatEncounter(x, y);
            emit combatRequested();
        } else if (const EventData *eventData =
                       EventDatabase::instance().randomEvent(m_eventManager.completedEvents())) {
            emit eventRequested(*eventData);
        }
        break;
    case NodeType::Shop:
        emit shopRequested();
        break;
    case NodeType::Campfire:
        emit campfireRequested();
        break;
    case NodeType::Empty:
    default:
        break;
    }
}

void GameManager::connectMapSignals()
{
    connect(&EventBus::instance(), &EventBus::shrinkTriggered, this, [this](int shrinkCount) {
        emit shrinkChanged(shrinkCount);
    });
}

QVector<Enemy *> GameManager::currentEncounterEnemies() const
{
    QVector<Enemy *> enemies;
    enemies.reserve(static_cast<int>(m_encounterEnemies.size()));
    for (const std::unique_ptr<Enemy> &enemy : m_encounterEnemies) {
        enemies.append(enemy.get());
    }
    return enemies;
}

Enemy *GameManager::activeEncounterEnemy() const
{
    for (const std::unique_ptr<Enemy> &enemy : m_encounterEnemies) {
        if (enemy != nullptr && enemy->currentHp() > 0) {
            return enemy.get();
        }
    }
    return m_encounterEnemies.empty() ? nullptr : m_encounterEnemies.front().get();
}

int GameManager::normalNodesCleared() const
{
    return m_normalNodesCleared;
}

QString GameManager::bossEnemyId() const
{
    return m_bossEnemyId;
}

void GameManager::assignRunBoss()
{
    m_bossEnemyId.clear();

    // Peek the boss pool without permanently advancing mid-run RNG.
    const std::mt19937 rngSnapshot = RNG::snapshot();
    const QVector<Enemy *> rolled =
        EnemyPool::instance().rollEncounter(QStringLiteral("boss"), 0, nullptr);
    RNG::restore(rngSnapshot);

    QString bossId;
    for (Enemy *enemy : rolled) {
        if (enemy != nullptr && bossId.isEmpty()) {
            bossId = enemy->enemyId();
        }
        delete enemy;
    }

    if (bossId.isEmpty()) {
        bossId = QStringLiteral("hexaghost");
    }

    m_bossEnemyId = bossId;
    constexpr int kCenter = 5;
    m_mapManager.setEnemyIdAt(kCenter, kCenter, bossId);
}

void GameManager::recordCombatVictory()
{
    const int x = m_mapManager.playerX();
    const int y = m_mapManager.playerY();
    if (x >= 0 && y >= 0 && m_mapManager.nodeType(x, y) == NodeType::NormalEnemy) {
        ++m_normalNodesCleared;
    }
    clearCombatEncounter();
}

void GameManager::clearCombatEncounter()
{
    m_encounterEnemies.clear();
}

void GameManager::prepareCombatEncounter(int x, int y)
{
    clearCombatEncounter();

    const NodeType type = m_mapManager.nodeType(x, y);

    // Boss is assigned once per run (for map icon + fight consistency).
    if (type == NodeType::Boss) {
        QString bossId = m_mapManager.enemyIdAt(x, y);
        if (bossId.isEmpty()) {
            bossId = m_bossEnemyId;
        }
        if (bossId.isEmpty()) {
            assignRunBoss();
            bossId = m_bossEnemyId;
        }
        EnemyDatabase::instance().load();
        Enemy *enemy = EnemyDatabase::instance().createEnemy(bossId, nullptr);
        if (enemy != nullptr) {
            m_encounterEnemies.emplace_back(enemy);
            m_mapManager.setEnemyIdAt(x, y, enemy->enemyId());
            m_bossEnemyId = enemy->enemyId();
        }
        return;
    }

    QString poolType = QStringLiteral("normal");
    if (type == NodeType::Elite) {
        poolType = QStringLiteral("elite");
    }

    const QVector<Enemy *> rolled = EnemyPool::instance().rollEncounter(poolType, m_normalNodesCleared, nullptr);
    const bool eliteHpReduction = type == NodeType::Elite && m_player != nullptr
                                  && m_player->hasRelic(QStringLiteral("insect_preserved_specimen"));
    for (Enemy *enemy : rolled) {
        if (enemy != nullptr) {
            if (eliteHpReduction) {
                const int reducedMaxHp = (enemy->maxHp() * 75) / 100;
                enemy->setMaxHp(qMax(1, reducedMaxHp));
                enemy->healToFull();
            }
            m_encounterEnemies.emplace_back(enemy);
        }
    }

    if (!m_encounterEnemies.empty() && m_encounterEnemies.front() != nullptr) {
        m_mapManager.setEnemyIdAt(x, y, m_encounterEnemies.front()->enemyId());
    }
}
