#pragma once

#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtGui/QPixmap>

class QLabel;
class QPushButton;

namespace ArtAssets {

QPixmap load(const QString &resourcePath);
QPixmap scaled(const QPixmap &source, const QSize &size);

QPixmap playerPortrait(const QSize &size = QSize());
QPixmap enemyPortrait(const QString &enemyId, const QSize &size = QSize(), bool defensiveMode = false);
QPixmap eventIllustration(const QString &eventId, const QSize &size = QSize());
QPixmap mapNodeIcon(const QString &nodeLetter, const QSize &size = QSize());
QPixmap mapBossIcon(const QString &bossEnemyId, const QSize &size = QSize());
QPixmap potionIcon(const QString &potionId, const QSize &size = QSize());
QPixmap relicIcon(const QString &relicId, const QSize &size = QSize());

bool applyToLabel(QLabel *label, const QPixmap &pixmap, const QString &fallbackText = QString());
bool applyToButton(QPushButton *button, const QPixmap &pixmap, const QString &fallbackText = QString());

void preloadStaticMapIcons();

} // namespace ArtAssets
