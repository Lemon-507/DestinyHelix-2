#pragma once

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <memory>
#include <vector>

#include "battle/entities/card.h"
#include "battle/entities/enemy.h"
#include "battle/logic/play_result.h"

class Player;
class ShrinkDamagePower;

enum class BattleState {
    Idle,
    PlayerTurnStart,
    PlayerTurn,
    EnemyTurn,
    Ended
};

class BattleFSM : public QObject
{
    Q_OBJECT

public:
    explicit BattleFSM(QObject *parent = nullptr);

    int currentEnergy() const;
    int maxEnergy() const;
    QVector<Card> hand() const;
    int drawPileCount() const;
    int discardPileCount() const;
    int exhaustPileCount() const;
    BattleState battleState() const;
    QString enemyIntent() const;
    int enemyIntentValue() const;
    ShrinkDamagePower *shrinkPower() const;
    int cardDamageBonus(const QString &cardId) const;
    QVector<Enemy *> enemies() const;
    void adoptEncounter(std::vector<std::unique_ptr<Enemy>> enemies);
    void releaseEncounter();

public slots:
    void startCombat(Player *player, const QVector<Enemy *> &enemies, int shrinkCount = 0);
    void playerEndTurn();
    PlayResult playCard(Card card, Actor *target);

    void drawExtraCards(int count);
    void gainEnergy(int amount);
    void addCardToHand(const Card &card);
    void exhaustNonAttackCardsGainBlock(int blockPerCard);
    void increaseCardDamageBonus(const QString &cardId, int amount);
    void concludeCombat(bool playerWon);
    void refreshEnemyIntentPreview();

signals:
    void combatEnded(bool playerWon);
    void enemiesChanged();

private:
    void beginPlayerTurn();
    void enemyTurn();
    void endCombat(bool playerWon);
    void shuffleDrawPile();
    bool drawCard();
    void drawCardsToHand();
    void discardHand();
    void processEndOfTurnHandKeywords();
    void clearShrinkPower();
    void applyShrinkPower(int shrinkCount);
    void updateEnemyIntentPreview();
    void executeEnemyIntent(const EnemyIntentDef &intent);
    void executeSplitIntent(const EnemyIntentDef &intent);
    void refreshEncounterPeers();
    Enemy *activeEnemy() const;
    void applyBuff(Actor *target, const QString &buffType, int value);
    void applyDebuff(Actor *target, const QString &debuffType, int value);
    QVector<Actor *> livingEnemyActors() const;
    QVector<EffectDef> prepareCardEffects(const Card &card) const;
    static QVector<EffectDef> toEffectVector(const std::vector<EffectDef> &effects);

    Player *m_player = nullptr;
    QVector<Enemy *> m_enemies;
    std::vector<std::unique_ptr<Enemy>> m_encounterOwned;
    std::vector<std::unique_ptr<Enemy>> m_ownedEnemies;
    Enemy *m_enemy = nullptr;
    int m_currentEnergy = 0;
    int m_maxEnergy = 3;
    BattleState m_state = BattleState::Idle;
    QVector<Card> m_hand;
    QVector<Card> m_drawPile;
    QVector<Card> m_discardPile;
    QVector<Card> m_exhaustPile;
    QVector<Card> m_deckReference;
    QHash<QString, int> m_cardDamageBonus;
    int m_turnNumber = 0;
    int m_attackCardsPlayed = 0;
    int m_shrinkCount = 0;
    bool m_shrinkActive = false;
    QString m_enemyIntent;
    int m_enemyIntentValue = 0;
    ShrinkDamagePower *m_shrinkPowerPtr = nullptr;
    QMetaObject::Connection m_shrinkConnection;
};
