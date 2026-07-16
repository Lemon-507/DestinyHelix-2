#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>

#include <memory>

class Actor;
class BattleFSM;
class Power;
struct EffectDef;

class ICommand
{
public:
    virtual ~ICommand() = default;
    virtual void execute() = 0;
};

class DamageCommand : public ICommand
{
public:
    DamageCommand(Actor *source, Actor *target, int baseDamage);
    void execute() override;

private:
    Actor *m_source;
    Actor *m_target;
    int m_baseDamage;
};

class GainBlockCommand : public ICommand
{
public:
    GainBlockCommand(Actor *target, int amount);
    void execute() override;

private:
    Actor *m_target;
    int m_amount;
};

class HealCommand : public ICommand
{
public:
    HealCommand(Actor *target, int amount);
    void execute() override;

private:
    Actor *m_target;
    int m_amount;
};

class ApplyPowerCommand : public ICommand
{
public:
    ApplyPowerCommand(Actor *target, std::unique_ptr<Power> power);
    void execute() override;

private:
    Actor *m_target;
    std::unique_ptr<Power> m_power;
};

class EffectResolver
{
public:
    static EffectResolver &instance();

    void execute(const QVector<EffectDef> &effects, Actor *caster, Actor *target,
                 const QVector<Actor *> &allEnemies = {}, BattleFSM *battleFsm = nullptr,
                 const QString &sourceCardId = QString());

private:
    EffectResolver() = default;
};
