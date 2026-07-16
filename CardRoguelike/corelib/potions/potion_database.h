#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include "battle/entities/card.h"

class Actor;
class BattleFSM;
class Player;

enum class GameContext {
    NonCombat,
    CombatPlayerTurn,
    CombatAny
};

struct PotionData {
    QString id;
    QString name;
    QString description;
    QVector<QString> allowedContexts;
    QVector<EffectDef> effects;
};

class PotionDatabase
{
public:
    static PotionDatabase &instance();

    void load();
    PotionData getPotion(const QString &id) const;
    QStringList allPotionIds() const;
    bool canUse(const PotionData &potion, GameContext context) const;
    void usePotion(const PotionData &potion, Player *player, Actor *target = nullptr,
                   const QVector<Actor *> &allEnemies = {}, BattleFSM *battleFsm = nullptr);

private:
    PotionDatabase() = default;
    static QString contextToString(GameContext context);

    QVector<PotionData> m_potions;
    bool m_loaded = false;
};
