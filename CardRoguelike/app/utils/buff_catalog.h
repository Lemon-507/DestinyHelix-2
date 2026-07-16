#pragma once

#include <QtCore/QMap>
#include <QtCore/QString>

struct BuffDefinitionEntry {
    QString id;
    QString name;
    QString description;
};

class BuffCatalog
{
public:
    static BuffCatalog &instance();

    void load();
    QString displayName(const QString &id, const QString &fallbackName = QString()) const;
    QString formatDescription(const QString &id, int value) const;
    QString tooltipText(const QString &id, const QString &name, int value, int duration) const;
    QString abbreviation(const QString &id, const QString &name) const;

private:
    BuffCatalog() = default;

    QMap<QString, BuffDefinitionEntry> m_entries;
    bool m_loaded = false;
};
