#include "card.h"

EffectDef EffectDef::damage(int amount)
{
    EffectDef effect;
    effect.type = QStringLiteral("deal_damage");
    effect.value = amount;
    effect.target = QStringLiteral("selected_enemy");
    return effect;
}

EffectDef EffectDef::gainBlock(int amount)
{
    EffectDef effect;
    effect.type = QStringLiteral("gain_block");
    effect.value = amount;
    effect.target = QStringLiteral("self");
    return effect;
}

EffectDef EffectDef::applyPower(const QString &powerId, int value, int duration)
{
    EffectDef effect;
    effect.type = QStringLiteral("apply_power");
    effect.powerId = powerId;
    effect.value = value;
    effect.duration = duration;
    effect.target = QStringLiteral("selected_enemy");
    return effect;
}

Card::Card()
    : m_data()
{
}

Card::Card(CardData data)
    : m_data(std::move(data))
{
}

const CardData &Card::data() const
{
    return m_data;
}

QString Card::id() const
{
    return m_data.id;
}

int Card::cost() const
{
    return m_data.baseCost;
}

int Card::baseCost() const
{
    return m_data.baseCost;
}

const std::vector<EffectDef> &Card::effects() const
{
    return m_data.effects;
}

bool Card::matches(const Card &other) const
{
    return m_data.id == other.m_data.id && m_data.baseCost == other.m_data.baseCost
           && m_data.type == other.m_data.type && m_data.effects.size() == other.m_data.effects.size();
}

bool Card::canUpgrade() const
{
    return !m_data.upgradedId.isEmpty();
}

bool Card::isUpgraded() const
{
    return m_data.id.endsWith(QStringLiteral("+"));
}

bool Card::hasEffectType(const QString &effectType) const
{
    for (const EffectDef &effect : m_data.effects) {
        if (effect.type == effectType) {
            return true;
        }
    }
    return false;
}

int Card::effectValue(const QString &effectType) const
{
    for (const EffectDef &effect : m_data.effects) {
        if (effect.type == effectType) {
            return effect.value;
        }
    }
    return 0;
}

CardBuilder::CardBuilder(const QString &id, CardType type)
{
    m_data.id = id;
    m_data.type = type;
}

CardBuilder &CardBuilder::setCost(int cost)
{
    m_data.baseCost = cost;
    return *this;
}

CardBuilder &CardBuilder::setRarity(CardRarity rarity)
{
    m_data.rarity = rarity;
    return *this;
}

CardBuilder &CardBuilder::setTarget(TargetType target)
{
    m_data.target = target;
    return *this;
}

CardBuilder &CardBuilder::addEffect(const EffectDef &effect)
{
    m_data.effects.push_back(effect);
    return *this;
}

Card CardBuilder::build() const
{
    return Card(m_data);
}
