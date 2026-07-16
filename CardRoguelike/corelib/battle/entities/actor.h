#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <memory>
#include <vector>

#include "power.h"

struct ActorStats {
    int hp = 0;
    int maxHp = 0;
    int block = 0;
    int strength = 0;
    int dexterity = 0;
    std::vector<std::unique_ptr<Power>> powers;
};

class Actor : public QObject
{
    Q_OBJECT

public:
    explicit Actor(QObject *parent = nullptr);
    ~Actor() override = default;

    int currentHp() const;
    int currentBlock() const;
    int maxHp() const;
    int strength() const;
    int dexterity() const;

    void setStrength(int value);

    void gainBlock(int amount);
    void addBlockRaw(int amount);
    void clearBlock();
    void takeDamage(int damage, Actor *source = nullptr);
    void takeDirectDamage(int damage);
    void setMaxHp(int value);
    void healToFull();
    void heal(int amount);
    void addPower(std::unique_ptr<Power> power);
    void removePower(const QString &id);

    int getPowerValue(const QString &id) const;
    int getPowerDuration(const QString &id) const;
    bool hasPower(const QString &id) const;

    int calculateOutgoingDamage(int baseDamage) const;
    int calculateIncomingDamage(int baseDamage, Actor *source = nullptr) const;
    int calculateBlockGained(int baseBlock) const;
    int applyDexterityToBlock(int block) const;

    virtual void beginTurn();
    virtual void endTurn();
    void notifyPlayerPlayedAttack(class Player *player);

signals:
    void hpChanged(int hp);
    void blockChanged(int block);
    void died();

protected:
    void notifyPowersDamageReceived(int damage, Actor *source);
    void removeExpiredPowers();

    ActorStats m_stats;
};
