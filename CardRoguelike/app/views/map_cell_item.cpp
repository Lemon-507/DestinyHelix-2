#include "map_cell_item.h"

#include <QtGui/QCursor>
#include <QtGui/QFont>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsSceneMouseEvent>

MapCellItem::MapCellItem(int x, int y, qreal size, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_x(x)
    , m_y(y)
    , m_size(size)
{
    setAcceptHoverEvents(true);
    setCursor(QCursor(Qt::PointingHandCursor));
}

int MapCellItem::gridX() const
{
    return m_x;
}

int MapCellItem::gridY() const
{
    return m_y;
}

QRectF MapCellItem::boundingRect() const
{
    return QRectF(0, 0, m_size, m_size);
}

void MapCellItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    painter->fillRect(boundingRect(), m_baseColor);

    if (m_shrinkAlpha > 0.0) {
        QColor overlay(180, 40, 80, static_cast<int>(255 * m_shrinkAlpha));
        painter->fillRect(boundingRect(), overlay);
    }

    if (m_highlight) {
        painter->setPen(QPen(m_highlightColor, 3));
        painter->drawRect(boundingRect().adjusted(2, 2, -2, -2));
    }

    if (m_playerHere) {
        painter->setPen(QPen(QColor(0, 220, 220), 3));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(boundingRect().adjusted(8, 8, -8, -8));
    }

    const bool drawIcon = !m_icon.isNull() && !m_playerHere;
    if (drawIcon) {
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        const QRectF target = boundingRect().adjusted(4, 4, -4, -4);
        const QSizeF fitted = m_icon.size().scaled(target.size().toSize(), Qt::KeepAspectRatio);
        const QRectF dest(target.center().x() - fitted.width() / 2.0,
                          target.center().y() - fitted.height() / 2.0, fitted.width(), fitted.height());
        painter->drawPixmap(dest.toRect(), m_icon);
        return;
    }

    if (!m_label.isEmpty()) {
        QFont font = painter->font();
        font.setBold(true);
        font.setPixelSize(qMax(14, static_cast<int>(m_size * 0.45)));
        painter->setFont(font);
        painter->setRenderHint(QPainter::TextAntialiasing, true);

        const QRectF rect = boundingRect();
        const QString letter = m_label.left(1);

        static const QColor kOutlineColor(0, 0, 0, 230);
        static const QColor kFillColor(255, 255, 235);

        for (int dx = -2; dx <= 2; ++dx) {
            for (int dy = -2; dy <= 2; ++dy) {
                if (dx == 0 && dy == 0) {
                    continue;
                }
                painter->setPen(kOutlineColor);
                painter->drawText(rect.translated(dx, dy), Qt::AlignCenter, letter);
            }
        }

        painter->setPen(kFillColor);
        painter->drawText(rect, Qt::AlignCenter, letter);
    }
}

void MapCellItem::setBaseColor(const QColor &color)
{
    m_baseColor = color;
    update();
}

void MapCellItem::setLabelText(const QString &text)
{
    m_label = text;
    update();
}

void MapCellItem::setIcon(const QPixmap &pixmap)
{
    m_icon = pixmap;
    update();
}

void MapCellItem::setHighlightVisible(bool visible, const QColor &color)
{
    m_highlight = visible;
    m_highlightColor = color;
    update();
}

void MapCellItem::setShrinkOverlay(double alpha)
{
    m_shrinkAlpha = qBound(0.0, alpha, 0.85);
    update();
}

void MapCellItem::setPlayerHere(bool here)
{
    m_playerHere = here;
    update();
}

void MapCellItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_x, m_y);
        event->accept();
        return;
    }
    QGraphicsObject::mousePressEvent(event);
}
