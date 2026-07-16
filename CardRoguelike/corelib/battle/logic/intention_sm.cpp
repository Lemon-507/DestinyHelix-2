#include "intention_sm.h"

#include <QtCore/QDebug>

#include "battle/entities/enemy.h"
#include "battle/logic/power_factory.h"
#include "utils/rng.h"

namespace {

bool otherEnemiesAlive(const Enemy *self, const QVector<Enemy *> *peers)
{
    if (self == nullptr || peers == nullptr) {
        return false;
    }

    for (const Enemy *enemy : *peers) {
        if (enemy != nullptr && enemy != self && enemy->currentHp() > 0) {
            return true;
        }
    }
    return false;
}

} // namespace

IntentionStateMachine::IntentionStateMachine(const EnemyData &data, Enemy *owner)
    : m_data(data)
    , m_owner(owner)
{
    if (m_data.behavior.type == QStringLiteral("guardian_mode")) {
        m_guardianCurrentThreshold = m_data.behavior.modeSwitchDamageThreshold;
    }
}

EnemyIntentDef IntentionStateMachine::peekNextIntent()
{
    const int index = resolveNextIntentIndex(false);
    if (index < 0 || index >= m_data.intents.size()) {
        return EnemyIntentDef();
    }
    return m_data.intents.at(index);
}

EnemyIntentDef IntentionStateMachine::nextIntent()
{
    const int index = resolveNextIntentIndex(true);
    if (index < 0 || index >= m_data.intents.size()) {
        return EnemyIntentDef();
    }

    m_lastIntentIndex = index;
    m_history.append(index);
    ++m_turnCount;
    m_previewProbabilityPick = -1;
    m_previewTurnZeroPick = -1;

    if (index == 0 && m_data.behavior.type == QStringLiteral("fungi_rules")) {
        ++m_consecutiveAttacks;
        m_forceGrowthNext = false;
    } else if (index == 1 && m_data.behavior.type == QStringLiteral("fungi_rules")) {
        m_consecutiveAttacks = 0;
        m_forceGrowthNext = false;
    } else if (m_data.behavior.type == QStringLiteral("fungi_rules")) {
        m_consecutiveAttacks = 0;
    }

    return m_data.intents.at(index);
}

int IntentionStateMachine::lastIntentIndex() const
{
    return m_lastIntentIndex;
}

int IntentionStateMachine::resolveNextIntentIndex(bool commit) const
{
    checkSplitTrigger();

    if (m_splitTriggerArmed && m_data.behavior.splitIntentIndex >= 0) {
        const int splitIndex = m_data.behavior.splitIntentIndex;
        if (commit) {
            m_splitTriggerArmed = false;
            m_splitUsed = true;
        }
        return splitIndex;
    }

    const QString behaviorType = m_data.behavior.type;
    if (behaviorType == QStringLiteral("sequence")) {
        return pickSequenceIntent(commit);
    }
    if (behaviorType == QStringLiteral("loop")) {
        Q_UNUSED(commit);
        return pickLoopIntent();
    }
    if (behaviorType == QStringLiteral("probability")) {
        return pickProbabilityIntent(commit);
    }
    if (behaviorType == QStringLiteral("alternating")) {
        return pickAlternatingIntent(commit);
    }
    if (behaviorType == QStringLiteral("conditional_loop")
        || behaviorType == QStringLiteral("conditional_intent")) {
        Q_UNUSED(commit);
        return pickConditionalIntent();
    }
    if (behaviorType == QStringLiteral("initial_then_loop")) {
        return pickInitialThenLoopIntent(commit);
    }
    if (behaviorType == QStringLiteral("initial_then_probability")) {
        return pickInitialThenProbabilityIntent(commit);
    }
    if (behaviorType == QStringLiteral("initial_then_sequence")) {
        return pickInitialThenSequenceIntent(commit);
    }
    if (behaviorType == QStringLiteral("phased_sequence")) {
        return pickPhasedSequenceIntent(commit);
    }
    if (behaviorType == QStringLiteral("fungi_rules")) {
        return pickFungiRulesIntent(commit);
    }
    if (behaviorType == QStringLiteral("guardian_mode")) {
        return pickGuardianModeIntent(commit);
    }

    if (!m_data.intents.isEmpty()) {
        qWarning() << "IntentionStateMachine: unknown behavior type" << behaviorType << "for enemy"
                   << m_data.id << "- defaulting to intent 0";
        return 0;
    }
    return -1;
}

int IntentionStateMachine::pickInitialThenLoopIntent(bool commit) const
{
    Q_UNUSED(commit);

    if (m_turnCount == 0) {
        return m_data.behavior.initialIntentIndex;
    }

    const QVector<int> &loopSequence = m_data.behavior.loopSequence;
    if (loopSequence.isEmpty()) {
        return m_data.behavior.initialIntentIndex;
    }

    const int loopPos = (m_turnCount - 1) % loopSequence.size();
    return loopSequence.at(loopPos);
}

