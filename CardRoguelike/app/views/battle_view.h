#pragma once

#include <QtCore/QPoint>
#include <QtCore/QVector>

#include <QtWidgets/QWidget>

class BattleViewModel;
class CardWidget;
class QEvent;
class QGraphicsView;
class QKeyEvent;
class QLabel;
class QMouseEvent;
class QPushButton;

struct EnemyDisplayGroup {
    QLabel *artPlaceholder = nullptr;
    QLabel *nameLabel = nullptr;
    QLabel *intentIcon = nullptr;
    QLabel *hpLabel = nullptr;
    QLabel *blockLabel = nullptr;
    QVector<QLabel *> buffSlots;
    QLabel *buffOverflow = nullptr;
    QString intentDescription;
};

class BattleView : public QWidget
{
    Q_OBJECT

public:
    explicit BattleView(BattleViewModel *viewModel, QWidget *parent = nullptr);

    void requestPotionUse(const QString &potionId, int statusBarSlotIndex,
                          const QPoint &startCenterInBattleCoords);

public slots:
    void updateFromViewModel();

signals:
    void potionTargetingChanged(int statusBarSlotIndex, bool active);

private slots:
    void onCardClicked(const QString &cardId);
    void onPotionClicked();
    void onEndTurnClicked();

private:
    void setupLayout();
    void setupPortrait();
    void setupPlayerBuffs();
    void setupEnemyGroups();
    void updateStats();
    void updateRelics();
    void updatePotions();
    void updateHand();
    void updateCombatHud();
    void updatePlayerBuffs();
    void layoutEnemyBuffs(int index, const QVariantMap &entry, int leftX, int artSize);
    void updateCardInteractivity();
    void applyCardAffordabilityStates();
    bool isHandIndexAffordable(int handIndex) const;
    void updateEndTurnButtonStyle();
    void updateEnemies();
    void updatePiles();
    void layoutEnemyGroups(const QVector<int> &leftXs, int artSize);
    void initializeCombatLayout(int initialEnemyCount);
    void resetCombatLayout();
    void setEnemyGroupVisible(int index, bool visible);
    void clearCardSelection();
    void selectCard(CardWidget *widget, const QString &cardId, int handIndex);
    void playSelectedCardAtEnemy(int enemyIndex);
    void executeCardPlay(CardWidget *widget, const QString &cardId, int handIndex, int enemyIndex,
                         const QString &targetType);
    void resetCardSlotGeometry(CardWidget *widget, int slotIndex);
    void clearCardSlot(CardWidget *widget, int slotIndex);
    QPoint cardWidgetCenter(const CardWidget *widget) const;
    QPoint enemyArtCenter(int enemyIndex) const;
    QPoint playerAreaCenter() const;
    QPoint flyAnimationEndPoint(int enemyIndex, const QString &targetType) const;
    void startCardFlyAnimation(const QString &name, int cost, const QPoint &start, const QPoint &end);
    void applyEnemyArtStyle(QLabel *artPlaceholder, bool hovered);
    void configureBuffLabel(QLabel *label) const;
    void showBuffTooltip(QLabel *source, const QString &text);
    void hideBuffTooltip();
    void showRelicTooltip(QLabel *source, const QString &text);
    void hideRelicTooltip();
    bool hasCardSelected() const;
    bool hasPotionTargeting() const;
    void clearPotionTargeting();
    void setPotionTargeting(const QString &potionId, int statusBarSlotIndex,
                            const QPoint &startCenterInBattleCoords);
    void highlightAliveEnemies(bool highlighted);
    void beginPotionExecution(const QString &potionId, int enemyIndex, const QString &effectTarget,
                              const QPoint &startCenter);
    void executePotionUse(const QString &potionId, int enemyIndex);
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

    static constexpr int kCardSlotCount = 10;
    static constexpr int kRelicSlotCount = 12;
    static constexpr int kPotionSlotCount = 3;
    static constexpr int kMaxEnemyDisplayCount = 5;
    static constexpr int kPlayerBuffMaxVisible = 10;
    static constexpr int kEnemyBuffMaxVisibleLarge = 7;
    static constexpr int kEnemyBuffMaxVisibleSmall = 4;

    BattleViewModel *m_viewModel = nullptr;

    QGraphicsView *m_portraitView = nullptr;
    QLabel *m_hpLabel = nullptr;
    QLabel *m_goldLabel = nullptr;
    QLabel *m_blockLabel = nullptr;
    QLabel *m_intentTooltip = nullptr;
    QLabel *m_buffTooltip = nullptr;
    QLabel *m_relicTooltip = nullptr;
    QLabel *m_energyLabel = nullptr;
    QPushButton *m_endTurnButton = nullptr;
    QPushButton *m_drawPileButton = nullptr;
    QPushButton *m_discardPileButton = nullptr;

    QVector<CardWidget *> m_cardSlots;
    QVector<QLabel *> m_relicSlots;
    QVector<QPushButton *> m_potionSlots;
    QVector<QString> m_potionSlotIds;
    QVector<EnemyDisplayGroup> m_enemyGroups;
    QVector<QLabel *> m_playerBuffSlots;
    QLabel *m_playerBuffOverflow = nullptr;

    bool m_combatLayoutInitialized = false;
    int m_lockedSlotCount = 0;
    int m_lockedArtSize = 0;
    QVector<int> m_lockedLeftXs;

    QString m_selectedCardId;
    int m_selectedHandIndex = -1;
    CardWidget *m_selectedCardWidget = nullptr;

    QString m_targetingPotionId;
    int m_targetingPotionSlotIndex = -1;
    QPoint m_targetingPotionStart;
    bool m_potionUseInFlight = false;
};
