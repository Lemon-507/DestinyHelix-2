#pragma once

#include <functional>

#include <QtCore/QPoint>
#include <QtGui/QPixmap>

class QWidget;

class CombatAnimations
{
public:
    static void flyPixmap(QWidget *parent, const QPixmap &pixmap, const QPoint &startCenter,
                          const QPoint &endCenter, int durationMs,
                          const std::function<void()> &onFinished = {});

    static void flyCardProxy(QWidget *parent, const QString &name, int cost, const QPoint &startCenter,
                             const QPoint &endCenter, int cardWidth, int cardHeight, int durationMs,
                             const std::function<void()> &onFinished = {});
};
