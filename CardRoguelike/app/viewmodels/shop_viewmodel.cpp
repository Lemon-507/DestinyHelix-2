#include "shop_viewmodel.h"

#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QStringList>
#include <QtCore/QtGlobal>

#include "battle/data/card_database.h"
#include "battle/data/relic_database.h"
#include "battle/entities/card.h"
#include "battle/entities/player.h"
#include "game_manager.h"
#include "potions/potion_database.h"
#include "utils/item_tooltips.h"
#include "utils/rng.h"

namespace {

constexpr int kCardSlotCount = 7;
constexpr int kRelicSlotCount = 3;
constexpr int kPotionSlotCount = 3;
constexpr int kMaxPotionBelt = 3;

QJsonArray loadJsonArray(const QString &resourcePath, const QString &objectKey)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (document.isArray()) {
        return document.array();
    }
    return document.object().value(objectKey).toArray();
}

} // namespace

ShopViewModel::ShopViewModel(GameManager *gameManager, QObject *parent)
    : QObject(parent)
    , m_gameManager(gameManager)
{
    generateStock();
}

int ShopViewModel::gold() const
{
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return 0;
    }
    return m_gameManager->player()->gold();
}

bool ShopViewModel::potionBeltFull() const
{
    return playerPotionTotal() >= kMaxPotionBelt;
}

const QVector<ShopViewModel::ShopSlot> &ShopViewModel::cardSlots() const
{
    return m_cardSlots;
}

const QVector<ShopViewModel::ShopSlot> &ShopViewModel::relicSlots() const
{
    return m_relicSlots;
}

const QVector<ShopViewModel::ShopSlot> &ShopViewModel::potionSlots() const
{
    return m_potionSlots;
}

QVector<ShopViewModel::CardCandidate> ShopViewModel::loadPurchasableCards()
{
    QVector<CardCandidate> candidates;
    const QJsonArray cards = loadJsonArray(QStringLiteral(":/resources/data/cards.json"),
                                           QStringLiteral("cards"));
    for (const QJsonValue &value : cards) {
        const QJsonObject obj = value.toObject();
        const QString rarity = obj.value(QStringLiteral("rarity")).toString();
        if (rarity != QStringLiteral("Common") && rarity != QStringLiteral("Uncommon")
            && rarity != QStringLiteral("Rare")) {
            continue;
        }

        CardCandidate candidate;
        candidate.id = obj.value(QStringLiteral("id")).toString();
        candidate.name = obj.value(QStringLiteral("name")).toString();
        candidate.rarity = rarity;
        if (candidate.id.isEmpty()) {
            continue;
        }
        if (candidate.name.isEmpty()) {
            candidate.name = candidate.id;
        }
        candidates.append(candidate);
    }
    return candidates;
}

QStringList ShopViewModel::loadPotionIds()
{
    QStringList ids;
    const QJsonArray potions = loadJsonArray(QStringLiteral(":/resources/data/potions.json"),
                                             QStringLiteral("potions"));
    for (const QJsonValue &value : potions) {
        const QString id = value.toObject().value(QStringLiteral("id")).toString();
        if (!id.isEmpty()) {
            ids.append(id);
        }
    }
    return ids;
}

QString ShopViewModel::pickUnique(QStringList &candidates)
{
    if (candidates.isEmpty()) {
        return {};
    }
    const int index = RNG::get(0, candidates.size() - 1);
    const QString picked = candidates.at(index);
    candidates.removeAt(index);
    return picked;
}

int ShopViewModel::priceForCardRarity(const QString &rarity)
{
    int minPrice = 20;
    int maxPrice = 40;
    if (rarity == QStringLiteral("Uncommon")) {
        minPrice = 40;
        maxPrice = 70;
    } else if (rarity == QStringLiteral("Rare")) {
        minPrice = 80;
        maxPrice = 110;
    }

    return qBound(minPrice, RNG::get(minPrice, maxPrice), maxPrice);
}

int ShopViewModel::playerPotionTotal() const
{
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return 0;
    }

    int total = 0;
    const QHash<QString, int> counts = m_gameManager->player()->potionCounts();
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > 0) {
            total += it.value();
        }
    }
    return total;
}

