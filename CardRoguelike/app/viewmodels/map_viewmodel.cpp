#include "map_viewmodel.h"

#include "battle/data/card_database.h"
#include "battle/data/relic_database.h"
#include "battle/entities/card.h"
#include "battle/entities/player.h"
#include "battle/logic/card_text.h"
#include "game_manager.h"
#include "map/map_manager.h"
#include "map/node_type.h"

namespace {

constexpr int kGridSize = 11;

QString textAbbreviation(const QString &name, const QString &fallbackId, int maxLen = 2)
{
    if (name.isEmpty()) {
        return fallbackId.left(maxLen).toUpper();
    }

    const QStringList words = name.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (words.size() >= 2) {
        QString abbr;
        for (const QString &word : words) {
            if (!word.isEmpty()) {
                abbr.append(word.at(0).toUpper());
            }
            if (abbr.size() >= maxLen) {
                break;
            }
        }
        return abbr.left(maxLen);
    }

    return name.left(maxLen).toUpper();
}

bool isFixedStartNode(int x, int y)
{
    return (x == 0 && y == 0) || (x == 0 && y == 5) || (x == 0 && y == 10) || (x == 5 && y == 0)
           || (x == 5 && y == 10) || (x == 10 && y == 0) || (x == 10 && y == 5) || (x == 10 && y == 10);
}

QString typeLetterFromNode(const MapNode &node)
{
    switch (node.type) {
    case NodeType::Boss:
        return QStringLiteral("B");
    case NodeType::Shop:
        return QStringLiteral("S");
    case NodeType::Campfire:
        return QStringLiteral("F");
    case NodeType::Elite:
        return QStringLiteral("E");
    case NodeType::NormalEnemy:
        return QStringLiteral("N");
    case NodeType::Event:
        return QStringLiteral("?");
    case NodeType::Empty:
        return QStringLiteral("H");
    }
    return QStringLiteral("H");
}

} // namespace

MapViewModel::MapViewModel(GameManager *gameManager, QObject *parent)
    : QObject(parent)
    , m_gameManager(gameManager)
{
    connectGameManager();
}

QVariant MapViewModel::gridData() const
{
    if (m_gameManager == nullptr || m_gameManager->mapManager() == nullptr) {
        return QVariant();
    }

    QVariantList cells;
    cells.reserve(kGridSize * kGridSize);

    for (int y = 0; y < kGridSize; ++y) {
        for (int x = 0; x < kGridSize; ++x) {
            cells.append(cellLetter(x, y));
        }
    }

    return QVariant::fromValue(cells);
}

QString MapViewModel::cellLetter(int x, int y) const
{
    if (m_gameManager == nullptr || m_gameManager->mapManager() == nullptr) {
        return QStringLiteral("H");
    }
    if (x < 0 || x >= kGridSize || y < 0 || y >= kGridSize) {
        return QStringLiteral("H");
    }

    const MapNode node = m_gameManager->mapManager()->nodeAt(x, y);
    if (node.explored) {
        return QStringLiteral("X");
    }

    return typeLetterFromNode(node);
}

bool MapViewModel::isNodeExplored(int x, int y) const
{
    if (m_gameManager == nullptr || m_gameManager->mapManager() == nullptr) {
        return false;
    }
    if (x < 0 || x >= kGridSize || y < 0 || y >= kGridSize) {
        return false;
    }

    return m_gameManager->mapManager()->isExplored(x, y);
}

QString MapViewModel::bossEnemyId() const
{
    if (m_gameManager == nullptr) {
        return QString();
    }
    return m_gameManager->bossEnemyId();
}

int MapViewModel::playerX() const
{
    if (m_gameManager == nullptr || m_gameManager->mapManager() == nullptr) {
        return -1;
    }
    return m_gameManager->mapManager()->playerX();
}

int MapViewModel::playerY() const
{
    if (m_gameManager == nullptr || m_gameManager->mapManager() == nullptr) {
        return -1;
    }
    return m_gameManager->mapManager()->playerY();
}

