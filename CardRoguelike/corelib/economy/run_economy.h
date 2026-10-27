#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include "map/node_type.h"

enum class ShopOfferKind {
    Card,
    Relic,
    Potion
};

struct ShopOffer {
    ShopOfferKind kind = ShopOfferKind::Card;
    QString itemId;
    QString displayName;
    int price = 0;
    bool sold = false;
};

struct ShopStock {
    QVector<ShopOffer> cards;
    QVector<ShopOffer> relics;
    QVector<ShopOffer> potions;
};

enum class RewardOfferKind {
    Gold = 0,
    Potion,
    Card,
    Relic
};

struct RewardOfferData {
    RewardOfferKind kind = RewardOfferKind::Gold;
    QString displayName;
    bool granted = false;
    bool claimed = false;
    int goldAmount = 0;
    QString itemId;
};

struct TransactionResult {
    bool ok = false;
    QString reason;
};

ShopStock generateShopStock();
QVector<RewardOfferData> generateRewardOffers(NodeType nodeType, const QStringList &ownedRelicIds);
