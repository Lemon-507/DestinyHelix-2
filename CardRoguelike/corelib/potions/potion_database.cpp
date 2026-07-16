#include "potion_database.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "battle/entities/actor.h"
#include "battle/entities/player.h"
#include "battle/logic/effectresolver.h"

PotionDatabase &PotionDatabase::instance()
{
    static PotionDatabase database;
    return database;
}

void PotionDatabase::load()
{
    if (m_loaded) {
        return;
    }

    QFile file(QStringLiteral(":/resources/data/potions.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    const QJsonArray potions = document.isArray()
        ? document.array()
        : document.object().value(QStringLiteral("potions")).toArray();
    for (const QJsonValue &value : potions) {
        const QJsonObject obj = value.toObject();
        PotionData potion;
        potion.id = obj.value(QStringLiteral("id")).toString();
        potion.name = obj.value(QStringLiteral("name")).toString();
        potion.description = obj.value(QStringLiteral("description")).toString();

        const QJsonArray contexts = obj.value(QStringLiteral("allowed_contexts")).toArray();
        for (const QJsonValue &contextValue : contexts) {
            potion.allowedContexts.append(contextValue.toString());
        }

        const QJsonArray effects = obj.value(QStringLiteral("effects")).toArray();
        for (const QJsonValue &effectValue : effects) {
            const QJsonObject effectObj = effectValue.toObject();
            EffectDef effect;
            effect.type = effectObj.value(QStringLiteral("type")).toString();
            effect.value = effectObj.value(QStringLiteral("value")).toInt();
            effect.target = effectObj.value(QStringLiteral("target")).toString();
            effect.powerId = effectObj.value(QStringLiteral("power_id")).toString();
            if (effect.powerId.isEmpty()) {
                effect.powerId = effectObj.value(QStringLiteral("power")).toString();
            }
            potion.effects.append(effect);
        }

        m_potions.append(potion);
    }

    m_loaded = true;
}

PotionData PotionDatabase::getPotion(const QString &id) const
{
    for (const PotionData &potion : m_potions) {
        if (potion.id == id) {
            return potion;
        }
    }
    return PotionData();
}

QStringList PotionDatabase::allPotionIds() const
{
    QStringList ids;
    ids.reserve(m_potions.size());
    for (const PotionData &potion : m_potions) {
        if (!potion.id.isEmpty()) {
            ids.append(potion.id);
        }
    }
    return ids;
}

bool PotionDatabase::canUse(const PotionData &potion, GameContext context) const
{
    const QString contextString = contextToString(context);
    for (const QString &allowed : potion.allowedContexts) {
        if (allowed == contextString) {
            return true;
        }
    }
    return false;
}

void PotionDatabase::usePotion(const PotionData &potion, Player *player, Actor *target,
                               const QVector<Actor *> &allEnemies, BattleFSM *battleFsm)
{
    if (player == nullptr) {
        return;
    }

    Actor *effectTarget = target != nullptr ? target : player;
    EffectResolver::instance().execute(potion.effects, player, effectTarget, allEnemies, battleFsm);
}

QString PotionDatabase::contextToString(GameContext context)
{
    switch (context) {
    case GameContext::NonCombat:
        return QStringLiteral("non_combat");
    case GameContext::CombatPlayerTurn:
        return QStringLiteral("combat_player_turn");
    case GameContext::CombatAny:
        return QStringLiteral("combat_any");
    }

    return QStringLiteral("non_combat");
}
