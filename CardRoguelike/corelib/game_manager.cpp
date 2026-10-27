#include "game_manager.h"

#include <QtCore/QMetaType>

#include "battle/data/card_database.h"
#include "battle/data/enemy_database.h"
#include "battle/data/enemy_pool.h"
#include "battle/data/relic_database.h"
#include "battle/entities/actor.h"
#include "battle/entities/card.h"
#include "battle/entities/enemy.h"
#include "battle/entities/player.h"
#include "battle/logic/battlefsm.h"
#include "battle/logic/relic_manager.h"
#include "character_factory.h"
#include "events/event_database.h"
#include "events/event_manager.h"
#include "map/map_manager.h"
#include "potions/potion_database.h"
#include "save/save_manager.h"
#include "systems/event_bus.h"
#include "utils/rng.h"

GameManager::GameManager(QObject *parent)
    : QObject(parent)
    , m_eventManager(this)
    , m_relicManager(std::make_unique<RelicManager>(this))
    , m_battleFsm(std::make_unique<BattleFSM>())
{
    qRegisterMetaType<EventData>("EventData");
    connectMapSignals();
    connect(m_battleFsm.get(), &BattleFSM::enemiesChanged, this, [this]() {
        updateRelicBattleContext();
    });
}

GameManager::~GameManager()
{
    if (m_relicManager != nullptr) {
        m_relicManager->clearBattleContext();
    }
}

void GameManager::startNewGame(const QString &characterId, int mapSeed)
{
    if (m_relicManager != nullptr) {
        m_relicManager->clearBattleContext();
    }
    clearCombatEncounter();

    CharacterFactory::initialize();
    m_player.reset(CharacterFactory::createCharacter(characterId));
    m_mapSeed = mapSeed;
    m_normalNodesCleared = 0;
    m_shopDeletionCount = 0;

    m_mapManager.generateMap(mapSeed);
    m_mapSeed = m_mapManager.mapSeed();
    m_mapManager.setPlayer(m_player.get());
    m_eventManager.setCompletedEvents({});
    assignRunBoss();

    m_relicManager->clear();
    registerPlayerRelics();
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

int GameManager::cardDeletionPrice() const
{
    return 50 + m_shopDeletionCount * 25;
}

bool GameManager::hasDeletedCardThisShopVisit() const
{
    return m_shopDeletedThisVisit;
}

bool GameManager::canDeleteCardAtShop() const
{
    return m_player != nullptr && !m_shopDeletedThisVisit && !m_player->deck().isEmpty()
           && m_player->gold() >= cardDeletionPrice();
}

void GameManager::rollShopStock()
{
    m_shopDeletedThisVisit = false;
    m_shopStock = generateShopStock();
}

const ShopStock &GameManager::shopStock() const
{
    return m_shopStock;
}

namespace {

int playerPotionTotal(const Player *player)
{
    if (player == nullptr) {
        return 0;
    }

    int total = 0;
    const QHash<QString, int> counts = player->potionCounts();
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > 0) {
            total += it.value();
        }
    }
    return total;
}

QVector<ShopOffer> *shopSlots(ShopStock &stock, ShopOfferKind kind)
{
    switch (kind) {
    case ShopOfferKind::Card:
        return &stock.cards;
    case ShopOfferKind::Relic:
        return &stock.relics;
    case ShopOfferKind::Potion:
        return &stock.potions;
    }
    return nullptr;
}

} // namespace

TransactionResult GameManager::purchaseShopOffer(ShopOfferKind kind, int index)
{
    QVector<ShopOffer> *offers = shopSlots(m_shopStock, kind);
    if (offers == nullptr || m_player == nullptr || index < 0 || index >= offers->size()) {
        return {};
    }

    ShopOffer &offer = (*offers)[index];
    if (offer.sold || offer.itemId.isEmpty()) {
        return {};
    }

    if (kind == ShopOfferKind::Potion && playerPotionTotal(m_player.get()) >= 3) {
        return TransactionResult{false, QStringLiteral("Potion belt full")};
    }

    if (m_player->gold() < offer.price) {
        return TransactionResult{false, QStringLiteral("Not enough gold")};
    }

    m_player->setGold(m_player->gold() - offer.price);

    switch (kind) {
    case ShopOfferKind::Card: {
        CardDatabase::instance().load();
        const Card card = CardDatabase::instance().getCard(offer.itemId);
        if (!card.id().isEmpty()) {
            m_player->addCardToDeck(card);
        }
        break;
    }
    case ShopOfferKind::Relic:
        addRelic(offer.itemId);
        break;
    case ShopOfferKind::Potion:
        m_player->addPotion(offer.itemId, 1);
        break;
    }

    offer.sold = true;
    return TransactionResult{true, QString()};
}

