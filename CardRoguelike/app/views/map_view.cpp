#include "map_view.h"

#include <QtCore/QTimer>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QMessageBox>

#include "map_cell_item.h"
#include "utils/art_assets.h"
#include "viewmodels/map_viewmodel.h"

MapView::MapView(MapViewModel *viewModel, QWidget *parent)
    : QWidget(parent)
    , m_viewModel(viewModel)
{
    setFixedSize(kViewSize, kViewSize);
    ArtAssets::preloadStaticMapIcons();

    m_scene = new QGraphicsScene(this);
    m_graphicsView = new QGraphicsView(m_scene, this);
    m_graphicsView->setRenderHint(QPainter::Antialiasing);
    m_graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_graphicsView->setFrameShape(QFrame::NoFrame);
    m_graphicsView->setFixedSize(kViewSize, kViewSize);
    m_graphicsView->setGeometry(0, 0, kViewSize, kViewSize);

    buildGrid();

    m_blinkTimer = new QTimer(this);
    m_blinkTimer->setInterval(500);
    connect(m_blinkTimer, &QTimer::timeout, this, &MapView::onBlinkTimer);
    m_blinkTimer->start();

    if (m_viewModel != nullptr) {
        connect(m_viewModel, &MapViewModel::gridDataChanged, this, &MapView::refreshCells);
        connect(m_viewModel, &MapViewModel::exploredNodeChanged, this, &MapView::onNodeExplored);
        connect(m_viewModel, &MapViewModel::mapReady, this, &MapView::updateFromViewModel);
        connect(m_viewModel, &MapViewModel::playerPlaced, this, &MapView::updateFromViewModel);
        connect(m_viewModel, &MapViewModel::playerXChanged, this, &MapView::updatePlayerPosition);
        connect(m_viewModel, &MapViewModel::playerYChanged, this, &MapView::updatePlayerPosition);
        connect(m_viewModel, &MapViewModel::shrinkCountChanged, this, &MapView::refreshCells);
    }

    updateFromViewModel();
}

void MapView::buildGrid()
{
    m_cells.clear();
    m_scene->clear();

    for (int y = 0; y < kGridSize; ++y) {
        QVector<MapCellItem *> row;
        for (int x = 0; x < kGridSize; ++x) {
            auto *cell = new MapCellItem(x, y, kCellSize);
            cell->setPos(x * kCellSize, y * kCellSize);
            m_scene->addItem(cell);
            connect(cell, &MapCellItem::clicked, this, &MapView::onNodeClicked);
            row.append(cell);
        }
        m_cells.append(row);
    }

    m_scene->setSceneRect(0, 0, kViewSize, kViewSize);
}

void MapView::updateFromViewModel()
{
    refreshCells();
    refreshHighlights();
}

void MapView::refreshCells()
{
    if (m_viewModel == nullptr) {
        return;
    }

    for (int y = 0; y < kGridSize; ++y) {
        for (int x = 0; x < kGridSize; ++x) {
            updateCell(x, y);
        }
    }

    m_lastPlayerX = m_viewModel->playerX();
    m_lastPlayerY = m_viewModel->playerY();
}

void MapView::updateCell(int x, int y)
{
    if (m_viewModel == nullptr || x < 0 || y < 0 || x >= kGridSize || y >= kGridSize) {
        return;
    }

    const int playerX = m_viewModel->playerX();
    const int playerY = m_viewModel->playerY();
    const int shrinkCount = m_viewModel->shrinkCount();
    const bool isPlayerCell = playerX >= 0 && playerY >= 0 && x == playerX && y == playerY;
    const QString baseLetter = m_viewModel->cellLetter(x, y);

    MapCellItem *cell = m_cells[y][x];
    cell->setBaseColor(nodeColorForLetter(baseLetter));

    // Empty (H) and cleared (X) are floor-only; player is marked by the cyan ring — no letter labels.
    const bool floorOnly = baseLetter == QStringLiteral("H") || baseLetter == QStringLiteral("X");
    if (isPlayerCell || floorOnly) {
        cell->setLabelText(QString());
    } else {
        cell->setLabelText(baseLetter);
    }

    if (isPlayerCell || floorOnly) {
        cell->setIcon(QPixmap());
    } else {
        const int iconPad = 8;
        const QSize iconSize(static_cast<int>(kCellSize) - iconPad, static_cast<int>(kCellSize) - iconPad);
        if (baseLetter == QStringLiteral("B")) {
            cell->setIcon(ArtAssets::mapBossIcon(m_viewModel->bossEnemyId(), iconSize));
        } else {
            cell->setIcon(ArtAssets::mapNodeIcon(baseLetter, iconSize));
        }
    }
    cell->setPlayerHere(isPlayerCell);

    if (shrinkCount > 0 && m_viewModel->isInShrinkZone(x, y)) {
        const double alpha = 0.2 + (0.12 * shrinkCount);
        cell->setShrinkOverlay(alpha);
    } else {
        cell->setShrinkOverlay(0.0);
    }
}

