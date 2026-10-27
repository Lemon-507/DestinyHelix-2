#include "enemy_database.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>

#include "utils/json_utils.h"
#include "utils/rng.h"

namespace {

struct FlexibleInt {
    int value = 0;
    bool isRange = false;
    int min = 0;
    int max = 0;
    bool valid = true;
};

FlexibleInt parseFlexibleInt(const QJsonValue &value, const QString &context)
{
    FlexibleInt result;
    if (value.isUndefined() || value.isNull()) {
        return result;
    }

    if (value.isDouble() || value.isBool()) {
        result.value = value.toInt();
        return result;
    }

    if (value.isObject()) {
        const QJsonObject obj = value.toObject();
        if (obj.contains(QStringLiteral("min")) && obj.contains(QStringLiteral("max"))) {
            result.isRange = true;
            result.min = obj.value(QStringLiteral("min")).toInt();
            result.max = obj.value(QStringLiteral("max")).toInt();
            if (result.min > result.max) {
                qWarning() << "EnemyDatabase: invalid range in" << context << result.min << result.max;
                result.valid = false;
            }
            return result;
        }
        qWarning() << "EnemyDatabase: malformed object value in" << context;
        result.valid = false;
        return result;
    }

    if (value.isString()) {
        const QString text = value.toString().trimmed();
        static const QRegularExpression rangePattern(
            QStringLiteral("^random_(\\d+)_to_(\\d+)$"), QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch match = rangePattern.match(text);
        if (match.hasMatch()) {
            result.isRange = true;
            result.min = match.captured(1).toInt();
            result.max = match.captured(2).toInt();
            if (result.min > result.max) {
                qWarning() << "EnemyDatabase: invalid range string in" << context << text;
                result.valid = false;
            }
            return result;
        }

        bool ok = false;
        const int parsed = text.toInt(&ok);
        if (ok) {
            result.value = parsed;
            return result;
        }

        qWarning() << "EnemyDatabase: unparsable numeric string in" << context << text;
        result.valid = false;
        return result;
    }

    qWarning() << "EnemyDatabase: unsupported JSON value type in" << context;
    result.valid = false;
    return result;
}

void assignFlexibleInt(const FlexibleInt &parsed, int &value, bool &rollFlag, int &minOut, int &maxOut)
{
    if (parsed.isRange) {
        rollFlag = true;
        minOut = parsed.min;
        maxOut = parsed.max;
        value = 0;
        return;
    }
    rollFlag = false;
    value = parsed.value;
}

QString intentNameFromObject(const QJsonObject &obj)
{
    const QString explicitName = obj.value(QStringLiteral("name")).toString();
    if (!explicitName.isEmpty()) {
        return explicitName;
    }

    QString description = obj.value(QStringLiteral("description")).toString();
    const int colonIndex = description.indexOf(QLatin1Char(':'));
    if (colonIndex > 0) {
        description = description.left(colonIndex).trimmed();
    }
    return description;
}

void parseProbabilityWeights(const QJsonObject &obj, QMap<int, double> &outWeights)
{
    const QJsonObject nested = obj.value(QStringLiteral("probability")).toObject();
    const QJsonObject &source = nested.isEmpty() ? obj : nested;

    for (auto it = source.constBegin(); it != source.constEnd(); ++it) {
        bool ok = false;
        const int intentIndex = it.key().toInt(&ok);
        if (ok && it.value().isDouble()) {
            outWeights.insert(intentIndex, it.value().toDouble());
        }
    }
}

void parseProbabilityObject(const QJsonObject &obj, BehaviorDef &behavior)
{
    if (obj.contains(QStringLiteral("choose_from"))) {
        const QJsonArray chooseFrom = obj.value(QStringLiteral("choose_from")).toArray();
        for (const QJsonValue &value : chooseFrom) {
            behavior.chooseFrom.append(value.toInt());
        }
    }

    parseProbabilityWeights(obj, behavior.probability);
}

void parseFirstTurnProbabilityObject(const QJsonObject &obj, BehaviorDef &behavior)
{
    parseProbabilityWeights(obj, behavior.firstTurnProbability);
    for (auto it = behavior.firstTurnProbability.constBegin();
         it != behavior.firstTurnProbability.constEnd();
         ++it) {
        behavior.probability.insert(it.key(), it.value());
    }
}

void parseSplitOverride(const QJsonValue &value, BehaviorDef &behavior)
{
    if (!value.isString()) {
        return;
    }

    static const QRegularExpression pattern(
        QStringLiteral("^when_hp_half_force_intent_(\\d+)_next_turn$"));
    const QRegularExpressionMatch match = pattern.match(value.toString());
    if (match.hasMatch()) {
        behavior.splitIntentIndex = match.captured(1).toInt();
        return;
    }

    qWarning() << "EnemyDatabase: unknown split_override" << value.toString();
}

void parseRulesArray(const QJsonValue &value, BehaviorDef &behavior)
{
    const QJsonArray rules = value.toArray();
    for (const QJsonValue &entry : rules) {
        const QString rule = entry.toString();
        behavior.rules.append(rule);
        if (rule.startsWith(QStringLiteral("cannot_"))) {
            behavior.restrictions.append(rule);
        }
    }
}

void parseRestrictions(const QJsonValue &value, BehaviorDef &behavior)
{
    if (value.isString()) {
        behavior.restrictions.append(value.toString());
        return;
    }

    const QJsonArray restrictions = value.toArray();
    for (const QJsonValue &entry : restrictions) {
        behavior.restrictions.append(entry.toString());
    }
}

} // namespace

EnemyDatabase &EnemyDatabase::instance()
{
    static EnemyDatabase database;
    return database;
}

void EnemyDatabase::load()
{
    if (m_loaded) {
        return;
    }

    Q_INIT_RESOURCE(resources);

    QFile file(QStringLiteral(":/resources/data/enemies.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QJsonArray enemies;
    if (document.isArray()) {
        enemies = document.array();
    } else {
        enemies = document.object().value(QStringLiteral("enemies")).toArray();
    }

    for (const QJsonValue &value : enemies) {
        parseEnemyObject(value.toObject());
    }

    m_loaded = true;
}

const EnemyData *EnemyDatabase::getEnemy(const QString &id) const
{
    for (const EnemyData &enemy : m_enemies) {
        if (enemy.id == id) {
            return &enemy;
        }
    }
    return nullptr;
}

Enemy *EnemyDatabase::createEnemy(const QString &id, QObject *parent) const
{
    const EnemyData *enemyData = getEnemy(id);
    if (enemyData == nullptr) {
        return nullptr;
    }

    auto *enemy = new Enemy(parent);
    enemy->loadFromData(*enemyData);
    return enemy;
}

QString EnemyDatabase::randomEnemyIdForType(const QString &type) const
{
    const QVector<const EnemyData *> matches = enemiesOfType(type);
    if (matches.isEmpty()) {
        return QString();
    }

    const int index = RNG::get(0, matches.size() - 1);
    return matches.at(index)->id;
}

QVector<const EnemyData *> EnemyDatabase::enemiesOfType(const QString &type) const
{
    QVector<const EnemyData *> matches;
    for (const EnemyData &enemy : m_enemies) {
        if (enemy.type == type) {
            matches.append(&enemy);
        }
    }
    return matches;
}

EnemyIntentDef EnemyDatabase::parseIntent(const QJsonObject &obj)
{
    EnemyIntentDef intent;
    intent.type = obj.value(QStringLiteral("type")).toString();
    intent.name = intentNameFromObject(obj);
    if (intent.type.isEmpty()) {
        qWarning() << "EnemyDatabase: intent missing type for" << intent.name;
    }

    QJsonValue baseValueJson = obj.value(QStringLiteral("base_value"));
    if (baseValueJson.isUndefined() || baseValueJson.isNull()) {
        baseValueJson = obj.value(QStringLiteral("damage"));
    }
    if (!baseValueJson.isUndefined() && !baseValueJson.isNull()) {
        assignFlexibleInt(parseFlexibleInt(baseValueJson, QStringLiteral("base_value")), intent.baseValue,
                          intent.rollBaseValue, intent.baseValueMin, intent.baseValueMax);
    }

    if (obj.contains(QStringLiteral("block_value"))) {
        assignFlexibleInt(parseFlexibleInt(obj.value(QStringLiteral("block_value")),
                                           QStringLiteral("block_value")),
                          intent.blockValue, intent.rollBlockValue, intent.blockValueMin,
                          intent.blockValueMax);
    }

    intent.buffType = obj.value(QStringLiteral("buff_type")).toString();
    intent.debuffType = obj.value(QStringLiteral("debuff_type")).toString();
    intent.cardId = obj.value(QStringLiteral("card_id")).toString();
    if (intent.cardId.isEmpty()) {
        intent.cardId = obj.value(QStringLiteral("card")).toString();
    }
    intent.cardCount = obj.value(QStringLiteral("card_count")).toInt();
    if (intent.cardCount == 0) {
        intent.cardCount = obj.value(QStringLiteral("count")).toInt();
    }
    intent.times = obj.value(QStringLiteral("times")).toInt(1);
    intent.chance = obj.value(QStringLiteral("chance")).toDouble(1.0);

    const QJsonArray intoArray = obj.value(QStringLiteral("into")).toArray();
    for (const QJsonValue &value : intoArray) {
        const QString childId = value.toString();
        if (!childId.isEmpty()) {
            intent.splitInto.append(childId);
        }
    }

    if (obj.contains(QStringLiteral("buff_value"))) {
        assignFlexibleInt(parseFlexibleInt(obj.value(QStringLiteral("buff_value")),
                                           QStringLiteral("buff_value")),
                          intent.buffValue, intent.rollBuffValue, intent.buffValueMin,
                          intent.buffValueMax);
    }
    if (obj.contains(QStringLiteral("debuff_value"))) {
        assignFlexibleInt(parseFlexibleInt(obj.value(QStringLiteral("debuff_value")),
                                           QStringLiteral("debuff_value")),
                          intent.debuffValue, intent.rollDebuffValue, intent.debuffValueMin,
                          intent.debuffValueMax);
    }

    const QJsonValue genericValueJson = obj.value(QStringLiteral("value"));
    if (!genericValueJson.isUndefined() && !genericValueJson.isNull()) {
        const FlexibleInt generic = parseFlexibleInt(genericValueJson, QStringLiteral("value"));
        if (intent.buffValue == 0 && !intent.rollBuffValue
            && (intent.type.contains(QStringLiteral("buff")) || !intent.buffType.isEmpty())) {
            assignFlexibleInt(generic, intent.buffValue, intent.rollBuffValue, intent.buffValueMin,
                              intent.buffValueMax);
        }
        if (intent.debuffValue == 0 && !intent.rollDebuffValue
            && (intent.type.contains(QStringLiteral("debuff")) || !intent.debuffType.isEmpty())) {
            assignFlexibleInt(generic, intent.debuffValue, intent.rollDebuffValue, intent.debuffValueMin,
                              intent.debuffValueMax);
        }
    }

    return intent;
}

EnemyAbilityDef EnemyDatabase::parseAbility(const QJsonObject &obj)
{
    EnemyAbilityDef ability;
    ability.type = obj.value(QStringLiteral("type")).toString();
    ability.description = obj.value(QStringLiteral("description")).toString();
    if (ability.type.isEmpty()) {
        qWarning() << "EnemyDatabase: ability missing type";
    }
    if (obj.contains(QStringLiteral("value"))) {
        assignFlexibleInt(parseFlexibleInt(obj.value(QStringLiteral("value")), QStringLiteral("ability.value")),
                          ability.value, ability.rollValue, ability.valueMin, ability.valueMax);
    }
    return ability;
}

BehaviorDef EnemyDatabase::parseBehavior(const QJsonObject &obj)
{
    BehaviorDef behavior;

    if (const auto explicitType = jsonValue<QString>(obj, QStringLiteral("type"))) {
        behavior.type = *explicitType;

        if (behavior.type == QStringLiteral("sequence")
            || behavior.type == QStringLiteral("conditional_loop")) {
            const QJsonArray sequence = obj.value(QStringLiteral("sequence")).toArray();
            for (const QJsonValue &value : sequence) {
                behavior.sequence.append(value.toInt());
            }
            if (const auto loop = jsonValue<bool>(obj, QStringLiteral("loop"))) {
                behavior.loop = *loop;
            }
        } else if (behavior.type == QStringLiteral("loop")) {
            if (const auto loopIntent = jsonValue<int>(obj, QStringLiteral("loop_intent"))) {
                behavior.loopIntentIndex = *loopIntent;
            }
        } else if (behavior.type == QStringLiteral("probability")) {
            parseProbabilityObject(obj, behavior);
            parseRestrictions(obj.value(QStringLiteral("restrictions")), behavior);
        } else if (behavior.type == QStringLiteral("alternating")) {
            if (const auto altA = jsonValue<int>(obj, QStringLiteral("alternating_a"))) {
                behavior.alternatingA = *altA;
            }
            if (const auto altB = jsonValue<int>(obj, QStringLiteral("alternating_b"))) {
                behavior.alternatingB = *altB;
            }
            if (const auto randomFirst = jsonValue<bool>(obj, QStringLiteral("random_first"))) {
                behavior.randomFirst = *randomFirst;
            }
        }

        return behavior;
    }

    if (obj.contains(QStringLiteral("turn_sequence"))) {
        const QJsonObject turnSequence = obj.value(QStringLiteral("turn_sequence")).toObject();
        behavior.type = QStringLiteral("phased_sequence");
        behavior.sequence.append(turnSequence.value(QStringLiteral("1")).toInt());
        behavior.sequence.append(turnSequence.value(QStringLiteral("2")).toInt());

        const QJsonObject cycle = turnSequence.value(QStringLiteral("3_to_9")).toObject();
        const QJsonArray cycleArray = cycle.value(QStringLiteral("cycle")).toArray();
        for (const QJsonValue &value : cycleArray) {
            behavior.loopSequence.append(value.toInt());
        }
        return behavior;
    }

    if (obj.contains(QStringLiteral("attack_mode_sequence"))) {
        if (obj.contains(QStringLiteral("defense_mode_sequence"))) {
            behavior.type = QStringLiteral("guardian_mode");
            const QJsonArray attackSequence = obj.value(QStringLiteral("attack_mode_sequence")).toArray();
            for (const QJsonValue &value : attackSequence) {
                behavior.attackModeSequence.append(value.toInt());
            }
            const QJsonArray defenseSequence = obj.value(QStringLiteral("defense_mode_sequence")).toArray();
            for (const QJsonValue &value : defenseSequence) {
                behavior.defenseModeSequence.append(value.toInt());
            }
            behavior.modeSwitchDamageThreshold =
                obj.value(QStringLiteral("mode_switch_damage_threshold")).toInt(30);
            behavior.thresholdIncreasePerSwitch =
                obj.value(QStringLiteral("threshold_increase_per_switch")).toInt(10);
            behavior.firstAttackModeTurn = obj.value(QStringLiteral("first_attack_mode_turn")).toInt(0);
            behavior.firstDefenseModeTurn = obj.value(QStringLiteral("first_defense_mode_turn")).toInt(4);
            return behavior;
        }

        behavior.type = QStringLiteral("sequence");
        const QJsonArray sequence = obj.value(QStringLiteral("attack_mode_sequence")).toArray();
        for (const QJsonValue &value : sequence) {
            behavior.sequence.append(value.toInt());
        }
        behavior.loop = true;
        return behavior;
    }

    if (obj.contains(QStringLiteral("awake_sequence"))) {
        behavior.type = QStringLiteral("sequence");
        const QJsonArray sequence = obj.value(QStringLiteral("awake_sequence")).toArray();
        for (const QJsonValue &value : sequence) {
            behavior.sequence.append(value.toInt());
        }
        behavior.loop = obj.value(QStringLiteral("loop")).toBool(true);
        return behavior;
    }

    if (obj.contains(QStringLiteral("sequence")) && obj.value(QStringLiteral("sequence")).isArray()) {
        behavior.type = QStringLiteral("sequence");
        const QJsonArray sequence = obj.value(QStringLiteral("sequence")).toArray();
        for (const QJsonValue &value : sequence) {
            behavior.sequence.append(value.toInt());
        }
        behavior.loop = obj.value(QStringLiteral("loop")).toBool(true);
        parseSplitOverride(obj.value(QStringLiteral("split_override")), behavior);
        return behavior;
    }

    if (obj.contains(QStringLiteral("loop")) && obj.value(QStringLiteral("loop")).isArray()) {
        const QJsonArray loopArray = obj.value(QStringLiteral("loop")).toArray();
        if (obj.contains(QStringLiteral("initial_intent"))) {
            behavior.type = QStringLiteral("initial_then_loop");
            behavior.initialIntentIndex = obj.value(QStringLiteral("initial_intent")).toInt();
            for (const QJsonValue &value : loopArray) {
                behavior.loopSequence.append(value.toInt());
            }
            return behavior;
        }

        if (loopArray.size() == 1) {
            behavior.type = QStringLiteral("loop");
            behavior.loopIntentIndex = loopArray.at(0).toInt();
            return behavior;
        }
    }

    if (obj.contains(QStringLiteral("initial_intent")) && obj.contains(QStringLiteral("after"))) {
        behavior.type = QStringLiteral("initial_then_probability");
        behavior.initialIntentIndex = obj.value(QStringLiteral("initial_intent")).toInt();
        parseProbabilityObject(obj.value(QStringLiteral("after")).toObject(), behavior);
        return behavior;
    }

    if (obj.contains(QStringLiteral("initial_intent")) && obj.contains(QStringLiteral("probability_rule"))) {
        behavior.type = QStringLiteral("initial_then_probability");
        behavior.initialIntentIndex = obj.value(QStringLiteral("initial_intent")).toInt();
        const QJsonObject rule = obj.value(QStringLiteral("probability_rule")).toObject();
        parseProbabilityObject(rule.value(QStringLiteral("after_initial")).toObject(), behavior);
        parseRestrictions(obj.value(QStringLiteral("restrictions")), behavior);
        return behavior;
    }

    if (obj.contains(QStringLiteral("choose_from")) || obj.contains(QStringLiteral("probability"))) {
        behavior.type = QStringLiteral("probability");
        parseProbabilityObject(obj, behavior);
        parseRestrictions(obj.value(QStringLiteral("restrictions")), behavior);
        return behavior;
    }

    if (obj.contains(QStringLiteral("normal_cycle"))) {
        const QJsonObject cycle = obj.value(QStringLiteral("normal_cycle")).toObject();
        behavior.type = QStringLiteral("probability");
        parseProbabilityObject(cycle, behavior);
        parseRestrictions(cycle.value(QStringLiteral("restrictions")), behavior);
        parseSplitOverride(obj.value(QStringLiteral("split_override")), behavior);
        return behavior;
    }

    if (obj.contains(QStringLiteral("first_turn"))) {
        behavior.type = QStringLiteral("initial_then_sequence");
        behavior.initialIntentIndex = obj.value(QStringLiteral("first_turn")).toInt();
        const QJsonObject after = obj.value(QStringLiteral("after")).toObject();
        const QJsonObject pattern = after.value(QStringLiteral("pattern")).toObject();
        if (!pattern.isEmpty()
            && pattern.value(QStringLiteral("type")).toString()
                   == QStringLiteral("weighted_random_with_memory")) {
            qWarning() << "EnemyDatabase: weighted_random_with_memory uses default_sequence fallback";
        }
        const QJsonArray sequence = pattern.value(QStringLiteral("default_sequence")).toArray();
        for (const QJsonValue &value : sequence) {
            behavior.loopSequence.append(value.toInt());
        }
        if (behavior.loopSequence.isEmpty()) {
            behavior.loopSequence = {1, 1, 0};
        }
        return behavior;
    }

    if (obj.contains(QStringLiteral("first_turn_probability"))) {
        behavior.type = QStringLiteral("fungi_rules");
        parseFirstTurnProbabilityObject(obj.value(QStringLiteral("first_turn_probability")).toObject(),
                                        behavior);
        parseRulesArray(obj.value(QStringLiteral("rules")), behavior);
        return behavior;
    }

    if (obj.contains(QStringLiteral("conditional_loop"))
        && obj.value(QStringLiteral("conditional_loop")).isObject()) {
        const QJsonObject cond = obj.value(QStringLiteral("conditional_loop")).toObject();
        behavior.type = QStringLiteral("conditional_intent");
        behavior.conditionType = cond.value(QStringLiteral("condition")).toString();
        behavior.alternatingA = cond.value(QStringLiteral("then")).toInt(0);
        behavior.alternatingB = cond.value(QStringLiteral("else")).toInt(1);
        return behavior;
    }

    if (obj.contains(QStringLiteral("alternating"))) {
        behavior.type = QStringLiteral("alternating");
        const QJsonArray alternating = obj.value(QStringLiteral("alternating")).toArray();
        if (alternating.size() >= 2) {
            behavior.alternatingA = alternating.at(0).toInt();
            behavior.alternatingB = alternating.at(1).toInt();
        }
        if (obj.value(QStringLiteral("first_turn_random")).toBool(false)) {
            behavior.randomFirst = true;
        }
        return behavior;
    }

    qWarning() << "EnemyDatabase: unrecognized behavior object" << obj
               << "- defaulting to loop intent 0";
    behavior.type = QStringLiteral("loop");
    behavior.loopIntentIndex = 0;
    return behavior;
}

void EnemyDatabase::parseEnemyObject(const QJsonObject &obj)
{
    EnemyData enemy;
    enemy.id = obj.value(QStringLiteral("id")).toString();
    enemy.name = obj.value(QStringLiteral("name")).toString();
    enemy.type = obj.value(QStringLiteral("type")).toString();
    enemy.minHp = obj.value(QStringLiteral("min_hp")).toInt();
    enemy.maxHp = obj.value(QStringLiteral("max_hp")).toInt();

    const QJsonArray abilities = obj.value(QStringLiteral("abilities")).toArray();
    for (const QJsonValue &value : abilities) {
        enemy.abilities.append(parseAbility(value.toObject()));
    }

    const QJsonArray intents = obj.value(QStringLiteral("intents")).toArray();
    for (const QJsonValue &value : intents) {
        enemy.intents.append(parseIntent(value.toObject()));
    }

    enemy.behavior = parseBehavior(obj.value(QStringLiteral("behavior")).toObject());
    m_enemies.append(enemy);
}
