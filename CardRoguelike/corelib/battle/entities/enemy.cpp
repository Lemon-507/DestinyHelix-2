#include "enemy.h"

#include <QtCore/QDebug>

#include "battle/logic/intention_sm.h"
#include "battle/logic/power_factory.h"
#include "utils/rng.h"

namespace {

int rollOrKeep(int current, bool shouldRoll, int minValue, int maxValue)
{
    if (!shouldRoll) {
        return current;
    }
    if (minValue > maxValue) {
        qWarning() << "Enemy: invalid combat-start range" << minValue << ".." << maxValue;
        return minValue;
    }
    return RNG::get(minValue, maxValue);
}

void resolveIntentValues(EnemyIntentDef &intent)
{
    intent.baseValue =
        rollOrKeep(intent.baseValue, intent.rollBaseValue, intent.baseValueMin, intent.baseValueMax);
    intent.blockValue = rollOrKeep(intent.blockValue, intent.rollBlockValue, intent.blockValueMin,
                                   intent.blockValueMax);
    intent.buffValue =
        rollOrKeep(intent.buffValue, intent.rollBuffValue, intent.buffValueMin, intent.buffValueMax);
    intent.debuffValue = rollOrKeep(intent.debuffValue, intent.rollDebuffValue, intent.debuffValueMin,
                                    intent.debuffValueMax);

    intent.rollBaseValue = false;
    intent.rollBlockValue = false;
    intent.rollBuffValue = false;
    intent.rollDebuffValue = false;
}

void resolveAbilityValues(EnemyAbilityDef &ability)
{
    ability.value = rollOrKeep(ability.value, ability.rollValue, ability.valueMin, ability.valueMax);
    ability.rollValue = false;
}

} // namespace

QString EnemyIntentDef::displayText() const
{
    if (!name.isEmpty()) {
        return name;
    }
    if (type == QStringLiteral("attack")) {
        return QStringLiteral("Attack");
    }
    if (type == QStringLiteral("block")) {
        return QStringLiteral("Defend");
    }
    if (type == QStringLiteral("buff")) {
        return QStringLiteral("Buff");
    }
    if (type == QStringLiteral("debuff")) {
        return QStringLiteral("Debuff");
    }
    if (type == QStringLiteral("none")) {
        return QStringLiteral("Prepare");
    }
    if (type == QStringLiteral("split")) {
        return QStringLiteral("Split");
    }
    return type;
}

int EnemyIntentDef::displayValue(int ownerStrength) const
{
    if (type == QStringLiteral("attack") || type == QStringLiteral("attack_and_block")
        || type == QStringLiteral("attack_and_debuff") || type == QStringLiteral("attack_multiple")
        || type == QStringLiteral("attack_multiple_and_add_cards")
        || type == QStringLiteral("attack_multiple_scaling")
        || type == QStringLiteral("attack_and_add_cards")) {
        return (baseValue + ownerStrength) * times;
    }
    if (type == QStringLiteral("block") || type == QStringLiteral("buff_and_block")) {
        return blockValue;
    }
    if (type == QStringLiteral("buff") || type == QStringLiteral("buff_and_block")) {
        return buffValue;
    }
    if (type == QStringLiteral("debuff") || type == QStringLiteral("attack_and_debuff")) {
        return debuffValue;
    }
    return baseValue;
}

Enemy::Enemy(QObject *parent)
    : Actor(parent)
{
}

Enemy::~Enemy() = default;

void Enemy::loadFromData(const EnemyData &data)
{
    m_data = data;
    resolveCombatStartValues();
    const int hp = RNG::get(m_data.minHp, m_data.maxHp);
    setMaxHp(hp);
    healToFull();
    m_intentMachine = std::make_unique<IntentionStateMachine>(m_data, this);
    applyAbilities();
}

const EnemyData *Enemy::data() const
{
    return &m_data;
}

EnemyIntentDef Enemy::peekNextIntent()
{
    if (!m_intentMachine) {
        return EnemyIntentDef();
    }
    return m_intentMachine->peekNextIntent();
}

EnemyIntentDef Enemy::consumeNextIntent()
{
    if (!m_intentMachine) {
        return EnemyIntentDef();
    }
    return m_intentMachine->nextIntent();
}

bool Enemy::isGuardianDefensiveMode() const
{
    return m_intentMachine != nullptr && m_intentMachine->isGuardianDefensiveMode();
}

int Enemy::guardianModeShiftThreshold() const
{
    return m_intentMachine != nullptr ? m_intentMachine->guardianModeShiftThreshold() : 0;
}

int Enemy::lastIntentIndex() const
{
    return m_intentMachine != nullptr ? m_intentMachine->lastIntentIndex() : -1;
}

void Enemy::notifyGuardianDamageTaken(int damage)
{
    if (m_intentMachine == nullptr || m_data.behavior.type != QStringLiteral("guardian_mode")) {
        return;
    }

    if (m_intentMachine->recordGuardianDamage(damage)) {
        m_intentMachine->guardianEnterDefensiveMode(this);
        emit guardianModeChanged(true);
    }
}

void Enemy::completeGuardianIntent(int intentIndex)
{
    if (m_intentMachine == nullptr || m_data.behavior.type != QStringLiteral("guardian_mode")) {
        return;
    }

    if (intentIndex == 6 && m_intentMachine->isGuardianDefensiveMode()) {
        m_intentMachine->guardianExitDefensiveMode(this);
        emit guardianModeChanged(false);
    }
}

bool Enemy::hasAbility(const QString &abilityType) const
{
    for (const EnemyAbilityDef &ability : m_data.abilities) {
        if (ability.type == abilityType) {
            return true;
        }
    }
    return false;
}

int Enemy::abilityValue(const QString &abilityType) const
{
    for (const EnemyAbilityDef &ability : m_data.abilities) {
        if (ability.type == abilityType) {
            return ability.value;
        }
    }
    return 0;
}

QString Enemy::enemyId() const
{
    return m_data.id;
}

void Enemy::setEncounterPeers(const QVector<Enemy *> *peers)
{
    m_encounterPeers = peers;
}

const QVector<Enemy *> *Enemy::encounterPeers() const
{
    return m_encounterPeers;
}

void Enemy::resolveCombatStartValues()
{
    for (EnemyIntentDef &intent : m_data.intents) {
        resolveIntentValues(intent);

        if (intent.type.isEmpty()) {
            qWarning() << "Enemy" << m_data.id << "has intent with empty type";
        }
        if ((intent.type == QStringLiteral("attack") || intent.type == QStringLiteral("attack_multiple")
             || intent.type == QStringLiteral("attack_and_block")
             || intent.type == QStringLiteral("attack_and_debuff")
             || intent.type == QStringLiteral("attack_and_add_cards")
             || intent.type == QStringLiteral("attack_multiple_and_add_cards"))
            && intent.baseValue <= 0 && intent.type != QStringLiteral("attack_multiple_scaling")) {
            qWarning() << "Enemy" << m_data.id << "intent" << intent.name
                       << "has non-positive baseValue" << intent.baseValue;
        }
    }
    for (EnemyAbilityDef &ability : m_data.abilities) {
        resolveAbilityValues(ability);
        if (ability.type.isEmpty()) {
            qWarning() << "Enemy" << m_data.id << "has ability with empty type";
        }
    }
}

void Enemy::applyAbilities()
{
    for (const EnemyAbilityDef &ability : m_data.abilities) {
        if (!ability.type.isEmpty()) {
            addPower(PowerFactory::createPower(ability.type, ability.value, -1));
        }
    }
}