int MapViewModel::totalSteps() const
{
    if (m_gameManager == nullptr || m_gameManager->mapManager() == nullptr) {
        return 0;
    }
    return m_gameManager->mapManager()->totalSteps();
}

int MapViewModel::nextShrinkStep() const
{
    if (m_gameManager == nullptr || m_gameManager->mapManager() == nullptr) {
        return 12;
    }
    return m_gameManager->mapManager()->nextShrinkStep();
}

int MapViewModel::shrinkCount() const
{
    if (m_gameManager == nullptr || m_gameManager->mapManager() == nullptr) {
        return 0;
    }
    return m_gameManager->mapManager()->currentShrinkCount();
}

bool MapViewModel::playerDead() const
{
    if (m_gameManager == nullptr || m_gameManager->mapManager() == nullptr) {
        return false;
    }
    return m_gameManager->mapManager()->isPlayerDead();
}

bool MapViewModel::gameStarted() const
{
    return m_gameStarted;
}

int MapViewModel::playerHp() const
{
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return 0;
    }
    return m_gameManager->player()->currentHp();
}

int MapViewModel::playerMaxHp() const
{
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return 0;
    }
    return m_gameManager->player()->maxHp();
}

int MapViewModel::gold() const
{
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return 0;
    }
    return m_gameManager->player()->gold();
}

QVariantList MapViewModel::relics() const
{
    QVariantList entries;
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return entries;
    }

    RelicDatabase::instance().load();
    const QStringList relicIds = m_gameManager->player()->relicIds();
    const int limit = qMin(12, relicIds.size());
    for (int i = 0; i < limit; ++i) {
        const RelicData relic = RelicDatabase::instance().getRelic(relicIds.at(i));
        QVariantMap entry;
        entry.insert(QStringLiteral("id"), relicIds.at(i));
        entry.insert(QStringLiteral("name"), relic.name.isEmpty() ? relicIds.at(i) : relic.name);
        entry.insert(QStringLiteral("abbreviation"),
                     textAbbreviation(relic.name, relicIds.at(i)));
        entry.insert(QStringLiteral("description"),
                     relic.description.isEmpty() ? relic.name : relic.description);
        entries.append(entry);
    }
    return entries;
}

GameManager *MapViewModel::gameManager() const
{
    return m_gameManager;
}

void MapViewModel::startNewGame()
{
    if (m_gameManager == nullptr) {
        return;
    }

    m_gameManager->startNewGame(QStringLiteral("ironclad"));
    m_gameStarted = true;
    emitAllChanged();
    emit playerStatsChanged();
    emit mapReady();
    emit gameStartedChanged();
}

void MapViewModel::selectStartNode(int x, int y)
{
    if (m_gameManager == nullptr || !isStartNode(x, y)) {
        return;
    }

    m_gameManager->placePlayerAtStart(x, y);
    emitPositionChanged();
    emit playerPlaced();
}

void MapViewModel::movePlayer(int x, int y)
{
    if (m_gameManager == nullptr || !canMoveTo(x, y)) {
        return;
    }

    m_gameManager->movePlayer(x, y);
    emitPositionChanged();
    emit gridDataChanged();
}

void MapViewModel::restAtCampfire()
{
    if (m_gameManager == nullptr) {
        return;
    }

    m_gameManager->campfireHeal();
    markCurrentNodeExplored();
    emit playerStatsChanged();
}

void MapViewModel::upgradeAtCampfire(int deckIndex)
{
    if (m_gameManager == nullptr || deckIndex < 0) {
        return;
    }

    m_gameManager->campfireUpgradeCard(deckIndex);
    markCurrentNodeExplored();
    emit playerStatsChanged();
}

void MapViewModel::returnFromCombatReward()
{
    if (m_gameManager == nullptr) {
        return;
    }

    m_gameManager->recordCombatVictory();
    markCurrentNodeExplored();
}

void MapViewModel::executeEventOption(const EventData &eventData, int optionIndex,
                                      int selectedDeckIndex)
{
    if (m_gameManager == nullptr) {
        return;
    }

    m_gameManager->executeEventOption(eventData, optionIndex, selectedDeckIndex);
    if (m_gameManager->player() != nullptr && m_gameManager->player()->currentHp() > 0) {
        markCurrentNodeExplored();
    }
    emit playerStatsChanged();
}

