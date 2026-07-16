#pragma once

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include <memory>

#include "actor.h"

struct EnemyIntentDef {
    QString name;
    QString type;
    int baseValue = 0;
    int blockValue = 0;
    QString buffType;
    int buffValue = 0;
    QString debuffType;
    int debuffValue = 0;
    QString cardId;
    int cardCount = 0;
    int times = 1;
    double chance = 1.0;
    QStringList splitInto;

    // Pending combat-start rolls (e.g. "random_5_to_7"). Cleared after resolve.
    bool rollBaseValue = false;
    int baseValueMin = 0;
    int baseValueMax = 0;
    bool rollBlockValue = false;
    int blockValueMin = 0;
    int blockValueMax = 0;
    bool rollBuffValue = false;
    int buffValueMin = 0;
    int buffValueMax = 0;
    bool rollDebuffValue = false;
    int debuffValueMin = 0;
    int debuffValueMax = 0;

    QString displayText() const;
    int displayValue(int ownerStrength = 0) const;
};

struct EnemyAbilityDef {
    QString type;
    int value = 0;
    QString description;

    bool rollValue = false;
    int valueMin = 0;
    int valueMax = 0;
};

struct BehaviorDef {
    QString type;
    QVector<int> sequence;
    QVector<int> loopSequence;
    QVector<int> chooseFrom;
    bool loop = true;
    QMap<int, double> probability;
    QMap<int, double> firstTurnProbability;
    QStringList restrictions;
    QStringList rules;
    int loopIntentIndex = 0;
    int initialIntentIndex = -1;
    int splitIntentIndex = -1;
    bool randomFirst = false;
    int alternatingA = 0;
    int alternatingB = 1;
    QString conditionType;
    QVector<int> attackModeSequence;
    QVector<int> defenseModeSequence;
    int modeSwitchDamageThreshold = 30;
    int thresholdIncreasePerSwitch = 10;
    int firstAttackModeTurn = 0;
    int firstDefenseModeTurn = 4;
};

struct EnemyData {
    QString id;
    QString name;
    QString type;
    int minHp = 0;
    int maxHp = 0;
    QVector<EnemyAbilityDef> abilities;
    QVector<EnemyIntentDef> intents;
    BehaviorDef behavior;
};

class IntentionStateMachine;

class Enemy : public Actor
{
    Q_OBJECT

public:
    explicit Enemy(QObject *parent = nullptr);
    ~Enemy() override;

    void loadFromData(const EnemyData &data);
    const EnemyData *data() const;

    EnemyIntentDef peekNextIntent();
    EnemyIntentDef consumeNextIntent();

    bool hasAbility(const QString &abilityType) const;
    int abilityValue(const QString &abilityType) const;
    bool isGuardianDefensiveMode() const;
    int guardianModeShiftThreshold() const;
    int lastIntentIndex() const;
    void notifyGuardianDamageTaken(int damage);
    void completeGuardianIntent(int intentIndex);

    QString enemyId() const;

    void setEncounterPeers(const QVector<Enemy *> *peers);
    const QVector<Enemy *> *encounterPeers() const;

signals:
    void guardianModeChanged(bool defensive);

private:
    void applyAbilities();
    void resolveCombatStartValues();

    EnemyData m_data;
    const QVector<Enemy *> *m_encounterPeers = nullptr;
    std::unique_ptr<IntentionStateMachine> m_intentMachine;
};
