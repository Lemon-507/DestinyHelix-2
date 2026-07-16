#include <QtTest>

#include "battle/data/card_database.h"
#include "battle/entities/card.h"
#include "battle/logic/card_text.h"

class TestCardText : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
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
};

int runCardTextTests(int argc, char *argv[])
{
    TestCardText tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "test_card_text.moc"
