#include "card_collection_viewmodel.h"

#include "battle/data/card_database.h"
#include "battle/entities/card.h"
#include "battle/entities/player.h"
#include "game_manager.h"
#include "utils/item_tooltips.h"

CardCollectionViewModel::CardCollectionViewModel(GameManager *gameManager, QObject *parent)
    : QObject(parent)
    , m_gameManager(gameManager)
{
    refresh();
}

const QVector<CardCollectionViewModel::CardEntry> &CardCollectionViewModel::cards() const
{
    return m_cards;
}

QString CardCollectionViewModel::selectionAction() const
{
    return m_selectionAction;
}

bool CardCollectionViewModel::selectionMode() const
{
    return m_selectionMode;
}

void CardCollectionViewModel::setSelectionAction(const QString &actionType)
{
    m_selectionAction = actionType;
    m_selectionMode = !actionType.isEmpty();
    refresh();
}

void CardCollectionViewModel::clearSelectionMode()
{
    m_selectionAction.clear();
    m_selectionMode = false;
    refresh();
}

void CardCollectionViewModel::refresh()
{
    m_cards.clear();

    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        emit cardsChanged();
        return;
    }

    CardDatabase::instance().load();
    const QVector<Card> &deck = m_gameManager->player()->deck();
    m_cards.reserve(deck.size());

    for (int i = 0; i < deck.size(); ++i) {
        const Card &card = deck.at(i);
        CardEntry entry;
        entry.id = card.id();
        entry.deckIndex = i;
        entry.displayName = CardDatabase::instance().displayName(card.id());
        if (entry.displayName.isEmpty()) {
            entry.displayName = card.id();
        }
        entry.abbreviation = abbreviationFor(entry.displayName, card.id());
        entry.tooltip = tooltipForCard(card);
        entry.selectable = !m_selectionMode || isEntrySelectable(card);
        m_cards.append(entry);
    }

    emit cardsChanged();
}

void CardCollectionViewModel::requestExit()
{
    if (m_selectionMode) {
        emit selectionCancelled();
    } else {
        emit exitRequested();
    }
}

bool CardCollectionViewModel::isEntrySelectable(const Card &card) const
{
    if (m_selectionAction == QStringLiteral("upgrade")) {
        CardDatabase::instance().load();
        return CardDatabase::instance().canUpgrade(card.id());
    }
    // remove / transform: any card
    return true;
}

QString CardCollectionViewModel::abbreviationFor(const QString &name, const QString &fallbackId)
{
    if (name.isEmpty()) {
        return fallbackId.left(6).toUpper();
    }

    const QStringList words = name.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (words.size() >= 2) {
        QString abbr;
        for (const QString &word : words) {
            if (!word.isEmpty()) {
                abbr.append(word.at(0).toUpper());
            }
            if (abbr.size() >= 6) {
                break;
            }
        }
        return abbr.left(6);
    }

    return name.left(8);
}

QString CardCollectionViewModel::tooltipForCard(const Card &card)
{
    QString tip = ItemTooltips::forCard(card.id());
    if (tip.isEmpty()) {
        tip = card.id();
    }

    CardDatabase::instance().load();
    if (CardDatabase::instance().canUpgrade(card.id())) {
        const QString upgradedId = CardDatabase::instance().upgradedIdFor(card.id());
        const QString upgradedTip = ItemTooltips::forCard(upgradedId);
        if (!upgradedTip.isEmpty()) {
            tip += QStringLiteral("\n\n--- Upgraded ---\n");
            tip += upgradedTip;
        }
    }

    return tip;
}
