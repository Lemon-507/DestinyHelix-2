#include "rng.h"

#include <QtCore/QDateTime>
#include <QtCore/QtGlobal>
#include <random>

static std::mt19937 s_engine;

int RNG::nonDeterministicSeed()
{
    std::random_device device;
    const qint64 timePart = QDateTime::currentMSecsSinceEpoch();
    const quint32 devicePart = device();
    return static_cast<int>((timePart ^ (static_cast<qint64>(devicePart) << 16)) & 0x7FFFFFFF);
}

void RNG::seed(int value)
{
    s_engine.seed(static_cast<std::mt19937::result_type>(value));
}

int RNG::get(int min, int max)
{
    if (min >= max) {
        return min;
    }

    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(s_engine);
}

double RNG::get(double min, double max)
{
    if (min >= max) {
        return min;
    }

    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(s_engine);
}

std::mt19937 RNG::snapshot()
{
    return s_engine;
}

void RNG::restore(const std::mt19937 &engine)
{
    s_engine = engine;
}
