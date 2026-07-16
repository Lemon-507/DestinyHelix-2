#include "combat_animations.h"

#include <QtCore/QPropertyAnimation>
#include <QtWidgets/QLabel>

void CombatAnimations::flyPixmap(QWidget *parent, const QPixmap &pixmap, const QPoint &startCenter,
                                 const QPoint &endCenter, int durationMs,
                                 const std::function<void()> &onFinished)
{
    if (parent == nullptr) {
        if (onFinished) {
            onFinished();
        }
        return;
    }

    const int width = pixmap.isNull() ? 30 : pixmap.width();
    const int height = pixmap.isNull() ? 30 : pixmap.height();

    auto *flyLabel = new QLabel(parent);
    if (!pixmap.isNull()) {
        flyLabel->setPixmap(pixmap);
    } else {
        flyLabel->setText(QStringLiteral("?"));
        flyLabel->setAlignment(Qt::AlignCenter);
        flyLabel->setStyleSheet(
            QStringLiteral("background-color: #2a2a3e; border: 1px solid #0f3460; color: #ffffff;"));
    }
    flyLabel->setFixedSize(width, height);
    flyLabel->move(startCenter.x() - width / 2, startCenter.y() - height / 2);
    flyLabel->show();
    flyLabel->raise();

    auto *anim = new QPropertyAnimation(flyLabel, "pos", parent);
    anim->setDuration(durationMs > 0 ? durationMs : 500);
    anim->setStartValue(flyLabel->pos());
    anim->setEndValue(QPoint(endCenter.x() - width / 2, endCenter.y() - height / 2));

    QObject::connect(anim, &QPropertyAnimation::finished, flyLabel, &QObject::deleteLater);
    QObject::connect(anim, &QPropertyAnimation::finished, anim, &QObject::deleteLater);
    if (onFinished) {
        QObject::connect(anim, &QPropertyAnimation::finished, parent, [onFinished]() { onFinished(); });
    }
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void CombatAnimations::flyCardProxy(QWidget *parent, const QString &name, int cost,
                                    const QPoint &startCenter, const QPoint &endCenter, int cardWidth,
                                    int cardHeight, int durationMs,
                                    const std::function<void()> &onFinished)
{
    if (parent == nullptr) {
        if (onFinished) {
            onFinished();
        }
        return;
    }

    auto *flyLabel = new QLabel(parent);
    flyLabel->setText(QStringLiteral("%1\n[%2]").arg(name).arg(cost));
    flyLabel->setAlignment(Qt::AlignCenter);
    flyLabel->setWordWrap(true);
    flyLabel->setFixedSize(cardWidth, cardHeight);
    flyLabel->setStyleSheet(
        QStringLiteral("background-color: #16213e; border: 2px solid #0f3460; border-radius: 8px; "
                       "color: #ffffff; font-size: 12px; padding: 4px;"));
    flyLabel->move(startCenter.x() - cardWidth / 2, startCenter.y() - cardHeight / 2);
    flyLabel->show();
    flyLabel->raise();

    auto *anim = new QPropertyAnimation(flyLabel, "pos", parent);
    anim->setDuration(durationMs > 0 ? durationMs : 300);
    anim->setStartValue(flyLabel->pos());
    anim->setEndValue(QPoint(endCenter.x() - cardWidth / 2, endCenter.y() - cardHeight / 2));

    QObject::connect(anim, &QPropertyAnimation::finished, flyLabel, &QObject::deleteLater);
    QObject::connect(anim, &QPropertyAnimation::finished, anim, &QObject::deleteLater);
    if (onFinished) {
        QObject::connect(anim, &QPropertyAnimation::finished, parent, [onFinished]() { onFinished(); });
    }
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}
