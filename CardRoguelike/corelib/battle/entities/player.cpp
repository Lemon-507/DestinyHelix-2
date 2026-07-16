#include "player.h"

#include "battle/data/card_database.h"
#include "utils/rng.h"

Player::Player(QObject *parent)
    : Actor(parent)
{
}

QVector<Card> &Player::deck()
{
    return m_deck;
}

const QVector<Card> &Player::deck() const
{
    return m_deck;
}

void Player::clearDeck()
{
    m_deck.clear();
}

void Player::addCardToDeck(const Card &card)
{
    m_deck.append(card);
}

bool Player::removeCardAt(int index)
{
    if (index < 0 || index >= m_deck.size()) {
        return false;
    }
    m_deck.removeAt(index);
    return true;
}

bool Player::transformCardAt(int index)
{
    if (index < 0 || index >= m_deck.size()) {
        return false;
    }

    CardDatabase::instance().load();
    const QString currentId = m_deck.at(index).id();
    QStringList pool = CardDatabase::instance().rewardCardIds();
    pool.removeAll(currentId);
    if (pool.isEmpty()) {
        return false;
    }

    const QString newId = pool.at(RNG::get(0, pool.size() - 1));
    const Card replacement = CardDatabase::instance().getCard(newId);
    if (replacement.id().isEmpty()) {
        return false;
    }

    m_deck[index] = replacement;
    return true;
}

bool Player::canUpgrade() const
{
    CardDatabase::instance().load();
    for (const Card &card : m_deck) {
        if (CardDatabase::instance().canUpgrade(card.id())) {
            return true;
        }
    }
    return false;
}

bool Player::upgradeCard(int index)
{
    if (index < 0 || index >= m_deck.size()) {
        return false;
    }

    CardDatabase::instance().load();
    const Card &current = m_deck.at(index);
    const QString upgradedId = CardDatabase::instance().upgradedIdFor(current.id());
    if (upgradedId.isEmpty() || !CardDatabase::instance().contains(upgradedId)) {
        return false;
    }

    m_deck[index] = CardDatabase::instance().getCard(upgradedId);
    return true;
}

int Player::gold() const
{
    return m_gold;
}

void Player::addGold(int amount)
{
    m_gold += amount;
}

void Player::setGold(int amount)
{
    m_gold = amount;
}

QStringList Player::relicIds() const
{
    return m_relicIds;
}

bool Player::hasRelic(const QString &relicId) const
{
    return m_relicIds.contains(relicId);
}

void Player::addRelicId(const QString &relicId)
{
    if (!m_relicIds.contains(relicId)) {
        m_relicIds.append(relicId);
    }
}

void Player::removeRelicId(const QString &relicId)
{
    m_relicIds.removeAll(relicId);
}

void Player::setRelicIds(const QStringList &relicIds)
{
    m_relicIds = relicIds;
}

void Player::markPenNibDoubleDamage()
{
    m_penNibDoubleNext = true;
}

bool Player::consumePenNibDoubleDamage()
{
    if (!m_penNibDoubleNext) {
        return false;
    }

    m_penNibDoubleNext = false;
    return true;
}

int Player::potionCount(const QString &potionId) const
{
    return m_potionCounts.value(potionId, 0);
}

void Player::addPotion(const QString &potionId, int count)
{
    m_potionCounts[potionId] = potionCount(potionId) + count;
}

void Player::setPotionCount(const QString &potionId, int count)
{
    m_potionCounts[potionId] = count;
}

QHash<QString, int> Player::potionCounts() const
{
    return m_potionCounts;
}

void Player::setPotionCounts(const QHash<QString, int> &counts)
{
    m_potionCounts = counts;
}
