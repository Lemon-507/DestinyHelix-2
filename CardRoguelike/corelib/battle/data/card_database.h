#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include "battle/entities/card.h"

class CardDatabase
{
public:
    static CardDatabase &instance();

    void load();
    Card getCard(const QString &id) const;
    bool contains(const QString &id) const;
    QString upgradedIdFor(const QString &id) const;
    bool canUpgrade(const QString &id) const;
    QString displayName(const QString &id) const;

    /// Base cards eligible for combat rewards / shop drafts: Common, Uncommon, Rare only.
    QStringList rewardCardIds() const;

private:
    CardDatabase() = default;
    void parseCardObject(const QJsonObject &obj);

    QVector<Card> m_cards;
    bool m_loaded = false;
};
