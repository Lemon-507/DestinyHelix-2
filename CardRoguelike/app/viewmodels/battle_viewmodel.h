#pragma once



#include <QtCore/QObject>

#include <QtCore/QVariant>

#include <QtCore/QVector>

#include "potions/potion_database.h"

class BattleFSM;
class Enemy;

struct EnemyIntentDef;

class GameManager;

class Player;



class BattleViewModel : public QObject

{

    Q_OBJECT



    Q_PROPERTY(int playerHp READ playerHp NOTIFY playerHpChanged)

    Q_PROPERTY(int playerMaxHp READ playerMaxHp NOTIFY playerMaxHpChanged)

    Q_PROPERTY(int playerBlock READ playerBlock NOTIFY playerBlockChanged)

    Q_PROPERTY(int enemyHp READ enemyHp NOTIFY enemyHpChanged)

    Q_PROPERTY(int enemyMaxHp READ enemyMaxHp NOTIFY enemyMaxHpChanged)

    Q_PROPERTY(int enemyBlock READ enemyBlock NOTIFY enemyBlockChanged)

    Q_PROPERTY(QString enemyName READ enemyName NOTIFY enemyNameChanged)

    Q_PROPERTY(int currentEnergy READ currentEnergy NOTIFY currentEnergyChanged)

    Q_PROPERTY(int maxEnergy READ maxEnergy NOTIFY maxEnergyChanged)

    Q_PROPERTY(QString enemyIntent READ enemyIntent NOTIFY enemyIntentChanged)

    Q_PROPERTY(int enemyIntentValue READ enemyIntentValue NOTIFY enemyIntentValueChanged)

    Q_PROPERTY(QVariantList handCards READ handCards NOTIFY handCardsChanged)

    Q_PROPERTY(int gold READ gold NOTIFY goldChanged)

    Q_PROPERTY(QVariantList relics READ relics NOTIFY relicsChanged)

    Q_PROPERTY(QVariantList potions READ potions NOTIFY potionsChanged)
    Q_PROPERTY(QVariantList playerBuffs READ playerBuffs NOTIFY playerBuffsChanged)
    Q_PROPERTY(bool playerBuffOverflow READ playerBuffOverflow NOTIFY playerBuffsChanged)
    Q_PROPERTY(QVariantList enemies READ enemies NOTIFY enemiesChanged)
    Q_PROPERTY(int drawPileCount READ drawPileCount NOTIFY drawPileCountChanged)
    Q_PROPERTY(int discardPileCount READ discardPileCount NOTIFY discardPileCountChanged)
    Q_PROPERTY(bool combatActive READ combatActive NOTIFY combatActiveChanged)



public:

    explicit BattleViewModel(GameManager *gameManager, QObject *parent = nullptr);

    ~BattleViewModel() override;



    int playerHp() const;

    int playerMaxHp() const;

    int playerBlock() const;

    int enemyHp() const;

    int enemyMaxHp() const;

    int enemyBlock() const;

    QString enemyName() const;

    int currentEnergy() const;

    int maxEnergy() const;

    QString enemyIntent() const;

    int enemyIntentValue() const;

    QVariantList handCards() const;

    int gold() const;

    QVariantList relics() const;

    QVariantList potions() const;
    QVariantList playerBuffs() const;
    bool playerBuffOverflow() const;
    QVariantList enemies() const;
    int drawPileCount() const;
    int discardPileCount() const;
    bool combatActive() const;
    bool canSelectCards() const;
    bool canAffordCard(int handIndex) const;



    Q_INVOKABLE void startCombat();

    Q_INVOKABLE void playCard(const QString &cardId, int enemyIndex = -1, int handIndex = -1);

    Q_INVOKABLE void endPlayerTurn();

    Q_INVOKABLE bool canUsePotion(const QString &potionId) const;

    Q_INVOKABLE QString potionPrimaryTarget(const QString &potionId) const;

    Q_INVOKABLE void usePotion(const QString &potionId, int enemyIndex = -1);



public slots:

    void refreshState();

private slots:

    void checkCombatResolution();



signals:

    void playerHpChanged();

    void playerMaxHpChanged();

    void playerBlockChanged();

    void enemyHpChanged();

    void enemyMaxHpChanged();

    void enemyBlockChanged();

    void enemyNameChanged();

    void currentEnergyChanged();

    void maxEnergyChanged();

    void enemyIntentChanged();

    void enemyIntentValueChanged();

    void handCardsChanged();

    void goldChanged();

    void relicsChanged();

    void potionsChanged();
    void playerBuffsChanged();
    void enemiesChanged();
    void drawPileCountChanged();
    void discardPileCountChanged();
    void combatActiveChanged();

    void combatEnded(bool playerWon);



private:

    void connectBattleFsm();

    void connectPlayerSignals();

    void connectEnemySignals();

    void connectEnemyHpSignals();

    void finishCombat(bool playerWon);

    GameContext currentPotionContext() const;

    Enemy *activeEnemy() const;

    Enemy *enemyAtIndex(int index) const;

    static QString cardDescription(const class Card &card);
    static QString cardDisplayName(const QString &id);
    static QString textAbbreviation(const QString &name, const QString &fallbackId, int maxLen = 2);
    static QString intentDescription(const EnemyIntentDef &intent, int ownerStrength);
    static QVariantList buildBuffEntries(const Actor *actor);
    static QVariantMap buffEntryFromPower(class Power *power);

    BattleFSM *battleFsm() const;



    GameManager *m_gameManager = nullptr;

    bool m_combatActive = false;

};

