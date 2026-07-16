#include "reward_viewmodel.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "battle/data/card_database.h"
#include "battle/data/relic_database.h"
#include "battle/entities/card.h"
#include "battle/entities/player.h"
#include "game_manager.h"
#include "map/map_manager.h"
#include "map/node_type.h"
#include "potions/potion_database.h"
#include "utils/item_tooltips.h"
#include "utils/rng.h"

namespace {

struct RewardTable {
    int goldMin = 0;
    int goldMax = 0;
    int potionPercent = 0;
    int relicPercent = 0;
};

RewardTable rewardTableForNode(NodeType type)
{
    RewardTable table;
    switch (type) {
    case NodeType::Elite:
        table.goldMin = 40;
        table.goldMax = 60;
        table.potionPercent = 60;
        table.relicPercent = 100;
        break;
    case NodeType::Boss:
        table.goldMin = 80;
        table.goldMax = 100;
        table.potionPercent = 100;
        table.relicPercent = 100;
        break;
    case NodeType::NormalEnemy:
    default:
        table.goldMin = 20;
        table.goldMax = 40;
        table.potionPercent = 30;
        table.relicPercent = 0;
        break;
    }
    return table;
}

} // namespace

RewardViewModel::RewardViewModel(GameManager *gameManager, QObject *parent)
    : QObject(parent)
    , m_gameManager(gameManager)
{
    m_offers.resize(4);
    m_offers[0].type = RewardType::Gold;
    m_offers[1].type = RewardType::Potion;
    m_offers[2].type = RewardType::Card;
    m_offers[3].type = RewardType::Relic;
    generateRewards();
}

const QVector<RewardViewModel::RewardOffer> &RewardViewModel::offers() const
{
    return m_offers;
}

bool RewardViewModel::rollPercent(int percent)
{
    if (percent <= 0) {
        return false;
    }
    if (percent >= 100) {
        return true;
    }
    return RNG::get(0, 99) < percent;
}

QStringList RewardViewModel::loadPotionIdsFromResources()
{
    QStringList ids;
    QFile file(QStringLiteral(":/resources/data/potions.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return ids;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    const QJsonArray potions = document.isArray()
        ? document.array()
        : document.object().value(QStringLiteral("potions")).toArray();
    for (const QJsonValue &value : potions) {
        const QString id = value.toObject().value(QStringLiteral("id")).toString();
        if (!id.isEmpty()) {
            ids.append(id);
        }
    }
    return ids;
}

QString RewardViewModel::randomCardId()
{
    CardDatabase::instance().load();
    const QStringList ids = CardDatabase::instance().rewardCardIds();
    if (ids.isEmpty()) {
        return {};
    }
    return ids.at(RNG::get(0, ids.size() - 1));
}

QString RewardViewModel::randomPotionId()
{
    const QStringList ids = loadPotionIdsFromResources();
    if (ids.isEmpty()) {
        return QStringLiteral("health_potion");
    }
    return ids.at(RNG::get(0, ids.size() - 1));
}

QString RewardViewModel::randomRelicId(const QStringList &ownedIds)
{
    RelicDatabase::instance().load();
    return RelicDatabase::instance().randomRelicIdExcluding(ownedIds);
}

void RewardViewModel::generateRewards()
{
    NodeType nodeType = NodeType::NormalEnemy;
    if (m_gameManager != nullptr && m_gameManager->mapManager() != nullptr) {
        MapManager *mapManager = m_gameManager->mapManager();
        nodeType = mapManager->nodeType(mapManager->playerX(), mapManager->playerY());
    }

    const RewardTable table = rewardTableForNode(nodeType);
    CardDatabase::instance().load();
    PotionDatabase::instance().load();

    const int goldAmount = RNG::get(table.goldMin, table.goldMax);
    m_offers[0].granted = true;
    m_offers[0].claimed = false;
    m_offers[0].goldAmount = goldAmount;
    m_offers[0].itemId.clear();
    m_offers[0].buttonText = QStringLiteral("Gold: %1").arg(goldAmount);
    m_offers[0].tooltip.clear();

    if (rollPercent(table.potionPercent)) {
        const QString potionId = randomPotionId();
        const PotionData potion = PotionDatabase::instance().getPotion(potionId);
        const QString potionName = potion.name.isEmpty() ? potionId : potion.name;
        m_offers[1].granted = true;
        m_offers[1].claimed = false;
        m_offers[1].itemId = potionId;
        m_offers[1].buttonText = QStringLiteral("Potion: %1").arg(potionName);
        m_offers[1].tooltip = ItemTooltips::forPotion(potionId);
    } else {
        m_offers[1].granted = false;
        m_offers[1].claimed = false;
        m_offers[1].itemId.clear();
        m_offers[1].buttonText.clear();
        m_offers[1].tooltip.clear();
    }

    const QString cardId = randomCardId();
    const QString cardName = CardDatabase::instance().displayName(cardId);
    m_offers[2].granted = true;
    m_offers[2].claimed = false;
    m_offers[2].itemId = cardId;
    m_offers[2].buttonText = QStringLiteral("Card: %1").arg(cardName);
    m_offers[2].tooltip = ItemTooltips::forCard(cardId);

    QStringList ownedRelics;
    if (m_gameManager != nullptr && m_gameManager->player() != nullptr) {
        ownedRelics = m_gameManager->player()->relicIds();
    }

    if (rollPercent(table.relicPercent)) {
        const QString relicId = randomRelicId(ownedRelics);
        RelicDatabase::instance().load();
        const RelicData relic = RelicDatabase::instance().getRelic(relicId);
        const QString relicName = relic.name.isEmpty() ? relicId : relic.name;
        m_offers[3].granted = true;
        m_offers[3].claimed = false;
        m_offers[3].itemId = relicId;
        m_offers[3].buttonText = QStringLiteral("Relic: %1").arg(relicName);
        m_offers[3].tooltip = ItemTooltips::forRelic(relicId);
    } else {
        m_offers[3].granted = false;
        m_offers[3].claimed = false;
        m_offers[3].itemId.clear();
        m_offers[3].buttonText.clear();
        m_offers[3].tooltip.clear();
    }

    emit offersChanged();
}

bool RewardViewModel::claimReward(RewardType type)
{
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return false;
    }

    const int index = static_cast<int>(type);
    if (index < 0 || index >= m_offers.size()) {
        return false;
    }

    RewardOffer &offer = m_offers[index];
    if (!offer.granted || offer.claimed) {
        return false;
    }

    Player *player = m_gameManager->player();
    switch (type) {
    case RewardType::Gold:
        player->addGold(offer.goldAmount);
        break;
    case RewardType::Potion:
        player->addPotion(offer.itemId, 1);
        break;
    case RewardType::Card: {
        CardDatabase::instance().load();
        const Card card = CardDatabase::instance().getCard(offer.itemId);
        if (!card.id().isEmpty()) {
            player->addCardToDeck(card);
        }
        break;
    }
    case RewardType::Relic:
        m_gameManager->addRelic(offer.itemId);
        break;
    }

    offer.claimed = true;
    emit offersChanged();
    return true;
}
