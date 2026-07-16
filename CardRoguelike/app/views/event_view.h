#pragma once

#include <QtWidgets/QWidget>

#include <QtCore/QVector>

class EventViewModel;
class QGraphicsView;
class QLabel;
class QPushButton;

class EventView : public QWidget
{
    Q_OBJECT

public:
    explicit EventView(EventViewModel *viewModel, QWidget *parent = nullptr);

signals:
    void exitClicked();

public slots:
    void updateFromViewModel();

private slots:
    void onOptionClicked();
    void onExitClicked();

private:
    void setupLayout();
    void refreshOptionButtons();
    void refreshExitVisibility();
    void refreshEventImage();

    EventViewModel *m_viewModel = nullptr;
    QGraphicsView *m_eventImage = nullptr;
    QLabel *m_eventName = nullptr;
    QLabel *m_eventDescription = nullptr;
    QVector<QPushButton *> m_optionButtons;
    QPushButton *m_exitButton = nullptr;
};
