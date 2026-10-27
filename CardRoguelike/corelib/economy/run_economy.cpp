#include "economy/run_economy.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QtGlobal>

#include "battle/data/card_database.h"
#include "battle/data/relic_database.h"
#include "potions/potion_database.h"
#include "utils/rng.h"

namespace {

constexpr int kCardSlotCount = 7;
constexpr int kRelicSlotCount = 3;
constexpr int kPotionSlotCount = 3;

struct CardCandidate {
    QString id;
    QString name;
    QString rarity;
};

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

QVector<CardCandidate> loadPurchasableCards()
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

QStringList loadPotionIds()
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

QString pickUnique(QStringList &candidates)
{
    if (candidates.isEmpty()) {
        return {};
    }
    const int index = RNG::get(0, candidates.size() - 1);
    const QString picked = candidates.at(index);
    candidates.removeAt(index);
    return picked;
}

int priceForCardRarity(const QString &rarity)
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

bool rollPercent(int percent)
{
    if (percent <= 0) {
        return false;
    }
    if (percent >= 100) {
        return true;
    }
    return RNG::get(0, 99) < percent;
}

} // namespace

ShopStock generateShopStock()
{
    ShopStock stock;
    stock.cards.reserve(kCardSlotCount);
    stock.relics.reserve(kRelicSlotCount);
    stock.potions.reserve(kPotionSlotCount);

    CardDatabase::instance().load();
    RelicDatabase::instance().load();
    PotionDatabase::instance().load();

    QVector<CardCandidate> cardPool = loadPurchasableCards();
    for (int i = 0; i < kCardSlotCount && !cardPool.isEmpty(); ++i) {
        const int pick = RNG::get(0, cardPool.size() - 1);
        const CardCandidate candidate = cardPool.takeAt(pick);

        ShopOffer slot;
        slot.kind = ShopOfferKind::Card;
        slot.itemId = candidate.id;
        slot.displayName = candidate.name;
        slot.price = priceForCardRarity(candidate.rarity);
        slot.sold = false;
        stock.cards.append(slot);
    }
    while (stock.cards.size() < kCardSlotCount) {
        ShopOffer empty;
        empty.kind = ShopOfferKind::Card;
        empty.sold = true;
        empty.displayName = QStringLiteral("Sold");
        stock.cards.append(empty);
    }

    QStringList relicCandidates = RelicDatabase::instance().allRelicIds();
    for (int i = 0; i < kRelicSlotCount; ++i) {
        ShopOffer slot;
        slot.kind = ShopOfferKind::Relic;
        slot.itemId = pickUnique(relicCandidates);
        if (slot.itemId.isEmpty()) {
            slot.sold = true;
            slot.displayName = QStringLiteral("Sold");
        } else {
            const RelicData relic = RelicDatabase::instance().getRelic(slot.itemId);
            slot.displayName = relic.name.isEmpty() ? slot.itemId : relic.name;
            slot.price = qBound(150, RNG::get(150, 200), 200);
            slot.sold = false;
        }
        stock.relics.append(slot);
    }

    const QStringList potionIds = loadPotionIds();
    for (int i = 0; i < kPotionSlotCount; ++i) {
        ShopOffer slot;
        slot.kind = ShopOfferKind::Potion;
        if (potionIds.isEmpty()) {
            slot.sold = true;
            slot.displayName = QStringLiteral("Sold");
        } else {
            slot.itemId = potionIds.at(RNG::get(0, potionIds.size() - 1));
            const PotionData potion = PotionDatabase::instance().getPotion(slot.itemId);
            slot.displayName = potion.name.isEmpty() ? slot.itemId : potion.name;
            slot.price = qBound(30, RNG::get(30, 50), 50);
            slot.sold = false;
        }
        stock.potions.append(slot);
    }

    return stock;
}

QVector<RewardOfferData> generateRewardOffers(NodeType nodeType, const QStringList &ownedRelicIds)
{
    QVector<RewardOfferData> offers(4);
    offers[0].kind = RewardOfferKind::Gold;
    offers[1].kind = RewardOfferKind::Potion;
    offers[2].kind = RewardOfferKind::Card;
    offers[3].kind = RewardOfferKind::Relic;

    const RewardTable table = rewardTableForNode(nodeType);
    CardDatabase::instance().load();
    PotionDatabase::instance().load();
    RelicDatabase::instance().load();

    const int goldAmount = RNG::get(table.goldMin, table.goldMax);
    offers[0].granted = true;
    offers[0].claimed = false;
    offers[0].goldAmount = goldAmount;
    offers[0].displayName = QStringLiteral("Gold: %1").arg(goldAmount);

    if (rollPercent(table.potionPercent)) {
        const QStringList potionIds = loadPotionIds();
        const QString potionId = potionIds.isEmpty()
                                     ? QStringLiteral("health_potion")
                                     : potionIds.at(RNG::get(0, potionIds.size() - 1));
        const PotionData potion = PotionDatabase::instance().getPotion(potionId);
        const QString potionName = potion.name.isEmpty() ? potionId : potion.name;
        offers[1].granted = true;
        offers[1].claimed = false;
        offers[1].itemId = potionId;
        offers[1].displayName = QStringLiteral("Potion: %1").arg(potionName);
    }

    const QStringList cardIds = CardDatabase::instance().rewardCardIds();
    const QString cardId = cardIds.isEmpty() ? QString() : cardIds.at(RNG::get(0, cardIds.size() - 1));
    offers[2].granted = true;
    offers[2].claimed = false;
    offers[2].itemId = cardId;
    offers[2].displayName = QStringLiteral("Card: %1").arg(CardDatabase::instance().displayName(cardId));

    if (rollPercent(table.relicPercent)) {
        const QString relicId = RelicDatabase::instance().randomRelicIdExcluding(ownedRelicIds);
        const RelicData relic = RelicDatabase::instance().getRelic(relicId);
        const QString relicName = relic.name.isEmpty() ? relicId : relic.name;
        offers[3].granted = true;
        offers[3].claimed = false;
        offers[3].itemId = relicId;
        offers[3].displayName = QStringLiteral("Relic: %1").arg(relicName);
    }

    return offers;
}
