#pragma once

#include <memory>

#include "power.h"

class Actor;

std::unique_ptr<Power> createStatusPower(const QString &type, Actor *owner, int amount);
