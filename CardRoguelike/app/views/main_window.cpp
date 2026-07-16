#include "main_window.h"

#include <QtWidgets/QHBoxLayout>
#include <QtCore/QFile>
#include <QtCore/QMetaType>
#include <QtGui/QCursor>
#include <QtGui/QFont>
#include <QtGui/QMouseEvent>
#include <QtGui/QPalette>
#include <QtGui/QPixmap>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QEvent>

#include "battle/entities/actor.h"
#include "battle/entities/player.h"
#include "events/event_database.h"
#include "game_manager.h"
#include "potions/potion_database.h"
#include "viewmodels/battle_viewmodel.h"
#include "viewmodels/campfire_viewmodel.h"
#include "viewmodels/card_collection_viewmodel.h"
#include "viewmodels/event_viewmodel.h"
#include "viewmodels/map_viewmodel.h"
#include "views/battle_view.h"
#include "views/campfire_view.h"
#include "views/card_collection_view.h"
#include "views/death_view.h"
#include "views/event_view.h"
#include "views/map_view.h"
#include "views/reward_view.h"
#include "views/shop_view.h"
#include "viewmodels/reward_viewmodel.h"
#include "viewmodels/shop_viewmodel.h"
#include "widgets/relic_bar_widget.h"
#include "utils/art_assets.h"

