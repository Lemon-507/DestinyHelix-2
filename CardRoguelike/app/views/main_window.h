#pragma once

#include <QtWidgets/QMainWindow>

class BattleView;
class BattleViewModel;
class CampfireView;
class CampfireViewModel;
class CardCollectionView;
class CardCollectionViewModel;
class DeathView;
class EventView;
class EventViewModel;
class GameManager;
class MapView;
class MapViewModel;
class RewardView;
class RewardViewModel;
class ShopView;
class ShopViewModel;
class QEvent;
class QPushButton;
class QStackedWidget;
class QLabel;
class QFrame;
class QWidget;
class RelicBarWidget;
struct EventData;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    void showMainMenu();
    void showMap();
    void showBattle();
    void showSettlement(bool victory, int steps, int gold, int hp);

private slots:
    void onNewGameClicked();
    void onMapReady();
    void onCombatTriggered();
    void onEventTriggered(const EventData &eventData);
    void onEventExit();
    void onEventCardSelectionNeeded(const QString &actionType);
    void onShopTriggered();
    void onShopLeave();
    void onShopDeleteCardClicked();
    void onCampfireTriggered();
    void onCampfireRest();
    void onCampfireUpgrade();
    void onBattleEnded(bool playerWon);
    void onRewardBackToMap();
    void onDeathReturnToMenu();
    void onGameOver(bool victory);
    void onCardCollectionClicked();
    void onCardCollectionExit();
    void onCardSelected(const QString &cardId, int deckIndex);
    void onCardSelectionCancelled();
    void onPotionTargetingChanged(int statusBarSlotIndex, bool active);

private:
    enum class CardSelectionContext {
        None,
        Browse,
        CampfireUpgrade,
        ShopRemove,
        EventAction
    };

    void applyDarkTheme();
    void setupMainMenu();
    void setupSettlement();
    void setupMapPanel();
    void setupPlayerPanel();
    void setupRelicBar();
    void refreshMapHud();
    void refreshPlayerPanel();
    void refreshRelicBar();
    void updateRelicBarVisibility();
    void connectPlayerSignals();
    void teardownBattle();
    void teardownReward();
    void teardownEvent();
    void teardownShop();
    void teardownCampfire();
    void teardownCardCollection();
    void showCardCollectionBrowse();
    void openCardSelection(const QString &actionType, CardSelectionContext context);
    void returnFromCardSelection();
    void applyPotionSlotStyle(int index, bool selected);
    void handlePotionSlotClicked(int slotIndex);
    bool eventFilter(QObject *watched, QEvent *event) override;

    GameManager *m_gameManager = nullptr;
    MapViewModel *m_mapViewModel = nullptr;
    MapView *m_mapView = nullptr;
    BattleViewModel *m_battleViewModel = nullptr;
    BattleView *m_battleView = nullptr;
    RewardViewModel *m_rewardViewModel = nullptr;
    RewardView *m_rewardView = nullptr;
    EventViewModel *m_eventViewModel = nullptr;
    EventView *m_eventView = nullptr;
    ShopViewModel *m_shopViewModel = nullptr;
    ShopView *m_shopView = nullptr;
    CampfireViewModel *m_campfireViewModel = nullptr;
    CampfireView *m_campfireView = nullptr;
    CardCollectionViewModel *m_cardCollectionViewModel = nullptr;
    CardCollectionView *m_cardCollectionView = nullptr;
    DeathView *m_deathView = nullptr;

    QStackedWidget *m_stack = nullptr;
    QWidget *m_mainMenuPage = nullptr;
    QWidget *m_mapPage = nullptr;
    QWidget *m_battlePage = nullptr;
    QWidget *m_settlementPage = nullptr;
    QWidget *m_rewardPage = nullptr;
    QWidget *m_deathPage = nullptr;
    QWidget *m_eventPage = nullptr;
    QWidget *m_shopPage = nullptr;
    QWidget *m_campfirePage = nullptr;
    QWidget *m_cardCollectionPage = nullptr;

    QLabel *m_settlementTitle = nullptr;
    QLabel *m_settlementStats = nullptr;
    QLabel *m_mapStepLabel = nullptr;
    QWidget *m_mapLegendBar = nullptr;
    QFrame *m_portraitPlaceholder = nullptr;
    QWidget *m_statusBar = nullptr;
    QLabel *m_playerHpLabel = nullptr;
    QLabel *m_playerGoldLabel = nullptr;
    RelicBarWidget *m_relicBar = nullptr;
    QLabel *m_potionSlots[3] = {nullptr, nullptr, nullptr};
    QString m_potionSlotIds[3];
    int m_selectedPotionSlotIndex = -1;
    QPushButton *m_newGameButton = nullptr;
    QPushButton *m_menuReturnButton = nullptr;

    CardSelectionContext m_cardSelectionContext = CardSelectionContext::None;
    int m_cardSelectionReturnPage = 0;
};