int IntentionStateMachine::pickInitialThenProbabilityIntent(bool commit) const
{
    if (m_turnCount == 0) {
        return m_data.behavior.initialIntentIndex;
    }
    return pickProbabilityIntent(commit);
}

int IntentionStateMachine::pickInitialThenSequenceIntent(bool commit) const
{
    if (m_turnCount == 0) {
        return m_data.behavior.initialIntentIndex;
    }

    const QVector<int> &loopSequence = m_data.behavior.loopSequence;
    if (loopSequence.isEmpty()) {
        return pickProbabilityIntent(commit);
    }

    const int loopPos = (m_turnCount - 1) % loopSequence.size();
    return loopSequence.at(loopPos);
}

int IntentionStateMachine::pickPhasedSequenceIntent(bool commit) const
{
    Q_UNUSED(commit);

    if (m_turnCount < m_data.behavior.sequence.size()) {
        return m_data.behavior.sequence.at(m_turnCount);
    }

    const QVector<int> &cycle = m_data.behavior.loopSequence;
    if (cycle.isEmpty()) {
        return m_data.behavior.sequence.isEmpty() ? 0 : m_data.behavior.sequence.last();
    }

    const int cyclePos = (m_turnCount - m_data.behavior.sequence.size()) % cycle.size();
    return cycle.at(cyclePos);
}

int IntentionStateMachine::pickSequenceIntent(bool commit) const
{
    if (m_data.behavior.sequence.isEmpty()) {
        return m_data.intents.isEmpty() ? -1 : 0;
    }

    const int seqLen = m_data.behavior.sequence.size();
    const int pos = m_sequenceIndex % seqLen;
    const int intentIndex = m_data.behavior.sequence.at(pos);

    if (commit) {
        ++m_sequenceIndex;
        if (!m_data.behavior.loop && m_sequenceIndex >= seqLen) {
            m_sequenceIndex = seqLen - 1;
        }
    }

    return intentIndex;
}

int IntentionStateMachine::pickLoopIntent() const
{
    return m_data.behavior.loopIntentIndex;
}

int IntentionStateMachine::pickProbabilityIntent(bool commit) const
{
    if (!commit) {
        if (m_previewProbabilityPick < 0
            || violatesRestrictions(m_previewProbabilityPick, true, true)) {
            m_previewProbabilityPick = rollProbabilityIntent();
        }
        return m_previewProbabilityPick;
    }

    if (m_previewProbabilityPick >= 0) {
        const int picked = m_previewProbabilityPick;
        if (!violatesRestrictions(picked, true, true)) {
            m_previewProbabilityPick = -1;
            return picked;
        }

        qWarning() << "IntentionStateMachine: preview pick" << picked
                   << "violates restrictions for enemy" << m_data.id << "- rerolling";
        m_previewProbabilityPick = -1;
    }

    return rollProbabilityIntent();
}

int IntentionStateMachine::rollProbabilityIntent() const
{
    if (m_data.behavior.probability.isEmpty() && m_data.behavior.firstTurnProbability.isEmpty()) {
        return m_data.intents.isEmpty() ? -1 : 0;
    }

    QVector<int> candidates = buildProbabilityCandidates(false);
    if (candidates.isEmpty()) {
        candidates = buildProbabilityCandidates(true);
    }
    if (candidates.isEmpty()) {
        qWarning() << "IntentionStateMachine: no valid probability intents for enemy" << m_data.id
                   << "- falling back to intent 0";
        return m_data.intents.isEmpty() ? -1 : 0;
    }

    double totalWeight = 0.0;
    for (int index : candidates) {
        totalWeight += weightForIntent(index);
    }
    if (totalWeight <= 0.0) {
        return candidates.first();
    }

    double roll = RNG::get(0, 10000) / 10000.0 * totalWeight;
    for (int index : candidates) {
        roll -= weightForIntent(index);
        if (roll <= 0.0) {
            return index;
        }
    }

    return candidates.last();
}

double IntentionStateMachine::weightForIntent(int intentIndex) const
{
    if (m_data.behavior.probability.contains(intentIndex)) {
        return m_data.behavior.probability.value(intentIndex);
    }
    return m_data.behavior.firstTurnProbability.value(intentIndex, 0.0);
}

