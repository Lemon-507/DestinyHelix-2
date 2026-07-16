#pragma once

#include <QtCore/QMap>
#include <QtCore/QString>
#include <memory>

class Actor;
class Power;

struct BuffDefinition {
    QString id;
    QString name;
    QString effect;
    QString stacking;
    int valueDefault = 1;
    int durationDefault = 1;
};

class PowerFactory
{
public:
    static PowerFactory &instance();

    void load();
    const BuffDefinition *definition(const QString &id) const;
    static std::unique_ptr<Power> createPower(const QString &id, int value, int duration = -1);

private:
    PowerFactory() = default;

    QMap<QString, BuffDefinition> m_definitions;
    bool m_loaded = false;
};

