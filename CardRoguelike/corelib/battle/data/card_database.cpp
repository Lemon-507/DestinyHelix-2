#include "card_database.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QStringList>

namespace {

CardType parseCardType(const QString &value)
{
    if (value == QStringLiteral("Skill") || value == QStringLiteral("Status")) {
        // Status cards (Wound, Burn, Dazed, Slimed) are not Attacks — map to Skill
        // so playing Slimed does not trigger OnAttackPlayed relics.
        return CardType::Skill;
    }
    if (value == QStringLiteral("Power")) {
        return CardType::Power;
    }
    return CardType::Attack;
}

CardRarity parseCardRarity(const QString &value)
{
    if (value == QStringLiteral("Common")) {
        return CardRarity::Common;
    }
    if (value == QStringLiteral("Uncommon")) {
        return CardRarity::Uncommon;
    }
    if (value == QStringLiteral("Rare")) {
        return CardRarity::Rare;
    }
    return CardRarity::Basic;
}

TargetType parseTargetType(const QString &value)
{
    if (value == QStringLiteral("Self") || value == QStringLiteral("None")) {
        // "None" (status cards like Slimed) needs no enemy selection — treat as Self.
        return TargetType::Self;
    }
    if (value == QStringLiteral("AllEnemies")) {
        return TargetType::AllEnemies;
    }
    return TargetType::Enemy;
}

EffectDef parseEffect(const QJsonObject &obj)
{
    EffectDef effect;
    effect.type = obj.value(QStringLiteral("type")).toString();
    effect.value = obj.value(QStringLiteral("value")).toInt();
    effect.target = obj.value(QStringLiteral("target")).toString();
    effect.powerId = obj.value(QStringLiteral("power_id")).toString();
    if (effect.powerId.isEmpty()) {
        effect.powerId = obj.value(QStringLiteral("power")).toString();
    }
    effect.cardId = obj.value(QStringLiteral("card_id")).toString();
    effect.duration = obj.value(QStringLiteral("duration")).toInt(1);
    return effect;
}

} // namespace

CardDatabase &CardDatabase::instance()
{
    static CardDatabase database;
    return database;
}

void CardDatabase::load()
{
    if (m_loaded) {
        return;
    }

    QFile file(QStringLiteral(":/resources/data/cards.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (document.isArray()) {
        for (const QJsonValue &value : document.array()) {
            parseCardObject(value.toObject());
        }
    } else {
        const QJsonArray cards = document.object().value(QStringLiteral("cards")).toArray();
        for (const QJsonValue &value : cards) {
            parseCardObject(value.toObject());
        }
    }

    m_loaded = true;
}

Card CardDatabase::getCard(const QString &id) const
{
    for (const Card &card : m_cards) {
        if (card.id() == id) {
            return card;
        }
    }
    return Card();
}

bool CardDatabase::contains(const QString &id) const
{
    for (const Card &card : m_cards) {
        if (card.id() == id) {
            return true;
        }
    }
    return false;
}

QString CardDatabase::upgradedIdFor(const QString &id) const
{
    for (const Card &card : m_cards) {
        if (card.id() == id) {
            return card.data().upgradedId;
        }
    }
    return QString();
}

bool CardDatabase::canUpgrade(const QString &id) const
{
    const QString upgradedId = upgradedIdFor(id);
    return !upgradedId.isEmpty() && contains(upgradedId);
}

QString CardDatabase::displayName(const QString &id) const
{
    if (id == QStringLiteral("strike") || id == QStringLiteral("strike+")) {
        return id.endsWith(QStringLiteral("+")) ? QStringLiteral("Strike+") : QStringLiteral("Strike");
    }
    if (id == QStringLiteral("defend") || id == QStringLiteral("defend+")) {
        return id.endsWith(QStringLiteral("+")) ? QStringLiteral("Defend+") : QStringLiteral("Defend");
    }
    if (id == QStringLiteral("bash") || id == QStringLiteral("bash+")) {
        return id.endsWith(QStringLiteral("+")) ? QStringLiteral("Bash+") : QStringLiteral("Bash");
    }
    return id;
}

QStringList CardDatabase::rewardCardIds() const
{
    QStringList ids;
    for (const Card &card : m_cards) {
        const QString id = card.id();
        if (id.isEmpty() || id.endsWith(QLatin1Char('+'))) {
            continue;
        }

        const CardRarity rarity = card.data().rarity;
        if (rarity != CardRarity::Common && rarity != CardRarity::Uncommon
            && rarity != CardRarity::Rare) {
            continue;
        }
        ids.append(id);
    }
    return ids;
}

void CardDatabase::parseCardObject(const QJsonObject &obj)
{
    const QString id = obj.value(QStringLiteral("id")).toString();
    CardBuilder builder(id, parseCardType(obj.value(QStringLiteral("type")).toString()));
    builder.setCost(obj.value(QStringLiteral("cost")).toInt());
    builder.setRarity(parseCardRarity(obj.value(QStringLiteral("rarity")).toString()));
    builder.setTarget(parseTargetType(obj.value(QStringLiteral("target")).toString()));

    CardData data = builder.build().data();

    const QJsonObject upgradedObj = obj.value(QStringLiteral("upgraded")).toObject();
    QString upgradedId = obj.value(QStringLiteral("upgraded_id")).toString();
    if (upgradedId.isEmpty() && !upgradedObj.isEmpty()) {
        upgradedId = id + QStringLiteral("+");
    }
    data.upgradedId = upgradedId;

    const QJsonArray effects = obj.value(QStringLiteral("effects")).toArray();
    data.effects.clear();
    for (const QJsonValue &effectValue : effects) {
        EffectDef effect = parseEffect(effectValue.toObject());
        data.effects.push_back(effect);
    }

    m_cards.append(Card(data));

    if (!upgradedObj.isEmpty() && !upgradedId.isEmpty()) {
        CardData upgraded = data;
        upgraded.id = upgradedId;
        upgraded.upgradedId.clear();
        if (upgradedObj.contains(QStringLiteral("cost"))) {
            upgraded.baseCost = upgradedObj.value(QStringLiteral("cost")).toInt();
        }
        if (upgradedObj.contains(QStringLiteral("effects"))) {
            upgraded.effects.clear();
            const QJsonArray upgradedEffects = upgradedObj.value(QStringLiteral("effects")).toArray();
            for (const QJsonValue &effectValue : upgradedEffects) {
                upgraded.effects.push_back(parseEffect(effectValue.toObject()));
            }
        }
        m_cards.append(Card(upgraded));
    }
}
