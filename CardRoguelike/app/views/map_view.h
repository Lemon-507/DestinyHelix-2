#pragma once

#include <QtWidgets/QWidget>

class MapViewModel;

class MapView : public QWidget
{
    Q_OBJECT

public:
    explicit MapView(MapViewModel *viewModel, QWidget *parent = nullptr);

public slots:
    void updateFromViewModel();
    void refreshCells();

private slots:
    void onBlinkTimer();
    void onNodeClicked(int x, int y);
    void onNodeExplored(int x, int y);

private:
    void buildGrid();
    void refreshHighlights();
    void updateCell(int x, int y);
    void updatePlayerPosition();
    QColor nodeColorForLetter(const QString &letter) const;

    MapViewModel *m_viewModel = nullptr;
    class QGraphicsView *m_graphicsView = nullptr;
    class QGraphicsScene *m_scene = nullptr;
    class QTimer *m_blinkTimer = nullptr;
    bool m_blinkState = false;
    int m_lastPlayerX = -1;
    int m_lastPlayerY = -1;
    QVector<QVector<class MapCellItem *>> m_cells;

    static constexpr int kGridSize = 11;
    static constexpr int kViewSize = 800;
    static constexpr qreal kCellSize = static_cast<qreal>(kViewSize) / kGridSize;
};
