#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>

#include "battle/entities/relic.h"

class RelicDatabase
{
public:
    static RelicDatabase &instance();

    void load();
    RelicData getRelic(const QString &id) const;
    QStringList allRelicIds() const;
    QString randomRelicIdExcluding(const QStringList &ownedIds) const;

private:
    RelicDatabase() = default;

    QVector<RelicData> m_relics;
    bool m_loaded = false;
};
