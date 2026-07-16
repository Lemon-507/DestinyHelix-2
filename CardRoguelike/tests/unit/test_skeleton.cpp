#include <QtTest>

#include "battle/entities/card.h"
#include "map/node_type.h"

class TestSkeleton : public QObject
{
    Q_OBJECT

private slots:
    void nodeTypeMetaEnumIsValid()
    {
        const QMetaEnum metaEnum = QMetaEnum::fromType<NodeType>();
        QVERIFY(metaEnum.isValid());
        QVERIFY(metaEnum.keyCount() > 0);
    }

    void cardBuilderProducesExpectedData()
    {
        const Card card = CardBuilder(QStringLiteral("Strike"), CardType::Attack)
                              .setCost(2)
                              .addEffect(EffectDef::damage(6))
                              .addEffect(EffectDef::damage(3))
                              .build();

        QCOMPARE(card.data().baseCost, 2);
        QCOMPARE(static_cast<int>(card.data().effects.size()), 2);
    }
};

int runSkeletonTests(int argc, char *argv[])
{
    TestSkeleton tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "test_skeleton.moc"
