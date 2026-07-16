#include "item_tooltips.h"

#include <QtCore/QStringList>

#include "battle/data/card_database.h"
#include "battle/data/relic_database.h"
#include "battle/entities/card.h"
#include "battle/logic/card_text.h"
#include "potions/potion_database.h"

namespace ItemTooltips {
namespace {

QString cardTypeName(CardType type)
{
    switch (type) {
    case CardType::Skill:
        return QStringLiteral("Skill");
    case CardType::Power:
        return QStringLiteral("Power");
    case CardType::Attack:
    default:
        return QStringLiteral("Attack");
    }
}

QString cardRarityName(CardRarity rarity)
{
    switch (rarity) {
    case CardRarity::Common:
        return QStringLiteral("Common");
    case CardRarity::Uncommon:
        return QStringLiteral("Uncommon");
    case CardRarity::Rare:
        return QStringLiteral("Rare");
    case CardRarity::Basic:
    default:
        return QStringLiteral("Basic");
    }
}

} // namespace

QString forCard(const QString &cardId)
{
    if (cardId.isEmpty()) {
        return {};
    }

    CardDatabase::instance().load();
    const Card card = CardDatabase::instance().getCard(cardId);
    if (card.id().isEmpty()) {
        return {};
    }

    const QString name = CardDatabase::instance().displayName(cardId);
    return QStringLiteral("%1\nCost: %2\nType: %3\nRarity: %4\n%5")
        .arg(name.isEmpty() ? cardId : name)
        .arg(card.cost())
        .arg(cardTypeName(card.data().type))
        .arg(cardRarityName(card.data().rarity))
        .arg(CardText::describeCard(card));
}

QString forRelic(const QString &relicId)
{
    if (relicId.isEmpty()) {
        return {};
    }

    RelicDatabase::instance().load();
    const RelicData relic = RelicDatabase::instance().getRelic(relicId);
    const QString name = relic.name.isEmpty() ? relicId : relic.name;
    if (relic.description.isEmpty()) {
        return name;
    }
    return QStringLiteral("%1\n%2").arg(name, relic.description);
}

QString forPotion(const QString &potionId)
{
    if (potionId.isEmpty()) {
        return {};
    }

    PotionDatabase::instance().load();
    const PotionData potion = PotionDatabase::instance().getPotion(potionId);
    const QString name = potion.name.isEmpty() ? potionId : potion.name;
    if (potion.description.isEmpty()) {
        return name;
    }
    return QStringLiteral("%1\n%2").arg(name, potion.description);
}

} // namespace ItemTooltips
