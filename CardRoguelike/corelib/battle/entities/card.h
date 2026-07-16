#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <vector>

namespace CardEnums {

Q_NAMESPACE

enum class CardType {
    Attack,
    Skill,
    Power
};

Q_ENUM_NS(CardType)

enum class CardRarity {
    Basic,
    Common,
    Uncommon,
    Rare
};

Q_ENUM_NS(CardRarity)

enum class TargetType {
    Enemy,
    AllEnemies,
    Self
};

Q_ENUM_NS(TargetType)

} // namespace CardEnums

using CardType = CardEnums::CardType;
using CardRarity = CardEnums::CardRarity;
using TargetType = CardEnums::TargetType;

struct EffectDef {
    QString type;
    int value = 0;
    QString target;
    QString powerId;
    QString cardId;
    int duration = 1;

    static EffectDef damage(int amount);
    static EffectDef gainBlock(int amount);
    static EffectDef applyPower(const QString &powerId, int value, int duration = 1);
};

struct CardData {
    QString id;
    QString upgradedId;
    int baseCost = 0;
    CardType type = CardType::Attack;
    CardRarity rarity = CardRarity::Basic;
    TargetType target = TargetType::Enemy;
    std::vector<EffectDef> effects;
};

class Card
{
public:
    Card();
    explicit Card(CardData data);

    const CardData &data() const;
    QString id() const;
    int cost() const;
    int baseCost() const;
    const std::vector<EffectDef> &effects() const;

    bool matches(const Card &other) const;
    bool canUpgrade() const;
    bool isUpgraded() const;
    bool hasEffectType(const QString &effectType) const;
    int effectValue(const QString &effectType) const;

private:
    CardData m_data;
};

class CardBuilder
{
public:
    CardBuilder(const QString &id, CardType type);

    CardBuilder &setCost(int cost);
    CardBuilder &setRarity(CardRarity rarity);
    CardBuilder &setTarget(TargetType target);
    CardBuilder &addEffect(const EffectDef &effect);
    Card build() const;

private:
    CardData m_data;
};
