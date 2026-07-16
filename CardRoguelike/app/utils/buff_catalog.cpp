#include "buff_catalog.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

BuffCatalog &BuffCatalog::instance()
{
    static BuffCatalog catalog;
    return catalog;
}

void BuffCatalog::load()
{
    if (m_loaded) {
        return;
    }

    Q_INIT_RESOURCE(resources);

    QFile file(QStringLiteral(":/resources/data/buffs.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QJsonArray buffs;
    if (document.isArray()) {
        buffs = document.array();
    } else {
        buffs = document.object().value(QStringLiteral("buffs")).toArray();
    }

    for (const QJsonValue &value : buffs) {
        const QJsonObject obj = value.toObject();
        BuffDefinitionEntry entry;
        entry.id = obj.value(QStringLiteral("id")).toString();
        entry.name = obj.value(QStringLiteral("name")).toString();
        entry.description = obj.value(QStringLiteral("description")).toString();
        if (!entry.id.isEmpty()) {
            m_entries.insert(entry.id, entry);
        }
    }

    m_loaded = true;
}

QString BuffCatalog::displayName(const QString &id, const QString &fallbackName) const
{
    const auto it = m_entries.constFind(id);
    if (it != m_entries.constEnd() && !it->name.isEmpty()) {
        return it->name;
    }
    return fallbackName.isEmpty() ? id : fallbackName;
}

QString BuffCatalog::formatDescription(const QString &id, int value) const
{
    const auto it = m_entries.constFind(id);
    if (it == m_entries.constEnd()) {
        return QString();
    }

    QString description = it->description;
    const QString valueText = QString::number(value);
    description.replace(QStringLiteral("{value}"), valueText);
    return description;
}

QString BuffCatalog::tooltipText(const QString &id, const QString &name, int value, int duration) const
{
    QString text = formatDescription(id, value);
    if (text.isEmpty()) {
        text = displayName(id, name);
    }

    if (duration > 0) {
        text += QStringLiteral("\nDuration: %1 turn(s)").arg(duration);
    } else if (duration < 0) {
        text += QStringLiteral("\nPermanent");
    }

    return text;
}

QString BuffCatalog::abbreviation(const QString &id, const QString &name) const
{
    const QString resolvedName = displayName(id, name);
    if (resolvedName.isEmpty()) {
        return id.left(3).toUpper();
    }

    const QStringList words = resolvedName.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (words.size() >= 2) {
        QString abbr;
        for (const QString &word : words) {
            if (!word.isEmpty()) {
                abbr.append(word.at(0).toUpper());
            }
            if (abbr.size() >= 3) {
                break;
            }
        }
        return abbr.left(3);
    }

    return resolvedName.left(3).toUpper();
}
