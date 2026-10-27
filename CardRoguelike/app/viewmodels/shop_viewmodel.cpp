#include "shop_viewmodel.h"

#include "battle/entities/player.h"
#include "game_manager.h"
#include "utils/item_tooltips.h"

namespace {

constexpr int kMaxPotionBelt = 3;

ShopViewModel::ItemType toItemType(ShopOfferKind kind)
{
    switch (kind) {
    case ShopOfferKind::Relic:
        return ShopViewModel::ItemType::Relic;
    case ShopOfferKind::Potion:
        return ShopViewModel::ItemType::Potion;
    case ShopOfferKind::Card:
    default:
        return ShopViewModel::ItemType::Card;
    }
}

QString tooltipFor(ShopOfferKind kind, const QString &itemId)
{
    switch (kind) {
    case ShopOfferKind::Relic:
        return ItemTooltips::forRelic(itemId);
    case ShopOfferKind::Potion:
        return ItemTooltips::forPotion(itemId);
    case ShopOfferKind::Card:
    default:
        return ItemTooltips::forCard(itemId);
    }
}

} // namespace

ShopViewModel::ShopViewModel(GameManager *gameManager, QObject *parent)
    : QObject(parent)
    , m_gameManager(gameManager)
{
    if (m_gameManager != nullptr) {
        m_gameManager->rollShopStock();
    }
    refreshFromGameManager();
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

void ShopViewModel::refreshFromGameManager()
{
    m_cardSlots.clear();
    m_relicSlots.clear();
    m_potionSlots.clear();
    if (m_gameManager == nullptr) {
        return;
    }

    const auto copySlots = [this](const QVector<ShopOffer> &source, QVector<ShopSlot> &dest) {
        dest.reserve(source.size());
        for (const ShopOffer &offer : source) {
            ShopSlot slot;
            slot.type = toItemType(offer.kind);
            slot.itemId = offer.itemId;
            slot.displayName = offer.displayName;
            slot.price = offer.price;
            slot.sold = offer.sold;
            if (!offer.itemId.isEmpty() && !offer.sold) {
                slot.tooltip = tooltipFor(offer.kind, offer.itemId);
            }
            dest.append(slot);
        }
    };

    const ShopStock &stock = m_gameManager->shopStock();
    copySlots(stock.cards, m_cardSlots);
    copySlots(stock.relics, m_relicSlots);
    copySlots(stock.potions, m_potionSlots);
    emit shopDataChanged();
    emit goldChanged();
}

bool ShopViewModel::applyPurchase(ShopOfferKind kind, int index)
{
    if (m_gameManager == nullptr) {
        return false;
    }

    const TransactionResult result = m_gameManager->purchaseShopOffer(kind, index);
    if (!result.ok) {
        if (!result.reason.isEmpty()) {
            emit purchaseDenied(result.reason);
        }
        return false;
    }

    refreshFromGameManager();
    return true;
}

bool ShopViewModel::purchaseCard(int index)
{
    return applyPurchase(ShopOfferKind::Card, index);
}

bool ShopViewModel::purchaseRelic(int index)
{
    return applyPurchase(ShopOfferKind::Relic, index);
}

bool ShopViewModel::purchasePotion(int index)
{
    return applyPurchase(ShopOfferKind::Potion, index);
}

int ShopViewModel::cardDeletionPrice() const
{
    return m_gameManager != nullptr ? m_gameManager->cardDeletionPrice() : 50;
}

bool ShopViewModel::hasDeletedThisVisit() const
{
    return m_gameManager != nullptr && m_gameManager->hasDeletedCardThisShopVisit();
}

bool ShopViewModel::canDeleteCard() const
{
    return m_gameManager != nullptr && m_gameManager->canDeleteCardAtShop();
}

void ShopViewModel::requestDeleteCard()
{
    if (canDeleteCard()) {
        emit deleteCardRequested();
        return;
    }

    if (hasDeletedThisVisit()) {
        emit purchaseDenied(QStringLiteral("Already deleted a card this visit"));
    } else {
        emit purchaseDenied(QStringLiteral("Not enough gold"));
    }
}

bool ShopViewModel::confirmDeleteCard(int deckIndex)
{
    if (m_gameManager == nullptr) {
        return false;
    }

    const TransactionResult result = m_gameManager->deleteCardAtShop(deckIndex);
    if (!result.ok) {
        if (!result.reason.isEmpty()) {
            emit purchaseDenied(result.reason);
        }
        return false;
    }

    refreshFromGameManager();
    return true;
}

void ShopViewModel::requestLeave()
{
    emit leaveRequested();
}
