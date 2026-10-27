#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSet>
#include <QtCore/QVector>
#include <QtTest>

#include "battle/data/card_database.h"
#include "battle/entities/card.h"
#include "battle/logic/card_text.h"
#include "battle/logic/effect_types.h"

namespace {

void collectEffectTypes(const QJsonValue &value, QSet<QString> *types)
{
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (const QJsonValue &child : array) {
            collectEffectTypes(child, types);
        }
        return;
    }
    if (!value.isObject()) {
        return;
    }

    const QJsonObject object = value.toObject();
    const QJsonArray effects = object.value(QStringLiteral("effects")).toArray();
    for (const QJsonValue &effectValue : effects) {
        const QString type = effectValue.toObject().value(QStringLiteral("type")).toString();
        if (!type.isEmpty()) {
            types->insert(type);
        }
    }

    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (it.key() == QStringLiteral("effects")) {
            continue;
        }
        collectEffectTypes(it.value(), types);
    }
}

QVector<EffectDef> cardEffectsIn(const QJsonValue &value)
{
    QVector<EffectDef> effects;
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (const QJsonValue &child : array) {
            effects += cardEffectsIn(child);
        }
        return effects;
    }
    if (!value.isObject()) {
        return effects;
    }

    const QJsonObject object = value.toObject();
    const QJsonArray effectArray = object.value(QStringLiteral("effects")).toArray();
    for (const QJsonValue &effectValue : effectArray) {
        const QJsonObject effectObject = effectValue.toObject();
        EffectDef effect;
        effect.type = effectObject.value(QStringLiteral("type")).toString();
        effect.value = effectObject.value(QStringLiteral("value")).toInt();
        effect.target = effectObject.value(QStringLiteral("target")).toString();
        effect.powerId = effectObject.value(QStringLiteral("power_id")).toString();
        if (effect.powerId.isEmpty()) {
            effect.powerId = effectObject.value(QStringLiteral("power")).toString();
        }
        effect.cardId = effectObject.value(QStringLiteral("card_id")).toString();
        if (!effect.type.isEmpty()) {
            effects.append(effect);
        }
    }

    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (it.key() == QStringLiteral("effects")) {
            continue;
        }
        effects += cardEffectsIn(it.value());
    }
    return effects;
}

QJsonDocument loadResourceJson(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll());
}

} // namespace

class TestCardText : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        Q_INIT_RESOURCE(resources);
        CardDatabase::instance().load();
    }

    void test_inflame_gain_strength()
    {
        const Card card = CardDatabase::instance().getCard(QStringLiteral("inflame"));
        QCOMPARE(CardText::describeCard(card), QStringLiteral("Gain 2 Strength."));
    }

    void test_inflame_upgraded()
    {
        const QString upgradedId = CardDatabase::instance().upgradedIdFor(QStringLiteral("inflame"));
        const Card card = CardDatabase::instance().getCard(upgradedId);
        QCOMPARE(CardText::describeCard(card), QStringLiteral("Gain 3 Strength."));
    }

    void test_pommel_strike_damage_and_draw()
    {
        const Card card = CardDatabase::instance().getCard(QStringLiteral("pommel_strike"));
        QCOMPARE(CardText::describeCard(card), QStringLiteral("Deal 9 damage. Draw 1 card."));
    }

    void test_pommel_strike_upgraded()
    {
        const QString upgradedId =
            CardDatabase::instance().upgradedIdFor(QStringLiteral("pommel_strike"));
        const Card card = CardDatabase::instance().getCard(upgradedId);
        QCOMPARE(CardText::describeCard(card), QStringLiteral("Deal 10 damage. Draw 2 cards."));
    }

    void test_bash_applies_vulnerable()
    {
        const Card card = CardDatabase::instance().getCard(QStringLiteral("bash"));
        QCOMPARE(CardText::describeCard(card),
                 QStringLiteral("Deal 8 damage. Apply 2 Vulnerable."));
    }

    void test_thunderclap_all_enemies()
    {
        const Card card = CardDatabase::instance().getCard(QStringLiteral("thunderclap"));
        QCOMPARE(CardText::describeCard(card),
                 QStringLiteral("Deal 4 damage to all enemies. Apply 1 Vulnerable to all enemies."));
    }

    void test_body_slam()
    {
        const Card card = CardDatabase::instance().getCard(QStringLiteral("body_slam"));
        QCOMPARE(CardText::describeCard(card), QStringLiteral("Deal damage equal to your Block."));
    }

    void test_seeing_red_energy_and_exhaust()
    {
        const Card card = CardDatabase::instance().getCard(QStringLiteral("seeing_red"));
        QCOMPARE(CardText::describeCard(card), QStringLiteral("Gain 2 Energy. Exhaust."));
    }

    void test_burn_end_of_turn()
    {
        const Card card = CardDatabase::instance().getCard(QStringLiteral("burn"));
        QCOMPARE(CardText::describeCard(card),
                 QStringLiteral("At end of turn, take 2 damage."));
    }

    void test_dazed_ethereal()
    {
        const Card card = CardDatabase::instance().getCard(QStringLiteral("dazed"));
        QCOMPARE(CardText::describeCard(card), QStringLiteral("Ethereal."));
    }

    void test_wound_unplayable_only()
    {
        const Card card = CardDatabase::instance().getCard(QStringLiteral("wound"));
        QCOMPARE(CardText::describeCard(card), QStringLiteral("No effect."));
    }

    void test_power_through_adds_wounds()
    {
        const Card card = CardDatabase::instance().getCard(QStringLiteral("power_through"));
        QCOMPARE(CardText::describeCard(card),
                 QStringLiteral("Add 2 Wound to your hand. Gain 15 Block."));
    }

    void test_json_effect_types_are_registered()
    {
        const QStringList paths = {
            QStringLiteral(":/resources/data/cards.json"),
            QStringLiteral(":/resources/data/relics.json"),
            QStringLiteral(":/resources/data/events.json"),
            QStringLiteral(":/resources/data/potions.json"),
        };

        for (const QString &path : paths) {
            const QJsonDocument document = loadResourceJson(path);
            QVERIFY2(!document.isNull(), qPrintable(path));

            QSet<QString> types;
            collectEffectTypes(document.isArray() ? QJsonValue(document.array())
                                                   : QJsonValue(document.object()),
                               &types);
            QVERIFY2(!types.isEmpty(), qPrintable(path));
            for (const QString &type : types) {
                QVERIFY2(isKnownEffectType(type), qPrintable(path + QStringLiteral(": ") + type));
            }
        }

        const QJsonDocument cards = loadResourceJson(QStringLiteral(":/resources/data/cards.json"));
        const QVector<EffectDef> effects = cardEffectsIn(QJsonValue(cards.array()));
        QVERIFY(!effects.isEmpty());
        for (const EffectDef &effect : effects) {
            if (effect.type == QStringLiteral("unplayable")) {
                continue;
            }
            const QString text = CardText::describeEffect(effect, TargetType::Enemy);
            QVERIFY2(!text.isEmpty(), qPrintable(effect.type));
        }
    }
};

int runCardTextTests(int argc, char *argv[])
{
    TestCardText tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "test_card_text.moc"
