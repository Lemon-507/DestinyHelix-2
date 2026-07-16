#pragma once

#include <QtWidgets/QWidget>

#include <QtCore/QVector>

class QLabel;
class QPushButton;
class RewardViewModel;

class RewardView : public QWidget
{
    Q_OBJECT

public:
    explicit RewardView(RewardViewModel *viewModel, QWidget *parent = nullptr);

signals:
    void backToMapClicked();

public slots:
    void updateFromViewModel();

private slots:
    void onRewardSlotClicked();

private:
    void setupLayout();
    void populateRewardSlots();

    RewardViewModel *m_viewModel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QPushButton *m_backButton = nullptr;
    QVector<QPushButton *> m_rewardSlots;
};