void MapView::updatePlayerPosition()
{
    if (m_viewModel == nullptr) {
        return;
    }

    const int newX = m_viewModel->playerX();
    const int newY = m_viewModel->playerY();

    if (newX == m_lastPlayerX && newY == m_lastPlayerY) {
        return;
    }

    if (m_lastPlayerX >= 0 && m_lastPlayerY >= 0) {
        updateCell(m_lastPlayerX, m_lastPlayerY);
    }
    if (newX >= 0 && newY >= 0) {
        updateCell(newX, newY);
    }

    m_lastPlayerX = newX;
    m_lastPlayerY = newY;
    refreshHighlights();
}

void MapView::onNodeExplored(int x, int y)
{
    updateCell(x, y);
    refreshHighlights();
}

void MapView::refreshHighlights()
{
    if (m_viewModel == nullptr) {
        return;
    }

    const int playerX = m_viewModel->playerX();
    const int playerY = m_viewModel->playerY();
    const bool waitingForStart = playerX < 0 || playerY < 0;

    for (int y = 0; y < kGridSize; ++y) {
        for (int x = 0; x < kGridSize; ++x) {
            MapCellItem *cell = m_cells[y][x];
            bool highlight = false;
            QColor color(80, 160, 255, 120);

            if (waitingForStart) {
                if (m_viewModel->isStartNode(x, y) && m_blinkState) {
                    highlight = true;
                    color = QColor(80, 220, 80, 140);
                }
            } else if (m_viewModel->canMoveTo(x, y) && m_blinkState) {
                highlight = true;
            }

            cell->setHighlightVisible(highlight, color);
        }
    }
}

QColor MapView::nodeColorForLetter(const QString &letter) const
{
    if (letter == QStringLiteral("H")) {
        return QColor(90, 90, 90);
    }
    if (letter == QStringLiteral("X")) {
        return QColor(70, 70, 70);
    }
    if (letter == QStringLiteral("S")) {
        return QColor(200, 180, 40);
    }
    if (letter == QStringLiteral("F")) {
        return QColor(220, 120, 40);
    }
    if (letter == QStringLiteral("E")) {
        return QColor(140, 60, 180);
    }
    if (letter == QStringLiteral("N")) {
        return QColor(180, 50, 50);
    }
    if (letter == QStringLiteral("?")) {
        return QColor(50, 160, 80);
    }
    if (letter == QStringLiteral("B")) {
        return QColor(60, 20, 80);
    }
    return QColor(100, 100, 100);
}

void MapView::onBlinkTimer()
{
    m_blinkState = !m_blinkState;
    refreshHighlights();
}

void MapView::onNodeClicked(int x, int y)
{
    if (m_viewModel == nullptr) {
        return;
    }

    try {
        const int playerX = m_viewModel->playerX();
        const int playerY = m_viewModel->playerY();

        if (playerX < 0 || playerY < 0) {
            if (m_viewModel->isStartNode(x, y)) {
                m_viewModel->selectStartNode(x, y);
                updateFromViewModel();
            }
            return;
        }

        if (m_viewModel->canMoveTo(x, y)) {
            m_viewModel->movePlayer(x, y);
            updateFromViewModel();
        }
    } catch (...) {
        QMessageBox::warning(this, QStringLiteral("Error"),
                             QStringLiteral("An unexpected error occurred while moving."));
    }
}