namespace {

enum PageIndex {
    MainMenuPage = 0,
    MapPage = 1,
    BattlePage = 2,
    SettlementPage = 3,
    RewardPage = 4,
    DeathPage = 5,
    EventPage = 6,
    ShopPage = 7,
    CampfirePage = 8,
    CardCollectionPage = 9
};

constexpr int kMapPanelX = 980;
constexpr int kMapPanelWidth = 800;
constexpr int kMapTopBoxY = 40;
constexpr int kMapTopBoxHeight = 100;
constexpr int kMapY = 140;
constexpr int kMapSize = 800;
constexpr int kMapBottomBoxY = 940;
constexpr int kMapBottomBoxHeight = 100;

constexpr int kPlayerPanelX = 140;
constexpr int kPlayerPanelWidth = 500;
constexpr int kPortraitY = 290;
constexpr int kPortraitSize = 500;

constexpr int kTopStatusBarX = 0;
constexpr int kTopStatusBarY = 0;
constexpr int kTopStatusBarWidth = 1920;
constexpr int kTopStatusBarHeight = 45;

constexpr int kRelicBarY = 45;
constexpr int kRelicY = 20;
constexpr int kRelicSize = 30;

void configureStatLabel(QLabel *label)
{
    if (label == nullptr) {
        return;
    }

    QFont font = label->font();
    font.setPixelSize(20);
    label->setFont(font);

    QPalette palette = label->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    label->setPalette(palette);

    label->setStyleSheet(
        QStringLiteral("color: #ffffff; background-color: transparent; border: none;"));
    label->setAutoFillBackground(false);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    label->setMinimumHeight(kTopStatusBarHeight);
}

// Qt QSS does not support background-size:cover; crop-scale like the main menu instead.
void installGameplayBackground(QWidget *page)
{
    if (page == nullptr) {
        return;
    }

    page->setObjectName(QStringLiteral("gameplayPage"));
    page->setAttribute(Qt::WA_StyledBackground, true);

    auto *background = new QLabel(page);
    background->setObjectName(QStringLiteral("gameplayBackground"));
    background->setGeometry(0, 0, 1920, 1080);
    background->setAlignment(Qt::AlignCenter);
    background->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    const QPixmap source(QStringLiteral(":/art/background/background.png"));
    if (!source.isNull()) {
        const QSize target(1920, 1080);
        QPixmap scaled = source.scaled(target, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const int cropX = qMax(0, (scaled.width() - target.width()) / 2);
        const int cropY = qMax(0, (scaled.height() - target.height()) / 2);
        background->setPixmap(scaled.copy(cropX, cropY, target.width(), target.height()));
    } else {
        background->setStyleSheet(
            QStringLiteral("background-color: #1a1a2e; "
                           "background-image: url(:/art/background/background.png); "
                           "background-repeat: no-repeat; background-position: center;"));
    }
    background->lower();
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_gameManager(new GameManager(this))
{
    qRegisterMetaType<EventData>("EventData");

    setWindowTitle(QStringLiteral("CardRoguelike"));
    setFixedSize(1920, 1080);

    applyDarkTheme();

    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    setupMainMenu();
    setupSettlement();

    m_mapPage = new QWidget(this);
    m_mapPage->setFixedSize(1920, 1080);
    installGameplayBackground(m_mapPage);

    m_mapViewModel = new MapViewModel(m_gameManager, this);
    setupMapPanel();
    setupPlayerPanel();
    setupRelicBar();

    m_battlePage = new QWidget(this);
    m_battlePage->setFixedSize(1920, 1080);
    installGameplayBackground(m_battlePage);

    m_rewardPage = new QWidget(this);
    m_rewardPage->setFixedSize(1920, 1080);
    installGameplayBackground(m_rewardPage);

    m_deathPage = new QWidget(this);
    m_deathPage->setFixedSize(1920, 1080);
    installGameplayBackground(m_deathPage);

    m_eventPage = new QWidget(this);
    m_eventPage->setFixedSize(1920, 1080);
    installGameplayBackground(m_eventPage);

    m_shopPage = new QWidget(this);
    m_shopPage->setFixedSize(1920, 1080);
    installGameplayBackground(m_shopPage);

    m_campfirePage = new QWidget(this);
    m_campfirePage->setFixedSize(1920, 1080);
    installGameplayBackground(m_campfirePage);

    m_cardCollectionPage = new QWidget(this);
    m_cardCollectionPage->setFixedSize(1920, 1080);
    installGameplayBackground(m_cardCollectionPage);

    m_stack->addWidget(m_mainMenuPage);
    m_stack->addWidget(m_mapPage);
    m_stack->addWidget(m_battlePage);
    m_stack->addWidget(m_settlementPage);
    m_stack->addWidget(m_rewardPage);
    m_stack->addWidget(m_deathPage);
    m_stack->addWidget(m_eventPage);
    m_stack->addWidget(m_shopPage);
    m_stack->addWidget(m_campfirePage);
    m_stack->addWidget(m_cardCollectionPage);

    connect(m_mapViewModel, &MapViewModel::mapReady, this, &MainWindow::onMapReady);
    connect(m_mapViewModel, &MapViewModel::mapReady, this, &MainWindow::refreshPlayerPanel);
    connect(m_mapViewModel, &MapViewModel::combatTriggered, this, &MainWindow::onCombatTriggered);
    connect(m_mapViewModel, &MapViewModel::eventTriggered, this, &MainWindow::onEventTriggered);
    connect(m_mapViewModel, &MapViewModel::shopTriggered, this, &MainWindow::onShopTriggered);
    connect(m_mapViewModel, &MapViewModel::campfireReached, this, &MainWindow::onCampfireTriggered);
    connect(m_mapViewModel, &MapViewModel::gameOver, this, &MainWindow::onGameOver);
    connect(m_mapViewModel, &MapViewModel::totalStepsChanged, this, &MainWindow::refreshMapHud);
    connect(m_mapViewModel, &MapViewModel::nextShrinkStepChanged, this, &MainWindow::refreshMapHud);
    connect(m_mapViewModel, &MapViewModel::shrinkCountChanged, this, &MainWindow::refreshMapHud);
    connect(m_mapViewModel, &MapViewModel::playerStatsChanged, this, &MainWindow::refreshPlayerPanel);
    connect(m_gameManager, &GameManager::gameOver, this, &MainWindow::onGameOver);

    showMainMenu();
}

MainWindow::~MainWindow() = default;

void MainWindow::applyDarkTheme()
{
    QFile styleFile(QStringLiteral(":/resources/dark_theme.qss"));
    if (styleFile.open(QIODevice::ReadOnly)) {
        setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    } else {
        setStyleSheet(QStringLiteral("QMainWindow { background-color: #1a1a2e; color: #e0e0e0; }"));
    }
}

void MainWindow::setupMainMenu()
{
    m_mainMenuPage = new QWidget(this);
    m_mainMenuPage->setFixedSize(1920, 1080);

    auto *background = new QLabel(m_mainMenuPage);
    background->setObjectName(QStringLiteral("mainMenuBackground"));
    background->setGeometry(0, 0, 1920, 1080);
    background->setAlignment(Qt::AlignCenter);

    const QPixmap source(QStringLiteral(":/art/background/main_theme.webp"));
    if (!source.isNull()) {
        const QSize target(1920, 1080);
        QPixmap scaled = source.scaled(target, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const int cropX = qMax(0, (scaled.width() - target.width()) / 2);
        const int cropY = qMax(0, (scaled.height() - target.height()) / 2);
        background->setPixmap(scaled.copy(cropX, cropY, target.width(), target.height()));
    } else {
        background->setStyleSheet(QStringLiteral("background-color: #1a1a2e;"));
    }
    background->lower();

    auto *layout = new QVBoxLayout(m_mainMenuPage);
    layout->setAlignment(Qt::AlignCenter);

    m_newGameButton = new QPushButton(QStringLiteral("New Game"), m_mainMenuPage);
    m_newGameButton->setMinimumWidth(200);
    connect(m_newGameButton, &QPushButton::clicked, this, &MainWindow::onNewGameClicked);

    // Equal stretches center the button; +600 spacing shifts it 300px downward.
    layout->addStretch();
    layout->addSpacing(600);
    layout->addWidget(m_newGameButton, 0, Qt::AlignCenter);
    layout->addStretch();
}

void MainWindow::setupSettlement()
{
    m_settlementPage = new QWidget(this);
    m_settlementPage->setFixedSize(1920, 1080);
    installGameplayBackground(m_settlementPage);

    auto *layout = new QVBoxLayout(m_settlementPage);
    layout->setAlignment(Qt::AlignCenter);

    m_settlementTitle = new QLabel(m_settlementPage);
    m_settlementTitle->setObjectName(QStringLiteral("titleLabel"));
    m_settlementTitle->setAlignment(Qt::AlignCenter);

    m_settlementStats = new QLabel(m_settlementPage);
    m_settlementStats->setAlignment(Qt::AlignCenter);
    m_settlementStats->setWordWrap(true);

    m_menuReturnButton = new QPushButton(QStringLiteral("Return to Menu"), m_settlementPage);
    connect(m_menuReturnButton, &QPushButton::clicked, this, &MainWindow::showMainMenu);

    layout->addStretch();
    layout->addWidget(m_settlementTitle);
    layout->addWidget(m_settlementStats);
    layout->addSpacing(16);
    layout->addWidget(m_menuReturnButton, 0, Qt::AlignCenter);
    layout->addStretch();
}

void MainWindow::setupMapPanel()
{
    m_mapStepLabel = new QLabel(m_mapPage);
    m_mapStepLabel->setObjectName(QStringLiteral("mapStepLabel"));
    m_mapStepLabel->setGeometry(kMapPanelX, kMapTopBoxY, kMapPanelWidth, kMapTopBoxHeight);
    m_mapStepLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_mapLegendBar = new QWidget(m_mapPage);
    m_mapLegendBar->setObjectName(QStringLiteral("mapLegendBar"));
    m_mapLegendBar->setGeometry(kMapPanelX, kMapBottomBoxY, kMapPanelWidth, kMapBottomBoxHeight);
    m_mapLegendBar->setAttribute(Qt::WA_StyledBackground, true);

    auto *legendLayout = new QHBoxLayout(m_mapLegendBar);
    legendLayout->setContentsMargins(10, 10, 10, 10);
    legendLayout->setSpacing(20);
    legendLayout->addStretch();

    const QSize iconSize(32, 32);
    QFont legendFont;
    legendFont.setPixelSize(14);

    struct LegendEntry {
        QPixmap icon;
        QString name;
    };
    const LegendEntry entries[] = {
        {ArtAssets::mapNodeIcon(QStringLiteral("N"), iconSize), QStringLiteral("Normal Enemy")},
        {ArtAssets::mapNodeIcon(QStringLiteral("E"), iconSize), QStringLiteral("Elite")},
        {ArtAssets::mapNodeIcon(QStringLiteral("B"), iconSize), QStringLiteral("Boss")},
        {ArtAssets::mapNodeIcon(QStringLiteral("?"), iconSize), QStringLiteral("Event")},
        {ArtAssets::mapNodeIcon(QStringLiteral("F"), iconSize), QStringLiteral("Campfire")},
        {ArtAssets::mapNodeIcon(QStringLiteral("S"), iconSize), QStringLiteral("Shop")},
    };

    for (const LegendEntry &entry : entries) {
        auto *item = new QWidget(m_mapLegendBar);
        auto *row = new QHBoxLayout(item);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(6);

        auto *iconLabel = new QLabel(item);
        iconLabel->setFixedSize(iconSize);
        iconLabel->setAlignment(Qt::AlignCenter);
        if (!entry.icon.isNull()) {
            iconLabel->setPixmap(entry.icon);
        }

        auto *textLabel = new QLabel(entry.name, item);
        textLabel->setObjectName(QStringLiteral("mapLegendItemLabel"));
        textLabel->setFont(legendFont);

        row->addWidget(iconLabel);
        row->addWidget(textLabel);
        legendLayout->addWidget(item, 0, Qt::AlignVCenter);
    }
    legendLayout->addStretch();

    m_mapView = new MapView(m_mapViewModel, m_mapPage);
    m_mapView->setGeometry(kMapPanelX, kMapY, kMapSize, kMapSize);

    refreshMapHud();
}

void MainWindow::setupPlayerPanel()
{
    // Persistent overlay on MainWindow so HP/gold/potions survive QStackedWidget page changes.
    m_statusBar = new QWidget(this);
    m_statusBar->setObjectName(QStringLiteral("playerStatusBar"));
    m_statusBar->setGeometry(kTopStatusBarX, kTopStatusBarY, kTopStatusBarWidth, kTopStatusBarHeight);
    m_statusBar->setAttribute(Qt::WA_StyledBackground, true);

    auto *statusLayout = new QHBoxLayout(m_statusBar);
    statusLayout->setContentsMargins(20, 0, 20, 0);
    statusLayout->setSpacing(0);

    m_playerHpLabel = new QLabel(QStringLiteral("HP: --/--"), m_statusBar);
    m_playerHpLabel->setObjectName(QStringLiteral("playerHpLabel"));
    configureStatLabel(m_playerHpLabel);

    m_playerGoldLabel = new QLabel(QStringLiteral("Gold: --"), m_statusBar);
    m_playerGoldLabel->setObjectName(QStringLiteral("playerGoldLabel"));
    configureStatLabel(m_playerGoldLabel);

    statusLayout->addWidget(m_playerHpLabel, 0, Qt::AlignVCenter);
    statusLayout->addSpacing(16);
    statusLayout->addWidget(m_playerGoldLabel, 0, Qt::AlignVCenter);
    statusLayout->addStretch(1);

    for (int i = 0; i < 3; ++i) {
        m_potionSlots[i] = new QLabel(m_statusBar);
        m_potionSlots[i]->setObjectName(QStringLiteral("potionSlot"));
        m_potionSlots[i]->setFixedSize(30, 30);
        m_potionSlots[i]->setAlignment(Qt::AlignCenter);
        m_potionSlots[i]->setCursor(Qt::PointingHandCursor);
        m_potionSlots[i]->installEventFilter(this);
        m_potionSlotIds[i].clear();
        applyPotionSlotStyle(i, false);
        m_potionSlots[i]->hide();
        statusLayout->addWidget(m_potionSlots[i], 0, Qt::AlignVCenter);
        if (i < 2) {
            statusLayout->addSpacing(5);
        }
    }

    m_portraitPlaceholder = new QFrame(m_mapPage);
    m_portraitPlaceholder->setObjectName(QStringLiteral("portraitPlaceholder"));
    m_portraitPlaceholder->setGeometry(kPlayerPanelX, kPortraitY, kPortraitSize, kPortraitSize);
    m_portraitPlaceholder->setStyleSheet(
        QStringLiteral("QFrame#portraitPlaceholder { background: transparent; border: none; }"));
    m_portraitPlaceholder->setAttribute(Qt::WA_TranslucentBackground);

    auto *portraitLabel = new QLabel(QStringLiteral("Portrait"), m_portraitPlaceholder);
    portraitLabel->setObjectName(QStringLiteral("portraitPlaceholderLabel"));
    portraitLabel->setAlignment(Qt::AlignCenter);
    portraitLabel->setGeometry(0, 0, kPortraitSize, kPortraitSize);
    portraitLabel->setStyleSheet(
        QStringLiteral("QLabel#portraitPlaceholderLabel { background: transparent; border: none; }"));
    ArtAssets::applyToLabel(portraitLabel,
                            ArtAssets::playerPortrait(QSize(kPortraitSize, kPortraitSize)),
                            QStringLiteral("Portrait"));

    m_portraitPlaceholder->raise();
    m_statusBar->hide();
}

void MainWindow::setupRelicBar()
{
    m_relicBar = new RelicBarWidget(this);
    m_relicBar->setGeometry(0, kRelicBarY, width(), kRelicY + kRelicSize);
    m_relicBar->hide();
}

void MainWindow::refreshRelicBar()
{
    if (m_relicBar == nullptr || m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        if (m_relicBar != nullptr) {
            m_relicBar->setRelics({});
        }
        return;
    }

    if (m_mapViewModel != nullptr) {
        m_relicBar->setRelics(m_mapViewModel->relics());
    }
}

void MainWindow::updateRelicBarVisibility()
{
    if (m_relicBar == nullptr || m_stack == nullptr) {
        return;
    }

    const int pageIndex = m_stack->currentIndex();
    const bool onMainMenu = pageIndex == MainMenuPage;

    if (m_statusBar != nullptr) {
        m_statusBar->setVisible(!onMainMenu);
        if (!onMainMenu) {
            m_statusBar->raise();
        }
    }

    const bool showRelicBar =
        pageIndex == MapPage || pageIndex == BattlePage || pageIndex == RewardPage
        || pageIndex == EventPage || pageIndex == ShopPage || pageIndex == CampfirePage
        || pageIndex == SettlementPage || pageIndex == DeathPage;
    m_relicBar->setVisible(showRelicBar);
    if (showRelicBar) {
        m_relicBar->raise();
        refreshRelicBar();
    }

    if (m_statusBar != nullptr && m_statusBar->isVisible()) {
        m_statusBar->raise();
    }
}

void MainWindow::refreshPlayerPanel()
{
    if (m_mapViewModel == nullptr || m_playerHpLabel == nullptr || m_playerGoldLabel == nullptr) {
        return;
    }

    connectPlayerSignals();

    Player *player = m_gameManager != nullptr ? m_gameManager->player() : nullptr;
    if (player == nullptr) {
        m_playerHpLabel->setText(QStringLiteral("HP: --/--"));
        m_playerGoldLabel->setText(QStringLiteral("Gold: --"));
        m_selectedPotionSlotIndex = -1;
        for (int i = 0; i < 3; ++i) {
            m_potionSlotIds[i].clear();
            if (m_potionSlots[i] != nullptr) {
                m_potionSlots[i]->clear();
                m_potionSlots[i]->setPixmap(QPixmap());
                m_potionSlots[i]->setToolTip(QString());
                applyPotionSlotStyle(i, false);
                m_potionSlots[i]->hide();
            }
        }
        refreshRelicBar();
        return;
    }

    m_playerHpLabel->setText(
        QStringLiteral("HP: %1/%2").arg(m_mapViewModel->playerHp()).arg(m_mapViewModel->playerMaxHp()));
    m_playerGoldLabel->setText(QStringLiteral("Gold: %1").arg(m_mapViewModel->gold()));

    if (QLayout *layout = m_statusBar != nullptr ? m_statusBar->layout() : nullptr) {
        layout->invalidate();
        layout->activate();
    }

    for (int i = 0; i < 3; ++i) {
        m_potionSlotIds[i].clear();
        if (m_potionSlots[i] != nullptr) {
            m_potionSlots[i]->clear();
            m_potionSlots[i]->setPixmap(QPixmap());
            m_potionSlots[i]->setToolTip(QString());
            applyPotionSlotStyle(i, false);
            m_potionSlots[i]->hide();
        }
    }

    PotionDatabase::instance().load();
    int slotIndex = 0;
    for (auto it = player->potionCounts().constBegin(); it != player->potionCounts().constEnd(); ++it) {
        if (it.value() <= 0 || slotIndex >= 3) {
            continue;
        }

        const PotionData potion = PotionDatabase::instance().getPotion(it.key());
        const QString letter = potion.name.isEmpty() ? it.key().left(1).toUpper()
                                                     : potion.name.left(1).toUpper();
        ArtAssets::applyToLabel(m_potionSlots[slotIndex],
                                ArtAssets::potionIcon(it.key(), QSize(30, 30)), letter);
        m_potionSlots[slotIndex]->setToolTip(
            potion.name.isEmpty() ? it.key() : QStringLiteral("%1 x%2").arg(potion.name).arg(it.value()));
        m_potionSlotIds[slotIndex] = it.key();
        const bool selected = (slotIndex == m_selectedPotionSlotIndex);
        applyPotionSlotStyle(slotIndex, selected);
        m_potionSlots[slotIndex]->show();
        ++slotIndex;
    }

    if (m_selectedPotionSlotIndex >= slotIndex) {
        m_selectedPotionSlotIndex = -1;
    }

    refreshRelicBar();
}

void MainWindow::applyPotionSlotStyle(int index, bool selected)
{
    if (index < 0 || index >= 3 || m_potionSlots[index] == nullptr) {
        return;
    }

    if (selected) {
        m_potionSlots[index]->setStyleSheet(
            QStringLiteral("QLabel#potionSlot { background-color: #2a2a3e; border: 2px solid blue; }"));
    } else {
        m_potionSlots[index]->setStyleSheet(
            QStringLiteral("QLabel#potionSlot { background-color: #2a2a3e; border: 1px solid #0f3460; }"));
    }
}

void MainWindow::onPotionTargetingChanged(int statusBarSlotIndex, bool active)
{
    if (m_selectedPotionSlotIndex >= 0 && m_selectedPotionSlotIndex < 3) {
        applyPotionSlotStyle(m_selectedPotionSlotIndex, false);
    }

    m_selectedPotionSlotIndex = active ? statusBarSlotIndex : -1;
    if (active && statusBarSlotIndex >= 0 && statusBarSlotIndex < 3) {
        applyPotionSlotStyle(statusBarSlotIndex, true);
    }
}

void MainWindow::handlePotionSlotClicked(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 3 || m_potionSlotIds[slotIndex].isEmpty()) {
        return;
    }

    if (m_stack == nullptr || m_stack->currentIndex() != BattlePage || m_battleView == nullptr
        || m_battleViewModel == nullptr) {
        return;
    }

    const QString potionId = m_potionSlotIds[slotIndex];
    if (!m_battleViewModel->canUsePotion(potionId)) {
        return;
    }

    QLabel *slot = m_potionSlots[slotIndex];
    if (slot == nullptr) {
        return;
    }

    const QPoint globalCenter = slot->mapToGlobal(slot->rect().center());
    const QPoint startInBattle = m_battleView->mapFromGlobal(globalCenter);
    m_battleView->requestPotionUse(potionId, slotIndex, startInBattle);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event != nullptr && event->type() == QEvent::MouseButtonPress) {
        for (int i = 0; i < 3; ++i) {
            if (watched == m_potionSlots[i]) {
                auto *mouseEvent = static_cast<QMouseEvent *>(event);
                if (mouseEvent->button() == Qt::LeftButton) {
                    handlePotionSlotClicked(i);
                    return true;
                }
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::connectPlayerSignals()
{
    if (m_gameManager == nullptr) {
        return;
    }

    Player *player = m_gameManager->player();
    if (player == nullptr) {
        return;
    }

    connect(player, &Actor::hpChanged, this, &MainWindow::refreshPlayerPanel, Qt::UniqueConnection);
}

void MainWindow::refreshMapHud()
{
    if (m_mapStepLabel == nullptr || m_mapViewModel == nullptr) {
        return;
    }

    m_mapStepLabel->setText(
        QStringLiteral("Steps: %1/%2  |  Shrink: %3")
            .arg(m_mapViewModel->totalSteps())
            .arg(m_mapViewModel->nextShrinkStep())
            .arg(m_mapViewModel->shrinkCount()));
}

void MainWindow::showMainMenu()
{
    teardownBattle();
    teardownReward();
    teardownEvent();
    teardownShop();
    teardownCampfire();
    teardownCardCollection();
    m_cardSelectionContext = CardSelectionContext::None;
    m_stack->setCurrentIndex(MainMenuPage);
    updateRelicBarVisibility();
}

void MainWindow::showMap()
{
    m_stack->setCurrentIndex(MapPage);
    refreshMapHud();
    refreshPlayerPanel();
    if (m_mapView != nullptr) {
        m_mapView->updateFromViewModel();
    }
    updateRelicBarVisibility();
}

void MainWindow::showBattle()
{
    m_stack->setCurrentIndex(BattlePage);
    updateRelicBarVisibility();
}

void MainWindow::showSettlement(bool victory, int steps, int gold, int hp)
{
    m_settlementTitle->setText(victory ? QStringLiteral("Victory!") : QStringLiteral("Defeat"));
    m_settlementStats->setText(QStringLiteral("Steps: %1\nGold: %2\nHP remaining: %3")
                                   .arg(steps)
                                   .arg(gold)
                                   .arg(hp));
    m_stack->setCurrentIndex(SettlementPage);
    updateRelicBarVisibility();
}

void MainWindow::onNewGameClicked()
{
    if (m_mapViewModel != nullptr) {
        m_mapViewModel->startNewGame();
    }
}

void MainWindow::onMapReady()
{
    showMap();
}

void MainWindow::onCombatTriggered()
{
    teardownBattle();
    teardownReward();
    teardownEvent();
    teardownShop();

    m_battleViewModel = new BattleViewModel(m_gameManager, this);
    connect(m_battleViewModel, &BattleViewModel::combatEnded, this, &MainWindow::onBattleEnded,
            Qt::QueuedConnection);
    connect(m_battleViewModel, &BattleViewModel::relicsChanged, this, &MainWindow::refreshRelicBar);
    connect(m_battleViewModel, &BattleViewModel::potionsChanged, this, &MainWindow::refreshPlayerPanel);
    connect(m_battleViewModel, &BattleViewModel::playerHpChanged, this, &MainWindow::refreshPlayerPanel);
    connect(m_battleViewModel, &BattleViewModel::playerMaxHpChanged, this,
            &MainWindow::refreshPlayerPanel);

    m_battleView = new BattleView(m_battleViewModel, m_battlePage);
    m_battleView->setGeometry(0, 0, 1920, 1080);
    m_battleView->show();
    connect(m_battleView, &BattleView::potionTargetingChanged, this,
            &MainWindow::onPotionTargetingChanged);

    m_battleViewModel->startCombat();
    showBattle();
}

void MainWindow::onEventTriggered(const EventData &eventData)
{
    teardownEvent();
    teardownShop();
    teardownCampfire();

    m_eventViewModel = new EventViewModel(m_mapViewModel, this);
    m_eventViewModel->setEvent(eventData);

    m_eventView = new EventView(m_eventViewModel, m_eventPage);
    m_eventView->setGeometry(0, 0, 1920, 1080);
    m_eventView->show();
    connect(m_eventView, &EventView::exitClicked, this, &MainWindow::onEventExit);
    connect(m_eventViewModel, &EventViewModel::cardSelectionNeeded, this,
            &MainWindow::onEventCardSelectionNeeded);

    m_stack->setCurrentIndex(EventPage);
    updateRelicBarVisibility();
    refreshPlayerPanel();
}

void MainWindow::onEventExit()
{
    if (m_mapViewModel != nullptr) {
        m_mapViewModel->refreshFromGameManager();
    }

    teardownEvent();

    if (m_stack->currentIndex() != SettlementPage && m_stack->currentIndex() != DeathPage) {
        showMap();
        refreshPlayerPanel();
    }
}

void MainWindow::onEventCardSelectionNeeded(const QString &actionType)
{
    openCardSelection(actionType, CardSelectionContext::EventAction);
}

void MainWindow::onShopTriggered()
{
    teardownShop();
    teardownCampfire();

    m_shopViewModel = new ShopViewModel(m_gameManager, this);
    connect(m_shopViewModel, &ShopViewModel::shopDataChanged, this, &MainWindow::refreshRelicBar);
    connect(m_shopViewModel, &ShopViewModel::goldChanged, this, &MainWindow::refreshPlayerPanel);
    m_shopView = new ShopView(m_shopViewModel, m_shopPage);
    m_shopView->setGeometry(0, 0, 1920, 1080);
    m_shopView->show();
    connect(m_shopView, &ShopView::leaveClicked, this, &MainWindow::onShopLeave);
    connect(m_shopView, &ShopView::deleteCardClicked, this, &MainWindow::onShopDeleteCardClicked);

    m_stack->setCurrentIndex(ShopPage);
    updateRelicBarVisibility();
    refreshPlayerPanel();
}

void MainWindow::onShopLeave()
{
    if (m_mapViewModel != nullptr) {
        m_mapViewModel->markCurrentNodeExplored();
        m_mapViewModel->refreshFromGameManager();
    }

    teardownShop();

    if (m_stack->currentIndex() != SettlementPage && m_stack->currentIndex() != DeathPage) {
        showMap();
        refreshPlayerPanel();
    }
}

void MainWindow::onShopDeleteCardClicked()
{
    openCardSelection(QStringLiteral("remove"), CardSelectionContext::ShopRemove);
}

void MainWindow::onCampfireTriggered()
{
    teardownCampfire();
    teardownShop();
    teardownEvent();

    m_campfireViewModel = new CampfireViewModel(m_gameManager, this);
    m_campfireView = new CampfireView(m_campfireViewModel, m_campfirePage);
    m_campfireView->setGeometry(0, 0, 1920, 1080);
    m_campfireView->show();
    connect(m_campfireView, &CampfireView::restClicked, this, &MainWindow::onCampfireRest);
    connect(m_campfireView, &CampfireView::upgradeClicked, this, &MainWindow::onCampfireUpgrade);

    m_stack->setCurrentIndex(CampfirePage);
    updateRelicBarVisibility();
    refreshPlayerPanel();
}

void MainWindow::onCampfireRest()
{
    if (m_mapViewModel != nullptr) {
        m_mapViewModel->handleCampfireChoice(0);
        m_mapViewModel->refreshFromGameManager();
    }
    teardownCampfire();
    showMap();
    refreshPlayerPanel();
}

void MainWindow::onCampfireUpgrade()
{
    openCardSelection(QStringLiteral("upgrade"), CardSelectionContext::CampfireUpgrade);
}

void MainWindow::onBattleEnded(bool playerWon)
{
    teardownBattle();

    if (playerWon) {
        teardownReward();
        m_rewardViewModel = new RewardViewModel(m_gameManager, this);
        connect(m_rewardViewModel, &RewardViewModel::offersChanged, this, &MainWindow::refreshRelicBar);
        m_rewardView = new RewardView(m_rewardViewModel, m_rewardPage);
        m_rewardView->setGeometry(0, 0, 1920, 1080);
        m_rewardView->show();
        connect(m_rewardView, &RewardView::backToMapClicked, this, &MainWindow::onRewardBackToMap);
        m_stack->setCurrentIndex(RewardPage);
        updateRelicBarVisibility();
        return;
    }

    teardownReward();
    if (m_deathView == nullptr) {
        m_deathView = new DeathView(m_deathPage);
        m_deathView->setGeometry(0, 0, 1920, 1080);
        connect(m_deathView, &DeathView::returnToMenuClicked, this, &MainWindow::onDeathReturnToMenu);
    }
    m_deathView->show();
    m_stack->setCurrentIndex(DeathPage);
    updateRelicBarVisibility();
}

void MainWindow::onRewardBackToMap()
{
    if (m_gameManager != nullptr) {
        m_gameManager->recordCombatVictory();
    }

    if (m_mapViewModel != nullptr) {
        m_mapViewModel->markCurrentNodeExplored();
        m_mapViewModel->refreshFromGameManager();
    }

    teardownReward();

    if (m_stack->currentIndex() != SettlementPage) {
        showMap();
        refreshPlayerPanel();
    }
}

void MainWindow::onDeathReturnToMenu()
{
    teardownReward();
    teardownEvent();
    teardownShop();
    teardownCampfire();
    showMainMenu();
}

void MainWindow::onGameOver(bool victory)
{
    const Player *player = m_gameManager->player();
    const int gold = player != nullptr ? player->gold() : 0;
    const int hp = player != nullptr ? player->currentHp() : 0;
    const int steps = m_gameManager->mapManager()->totalSteps();

    teardownBattle();
    teardownEvent();
    teardownShop();
    teardownCampfire();
    showSettlement(victory, steps, gold, hp);
}

void MainWindow::teardownBattle()
{
    m_selectedPotionSlotIndex = -1;
    for (int i = 0; i < 3; ++i) {
        applyPotionSlotStyle(i, false);
    }

    if (m_battleView != nullptr) {
        m_battleView->deleteLater();
        m_battleView = nullptr;
    }

    if (m_battleViewModel != nullptr) {
        m_battleViewModel->deleteLater();
        m_battleViewModel = nullptr;
    }
}

void MainWindow::teardownReward()
{
    if (m_rewardView != nullptr) {
        m_rewardView->deleteLater();
        m_rewardView = nullptr;
    }

    if (m_rewardViewModel != nullptr) {
        m_rewardViewModel->deleteLater();
        m_rewardViewModel = nullptr;
    }
}

void MainWindow::teardownEvent()
{
    if (m_eventView != nullptr) {
        m_eventView->deleteLater();
        m_eventView = nullptr;
    }

    if (m_eventViewModel != nullptr) {
        m_eventViewModel->deleteLater();
        m_eventViewModel = nullptr;
    }
}

void MainWindow::teardownShop()
{
    if (m_shopView != nullptr) {
        m_shopView->deleteLater();
        m_shopView = nullptr;
    }

    if (m_shopViewModel != nullptr) {
        m_shopViewModel->deleteLater();
        m_shopViewModel = nullptr;
    }
}

void MainWindow::teardownCampfire()
{
    if (m_campfireView != nullptr) {
        m_campfireView->deleteLater();
        m_campfireView = nullptr;
    }

    if (m_campfireViewModel != nullptr) {
        m_campfireViewModel->deleteLater();
        m_campfireViewModel = nullptr;
    }
}

void MainWindow::teardownCardCollection()
{
    if (m_cardCollectionView != nullptr) {
        m_cardCollectionView->deleteLater();
        m_cardCollectionView = nullptr;
    }

    if (m_cardCollectionViewModel != nullptr) {
        m_cardCollectionViewModel->deleteLater();
        m_cardCollectionViewModel = nullptr;
    }
}

void MainWindow::openCardSelection(const QString &actionType, CardSelectionContext context)
{
    m_cardSelectionContext = context;
    m_cardSelectionReturnPage = m_stack->currentIndex();

    teardownCardCollection();
    m_cardCollectionViewModel = new CardCollectionViewModel(m_gameManager, this);
    m_cardCollectionView = new CardCollectionView(m_cardCollectionViewModel, m_cardCollectionPage);
    m_cardCollectionView->setGeometry(0, 0, 1920, 1080);
    connect(m_cardCollectionView, &CardCollectionView::cardSelected, this,
            &MainWindow::onCardSelected);
    connect(m_cardCollectionView, &CardCollectionView::selectionCancelled, this,
            &MainWindow::onCardSelectionCancelled);
    connect(m_cardCollectionView, &CardCollectionView::exitClicked, this,
            &MainWindow::onCardCollectionExit);

    m_cardCollectionView->startSelectionMode(actionType);
    m_stack->setCurrentIndex(CardCollectionPage);
    updateRelicBarVisibility();
}

void MainWindow::returnFromCardSelection()
{
    teardownCardCollection();
    const int returnPage = m_cardSelectionReturnPage;
    m_cardSelectionContext = CardSelectionContext::None;
    m_stack->setCurrentIndex(returnPage);
    updateRelicBarVisibility();
    refreshPlayerPanel();
}

void MainWindow::showCardCollectionBrowse()
{
    teardownCardCollection();

    if (m_gameManager != nullptr && m_gameManager->player() == nullptr) {
        m_gameManager->startNewGame(QStringLiteral("ironclad"));
    }

    m_cardSelectionContext = CardSelectionContext::Browse;
    m_cardSelectionReturnPage = MainMenuPage;

    m_cardCollectionViewModel = new CardCollectionViewModel(m_gameManager, this);
    m_cardCollectionView = new CardCollectionView(m_cardCollectionViewModel, m_cardCollectionPage);
    m_cardCollectionView->setGeometry(0, 0, 1920, 1080);
    m_cardCollectionView->show();
    connect(m_cardCollectionView, &CardCollectionView::exitClicked, this,
            &MainWindow::onCardCollectionExit);

    m_stack->setCurrentIndex(CardCollectionPage);
    updateRelicBarVisibility();
}

void MainWindow::onCardCollectionClicked()
{
    showCardCollectionBrowse();
}

void MainWindow::onCardCollectionExit()
{
    teardownCardCollection();
    m_cardSelectionContext = CardSelectionContext::None;
    m_stack->setCurrentIndex(MainMenuPage);
    updateRelicBarVisibility();
}

void MainWindow::onCardSelected(const QString &cardId, int deckIndex)
{
    Q_UNUSED(cardId);

    switch (m_cardSelectionContext) {
    case CardSelectionContext::CampfireUpgrade:
        if (m_mapViewModel != nullptr) {
            m_mapViewModel->handleCampfireChoice(1, deckIndex);
            m_mapViewModel->refreshFromGameManager();
        }
        teardownCardCollection();
        teardownCampfire();
        m_cardSelectionContext = CardSelectionContext::None;
        showMap();
        refreshPlayerPanel();
        break;

    case CardSelectionContext::ShopRemove:
        if (m_shopViewModel != nullptr) {
            m_shopViewModel->confirmDeleteCard(deckIndex);
        }
        returnFromCardSelection();
        if (m_shopView != nullptr) {
            m_shopView->updateFromViewModel();
        }
        break;

    case CardSelectionContext::EventAction:
        if (m_eventViewModel != nullptr) {
            m_eventViewModel->completePendingOption(deckIndex);
        }
        returnFromCardSelection();
        if (m_eventView != nullptr) {
            m_eventView->updateFromViewModel();
        }
        refreshPlayerPanel();
        break;

    case CardSelectionContext::Browse:
    case CardSelectionContext::None:
    default:
        teardownCardCollection();
        m_cardSelectionContext = CardSelectionContext::None;
        break;
    }
}

void MainWindow::onCardSelectionCancelled()
{
    const int returnPage = m_cardSelectionReturnPage;
    const CardSelectionContext context = m_cardSelectionContext;

    if (context == CardSelectionContext::EventAction && m_eventViewModel != nullptr) {
        m_eventViewModel->cancelPendingOption();
    }

    if (context == CardSelectionContext::Browse) {
        onCardCollectionExit();
        return;
    }

    returnFromCardSelection();

    if (returnPage == CampfirePage && m_campfireView != nullptr) {
        m_campfireView->updateFromViewModel();
    } else if (returnPage == ShopPage && m_shopView != nullptr) {
        m_shopView->updateFromViewModel();
    } else if (returnPage == EventPage && m_eventView != nullptr) {
        m_eventView->updateFromViewModel();
    }
}
