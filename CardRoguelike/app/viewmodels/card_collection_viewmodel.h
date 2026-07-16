#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

class Card;
class GameManager;

class CardCollectionViewModel : public QObject
{
    Q_OBJECT

public:
    struct CardEntry {
        QString id;
        QString displayName;
        QString abbreviation;
        QString tooltip;
        bool selectable = true;
        int deckIndex = -1;
    };

    explicit CardCollectionViewModel(GameManager *gameManager, QObject *parent = nullptr);

    const QVector<CardEntry> &cards() const;
    QString selectionAction() const;
    bool selectionMode() const;

    void setSelectionAction(const QString &actionType);
    void clearSelectionMode();
    void refresh();

signals:
    void cardsChanged();
    void exitRequested();
    void selectionCancelled();

public slots:
    void requestExit();

private:
    static QString abbreviationFor(const QString &name, const QString &fallbackId);
    static QString tooltipForCard(const Card &card);
    bool isEntrySelectable(const Card &card) const;

    GameManager *m_gameManager = nullptr;
    QVector<CardEntry> m_cards;
    QString m_selectionAction;
    bool m_selectionMode = false;
};