TransactionResult GameManager::deleteCardAtShop(int deckIndex)
{
    if (m_player == nullptr) {
        return {};
    }
    if (m_shopDeletedThisVisit) {
        return TransactionResult{false, QStringLiteral("Already deleted a card this visit")};
    }

    const int price = cardDeletionPrice();
    if (m_player->gold() < price || m_player->deck().isEmpty()) {
        return TransactionResult{false, QStringLiteral("Not enough gold")};
    }
    if (!m_player->removeCardAt(deckIndex)) {
        return {};
    }

    m_player->setGold(m_player->gold() - price);
    m_shopDeletedThisVisit = true;
    incrementShopDeletionCount();
    return TransactionResult{true, QString()};
}

void GameManager::rollRewards()
{
    NodeType nodeType = NodeType::NormalEnemy;
    if (m_mapManager.playerX() >= 0 && m_mapManager.playerY() >= 0) {
        nodeType = m_mapManager.nodeType(m_mapManager.playerX(), m_mapManager.playerY());
    }

    QStringList ownedRelics;
    if (m_player != nullptr) {
        ownedRelics = m_player->relicIds();
    }
    m_rewardOffers = generateRewardOffers(nodeType, ownedRelics);
}

const QVector<RewardOfferData> &GameManager::rewardOffers() const
{
    return m_rewardOffers;
}

bool GameManager::claimReward(int index)
{
    if (m_player == nullptr || index < 0 || index >= m_rewardOffers.size()) {
        return false;
    }

    RewardOfferData &offer = m_rewardOffers[index];
    if (!offer.granted || offer.claimed) {
        return false;
    }

    switch (offer.kind) {
    case RewardOfferKind::Gold:
        m_player->addGold(offer.goldAmount);
        break;
    case RewardOfferKind::Potion:
        m_player->addPotion(offer.itemId, 1);
        break;
    case RewardOfferKind::Card: {
        CardDatabase::instance().load();
        const Card card = CardDatabase::instance().getCard(offer.itemId);
        if (!card.id().isEmpty()) {
            m_player->addCardToDeck(card);
        }
        break;
    }
    case RewardOfferKind::Relic:
        addRelic(offer.itemId);
        break;
    }

    offer.claimed = true;
    return true;
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
    if (!m_encounterEnemies.empty()) {
        QVector<Enemy *> enemies;
        enemies.reserve(static_cast<int>(m_encounterEnemies.size()));
        for (const std::unique_ptr<Enemy> &enemy : m_encounterEnemies) {
            enemies.append(enemy.get());
        }
        return enemies;
    }

    if (m_battleFsm != nullptr) {
        return m_battleFsm->enemies();
    }
    return {};
}

Enemy *GameManager::activeEncounterEnemy() const
{
    const QVector<Enemy *> enemies = currentEncounterEnemies();
    for (Enemy *enemy : enemies) {
        if (enemy != nullptr && enemy->currentHp() > 0) {
            return enemy;
        }
    }
    return enemies.isEmpty() ? nullptr : enemies.front();
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
    if (m_battleFsm != nullptr) {
        m_battleFsm->releaseEncounter();
    }
}

void GameManager::startCombat()
{
    if (m_player == nullptr || m_battleFsm == nullptr || m_encounterEnemies.empty()) {
        return;
    }

    std::vector<std::unique_ptr<Enemy>> owned = std::move(m_encounterEnemies);
    m_encounterEnemies.clear();

    QVector<Enemy *> enemies;
    enemies.reserve(static_cast<int>(owned.size()));
    for (const std::unique_ptr<Enemy> &enemy : owned) {
        if (enemy != nullptr) {
            enemies.append(enemy.get());
        }
    }

    m_battleFsm->adoptEncounter(std::move(owned));
    if (enemies.isEmpty()) {
        m_battleFsm->releaseEncounter();
        return;
    }

    const int shrinkCount = m_mapManager.combatShrinkCount();
    m_battleFsm->startCombat(m_player.get(), enemies, shrinkCount);
    updateRelicBattleContext();
}

PlayResult GameManager::playCard(int handIndex, int enemyIndex)
{
    if (m_player == nullptr || m_battleFsm == nullptr) {
        return PlayResult::failure(QStringLiteral("No active combat"));
    }
    if (m_battleFsm->battleState() != BattleState::PlayerTurn) {
        return PlayResult::failure(QStringLiteral("Not the player turn"));
    }

    const QVector<Card> hand = m_battleFsm->hand();
    if (handIndex < 0 || handIndex >= hand.size()) {
        return PlayResult::failure(QStringLiteral("Invalid hand index"));
    }

    const Card &card = hand.at(handIndex);
    if (!card.isPlayable()) {
        return PlayResult::failure(QStringLiteral("Card cannot be played"));
    }
    if (card.cost() > m_battleFsm->currentEnergy()) {
        return PlayResult::failure(QStringLiteral("Not enough energy"));
    }

    Actor *target = nullptr;
    if (card.data().target == TargetType::Self) {
        target = m_player.get();
    } else if (card.data().target == TargetType::AllEnemies) {
        for (Enemy *enemy : currentEncounterEnemies()) {
            if (enemy != nullptr && enemy->currentHp() > 0) {
                target = enemy;
                break;
            }
        }
        if (target == nullptr) {
            return PlayResult::failure(QStringLiteral("No living enemy"));
        }
    } else {
        const QVector<Enemy *> enemies = currentEncounterEnemies();
        if (enemyIndex < 0 || enemyIndex >= enemies.size()) {
            return PlayResult::failure(QStringLiteral("Invalid enemy index"));
        }
        Enemy *enemy = enemies.at(enemyIndex);
        if (enemy == nullptr || enemy->currentHp() <= 0) {
            return PlayResult::failure(QStringLiteral("Enemy is already defeated"));
        }
        target = enemy;
    }

    return m_battleFsm->playCard(card, target);
}

