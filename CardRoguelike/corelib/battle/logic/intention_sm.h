#pragma once

#include "battle/entities/enemy.h"

class IntentionStateMachine
{
public:
    IntentionStateMachine(const EnemyData &data, Enemy *owner);

    EnemyIntentDef peekNextIntent();
    EnemyIntentDef nextIntent();

    int lastIntentIndex() const;

    bool isGuardianDefensiveMode() const;
    int guardianModeShiftThreshold() const;
    bool recordGuardianDamage(int damage);
    void guardianEnterDefensiveMode(Enemy *owner);
    void guardianExitDefensiveMode(Enemy *owner);

private:
    int resolveNextIntentIndex(bool commit) const;
    int pickSequenceIntent(bool commit) const;
    int pickLoopIntent() const;
    int pickProbabilityIntent(bool commit) const;
    int rollProbabilityIntent() const;
    int pickAlternatingIntent(bool commit) const;
    int pickInitialThenLoopIntent(bool commit) const;
    int pickInitialThenProbabilityIntent(bool commit) const;
    int pickInitialThenSequenceIntent(bool commit) const;
    int pickPhasedSequenceIntent(bool commit) const;
    int pickFungiRulesIntent(bool commit) const;
    int pickGuardianModeIntent(bool commit) const;
    int pickConditionalIntent() const;
    void checkSplitTrigger() const;
    QVector<int> buildProbabilityCandidates(bool relaxTwoConsecutive) const;
    bool violatesRestrictions(int intentIndex, bool enforceTwoConsecutive,
                              bool enforceThreeConsecutive) const;
    double weightForIntent(int intentIndex) const;

    const EnemyData &m_data;
    Enemy *m_owner = nullptr;
    mutable int m_sequenceIndex = 0;
    int m_turnCount = 0;
    int m_lastIntentIndex = -1;
    mutable int m_alternatingNext = -1;
    mutable int m_previewProbabilityPick = -1;
    mutable int m_previewTurnZeroPick = -1;
    mutable bool m_splitTriggerArmed = false;
    mutable bool m_splitUsed = false;
    mutable bool m_forceGrowthNext = false;
    mutable int m_consecutiveAttacks = 0;
    mutable bool m_guardianDefensiveMode = false;
    mutable int m_guardianSequenceIndex = 0;
    mutable int m_guardianDamageAccumulator = 0;
    mutable int m_guardianCurrentThreshold = 30;
    mutable int m_guardianResumeAttackIndex = -1;
    QVector<int> m_history;
};
