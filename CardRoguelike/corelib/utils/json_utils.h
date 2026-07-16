#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QString>

#include <optional>
#include <type_traits>

template<typename T>
std::optional<T> jsonValue(const QJsonObject &obj, const QString &key)
{
    if (!obj.contains(key)) {
        return std::nullopt;
    }

    const QJsonValue value = obj.value(key);
    if constexpr (std::is_same_v<T, int>) {
        return value.toInt();
    } else if constexpr (std::is_same_v<T, QString>) {
        return value.toString();
    } else if constexpr (std::is_same_v<T, bool>) {
        return value.toBool();
    }

    return std::nullopt;
}