void GameManager::endPlayerTurn()
{
    if (m_battleFsm == nullptr || m_battleFsm->battleState() != BattleState::PlayerTurn) {
        return;
    }
    m_battleFsm->playerEndTurn();
}

PlayResult GameManager::usePotion(const QString &potionId, int enemyIndex)
{
    if (m_player == nullptr || m_player->currentHp() <= 0) {
        return PlayResult::failure(QStringLiteral("Potion cannot be used"));
    }
    if (m_player->potionCount(potionId) <= 0) {
        return PlayResult::failure(QStringLiteral("No potion available"));
    }

    PotionDatabase::instance().load();
    const PotionData potion = PotionDatabase::instance().getPotion(potionId);
    if (potion.id.isEmpty() || !PotionDatabase::instance().canUse(potion, potionContext())) {
        return PlayResult::failure(QStringLiteral("Potion cannot be used"));
    }

    QString primaryTarget = QStringLiteral("self");
    if (!potion.effects.isEmpty() && !potion.effects.first().target.isEmpty()) {
        primaryTarget = potion.effects.first().target;
    }

    Actor *target = m_player.get();
    if (primaryTarget == QStringLiteral("selected_enemy")) {
        const QVector<Enemy *> enemies = currentEncounterEnemies();
        if (enemyIndex < 0 || enemyIndex >= enemies.size()) {
            return PlayResult::failure(QStringLiteral("Invalid enemy index"));
        }
        Enemy *enemy = enemies.at(enemyIndex);
        if (enemy == nullptr || enemy->currentHp() <= 0) {
            return PlayResult::failure(QStringLiteral("Enemy is already defeated"));
        }
        target = enemy;
    }

    QVector<Actor *> allEnemies;
    for (Enemy *enemy : currentEncounterEnemies()) {
        if (enemy != nullptr && enemy->currentHp() > 0) {
            allEnemies.append(enemy);
        }
    }

    if (primaryTarget == QStringLiteral("all_enemies") && allEnemies.isEmpty()) {
        return PlayResult::failure(QStringLiteral("No living enemy"));
    }

    PotionDatabase::instance().usePotion(potion, m_player.get(), target, allEnemies, m_battleFsm.get());
    m_player->setPotionCount(potionId, m_player->potionCount(potionId) - 1);
    EventBus::instance().triggerPotionUsed();
    return PlayResult::success();
}

void GameManager::finishCombat(bool playerWon)
{
    if (m_battleFsm != nullptr && m_battleFsm->battleState() != BattleState::Ended) {
        m_battleFsm->concludeCombat(playerWon);
    }
    if (m_relicManager != nullptr) {
        m_relicManager->clearBattleContext();
    }
    if (!playerWon) {
        clearCombatEncounter();
    }
}

BattleFSM *GameManager::battleFsm() const
{
    return m_battleFsm.get();
}

void GameManager::updateRelicBattleContext()
{
    if (m_relicManager == nullptr || m_battleFsm == nullptr) {
        return;
    }

    const BattleState state = m_battleFsm->battleState();
    if (state == BattleState::Idle || state == BattleState::Ended) {
        return;
    }

    QVector<Actor *> battleEnemies;
    for (Enemy *enemy : m_battleFsm->enemies()) {
        if (enemy != nullptr) {
            battleEnemies.append(enemy);
        }
    }
    m_relicManager->setBattleContext(m_battleFsm.get(), battleEnemies);
}

GameContext GameManager::potionContext() const
{
    if (m_battleFsm == nullptr) {
        return GameContext::NonCombat;
    }

    switch (m_battleFsm->battleState()) {
    case BattleState::PlayerTurn:
        return GameContext::CombatPlayerTurn;
    case BattleState::PlayerTurnStart:
    case BattleState::EnemyTurn:
        return GameContext::CombatAny;
    case BattleState::Idle:
    case BattleState::Ended:
        break;
    }
    return GameContext::NonCombat;
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
