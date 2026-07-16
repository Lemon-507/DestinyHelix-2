#pragma once

#include <QtWidgets/QWidget>

#include <QtCore/QString>
#include <QtCore/QVector>

class CardCollectionViewModel;
class QPushButton;

class CardCollectionView : public QWidget
{
    Q_OBJECT

public:
    explicit CardCollectionView(CardCollectionViewModel *viewModel, QWidget *parent = nullptr);

    void startSelectionMode(const QString &actionType);
    void clearSelectionMode();

signals:
    void exitClicked();
    void cardSelected(const QString &cardId, int deckIndex);
    void selectionCancelled();

public slots:
    void updateFromViewModel();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onExitClicked();
    void onCardSlotClicked();

private:
    void setupLayout();
    void populateSlots();

    CardCollectionViewModel *m_viewModel = nullptr;
    QPushButton *m_exitButton = nullptr;
    QVector<QPushButton *> m_cardSlots;
    bool m_selectionMode = false;
};