QVector<int> IntentionStateMachine::buildProbabilityCandidates(bool relaxTwoConsecutive) const
{
    QVector<int> candidates;

    const auto consider = [&](int index) {
        const bool hasWeight = m_data.behavior.probability.contains(index)
                               || m_data.behavior.firstTurnProbability.contains(index);
        if (!hasWeight) {
            return;
        }
        if (violatesRestrictions(index, !relaxTwoConsecutive, true)) {
            return;
        }
        candidates.append(index);
    };

    if (!m_data.behavior.chooseFrom.isEmpty()) {
        for (int index : m_data.behavior.chooseFrom) {
            consider(index);
        }
    } else if (!m_data.behavior.probability.isEmpty()) {
        for (auto it = m_data.behavior.probability.constBegin();
             it != m_data.behavior.probability.constEnd();
             ++it) {
            consider(it.key());
        }
    } else {
        for (auto it = m_data.behavior.firstTurnProbability.constBegin();
             it != m_data.behavior.firstTurnProbability.constEnd();
             ++it) {
            consider(it.key());
        }
    }

    return candidates;
}

int IntentionStateMachine::pickAlternatingIntent(bool commit) const
{
    const int a = m_data.behavior.alternatingA;
    const int b = m_data.behavior.alternatingB;

    if (m_turnCount == 0 && m_data.behavior.randomFirst) {
        if (!commit) {
            if (m_previewTurnZeroPick < 0) {
                m_previewTurnZeroPick = RNG::get(0, 1) == 0 ? a : b;
            }
            return m_previewTurnZeroPick;
        }

        if (m_previewTurnZeroPick >= 0) {
            m_alternatingNext = (m_previewTurnZeroPick == a) ? b : a;
            const int chosen = m_previewTurnZeroPick;
            m_previewTurnZeroPick = -1;
            return chosen;
        }

        const int chosen = RNG::get(0, 1) == 0 ? a : b;
        m_alternatingNext = (chosen == a) ? b : a;
        return chosen;
    }

    if (m_alternatingNext < 0) {
        m_alternatingNext = a;
    }

    const int chosen = m_alternatingNext;
    if (commit) {
        m_alternatingNext = (chosen == a) ? b : a;
    }

    return chosen;
}

int IntentionStateMachine::pickFungiRulesIntent(bool commit) const
{
    if (m_turnCount == 0) {
        return rollProbabilityIntent();
    }

    if (m_forceGrowthNext) {
        if (commit) {
            m_forceGrowthNext = false;
        }
        return 1;
    }

    if (m_lastIntentIndex == 1) {
        if (commit) {
            m_forceGrowthNext = false;
        }
        return 0;
    }

    if (m_consecutiveAttacks >= 2) {
        if (commit) {
            m_consecutiveAttacks = 0;
        }
        return 1;
    }

    return pickProbabilityIntent(commit);
}

bool IntentionStateMachine::isGuardianDefensiveMode() const
{
    return m_guardianDefensiveMode;
}

int IntentionStateMachine::guardianModeShiftThreshold() const
{
    return m_guardianCurrentThreshold;
}

bool IntentionStateMachine::recordGuardianDamage(int damage)
{
    if (m_data.behavior.type != QStringLiteral("guardian_mode") || m_guardianDefensiveMode || damage <= 0) {
        return false;
    }

    m_guardianDamageAccumulator += damage;
    return m_guardianDamageAccumulator >= m_guardianCurrentThreshold;
}

void IntentionStateMachine::guardianEnterDefensiveMode(Enemy *owner)
{
    if (owner == nullptr || m_guardianDefensiveMode) {
        return;
    }

    m_guardianDefensiveMode = true;
    m_guardianSequenceIndex = 0;
    m_guardianDamageAccumulator = 0;
    m_guardianResumeAttackIndex = -1;
    m_previewProbabilityPick = -1;

    owner->removePower(QStringLiteral("mode_shift"));
    // Sharp Hide is applied only by the Strengthen intent (first Defensive Mode action).
}

void IntentionStateMachine::guardianExitDefensiveMode(Enemy *owner)
{
    if (owner == nullptr || !m_guardianDefensiveMode) {
        return;
    }

    m_guardianDefensiveMode = false;
    m_guardianSequenceIndex = 0;
    m_guardianDamageAccumulator = 0;
    m_guardianCurrentThreshold += m_data.behavior.thresholdIncreasePerSwitch;
    m_guardianResumeAttackIndex = 3;

    owner->removePower(QStringLiteral("sharp_hide"));
    owner->addPower(
        PowerFactory::createPower(QStringLiteral("mode_shift"), m_guardianCurrentThreshold, -1));
}

