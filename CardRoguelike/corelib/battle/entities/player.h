#pragma once

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include "actor.h"
#include "card.h"

class Player : public Actor
{
    Q_OBJECT

public:
    explicit Player(QObject *parent = nullptr);

    QVector<Card> &deck();
    const QVector<Card> &deck() const;
    void clearDeck();
    void addCardToDeck(const Card &card);
    bool removeCardAt(int index);
    bool transformCardAt(int index);

    bool canUpgrade() const;
    bool upgradeCard(int index);

    int gold() const;
    void addGold(int amount);
    void setGold(int amount);

    QStringList relicIds() const;
    bool hasRelic(const QString &relicId) const;
    void addRelicId(const QString &relicId);
    void removeRelicId(const QString &relicId);
    void setRelicIds(const QStringList &relicIds);

    void markPenNibDoubleDamage();
    bool consumePenNibDoubleDamage();

    int potionCount(const QString &potionId) const;
    void addPotion(const QString &potionId, int count = 1);
    void setPotionCount(const QString &potionId, int count);
    QHash<QString, int> potionCounts() const;
    void setPotionCounts(const QHash<QString, int> &counts);

private:
    QVector<Card> m_deck;
    int m_gold = 0;
    QStringList m_relicIds;
    QHash<QString, int> m_potionCounts;
    bool m_penNibDoubleNext = false;
};
