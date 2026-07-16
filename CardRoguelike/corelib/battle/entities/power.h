#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

class Actor;

class Power : public QObject
{
    Q_OBJECT

public:
    explicit Power(Actor *owner, const QString &id, int value = 1, int duration = -1, QObject *parent = nullptr);
    ~Power() override = default;

    Actor *owner() const;
    QString powerId() const;
    int value() const;
    int duration() const;

    void addValue(int amount);
    void addDuration(int amount);
    bool isExpired() const;
    void bindOwner(Actor *owner);

    virtual void onTurnStart();
    virtual void onTurnEnd();
    virtual void onDamageReceived(int damage, Actor *source);
    virtual void onPlayerPlayedAttack(class Player *player);
    virtual void onOwnerBound();
    virtual QString name() const;
    virtual bool isDispellable() const;
    virtual int modifyOutgoingDamage(int damage) const;
    virtual int modifyIncomingDamage(int damage) const;
    virtual int modifyBlockGained(int block) const;

protected:
    void tickDuration();

    int m_value = 1;

private:
    Actor *m_owner = nullptr;
    QString m_id;
    int m_duration = -1;
};