int IntentionStateMachine::pickGuardianModeIntent(bool commit) const
{
    if (m_guardianDefensiveMode) {
        const QVector<int> &sequence = m_data.behavior.defenseModeSequence;
        if (sequence.isEmpty()) {
            return 0;
        }

        const int position = m_guardianSequenceIndex % sequence.size();
        const int intentIndex = sequence.at(position);
        if (commit) {
            ++m_guardianSequenceIndex;
        }
        return intentIndex;
    }

    const QVector<int> &sequence = m_data.behavior.attackModeSequence;
    if (sequence.isEmpty()) {
        return 0;
    }

    if (m_guardianResumeAttackIndex >= 0) {
        const int resumePos = qBound(0, m_guardianResumeAttackIndex, sequence.size() - 1);
        const int intentIndex = sequence.at(resumePos);
        if (commit) {
            m_guardianSequenceIndex = resumePos + 1;
            m_guardianResumeAttackIndex = -1;
        }
        return intentIndex;
    }

    const int position = m_guardianSequenceIndex % sequence.size();
    const int intentIndex = sequence.at(position);
    if (commit) {
        ++m_guardianSequenceIndex;
    }
    return intentIndex;
}

int IntentionStateMachine::pickConditionalIntent() const
{
    if (m_data.behavior.conditionType == QStringLiteral("if_other_enemies_alive")) {
        const QVector<Enemy *> *peers =
            m_owner != nullptr ? m_owner->encounterPeers() : nullptr;
        if (otherEnemiesAlive(m_owner, peers)) {
            return m_data.behavior.alternatingA;
        }
        return m_data.behavior.alternatingB;
    }

    qWarning() << "IntentionStateMachine: unsupported condition" << m_data.behavior.conditionType
               << "for enemy" << m_data.id;
    return m_data.behavior.alternatingB;
}

void IntentionStateMachine::checkSplitTrigger() const
{
    if (m_owner == nullptr || m_data.behavior.splitIntentIndex < 0 || m_splitTriggerArmed
        || m_splitUsed) {
        return;
    }

    if (m_owner->maxHp() <= 0) {
        return;
    }

    if (m_owner->currentHp() * 2 <= m_owner->maxHp()) {
        m_splitTriggerArmed = true;
    }
}

bool IntentionStateMachine::violatesRestrictions(int intentIndex, bool enforceTwoConsecutive,
                                                 bool enforceThreeConsecutive) const
{
    for (const QString &restriction : m_data.behavior.restrictions) {
        if (enforceTwoConsecutive && restriction.startsWith(QStringLiteral("cannot_use_intent_"))
            && restriction.endsWith(QStringLiteral("_two_consecutive"))) {
            const QString middle = restriction.mid(QStringLiteral("cannot_use_intent_").size(),
                                                   restriction.size()
                                                       - QStringLiteral("cannot_use_intent_").size()
                                                       - QStringLiteral("_two_consecutive").size());
            bool ok = false;
            const int restrictedIndex = middle.toInt(&ok);
            if (ok && restrictedIndex == intentIndex && m_lastIntentIndex == intentIndex) {
                return true;
            }
        }

        if (enforceThreeConsecutive && restriction.startsWith(QStringLiteral("cannot_use_intent_"))
            && restriction.endsWith(QStringLiteral("_three_consecutive"))) {
            const QString middle = restriction.mid(QStringLiteral("cannot_use_intent_").size(),
                                                   restriction.size()
                                                       - QStringLiteral("cannot_use_intent_").size()
                                                       - QStringLiteral("_three_consecutive").size());
            bool ok = false;
            const int restrictedIndex = middle.toInt(&ok);
            if (ok && restrictedIndex == intentIndex && m_history.size() >= 2) {
                const int prev = m_history.at(m_history.size() - 1);
                const int prev2 = m_history.at(m_history.size() - 2);
                if (prev == restrictedIndex && prev2 == restrictedIndex) {
                    return true;
                }
            }
        }

        if (enforceThreeConsecutive
            && restriction == QStringLiteral("cannot_same_intent_three_consecutive")) {
            if (m_history.size() >= 2) {
                const int prev = m_history.at(m_history.size() - 1);
                const int prev2 = m_history.at(m_history.size() - 2);
                if (prev == intentIndex && prev2 == intentIndex) {
                    return true;
                }
            }
        }

        if (enforceThreeConsecutive
            && restriction == QStringLiteral("cannot_same_intent_three_consecutive_for_0_and_2")) {
            if ((intentIndex == 0 || intentIndex == 2) && m_history.size() >= 2) {
                const int prev = m_history.at(m_history.size() - 1);
                const int prev2 = m_history.at(m_history.size() - 2);
                if (prev == intentIndex && prev2 == intentIndex) {
                    return true;
                }
            }
        }

        if (enforceTwoConsecutive && restriction == QStringLiteral("cannot_attack_three_consecutive")
            && intentIndex == 0 && m_history.size() >= 2) {
            const int prev = m_history.at(m_history.size() - 1);
            const int prev2 = m_history.at(m_history.size() - 2);
            if (prev == 0 && prev2 == 0) {
                return true;
            }
        }

        if (enforceTwoConsecutive && restriction == QStringLiteral("cannot_growth_two_consecutive")
            && intentIndex == 1 && m_lastIntentIndex == 1) {
            return true;
        }
    }
    return false;
}
