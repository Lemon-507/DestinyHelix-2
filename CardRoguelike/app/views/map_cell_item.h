#pragma once

#include <QtCore/QObject>
#include <QtGui/QPixmap>
#include <QtWidgets/QGraphicsObject>
#include <QtWidgets/QGraphicsRectItem>

class MapCellItem : public QGraphicsObject
{
    Q_OBJECT

public:
    MapCellItem(int x, int y, qreal size, QGraphicsItem *parent = nullptr);

    int gridX() const;
    int gridY() const;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setBaseColor(const QColor &color);
    void setLabelText(const QString &text);
    void setIcon(const QPixmap &pixmap);
    void setHighlightVisible(bool visible, const QColor &color = QColor(80, 160, 255, 120));
    void setShrinkOverlay(double alpha);
    void setPlayerHere(bool here);

signals:
    void clicked(int x, int y);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    int m_x = 0;
    int m_y = 0;
    qreal m_size = 60.0;
    QColor m_baseColor = QColor(100, 100, 100);
    QString m_label;
    QPixmap m_icon;
    bool m_highlight = false;
    QColor m_highlightColor = QColor(80, 160, 255, 120);
    double m_shrinkAlpha = 0.0;
    bool m_playerHere = false;
};
