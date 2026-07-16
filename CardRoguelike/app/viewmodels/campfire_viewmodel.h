#pragma once

#include <QtCore/QObject>

class GameManager;

class CampfireViewModel : public QObject
{
    Q_OBJECT

public:
    explicit CampfireViewModel(GameManager *gameManager, QObject *parent = nullptr);

    bool canUpgrade() const;

public slots:
    void requestRest();
    void requestUpgrade();

signals:
    void restChosen();
    void upgradeChosen();
    void canUpgradeChanged();

private:
    GameManager *m_gameManager = nullptr;
};
