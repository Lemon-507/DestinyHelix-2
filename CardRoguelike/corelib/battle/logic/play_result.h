#pragma once

#include <QtCore/QString>

struct PlayResult
{
    bool ok = false;
    QString reason;

    static PlayResult success()
    {
        return PlayResult{true, QString()};
    }

    static PlayResult failure(const QString &reason)
    {
        return PlayResult{false, reason};
    }
};
