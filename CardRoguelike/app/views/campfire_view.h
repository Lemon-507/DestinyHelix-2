#pragma once

#include <QtWidgets/QWidget>

class CampfireViewModel;
class QLabel;
class QPushButton;

class CampfireView : public QWidget
{
    Q_OBJECT

public:
    explicit CampfireView(CampfireViewModel *viewModel, QWidget *parent = nullptr);

signals:
    void restClicked();
    void upgradeClicked();

public slots:
    void updateFromViewModel();

private slots:
    void onRestClicked();
    void onUpgradeClicked();

private:
    void setupLayout();

    CampfireViewModel *m_viewModel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QPushButton *m_restButton = nullptr;
    QPushButton *m_upgradeButton = nullptr;
};