void ShopViewModel::generateStock()
{
    m_cardSlots.clear();
    m_relicSlots.clear();
    m_potionSlots.clear();
    m_cardSlots.reserve(kCardSlotCount);
    m_relicSlots.reserve(kRelicSlotCount);
    m_potionSlots.reserve(kPotionSlotCount);

    CardDatabase::instance().load();
    RelicDatabase::instance().load();
    PotionDatabase::instance().load();

    QVector<CardCandidate> cardPool = loadPurchasableCards();
    for (int i = 0; i < kCardSlotCount && !cardPool.isEmpty(); ++i) {
        const int pick = RNG::get(0, cardPool.size() - 1);
        const CardCandidate candidate = cardPool.takeAt(pick);

        ShopSlot slot;
        slot.type = ItemType::Card;
        slot.itemId = candidate.id;
        slot.displayName = candidate.name;
        slot.tooltip = ItemTooltips::forCard(candidate.id);
        slot.price = priceForCardRarity(candidate.rarity);
        slot.sold = false;
        m_cardSlots.append(slot);
    }
    while (m_cardSlots.size() < kCardSlotCount) {
        ShopSlot empty;
        empty.type = ItemType::Card;
        empty.sold = true;
        empty.displayName = QStringLiteral("Sold");
        m_cardSlots.append(empty);
    }

    QStringList relicCandidates = RelicDatabase::instance().allRelicIds();
    for (int i = 0; i < kRelicSlotCount; ++i) {
        ShopSlot slot;
        slot.type = ItemType::Relic;
        slot.itemId = pickUnique(relicCandidates);
        if (slot.itemId.isEmpty()) {
            slot.sold = true;
            slot.displayName = QStringLiteral("Sold");
        } else {
            const RelicData relic = RelicDatabase::instance().getRelic(slot.itemId);
            slot.displayName = relic.name.isEmpty() ? slot.itemId : relic.name;
            slot.tooltip = ItemTooltips::forRelic(slot.itemId);
            slot.price = qBound(150, RNG::get(150, 200), 200);
            slot.sold = false;
        }
        m_relicSlots.append(slot);
    }

    const QStringList potionIds = loadPotionIds();
    for (int i = 0; i < kPotionSlotCount; ++i) {
        ShopSlot slot;
        slot.type = ItemType::Potion;
        if (potionIds.isEmpty()) {
            slot.sold = true;
            slot.displayName = QStringLiteral("Sold");
        } else {
            slot.itemId = potionIds.at(RNG::get(0, potionIds.size() - 1));
            const PotionData potion = PotionDatabase::instance().getPotion(slot.itemId);
            slot.displayName = potion.name.isEmpty() ? slot.itemId : potion.name;
            slot.tooltip = ItemTooltips::forPotion(slot.itemId);
            slot.price = qBound(30, RNG::get(30, 50), 50);
            slot.sold = false;
        }
        m_potionSlots.append(slot);
    }

    emit shopDataChanged();
    emit goldChanged();
}

bool ShopViewModel::tryPurchase(ShopSlot &slot)
{
    if (slot.sold || slot.itemId.isEmpty() || m_gameManager == nullptr
        || m_gameManager->player() == nullptr) {
        return false;
    }

    Player *player = m_gameManager->player();
    if (player->gold() < slot.price) {
        emit purchaseDenied(QStringLiteral("Not enough gold"));
        return false;
    }

    player->setGold(player->gold() - slot.price);

    switch (slot.type) {
    case ItemType::Card: {
        CardDatabase::instance().load();
        const Card card = CardDatabase::instance().getCard(slot.itemId);
        if (!card.id().isEmpty()) {
            player->addCardToDeck(card);
        }
        break;
    }
    case ItemType::Relic:
        m_gameManager->addRelic(slot.itemId);
        break;
    case ItemType::Potion:
        player->addPotion(slot.itemId, 1);
        break;
    }

    slot.sold = true;
    emit goldChanged();
    emit shopDataChanged();
    return true;
}

bool ShopViewModel::purchaseCard(int index)
{
    if (index < 0 || index >= m_cardSlots.size()) {
        return false;
    }
    return tryPurchase(m_cardSlots[index]);
}

bool ShopViewModel::purchaseRelic(int index)
{
    if (index < 0 || index >= m_relicSlots.size()) {
        return false;
    }
    return tryPurchase(m_relicSlots[index]);
}

bool ShopViewModel::purchasePotion(int index)
{
    if (index < 0 || index >= m_potionSlots.size()) {
        return false;
    }

    if (playerPotionTotal() >= kMaxPotionBelt) {
        emit purchaseDenied(QStringLiteral("Potion belt full"));
        return false;
    }

    return tryPurchase(m_potionSlots[index]);
}

int ShopViewModel::cardDeletionPrice() const
{
    const int deletionCount = m_gameManager != nullptr ? m_gameManager->shopDeletionCount() : 0;
    return 50 + deletionCount * 25;
}

bool ShopViewModel::hasDeletedThisVisit() const
{
    return m_hasDeletedThisVisit;
}

bool ShopViewModel::canDeleteCard() const
{
    if (m_hasDeletedThisVisit || m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return false;
    }
    if (m_gameManager->player()->deck().isEmpty()) {
        return false;
    }
    return m_gameManager->player()->gold() >= cardDeletionPrice();
}

void ShopViewModel::requestDeleteCard()
{
    if (!canDeleteCard()) {
        if (m_hasDeletedThisVisit) {
            emit purchaseDenied(QStringLiteral("Already deleted a card this visit"));
        } else {
            emit purchaseDenied(QStringLiteral("Not enough gold"));
        }
        return;
    }
    emit deleteCardRequested();
}

bool ShopViewModel::confirmDeleteCard(int deckIndex)
{
    if (!canDeleteCard() || m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return false;
    }

    Player *player = m_gameManager->player();
    const int price = cardDeletionPrice();
    if (player->gold() < price) {
        emit purchaseDenied(QStringLiteral("Not enough gold"));
        return false;
    }

    if (!player->removeCardAt(deckIndex)) {
        return false;
    }

    player->setGold(player->gold() - price);
    m_hasDeletedThisVisit = true;
    m_gameManager->incrementShopDeletionCount();
    emit goldChanged();
    emit shopDataChanged();
    return true;
}

void ShopViewModel::requestLeave()
{
    emit leaveRequested();
}
