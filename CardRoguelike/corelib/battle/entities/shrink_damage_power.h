#pragma once

#include "power.h"

class ShrinkDamagePower : public Power
{
    Q_OBJECT

public:
    ShrinkDamagePower(Actor *owner, int initialDamage, QObject *parent = nullptr);

    void setDamage(int newDamage);
    int damagePerTurn() const;

    void onTurnStart() override;
    QString name() const override;
    bool isDispellable() const override;

private:
    int m_damagePerTurn = 0;
};