void MapViewModel::markCurrentNodeExplored()
{
    if (m_gameManager == nullptr || m_gameManager->mapManager() == nullptr) {
        return;
    }

    MapManager *mapManager = m_gameManager->mapManager();
    const int x = mapManager->playerX();
    const int y = mapManager->playerY();
    if (x >= 0 && y >= 0) {
        mapManager->setNodeExplored(x, y);
        emit exploredNodeChanged(x, y);

        const NodeType type = mapManager->nodeType(x, y);
        if (type == NodeType::Boss && m_gameManager->player() != nullptr
            && m_gameManager->player()->currentHp() > 0) {
            emit gameOver(true);
        }
    }

    emit gridDataChanged();
}

bool MapViewModel::isInShrinkZone(int x, int y) const
{
    if (m_gameManager == nullptr || m_gameManager->mapManager() == nullptr) {
        return false;
    }

    MapManager *mapManager = m_gameManager->mapManager();
    return mapManager->isInShrinkZone(x, y, mapManager->currentShrinkCount());
}

bool MapViewModel::isStartNode(int x, int y) const
{
    return isFixedStartNode(x, y);
}

bool MapViewModel::canMoveTo(int x, int y) const
{
    if (m_gameManager == nullptr || m_gameManager->mapManager() == nullptr) {
        return false;
    }
    return m_gameManager->mapManager()->canMoveTo(x, y);
}

QStringList MapViewModel::deckCardIds() const
{
    QStringList ids;
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return ids;
    }

    for (const Card &card : m_gameManager->player()->deck()) {
        ids.append(card.id());
    }
    return ids;
}

QVariantList MapViewModel::upgradeableDeckCards() const
{
    QVariantList entries;
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return entries;
    }

    CardDatabase::instance().load();
    const QVector<Card> &deck = m_gameManager->player()->deck();
    for (int i = 0; i < deck.size(); ++i) {
        const Card &card = deck.at(i);
        if (!CardDatabase::instance().canUpgrade(card.id())) {
            continue;
        }

        QVariantMap entry;
        entry.insert(QStringLiteral("index"), i);
        entry.insert(QStringLiteral("id"), card.id());
        entry.insert(QStringLiteral("name"), CardDatabase::instance().displayName(card.id()));
        entry.insert(QStringLiteral("description"), CardText::describeCard(card));
        entries.append(entry);
    }

    return entries;
}

bool MapViewModel::canUpgradeAtCampfire() const
{
    return m_gameManager != nullptr && m_gameManager->canUpgradeAtCampfire();
}

void MapViewModel::refreshFromGameManager()
{
    emitAllChanged();
    emit playerStatsChanged();
}

void MapViewModel::connectGameManager()
{
    if (m_gameManager == nullptr) {
        return;
    }

    connect(m_gameManager, &GameManager::eventRequested, this, &MapViewModel::eventTriggered);
    connect(m_gameManager, &GameManager::combatRequested, this, &MapViewModel::combatTriggered);
    connect(m_gameManager, &GameManager::shopRequested, this, &MapViewModel::shopTriggered);
    connect(m_gameManager, &GameManager::campfireRequested, this, &MapViewModel::campfireReached);
    connect(m_gameManager, &GameManager::gameOver, this, &MapViewModel::gameOver);
    connect(m_gameManager, &GameManager::stepsChanged, this, [this](int) {
        emit totalStepsChanged();
        emit nextShrinkStepChanged();
    });
    connect(m_gameManager, &GameManager::shrinkChanged, this, [this](int) {
        emit shrinkCountChanged();
        emit gridDataChanged();
    });
}

void MapViewModel::emitAllChanged()
{
    emit gridDataChanged();
    emitPositionChanged();
}

void MapViewModel::emitPositionChanged()
{
    emit playerXChanged();
    emit playerYChanged();
    emit totalStepsChanged();
    emit nextShrinkStepChanged();
    emit shrinkCountChanged();
    emit playerDeadChanged();
}
