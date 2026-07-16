#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

class GameManager;

class RewardViewModel : public QObject
{
    Q_OBJECT

public:
    enum class RewardType {
        Gold = 0,
        Potion,
        Card,
        Relic
    };

    struct RewardOffer {
        RewardType type = RewardType::Gold;
        QString buttonText;
        QString tooltip;
        bool granted = false;
        bool claimed = false;
        int goldAmount = 0;
        QString itemId;
    };

    explicit RewardViewModel(GameManager *gameManager, QObject *parent = nullptr);

    const QVector<RewardOffer> &offers() const;

    Q_INVOKABLE bool claimReward(RewardType type);

signals:
    void offersChanged();

private:
    static bool rollPercent(int percent);
    static QStringList loadPotionIdsFromResources();
    static QString randomCardId();
    static QString randomPotionId();
    static QString randomRelicId(const QStringList &ownedIds);

    void generateRewards();

    GameManager *m_gameManager = nullptr;
    QVector<RewardOffer> m_offers;
};
