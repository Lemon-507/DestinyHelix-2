#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include "economy/run_economy.h"

class GameManager;

class ShopViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int gold READ gold NOTIFY goldChanged)

public:
    enum class ItemType {
        Card,
        Relic,
        Potion
    };

    struct ShopSlot {
        ItemType type = ItemType::Card;
        QString itemId;
        QString displayName;
        QString tooltip;
        int price = 0;
        bool sold = false;
    };

    explicit ShopViewModel(GameManager *gameManager, QObject *parent = nullptr);

    int gold() const;
    bool potionBeltFull() const;
    const QVector<ShopSlot> &cardSlots() const;
    const QVector<ShopSlot> &relicSlots() const;
    const QVector<ShopSlot> &potionSlots() const;

    int cardDeletionPrice() const;
    bool canDeleteCard() const;
    bool hasDeletedThisVisit() const;

    Q_INVOKABLE bool purchaseCard(int index);
    Q_INVOKABLE bool purchaseRelic(int index);
    Q_INVOKABLE bool purchasePotion(int index);
    Q_INVOKABLE void requestDeleteCard();
    Q_INVOKABLE bool confirmDeleteCard(int deckIndex);
    Q_INVOKABLE void requestLeave();

signals:
    void shopDataChanged();
    void goldChanged();
    void leaveRequested();
    void deleteCardRequested();
    void purchaseDenied(const QString &reason);

private:
    void refreshFromGameManager();
    bool applyPurchase(ShopOfferKind kind, int index);
    int playerPotionTotal() const;

    GameManager *m_gameManager = nullptr;
    QVector<ShopSlot> m_cardSlots;
    QVector<ShopSlot> m_relicSlots;
    QVector<ShopSlot> m_potionSlots;
};
