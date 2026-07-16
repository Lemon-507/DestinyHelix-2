#include "status_powers.h"

#include "battle/logic/power_factory.h"

std::unique_ptr<Power> createStatusPower(const QString &type, Actor *owner, int amount)
{
    auto power = PowerFactory::createPower(type, amount, -1);
    if (power && owner != nullptr) {
        power->bindOwner(owner);
    }
    return power;
}
