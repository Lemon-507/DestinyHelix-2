#pragma once

#include <QtWidgets/QWidget>

#include <QtCore/QVector>

class QLabel;
class QPushButton;
class ShopViewModel;

class ShopView : public QWidget
{
    Q_OBJECT

public:
    explicit ShopView(ShopViewModel *viewModel = nullptr, QWidget *parent = nullptr);

signals:
    void leaveClicked();
    void deleteCardClicked();

public slots:
    void updateFromViewModel();

private slots:
    void onCardClicked();
    void onRelicClicked();
    void onPotionClicked();
    void onDeleteCardClicked();
    void onLeaveClicked();
    void onPurchaseDenied(const QString &reason);

private:
    void setupLayout();
    void refreshGold();
    void refreshCardSlots();
    void refreshRelicSlots();
    void refreshPotionSlots();
    void refreshDeleteCard();

    ShopViewModel *m_viewModel = nullptr;
    QLabel *m_goldLabel = nullptr;
    QPushButton *m_leaveButton = nullptr;
    QVector<QPushButton *> m_cardButtons;
    QVector<QLabel *> m_cardCostLabels;
    QPushButton *m_deleteCardButton = nullptr;
    QLabel *m_deleteCardCostLabel = nullptr;
    QVector<QPushButton *> m_relicButtons;
    QVector<QLabel *> m_relicCostLabels;
    QVector<QPushButton *> m_potionButtons;
    QVector<QLabel *> m_potionCostLabels;
};
