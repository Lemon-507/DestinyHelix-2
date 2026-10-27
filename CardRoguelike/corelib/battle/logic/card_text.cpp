#include "card_text.h"

#include <QtCore/QDebug>
#include <QtCore/QtGlobal>
#include <QtCore/QStringList>

#include "battle/data/card_database.h"

namespace CardText {
namespace {

QString titleCaseId(const QString &id)
{
    if (id.isEmpty()) {
        return {};
    }

    QStringList words = id.split(QLatin1Char('_'), Qt::SkipEmptyParts);
    for (QString &word : words) {
        if (!word.isEmpty()) {
            word[0] = word[0].toUpper();
        }
    }
    return words.join(QLatin1Char(' '));
}

QString resolveCardName(const QString &cardId)
{
    if (cardId.isEmpty()) {
        return QStringLiteral("card");
    }

    CardDatabase::instance().load();
    const QString name = CardDatabase::instance().displayName(cardId);
    // displayName often echoes the raw id for unmapped cards; prefer Title Case.
    if (!name.isEmpty() && name != cardId) {
        return name;
    }
    return titleCaseId(cardId);
}

QString pluralizeCards(int count)
{
    return count == 1 ? QStringLiteral("card") : QStringLiteral("cards");
}

} // namespace

QString describeEffect(const EffectDef &effect, TargetType cardTarget)
{
    const QString &type = effect.type;
    if (type.isEmpty()) {
        return {};
    }

    if (type == QStringLiteral("deal_damage")) {
        if (cardTarget == TargetType::AllEnemies) {
            return QStringLiteral("Deal %1 damage to all enemies").arg(effect.value);
        }
        return QStringLiteral("Deal %1 damage").arg(effect.value);
    }
    if (type == QStringLiteral("deal_damage_per_block")) {
        return QStringLiteral("Deal damage equal to your Block");
    }
    if (type == QStringLiteral("gain_block")) {
        return QStringLiteral("Gain %1 Block").arg(effect.value);
    }
    if (type == QStringLiteral("gain_strength")) {
        return QStringLiteral("Gain %1 Strength").arg(effect.value);
    }
    if (type == QStringLiteral("gain_energy")) {
        return QStringLiteral("Gain %1 Energy").arg(effect.value);
    }
    if (type == QStringLiteral("draw_card") || type == QStringLiteral("draw_cards")) {
        return QStringLiteral("Draw %1 %2").arg(effect.value).arg(pluralizeCards(effect.value));
    }
    if (type == QStringLiteral("lose_hp")) {
        return QStringLiteral("Lose %1 HP").arg(effect.value);
    }
    if (type == QStringLiteral("restore_hp")) {
        return QStringLiteral("Heal %1 HP").arg(effect.value);
    }
    if (type == QStringLiteral("restore_hp_percent")) {
        return QStringLiteral("Heal %1% max HP").arg(effect.value);
    }
    if (type == QStringLiteral("increase_damage_per_play")) {
        return QStringLiteral("Increase this card's damage by %1 for this combat").arg(effect.value);
    }
    if (type == QStringLiteral("exhaust_non_attack_gain_block")) {
        return QStringLiteral("Exhaust all non-Attack cards. Gain %1 Block per card")
            .arg(effect.value);
    }
    if (type == QStringLiteral("add_card_to_hand")) {
        const QString cardName = resolveCardName(effect.cardId);
        const int count = qMax(1, effect.value);
        if (count == 1) {
            return QStringLiteral("Add %1 to your hand").arg(cardName);
        }
        return QStringLiteral("Add %1 %2 to your hand").arg(count).arg(cardName);
    }
    if (type == QStringLiteral("exhaust")) {
        return QStringLiteral("Exhaust");
    }
    if (type == QStringLiteral("ethereal")) {
        return QStringLiteral("Ethereal");
    }
    if (type == QStringLiteral("unplayable")) {
        return {};
    }
    if (type == QStringLiteral("end_of_turn_damage")) {
        return QStringLiteral("At end of turn, take %1 damage").arg(effect.value);
    }
    if (type == QStringLiteral("apply_power")
        || type == QStringLiteral("apply_power_to_all_enemies")) {
        const QString powerName = titleCaseId(effect.powerId);
        if (powerName.isEmpty()) {
            return QStringLiteral("Apply a power");
        }

        const bool toAll = type == QStringLiteral("apply_power_to_all_enemies")
                           || cardTarget == TargetType::AllEnemies;
        if (toAll) {
            return QStringLiteral("Apply %1 %2 to all enemies").arg(effect.value).arg(powerName);
        }
        if (cardTarget == TargetType::Self) {
            return QStringLiteral("Gain %1 %2").arg(effect.value).arg(powerName);
        }
        return QStringLiteral("Apply %1 %2").arg(effect.value).arg(powerName);
    }

    qWarning("CardText: unknown effect type \"%s\"", qPrintable(type));
    return {};
}

QString describeCard(const Card &card)
{
    QStringList parts;
    parts.reserve(static_cast<int>(card.effects().size()));
    for (const EffectDef &effect : card.effects()) {
        const QString part = describeEffect(effect, card.data().target);
        if (!part.isEmpty()) {
            parts.append(part);
        }
    }

    if (parts.isEmpty()) {
        return QStringLiteral("No effect.");
    }

    return parts.join(QStringLiteral(". ")) + QLatin1Char('.');
}

} // namespace CardText
