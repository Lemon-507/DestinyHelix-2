#include "battle_view.h"

#include <QtCore/QPropertyAnimation>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QKeyEvent>
#include <QEvent>
#include <QtGui/QFont>
#include <QtGui/QFontMetrics>
#include <QtGui/QMouseEvent>
#include <QtGui/QPalette>
#include <QtGui/QPen>
#include <QtGui/QPixmap>
#include <QtWidgets/QGraphicsPixmapItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSimpleTextItem>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

#include "viewmodels/battle_viewmodel.h"
#include "views/combat_animations.h"
#include "widgets/card_widget.h"
#include "utils/art_assets.h"

namespace {



constexpr int kWindowWidth = 1920;

constexpr int kWindowHeight = 1080;

constexpr int kFlyAnimationDurationMs = 300;
constexpr int kPotionFlyAnimationDurationMs = 500;
constexpr int kPotionIconSize = 30;



constexpr int kPortraitX = 200;

constexpr int kPortraitY = 200;

constexpr int kPortraitSize = 500;



constexpr int kStatusBarY = 120;
constexpr int kStatusBarHeight = 80;
constexpr int kBlockLabelWidth = 95;

void configureStatLabel(QLabel *label, int width)
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
        QStringLiteral("color: #ffffff; background-color: transparent; border: none; "
                       "font-size: 20px;"));
    label->setAutoFillBackground(false);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label->setFixedWidth(width);
    label->setFixedHeight(kStatusBarHeight);
}

constexpr int kCardY = 850;

constexpr int kCardWidth = 100;

constexpr int kCardHeight = 160;

constexpr int kCardSlotXs[10] = {150, 270, 390, 510, 630, 750, 870, 990, 1110, 1230};



constexpr int kRelicY = 20;

constexpr int kRelicSize = 30;

constexpr int kRelicSlotXs[12] = {20,  60,  100, 140, 180, 220,
                                  260, 300, 340, 380, 420, 460};



constexpr int kPotionY = 150;

constexpr int kPotionSize = 30;

constexpr int kPotionSlotXs[3] = {580, 620, 660};

constexpr int kEndTurnWidth = 120;
constexpr int kEndTurnHeight = 40;
constexpr int kEndTurnX = 150;
constexpr int kEndTurnY = kCardY - 15 - kEndTurnHeight;
constexpr int kEnergyX = kEndTurnX + kEndTurnWidth + 20;
constexpr int kEnergyY = kEndTurnY;

constexpr int kPileSize = 30;
constexpr int kDrawPileX = 10;
constexpr int kDrawPileY = kWindowHeight - 10 - kPileSize;
constexpr int kDiscardPileX = kWindowWidth - 10 - kPileSize;
constexpr int kDiscardPileY = kDrawPileY;

constexpr char kPileButtonStyle[] =
    "QPushButton#pileButton { background-color: #2a2a3e; border: 1px solid #0f3460; "
    "color: #e0e0e0; font-size: 11px; padding: 0px; min-height: 0px; }"
    "QPushButton#pileButton:hover { border-color: #e94560; }";

constexpr char kEndTurnDefaultStyle[] =
    "QPushButton { color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
    "font-size: 14px; padding: 0px; min-height: 0px; }"
    "QPushButton:hover { border-color: #e94560; }";

constexpr char kEndTurnHighlightStyle[] =
    "QPushButton { color: #ffffff; background-color: #16213e; border: 2px solid red; "
    "font-size: 14px; padding: 0px; min-height: 0px; }"
    "QPushButton:hover { border: 2px solid red; }";

constexpr char kCardUnaffordableStyle[] =
    "CardWidget#cardWidget { background-color: #0d1220; border: 2px solid #0a0a14; "
    "border-radius: 8px; color: #666666; }";

constexpr int kEnemyArtSizeLarge = 300;
constexpr int kEnemyArtSizeSmall = 150;
constexpr int kEnemyIntentIconSize = 30;
constexpr int kEnemyNameLabelHeight = 30;
constexpr int kEnemyGroupGap = 10;
constexpr int kEnemyArtY = 300;
constexpr int kEnemyStatLabelHeightLarge = 28;
constexpr int kEnemyStatLabelHeightSmall = 22;
constexpr int kEnemyHpLabelWidthLarge = 150;
constexpr int kEnemyBlockLabelWidthLarge = 150;

constexpr int kBuffSize = 30;
constexpr int kPlayerBuffColumnX = kPortraitX + kPortraitSize + 15;
constexpr int kPlayerBuffColumnY = kPortraitY;
constexpr int kPlayerBuffVerticalSpacing = 20;
constexpr int kEnemyBuffHorizontalSpacing = 10;
constexpr int kEnemyBuffOffsetBelowStats = 10;

constexpr char kBuffSlotStyle[] =
    "background-color: #2a2a3e; border: 1px solid #0f3460; color: #e0e0e0; font-size: 10px;";

constexpr char kBuffOverflowStyle[] =
    "background-color: #16213e; border: 1px solid #0f3460; color: #e0e0e0; font-size: 16px;";

constexpr char kEnemyArtStyle[] =
    "background-color: transparent; border: none; color: #666666; font-size: 24px;";

constexpr char kEnemyArtStyleSmall[] =
    "background-color: transparent; border: none; color: #666666; font-size: 14px;";

constexpr char kEnemyArtHoverStyle[] =
    "background-color: transparent; border: 2px solid #e94560; color: #666666; font-size: 24px;";

constexpr char kEnemyArtHoverStyleSmall[] =
    "background-color: transparent; border: 2px solid #e94560; color: #666666; font-size: 14px;";

int enemyArtSizeForCount(int enemyCount)
{
    return enemyCount >= 4 ? kEnemyArtSizeSmall : kEnemyArtSizeLarge;
}

int playerPortraitMidlineX()
{
    return kPortraitX + kPortraitSize / 2;
}

int enemySymmetryMidlineX()
{
    return kWindowWidth - playerPortraitMidlineX();
}

QVector<int> computeEnemyGroupLeftXs(int enemyCount, int artSize)
{
    QVector<int> leftXs;
    if (enemyCount <= 0 || enemyCount > 5) {
        return leftXs;
    }

    const int symmetryX = enemySymmetryMidlineX();
    const int gap = kEnemyGroupGap;
    const int totalWidth = enemyCount * artSize + (enemyCount - 1) * gap;
    leftXs.reserve(enemyCount);

    if (enemyCount % 2 == 1) {
        const int middleIndex = enemyCount / 2;
        const int middleLeft = symmetryX - artSize / 2;
        for (int i = 0; i < enemyCount; ++i) {
            leftXs.append(middleLeft + (i - middleIndex) * (artSize + gap));
        }
        return leftXs;
    }

    const int startX = symmetryX - totalWidth / 2;
    for (int i = 0; i < enemyCount; ++i) {
        leftXs.append(startX + i * (artSize + gap));
    }
    return leftXs;
}

void configureEnemyStatLabel(QLabel *label, Qt::Alignment alignment, int fontSize, int height)
{
    if (label == nullptr) {
        return;
    }

    QFont font = label->font();
    font.setPixelSize(fontSize);
    label->setFont(font);
    label->setStyleSheet(
        QStringLiteral("color: #ffffff; background-color: transparent; border: none; font-size: %1px;")
            .arg(fontSize));
    label->setAlignment(alignment | Qt::AlignVCenter);
    label->setFixedHeight(height);
}

void configureEnemyNameLabel(QLabel *label, int fontSize)
{
    if (label == nullptr) {
        return;
    }

    QFont font = label->font();
    font.setPixelSize(fontSize);
    label->setFont(font);
    label->setStyleSheet(
        QStringLiteral("color: #ffffff; background-color: transparent; border: none; font-size: %1px;")
            .arg(fontSize));
}

void setEnemyNameLabelText(QLabel *label, const QString &name)
{
    if (label == nullptr) {
        return;
    }

    const int availableWidth = label->width();
    if (availableWidth <= 0 || name.isEmpty()) {
        label->setText(name);
        return;
    }

    const QFontMetrics metrics(label->font());
    if (metrics.horizontalAdvance(name) <= availableWidth) {
        label->setText(name);
        return;
    }

    label->setText(metrics.elidedText(name, Qt::ElideRight, availableWidth));
}

int enemyBuffRowY(int artSize)
{
    const bool compactLayout = artSize <= kEnemyArtSizeSmall;
    const int statHeight =
        compactLayout ? kEnemyStatLabelHeightSmall : kEnemyStatLabelHeightLarge;
    const int statsY = kEnemyArtY + artSize;
    if (compactLayout) {
        return statsY + (2 * statHeight) + kEnemyBuffOffsetBelowStats;
    }
    return statsY + statHeight + kEnemyBuffOffsetBelowStats;
}

bool isBuffLabel(const QLabel *label)
{
    return label != nullptr && label->objectName() == QStringLiteral("buffSlot");
}

bool isBuffOverflowLabel(const QLabel *label)
{
    return label != nullptr && label->objectName() == QStringLiteral("buffOverflow");
}

} // namespace



BattleView::BattleView(BattleViewModel *viewModel, QWidget *parent)

    : QWidget(parent)

    , m_viewModel(viewModel)

{
    setFixedSize(kWindowWidth, kWindowHeight);
    setObjectName(QStringLiteral("gameplayScene"));
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(false);
    setFocusPolicy(Qt::StrongFocus);
    setupLayout();
    setupPortrait();

    if (m_viewModel != nullptr) {

        connect(m_viewModel, &BattleViewModel::playerHpChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::playerMaxHpChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::playerBlockChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::enemyHpChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::enemyMaxHpChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::enemyBlockChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::enemiesChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::currentEnergyChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::maxEnergyChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::enemyNameChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::enemyIntentChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::enemyIntentValueChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::handCardsChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::goldChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::relicsChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::potionsChanged, this, &BattleView::updateFromViewModel);
        connect(m_viewModel, &BattleViewModel::playerBuffsChanged, this, &BattleView::updateFromViewModel);
        connect(m_viewModel, &BattleViewModel::drawPileCountChanged, this, &BattleView::updateFromViewModel);
        connect(m_viewModel, &BattleViewModel::discardPileCountChanged, this, &BattleView::updateFromViewModel);

        connect(m_viewModel, &BattleViewModel::combatActiveChanged, this, [this]() {
            if (m_viewModel != nullptr && !m_viewModel->combatActive()) {
                resetCombatLayout();
            }
            updateFromViewModel();
        });
    }



    updateFromViewModel();

}



void BattleView::setupLayout()
{
    const int blockX = kPortraitX + (kPortraitSize - kBlockLabelWidth) / 2;
    m_blockLabel = new QLabel(QStringLiteral("Block: 0"), this);
    m_blockLabel->setObjectName(QStringLiteral("playerBlockLabel"));
    configureStatLabel(m_blockLabel, kBlockLabelWidth);
    m_blockLabel->setGeometry(blockX, kStatusBarY, kBlockLabelWidth, kStatusBarHeight);
    m_blockLabel->setAlignment(Qt::AlignCenter);

    m_hpLabel = nullptr;
    m_goldLabel = nullptr;

    m_portraitView = new QGraphicsView(this);

    m_portraitView->setObjectName(QStringLiteral("playerPortraitView"));
    m_portraitView->setGeometry(kPortraitX, kPortraitY, kPortraitSize, kPortraitSize);
    m_portraitView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_portraitView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_portraitView->setFrameShape(QFrame::NoFrame);
    m_portraitView->setStyleSheet(
        QStringLiteral("QGraphicsView#playerPortraitView { background: transparent; border: none; }"));
    m_portraitView->setAttribute(Qt::WA_TranslucentBackground);
    m_portraitView->viewport()->setAutoFillBackground(false);



    m_cardSlots.reserve(kCardSlotCount);

    for (int i = 0; i < kCardSlotCount; ++i) {

        auto *cardWidget = new CardWidget(this);

        cardWidget->setFixedSize(kCardWidth, kCardHeight);

        cardWidget->setGeometry(kCardSlotXs[i], kCardY, kCardWidth, kCardHeight);

        cardWidget->hide();

        connect(cardWidget, &CardWidget::clicked, this, &BattleView::onCardClicked);

        m_cardSlots.append(cardWidget);

    }



    m_relicSlots.reserve(kRelicSlotCount);

    for (int i = 0; i < kRelicSlotCount; ++i) {

        auto *relicSlot = new QLabel(this);

        relicSlot->setObjectName(QStringLiteral("relicSlot"));

        relicSlot->setGeometry(kRelicSlotXs[i], kRelicY, kRelicSize, kRelicSize);

        relicSlot->setAlignment(Qt::AlignCenter);

        relicSlot->setStyleSheet(

            QStringLiteral("background-color: #2a2a3e; border: 1px solid #0f3460; color: #666666; "

                           "font-size: 10px;"));
        relicSlot->setAttribute(Qt::WA_Hover, true);
        relicSlot->installEventFilter(this);
        relicSlot->hide();

        m_relicSlots.append(relicSlot);

    }



    m_potionSlotIds = QVector<QString>(kPotionSlotCount);

    m_potionSlots.reserve(kPotionSlotCount);

    for (int i = 0; i < kPotionSlotCount; ++i) {

        auto *potionButton = new QPushButton(this);

        potionButton->setObjectName(QStringLiteral("potionSlot"));

        potionButton->setGeometry(kPotionSlotXs[i], kPotionY, kPotionSize, kPotionSize);

        potionButton->setStyleSheet(

            QStringLiteral("QPushButton#potionSlot { background-color: #2a2a3e; border: 1px solid #0f3460; "

                           "color: #e0e0e0; font-size: 14px; padding: 0px; min-height: 0px; }"

                           "QPushButton#potionSlot:hover:enabled { border-color: #e94560; }"

                           "QPushButton#potionSlot:disabled { color: #666666; }"));

        connect(potionButton, &QPushButton::clicked, this, &BattleView::onPotionClicked);
        potionButton->hide();

        m_potionSlots.append(potionButton);

    }



    setupEnemyGroups();
    setupPlayerBuffs();
    m_endTurnButton = new QPushButton(QStringLiteral("End Turn"), this);
    m_endTurnButton->setGeometry(kEndTurnX, kEndTurnY, kEndTurnWidth, kEndTurnHeight);
    m_endTurnButton->setStyleSheet(QString::fromLatin1(kEndTurnDefaultStyle));
    m_endTurnButton->setEnabled(true);
    connect(m_endTurnButton, &QPushButton::clicked, this, &BattleView::onEndTurnClicked);

    m_energyLabel = new QLabel(QStringLiteral("Energy: 3/3"), this);
    m_energyLabel->setObjectName(QStringLiteral("energyLabel"));
    m_energyLabel->setGeometry(kEnergyX, kEnergyY, kEndTurnWidth, kEndTurnHeight);
    m_energyLabel->setAlignment(Qt::AlignCenter);
    m_energyLabel->setStyleSheet(
        QStringLiteral("color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
                       "font-size: 14px;"));

    m_drawPileButton = new QPushButton(this);
    m_drawPileButton->setObjectName(QStringLiteral("pileButton"));
    m_drawPileButton->setGeometry(kDrawPileX, kDrawPileY, kPileSize, kPileSize);
    m_drawPileButton->setStyleSheet(QString::fromLatin1(kPileButtonStyle));
    m_drawPileButton->setToolTip(QStringLiteral("Draw pile"));

    m_discardPileButton = new QPushButton(this);
    m_discardPileButton->setObjectName(QStringLiteral("pileButton"));
    m_discardPileButton->setGeometry(kDiscardPileX, kDiscardPileY, kPileSize, kPileSize);
    m_discardPileButton->setStyleSheet(QString::fromLatin1(kPileButtonStyle));
    m_discardPileButton->setToolTip(QStringLiteral("Discard pile"));
}



void BattleView::setupPortrait()

{

    if (m_portraitView == nullptr) {

        return;

    }



    auto *scene = new QGraphicsScene(m_portraitView);

    scene->setSceneRect(0, 0, kPortraitSize, kPortraitSize);
    scene->setBackgroundBrush(Qt::NoBrush);

    const QPixmap portrait = ArtAssets::playerPortrait(QSize(kPortraitSize, kPortraitSize));
    if (!portrait.isNull()) {
        auto *pixmapItem = scene->addPixmap(portrait);
        const QRectF bounds = pixmapItem->boundingRect();
        pixmapItem->setPos((kPortraitSize - bounds.width()) / 2.0,
                           (kPortraitSize - bounds.height()) / 2.0);
    } else {
        auto *portraitText = scene->addSimpleText(QStringLiteral("Portrait"));

        portraitText->setBrush(QColor(QStringLiteral("#666666")));

        QFont font = portraitText->font();

        font.setPixelSize(24);

        portraitText->setFont(font);

        const QRectF textBounds = portraitText->boundingRect();

        portraitText->setPos((kPortraitSize - textBounds.width()) / 2.0,

                             (kPortraitSize - textBounds.height()) / 2.0);
    }



    m_portraitView->setScene(scene);

}

void BattleView::setupPlayerBuffs()
{
    m_buffTooltip = new QLabel(this);
    m_buffTooltip->setWordWrap(true);
    m_buffTooltip->setAlignment(Qt::AlignCenter);
    m_buffTooltip->setStyleSheet(
        QStringLiteral("color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
                       "padding: 6px 10px; font-size: 14px;"));
    m_buffTooltip->hide();

    m_relicTooltip = new QLabel(this);
    m_relicTooltip->setWordWrap(true);
    m_relicTooltip->setAlignment(Qt::AlignCenter);
    m_relicTooltip->setStyleSheet(
        QStringLiteral("color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
                       "padding: 6px 10px; font-size: 12px;"));
    m_relicTooltip->hide();

    m_playerBuffSlots.reserve(kPlayerBuffMaxVisible);
    for (int i = 0; i < kPlayerBuffMaxVisible; ++i) {
        auto *buffSlot = new QLabel(this);
        buffSlot->setObjectName(QStringLiteral("buffSlot"));
        configureBuffLabel(buffSlot);
        buffSlot->installEventFilter(this);
        buffSlot->hide();
        m_playerBuffSlots.append(buffSlot);
    }

    m_playerBuffOverflow = new QLabel(QStringLiteral("…"), this);
    m_playerBuffOverflow->setObjectName(QStringLiteral("buffOverflow"));
    m_playerBuffOverflow->setGeometry(kPortraitX, kPortraitY, kBuffSize, kBuffSize);
    m_playerBuffOverflow->setAlignment(Qt::AlignCenter);
    m_playerBuffOverflow->setStyleSheet(QString::fromLatin1(kBuffOverflowStyle));
    m_playerBuffOverflow->setProperty("buffTooltip", QStringLiteral("More buffs…"));
    m_playerBuffOverflow->installEventFilter(this);
    m_playerBuffOverflow->hide();
}

void BattleView::configureBuffLabel(QLabel *label) const
{
    if (label == nullptr) {
        return;
    }

    label->setFixedSize(kBuffSize, kBuffSize);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QString::fromLatin1(kBuffSlotStyle));
    label->setAttribute(Qt::WA_Hover, true);
}

void BattleView::showBuffTooltip(QLabel *source, const QString &text)
{
    if (m_buffTooltip == nullptr || source == nullptr || text.isEmpty()) {
        return;
    }

    if (m_intentTooltip != nullptr) {
        m_intentTooltip->hide();
    }
    hideRelicTooltip();

    m_buffTooltip->setText(text);
    m_buffTooltip->adjustSize();

    const int tooltipX = source->x() + (source->width() - m_buffTooltip->width()) / 2;
    const int tooltipY = source->y() - m_buffTooltip->height() - 4;
    m_buffTooltip->move(tooltipX, tooltipY);
    m_buffTooltip->show();
    m_buffTooltip->raise();
}

void BattleView::hideBuffTooltip()
{
    if (m_buffTooltip != nullptr) {
        m_buffTooltip->hide();
    }
}

void BattleView::showRelicTooltip(QLabel *source, const QString &text)
{
    if (m_relicTooltip == nullptr || source == nullptr || text.isEmpty()) {
        return;
    }

    hideBuffTooltip();
    if (m_intentTooltip != nullptr) {
        m_intentTooltip->hide();
    }
    hideRelicTooltip();

    m_relicTooltip->setText(text);
    m_relicTooltip->adjustSize();

    const int tooltipX = source->x() + (source->width() - m_relicTooltip->width()) / 2;
    const int tooltipY = source->y() + source->height() + 4;
    m_relicTooltip->move(tooltipX, tooltipY);
    m_relicTooltip->show();
    m_relicTooltip->raise();
}

void BattleView::hideRelicTooltip()
{
    if (m_relicTooltip != nullptr) {
        m_relicTooltip->hide();
    }
}



void BattleView::updateFromViewModel()

{

    if (m_viewModel == nullptr) {

        return;

    }



    updateStats();
    updateRelics();
    updatePotions();
    updateHand();
    updateCombatHud();
    updatePlayerBuffs();
    updateEnemies();
    updatePiles();

    if (hasPotionTargeting()) {
        highlightAliveEnemies(true);
    }
}



void BattleView::updateStats()
{
    if (m_blockLabel != nullptr) {
        m_blockLabel->setText(QStringLiteral("Block: %1").arg(m_viewModel->playerBlock()));
    }
}



void BattleView::updateRelics()
{
    hideRelicTooltip();

    for (QLabel *slot : m_relicSlots) {
        if (slot != nullptr) {
            slot->clear();
            slot->setProperty("relicTooltip", QString());
            slot->hide();
        }
    }
}



void BattleView::updatePotions()
{
    // Potion bar is hidden in battle; keep slot IDs updated for data, but never show buttons.
    const QVariantList potions = m_viewModel->potions();

    for (int i = 0; i < kPotionSlotCount; ++i) {
        QPushButton *slot = m_potionSlots.at(i);
        if (i < potions.size()) {
            const QVariantMap entry = potions.at(i).toMap();
            m_potionSlotIds[i] = entry.value(QStringLiteral("id")).toString();
        } else {
            m_potionSlotIds[i].clear();
        }
        if (slot != nullptr) {
            slot->hide();
        }
    }
}



void BattleView::updateHand()

{

    const QVariantList hand = m_viewModel->handCards();

    for (int i = 0; i < kCardSlotCount; ++i) {

        CardWidget *slot = m_cardSlots.at(i);
        slot->setGeometry(kCardSlotXs[i], kCardY, kCardWidth, kCardHeight);

        if (i < hand.size()) {

            const QVariantMap entry = hand.at(i).toMap();

            slot->setCardData(entry.value(QStringLiteral("id")).toString(),

                              entry.value(QStringLiteral("name")).toString(),

                              entry.value(QStringLiteral("cost")).toInt(),

                              entry.value(QStringLiteral("description")).toString());

            slot->setHandIndex(i);

            slot->show();

        } else {

            slot->setHandIndex(-1);
            slot->setSelected(false);
            slot->hide();

        }

    }

    applyCardAffordabilityStates();

}



void BattleView::setupEnemyGroups()
{
    m_intentTooltip = new QLabel(this);
    m_intentTooltip->setWordWrap(true);
    m_intentTooltip->setAlignment(Qt::AlignCenter);
    m_intentTooltip->setStyleSheet(
        QStringLiteral("color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
                       "padding: 6px 10px; font-size: 16px;"));
    m_intentTooltip->hide();

    m_enemyGroups.reserve(kMaxEnemyDisplayCount);
    for (int i = 0; i < kMaxEnemyDisplayCount; ++i) {
        EnemyDisplayGroup group;

        group.artPlaceholder = new QLabel(this);
        group.artPlaceholder->setObjectName(QStringLiteral("enemyArtPlaceholder"));
        group.artPlaceholder->setAlignment(Qt::AlignCenter);
        group.artPlaceholder->setText(QStringLiteral("Enemy Art"));
        group.artPlaceholder->setStyleSheet(QString::fromLatin1(kEnemyArtStyle));
        group.artPlaceholder->setAttribute(Qt::WA_Hover, true);
        group.artPlaceholder->installEventFilter(this);

        group.nameLabel = new QLabel(this);
        group.nameLabel->setObjectName(QStringLiteral("enemyNameLabel"));
        group.nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        group.nameLabel->setFixedHeight(kEnemyNameLabelHeight);
        group.nameLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        group.intentIcon = new QLabel(this);
        group.intentIcon->setObjectName(QStringLiteral("enemyIntentIcon"));
        group.intentIcon->setAlignment(Qt::AlignCenter);
        group.intentIcon->setStyleSheet(
            QStringLiteral("background-color: #2a2a3e; border: 1px solid #0f3460; color: #e0e0e0; "
                           "font-size: 12px;"));
        group.intentIcon->setFixedSize(kEnemyIntentIconSize, kEnemyIntentIconSize);
        group.intentIcon->setAttribute(Qt::WA_Hover, true);
        group.intentIcon->installEventFilter(this);

        group.hpLabel = new QLabel(this);
        group.hpLabel->setObjectName(QStringLiteral("enemyHpLabel"));

        group.blockLabel = new QLabel(this);
        group.blockLabel->setObjectName(QStringLiteral("enemyBlockLabel"));

        group.buffSlots.reserve(kEnemyBuffMaxVisibleLarge);
        for (int slot = 0; slot < kEnemyBuffMaxVisibleLarge; ++slot) {
            auto *buffSlot = new QLabel(this);
            buffSlot->setObjectName(QStringLiteral("buffSlot"));
            configureBuffLabel(buffSlot);
            buffSlot->installEventFilter(this);
            buffSlot->hide();
            group.buffSlots.append(buffSlot);
        }

        group.buffOverflow = new QLabel(QStringLiteral("…"), this);
        group.buffOverflow->setObjectName(QStringLiteral("buffOverflow"));
        group.buffOverflow->setFixedSize(kBuffSize, kBuffSize);
        group.buffOverflow->setAlignment(Qt::AlignCenter);
        group.buffOverflow->setStyleSheet(QString::fromLatin1(kBuffOverflowStyle));
        group.buffOverflow->setProperty("buffTooltip", QStringLiteral("More buffs…"));
        group.buffOverflow->installEventFilter(this);
        group.buffOverflow->hide();

        group.artPlaceholder->hide();
        group.nameLabel->hide();
        group.intentIcon->hide();
        group.hpLabel->hide();
        group.blockLabel->hide();

        m_enemyGroups.append(group);
    }
}

void BattleView::layoutEnemyGroups(const QVector<int> &leftXs, int artSize)
{
    const bool compactLayout = artSize <= kEnemyArtSizeSmall;
    const int statFontSize = compactLayout ? 16 : 20;
    const int nameFontSize = compactLayout ? 16 : 20;
    const int statHeight = compactLayout ? kEnemyStatLabelHeightSmall : kEnemyStatLabelHeightLarge;
    const int statsY = kEnemyArtY + artSize;
    const int intentY = kEnemyArtY - kEnemyIntentIconSize;
    const int intentXOffset = (artSize - kEnemyIntentIconSize) / 2;

    for (int i = 0; i < m_enemyGroups.size(); ++i) {
        EnemyDisplayGroup &group = m_enemyGroups[i];
        const bool visible = i < leftXs.size();
        if (!visible) {
            group.artPlaceholder->hide();
            group.nameLabel->hide();
            group.intentIcon->hide();
            group.hpLabel->hide();
            group.blockLabel->hide();
            for (QLabel *buffSlot : group.buffSlots) {
                if (buffSlot != nullptr) {
                    buffSlot->hide();
                }
            }
            if (group.buffOverflow != nullptr) {
                group.buffOverflow->hide();
            }
            continue;
        }

        const int leftX = leftXs.at(i);
        group.artPlaceholder->setGeometry(leftX, kEnemyArtY, artSize, artSize);
        applyEnemyArtStyle(group.artPlaceholder, false);
        group.nameLabel->setGeometry(leftX, kEnemyArtY, artSize, kEnemyNameLabelHeight);
        configureEnemyNameLabel(group.nameLabel, nameFontSize);
        group.intentIcon->setGeometry(leftX + intentXOffset, intentY, kEnemyIntentIconSize,
                                      kEnemyIntentIconSize);

        if (compactLayout) {
            configureEnemyStatLabel(group.hpLabel, Qt::AlignHCenter, statFontSize, statHeight);
            configureEnemyStatLabel(group.blockLabel, Qt::AlignHCenter, statFontSize, statHeight);
            group.hpLabel->setGeometry(leftX, statsY, artSize, statHeight);
            group.blockLabel->setGeometry(leftX, statsY + statHeight, artSize, statHeight);
        } else {
            configureEnemyStatLabel(group.hpLabel, Qt::AlignLeft, statFontSize, statHeight);
            configureEnemyStatLabel(group.blockLabel, Qt::AlignRight, statFontSize, statHeight);
            group.hpLabel->setGeometry(leftX, statsY, kEnemyHpLabelWidthLarge, statHeight);
            group.blockLabel->setGeometry(leftX + artSize - kEnemyBlockLabelWidthLarge, statsY,
                                          kEnemyBlockLabelWidthLarge, statHeight);
        }

        group.artPlaceholder->show();
        group.nameLabel->show();
        group.nameLabel->raise();
        group.intentIcon->show();
        group.intentIcon->raise();
        group.hpLabel->show();
        group.blockLabel->show();
    }
}

void BattleView::setEnemyGroupVisible(int index, bool visible)
{
    if (index < 0 || index >= m_enemyGroups.size()) {
        return;
    }

    EnemyDisplayGroup &group = m_enemyGroups[index];
    if (!visible) {
        group.artPlaceholder->hide();
        group.nameLabel->hide();
        group.intentIcon->hide();
        group.hpLabel->hide();
        group.blockLabel->hide();
        for (QLabel *buffSlot : group.buffSlots) {
            if (buffSlot != nullptr) {
                buffSlot->hide();
            }
        }
        if (group.buffOverflow != nullptr) {
            group.buffOverflow->hide();
        }
        return;
    }

    group.artPlaceholder->show();
    group.nameLabel->show();
    group.nameLabel->raise();
    group.intentIcon->show();
    group.intentIcon->raise();
    group.hpLabel->show();
    group.blockLabel->show();
}

void BattleView::initializeCombatLayout(int initialEnemyCount)
{
    m_lockedSlotCount = qMin(initialEnemyCount, kMaxEnemyDisplayCount);
    m_lockedArtSize = enemyArtSizeForCount(m_lockedSlotCount);
    m_lockedLeftXs = computeEnemyGroupLeftXs(m_lockedSlotCount, m_lockedArtSize);
    layoutEnemyGroups(m_lockedLeftXs, m_lockedArtSize);
    m_combatLayoutInitialized = true;
}

void BattleView::resetCombatLayout()
{
    m_combatLayoutInitialized = false;
    m_lockedSlotCount = 0;
    m_lockedArtSize = 0;
    m_lockedLeftXs.clear();
    hideBuffTooltip();
    hideRelicTooltip();

    for (int i = 0; i < m_enemyGroups.size(); ++i) {
        m_enemyGroups[i].intentDescription.clear();
        setEnemyGroupVisible(i, false);
    }

    for (QLabel *buffSlot : m_playerBuffSlots) {
        if (buffSlot != nullptr) {
            buffSlot->hide();
        }
    }
    if (m_playerBuffOverflow != nullptr) {
        m_playerBuffOverflow->hide();
    }
}

bool BattleView::hasCardSelected() const
{
    return m_selectedCardWidget != nullptr && m_selectedHandIndex >= 0;
}

bool BattleView::hasPotionTargeting() const
{
    return !m_targetingPotionId.isEmpty() && m_targetingPotionSlotIndex >= 0;
}

void BattleView::clearPotionTargeting()
{
    if (!hasPotionTargeting() && m_targetingPotionSlotIndex < 0) {
        highlightAliveEnemies(false);
        return;
    }

    const int slotIndex = m_targetingPotionSlotIndex;
    m_targetingPotionId.clear();
    m_targetingPotionSlotIndex = -1;
    m_targetingPotionStart = QPoint();
    highlightAliveEnemies(false);
    emit potionTargetingChanged(slotIndex, false);
}

void BattleView::setPotionTargeting(const QString &potionId, int statusBarSlotIndex,
                                    const QPoint &startCenterInBattleCoords)
{
    clearCardSelection();
    m_targetingPotionId = potionId;
    m_targetingPotionSlotIndex = statusBarSlotIndex;
    m_targetingPotionStart = startCenterInBattleCoords;
    highlightAliveEnemies(true);
    emit potionTargetingChanged(statusBarSlotIndex, true);
    setFocus(Qt::OtherFocusReason);
}

void BattleView::highlightAliveEnemies(bool highlighted)
{
    if (m_viewModel == nullptr) {
        return;
    }

    const QVariantList enemySlots = m_viewModel->enemies();
    for (int i = 0; i < m_enemyGroups.size(); ++i) {
        const EnemyDisplayGroup &group = m_enemyGroups.at(i);
        if (group.artPlaceholder == nullptr || !group.artPlaceholder->isVisible()) {
            continue;
        }
        bool alive = false;
        if (i < enemySlots.size()) {
            alive = enemySlots.at(i).toMap().value(QStringLiteral("alive"), true).toBool();
        }
        applyEnemyArtStyle(group.artPlaceholder, highlighted && alive);
    }
}

void BattleView::executePotionUse(const QString &potionId, int enemyIndex)
{
    if (m_viewModel == nullptr) {
        return;
    }

    try {
        m_viewModel->usePotion(potionId, enemyIndex);
        updateFromViewModel();
    } catch (...) {
        QMessageBox::warning(this, QStringLiteral("Error"),
                             QStringLiteral("An unexpected error occurred while using a potion."));
    }
}

void BattleView::beginPotionExecution(const QString &potionId, int enemyIndex,
                                      const QString &effectTarget, const QPoint &startCenter)
{
    if (m_viewModel == nullptr || m_potionUseInFlight) {
        return;
    }

    if (!m_viewModel->canUsePotion(potionId)) {
        clearPotionTargeting();
        return;
    }

    if (effectTarget == QStringLiteral("selected_enemy") && enemyIndex < 0) {
        clearPotionTargeting();
        return;
    }

    if (effectTarget == QStringLiteral("all_enemies")) {
        bool anyAlive = false;
        for (const QVariant &entry : m_viewModel->enemies()) {
            if (entry.toMap().value(QStringLiteral("alive"), true).toBool()) {
                anyAlive = true;
                break;
            }
        }
        if (!anyAlive) {
            clearPotionTargeting();
            return;
        }
    }

    const QPoint end = flyAnimationEndPoint(enemyIndex, effectTarget);
    clearPotionTargeting();
    m_potionUseInFlight = true;

    const QPixmap icon = ArtAssets::potionIcon(potionId, QSize(kPotionIconSize, kPotionIconSize));
    CombatAnimations::flyPixmap(
        this, icon, startCenter, end, kPotionFlyAnimationDurationMs,
        [this, potionId, enemyIndex]() {
            m_potionUseInFlight = false;
            executePotionUse(potionId, enemyIndex);
        });
}

void BattleView::requestPotionUse(const QString &potionId, int statusBarSlotIndex,
                                  const QPoint &startCenterInBattleCoords)
{
    if (m_viewModel == nullptr || m_potionUseInFlight || potionId.isEmpty()) {
        return;
    }

    if (!m_viewModel->canUsePotion(potionId)) {
        return;
    }

    const QString effectTarget = m_viewModel->potionPrimaryTarget(potionId);

    if (effectTarget == QStringLiteral("selected_enemy")) {
        if (hasPotionTargeting() && m_targetingPotionId == potionId
            && m_targetingPotionSlotIndex == statusBarSlotIndex) {
            clearPotionTargeting();
            return;
        }

        bool anyAlive = false;
        for (const QVariant &entry : m_viewModel->enemies()) {
            if (entry.toMap().value(QStringLiteral("alive"), true).toBool()) {
                anyAlive = true;
                break;
            }
        }
        if (!anyAlive) {
            return;
        }

        if (hasPotionTargeting()) {
            clearPotionTargeting();
        }
        setPotionTargeting(potionId, statusBarSlotIndex, startCenterInBattleCoords);
        return;
    }

    if (hasPotionTargeting()) {
        clearPotionTargeting();
    }
    beginPotionExecution(potionId, -1, effectTarget, startCenterInBattleCoords);
}

void BattleView::clearCardSelection()
{
    if (m_selectedCardWidget != nullptr) {
        m_selectedCardWidget->setSelected(false);
    }

    m_selectedCardWidget = nullptr;
    m_selectedCardId.clear();
    m_selectedHandIndex = -1;

    if (!hasPotionTargeting()) {
        for (const EnemyDisplayGroup &group : m_enemyGroups) {
            applyEnemyArtStyle(group.artPlaceholder, false);
        }
    }
}

void BattleView::selectCard(CardWidget *widget, const QString &cardId, int handIndex)
{
    m_selectedCardWidget = widget;
    m_selectedCardId = cardId;
    m_selectedHandIndex = handIndex;
    if (widget != nullptr) {
        widget->setSelected(true);
    }
}

void BattleView::applyEnemyArtStyle(QLabel *artPlaceholder, bool hovered)
{
    if (artPlaceholder == nullptr) {
        return;
    }

    const bool compact = artPlaceholder->width() <= kEnemyArtSizeSmall;
    if (hovered) {
        artPlaceholder->setStyleSheet(
            QString::fromLatin1(compact ? kEnemyArtHoverStyleSmall : kEnemyArtHoverStyle));
    } else {
        artPlaceholder->setStyleSheet(QString::fromLatin1(compact ? kEnemyArtStyleSmall : kEnemyArtStyle));
    }
}

void BattleView::resetCardSlotGeometry(CardWidget *widget, int slotIndex)
{
    if (widget == nullptr || slotIndex < 0 || slotIndex >= kCardSlotCount) {
        return;
    }

    widget->setGeometry(kCardSlotXs[slotIndex], kCardY, kCardWidth, kCardHeight);
}

void BattleView::clearCardSlot(CardWidget *widget, int slotIndex)
{
    if (widget == nullptr) {
        return;
    }

    resetCardSlotGeometry(widget, slotIndex);
    widget->setHandIndex(-1);
    widget->setSelected(false);
    widget->setEnabled(true);
    widget->setWindowOpacity(1.0);
    widget->hide();
}

QPoint BattleView::cardWidgetCenter(const CardWidget *widget) const
{
    if (widget == nullptr) {
        return QPoint(kWindowWidth / 2, kCardY + kCardHeight / 2);
    }

    return widget->geometry().center();
}

QPoint BattleView::enemyArtCenter(int enemyIndex) const
{
    if (enemyIndex < 0 || enemyIndex >= m_enemyGroups.size()) {
        return QPoint(kWindowWidth / 2, kWindowHeight / 2);
    }

    const EnemyDisplayGroup &group = m_enemyGroups.at(enemyIndex);
    const QLabel *art = group.artPlaceholder;
    if (art == nullptr || !art->isVisible()) {
        return QPoint(kWindowWidth / 2, kWindowHeight / 2);
    }

    return art->geometry().center();
}

QPoint BattleView::playerAreaCenter() const
{
    return QPoint(kPortraitX + kPortraitSize / 2, kPortraitY + kPortraitSize / 2);
}

QPoint BattleView::flyAnimationEndPoint(int enemyIndex, const QString &targetType) const
{
    if (targetType == QStringLiteral("allEnemies")
        || targetType == QStringLiteral("all_enemies")) {
        return QPoint(kWindowWidth / 2, kWindowHeight / 2);
    }

    if (targetType == QStringLiteral("self")) {
        return playerAreaCenter();
    }

    return enemyArtCenter(enemyIndex);
}

void BattleView::startCardFlyAnimation(const QString &name, int cost, const QPoint &start,
                                       const QPoint &end)
{
    CombatAnimations::flyCardProxy(this, name, cost, start, end, kCardWidth, kCardHeight,
                                   kFlyAnimationDurationMs);
}

void BattleView::executeCardPlay(CardWidget *widget, const QString &cardId, int handIndex,
                                 int enemyIndex, const QString &targetType)
{
    if (widget == nullptr || m_viewModel == nullptr || handIndex < 0) {
        return;
    }

    if (!m_viewModel->canSelectCards()) {
        clearCardSelection();
        return;
    }

    const QVariantList hand = m_viewModel->handCards();
    if (handIndex >= hand.size()) {
        return;
    }

    const QVariantMap cardEntry = hand.at(handIndex).toMap();
    if (!m_viewModel->canAffordCard(handIndex)) {
        clearCardSelection();
        return;
    }

    const int cost = cardEntry.value(QStringLiteral("cost")).toInt();
    const QString cardName = cardEntry.value(QStringLiteral("name")).toString();
    const QPoint start = cardWidgetCenter(widget);
    const QPoint end = flyAnimationEndPoint(enemyIndex, targetType);

    clearCardSelection();
    clearCardSlot(widget, handIndex);

    try {
        m_viewModel->playCard(cardId, enemyIndex, handIndex);
        updateFromViewModel();
    } catch (...) {
        QMessageBox::warning(this, QStringLiteral("Error"),
                             QStringLiteral("An unexpected error occurred while playing a card."));
        return;
    }

    startCardFlyAnimation(cardName, cost, start, end);
}

void BattleView::playSelectedCardAtEnemy(int enemyIndex)
{
    if (!hasCardSelected() || m_viewModel == nullptr || !isHandIndexAffordable(m_selectedHandIndex)) {
        clearCardSelection();
        return;
    }

    const QVariantList hand = m_viewModel->handCards();
    if (m_selectedHandIndex < 0 || m_selectedHandIndex >= hand.size()) {
        return;
    }

    const QString targetType =
        hand.at(m_selectedHandIndex).toMap().value(QStringLiteral("targetType")).toString();

    executeCardPlay(m_selectedCardWidget, m_selectedCardId, m_selectedHandIndex, enemyIndex,
                    targetType);
}

bool BattleView::eventFilter(QObject *watched, QEvent *event)
{
    auto *relicLabel = qobject_cast<QLabel *>(watched);
    if (relicLabel != nullptr && relicLabel->objectName() == QStringLiteral("relicSlot")) {
        if (event->type() == QEvent::Enter) {
            const QString tooltip = relicLabel->property("relicTooltip").toString();
            if (!tooltip.isEmpty()) {
                showRelicTooltip(relicLabel, tooltip);
            }
        } else if (event->type() == QEvent::Leave) {
            hideRelicTooltip();
        }
        return false;
    }

    auto *buffLabel = qobject_cast<QLabel *>(watched);
    if (buffLabel != nullptr && (isBuffLabel(buffLabel) || isBuffOverflowLabel(buffLabel))) {
        if (event->type() == QEvent::Enter) {
            const QString tooltip = buffLabel->property("buffTooltip").toString();
            if (!tooltip.isEmpty()) {
                showBuffTooltip(buffLabel, tooltip);
            }
        } else if (event->type() == QEvent::Leave) {
            hideBuffTooltip();
    hideRelicTooltip();
        }
        return false;
    }

    for (int i = 0; i < m_enemyGroups.size(); ++i) {
        const EnemyDisplayGroup &group = m_enemyGroups.at(i);

        if (watched == group.intentIcon) {
            if (event->type() == QEvent::Enter) {
                const QString description = group.intentDescription;
                if (description.isEmpty() || m_intentTooltip == nullptr) {
                    break;
                }

                m_intentTooltip->setText(description);
                m_intentTooltip->adjustSize();

                const QLabel *icon = group.intentIcon;
                const int tooltipX = icon->x() + (icon->width() - m_intentTooltip->width()) / 2;
                const int tooltipY = icon->y() - m_intentTooltip->height() - 4;
                m_intentTooltip->move(tooltipX, tooltipY);
                m_intentTooltip->show();
                m_intentTooltip->raise();
            } else if (event->type() == QEvent::Leave) {
                if (m_intentTooltip != nullptr) {
                    m_intentTooltip->hide();
                }
            }
            break;
        }

        if (watched == group.artPlaceholder) {
            if (event->type() == QEvent::Enter) {
                if ((hasCardSelected() && m_viewModel != nullptr
                     && isHandIndexAffordable(m_selectedHandIndex))
                    || hasPotionTargeting()) {
                    applyEnemyArtStyle(group.artPlaceholder, true);
                }
            } else if (event->type() == QEvent::Leave) {
                if (hasPotionTargeting()) {
                    applyEnemyArtStyle(group.artPlaceholder, true);
                } else {
                    applyEnemyArtStyle(group.artPlaceholder, false);
                }
            } else if (event->type() == QEvent::MouseButtonPress) {
                auto *mouseEvent = static_cast<QMouseEvent *>(event);
                if (mouseEvent->button() == Qt::LeftButton) {
                    if (hasPotionTargeting()) {
                        const QString potionId = m_targetingPotionId;
                        const QPoint start = m_targetingPotionStart;
                        beginPotionExecution(potionId, i, QStringLiteral("selected_enemy"), start);
                        return true;
                    }
                    if (hasCardSelected() && isHandIndexAffordable(m_selectedHandIndex)) {
                        playSelectedCardAtEnemy(i);
                        return true;
                    }
                }
            }
            break;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void BattleView::keyPressEvent(QKeyEvent *event)
{
    if (event != nullptr && event->key() == Qt::Key_Escape && hasPotionTargeting()) {
        clearPotionTargeting();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void BattleView::mousePressEvent(QMouseEvent *event)
{
    if (event != nullptr && event->button() == Qt::LeftButton && hasPotionTargeting()) {
        clearPotionTargeting();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void BattleView::updatePlayerBuffs()
{
    if (m_viewModel == nullptr || !m_viewModel->combatActive()) {
        for (QLabel *buffSlot : m_playerBuffSlots) {
            if (buffSlot != nullptr) {
                buffSlot->hide();
            }
        }
        if (m_playerBuffOverflow != nullptr) {
            m_playerBuffOverflow->hide();
        }
        return;
    }

    const QVariantList buffs = m_viewModel->playerBuffs();
    const int visibleCount = qMin(buffs.size(), kPlayerBuffMaxVisible);

    for (int i = 0; i < m_playerBuffSlots.size(); ++i) {
        QLabel *slot = m_playerBuffSlots.at(i);
        if (slot == nullptr) {
            continue;
        }

        if (i < visibleCount) {
            const QVariantMap entry = buffs.at(i).toMap();
            slot->setText(entry.value(QStringLiteral("abbreviation")).toString());
            slot->setProperty("buffTooltip", entry.value(QStringLiteral("tooltip")));
            slot->setGeometry(kPlayerBuffColumnX,
                              kPlayerBuffColumnY + i * (kBuffSize + kPlayerBuffVerticalSpacing),
                              kBuffSize,
                              kBuffSize);
            slot->show();
            slot->raise();
        } else {
            slot->hide();
        }
    }

    if (m_playerBuffOverflow != nullptr) {
        if (m_viewModel->playerBuffOverflow()) {
            m_playerBuffOverflow->show();
            m_playerBuffOverflow->raise();
        } else {
            m_playerBuffOverflow->hide();
        }
    }
}

void BattleView::layoutEnemyBuffs(int index, const QVariantMap &entry, int leftX, int artSize)
{
    if (index < 0 || index >= m_enemyGroups.size()) {
        return;
    }

    EnemyDisplayGroup &group = m_enemyGroups[index];
    const QVariantList buffs = entry.value(QStringLiteral("buffs")).toList();
    const int maxVisible = entry.value(QStringLiteral("buffMaxVisible"), kEnemyBuffMaxVisibleLarge).toInt();
    const int visibleCount = qMin(buffs.size(), maxVisible);
    const int rowY = enemyBuffRowY(artSize);

    for (int i = 0; i < group.buffSlots.size(); ++i) {
        QLabel *slot = group.buffSlots.at(i);
        if (slot == nullptr) {
            continue;
        }

        if (i < visibleCount) {
            const QVariantMap buffEntry = buffs.at(i).toMap();
            slot->setText(buffEntry.value(QStringLiteral("abbreviation")).toString());
            slot->setProperty("buffTooltip", buffEntry.value(QStringLiteral("tooltip")));
            slot->setGeometry(leftX + i * (kBuffSize + kEnemyBuffHorizontalSpacing),
                              rowY,
                              kBuffSize,
                              kBuffSize);
            slot->show();
            slot->raise();
        } else {
            slot->hide();
        }
    }

    if (group.buffOverflow != nullptr) {
        if (entry.value(QStringLiteral("buffOverflow")).toBool()) {
            group.buffOverflow->setGeometry(leftX, kEnemyArtY, kBuffSize, kBuffSize);
            group.buffOverflow->show();
            group.buffOverflow->raise();
        } else {
            group.buffOverflow->hide();
        }
    }
}

void BattleView::updateEnemies()
{
    if (m_intentTooltip != nullptr) {
        m_intentTooltip->hide();
    }
    hideBuffTooltip();
    hideRelicTooltip();

    if (m_viewModel == nullptr || !m_viewModel->combatActive()) {
        resetCombatLayout();
        return;
    }

    const QVariantList enemySlots = m_viewModel->enemies();
    if (!m_combatLayoutInitialized) {
        initializeCombatLayout(enemySlots.size());
    }

    for (int i = 0; i < m_enemyGroups.size(); ++i) {
        EnemyDisplayGroup &group = m_enemyGroups[i];
        if (i >= m_lockedSlotCount || i >= enemySlots.size()) {
            group.intentDescription.clear();
            setEnemyGroupVisible(i, false);
            continue;
        }

        const QVariantMap entry = enemySlots.at(i).toMap();
        const bool alive = entry.value(QStringLiteral("alive"), true).toBool();
        if (!alive) {
            group.intentDescription.clear();
            setEnemyGroupVisible(i, false);
            continue;
        }

        const int hp = entry.value(QStringLiteral("hp")).toInt();
        const int maxHp = entry.value(QStringLiteral("maxHp")).toInt();
        const int block = entry.value(QStringLiteral("block")).toInt();
        const QString intentAbbreviation = entry.value(QStringLiteral("intentAbbreviation")).toString();
        const QString name = entry.value(QStringLiteral("name")).toString();
        const QString enemyId = entry.value(QStringLiteral("id")).toString();
        const bool defensiveMode = entry.value(QStringLiteral("defensiveMode")).toBool();

        setEnemyNameLabelText(group.nameLabel, name);
        group.hpLabel->setText(QStringLiteral("HP: %1/%2").arg(hp).arg(maxHp));
        group.blockLabel->setText(QStringLiteral("Block: %1").arg(block));
        group.intentDescription = entry.value(QStringLiteral("intentDescription")).toString();
        group.intentIcon->setText(intentAbbreviation);
        const QSize artSize(m_lockedArtSize, m_lockedArtSize);
        ArtAssets::applyToLabel(group.artPlaceholder,
                                ArtAssets::enemyPortrait(enemyId, artSize, defensiveMode),
                                QStringLiteral("Enemy Art"));
        setEnemyGroupVisible(i, true);

        const int leftX = i < m_lockedLeftXs.size() ? m_lockedLeftXs.at(i) : 0;
        layoutEnemyBuffs(i, entry, leftX, m_lockedArtSize);
    }
}

void BattleView::updateCombatHud()
{
    if (m_viewModel == nullptr || m_energyLabel == nullptr) {
        return;
    }

    m_energyLabel->setText(QStringLiteral("Energy: %1/%2")
                               .arg(m_viewModel->currentEnergy())
                               .arg(m_viewModel->maxEnergy()));

    updateCardInteractivity();
    updateEndTurnButtonStyle();
}

void BattleView::updateCardInteractivity()
{
    applyCardAffordabilityStates();
}

bool BattleView::isHandIndexAffordable(int handIndex) const
{
    if (m_viewModel == nullptr || handIndex < 0) {
        return false;
    }

    const QVariantList hand = m_viewModel->handCards();
    if (handIndex >= hand.size()) {
        return false;
    }

    return m_viewModel->canAffordCard(handIndex);
}

void BattleView::applyCardAffordabilityStates()
{
    if (m_viewModel == nullptr) {
        return;
    }

    if (hasCardSelected() && !isHandIndexAffordable(m_selectedHandIndex)) {
        clearCardSelection();
        for (const EnemyDisplayGroup &group : m_enemyGroups) {
            applyEnemyArtStyle(group.artPlaceholder, false);
        }
    }

    for (int i = 0; i < m_cardSlots.size(); ++i) {
        CardWidget *slot = m_cardSlots.at(i);
        if (!slot->isVisible()) {
            continue;
        }

        const bool affordable = isHandIndexAffordable(i);

        slot->setEnabled(affordable);
        slot->setWindowOpacity(affordable ? 1.0 : 0.45);

        if (!affordable) {
            slot->setSelected(false);
            slot->setStyleSheet(QString::fromLatin1(kCardUnaffordableStyle));
            slot->setCursor(Qt::ArrowCursor);
        } else {
            slot->setCursor(Qt::PointingHandCursor);
            slot->setSelected(m_selectedHandIndex == i);
        }
    }
}

void BattleView::updateEndTurnButtonStyle()
{
    if (m_endTurnButton == nullptr || m_viewModel == nullptr) {
        return;
    }

    m_endTurnButton->setEnabled(true);
    if (m_viewModel->combatActive() && m_viewModel->currentEnergy() == 0) {
        m_endTurnButton->setStyleSheet(QString::fromLatin1(kEndTurnHighlightStyle));
    } else {
        m_endTurnButton->setStyleSheet(QString::fromLatin1(kEndTurnDefaultStyle));
    }
}

void BattleView::updatePiles()
{
    if (m_drawPileButton == nullptr || m_discardPileButton == nullptr) {
        return;
    }

    const int drawCount = m_viewModel->drawPileCount();
    const int discardCount = m_viewModel->discardPileCount();

    m_drawPileButton->setText(QString::number(drawCount));
    m_drawPileButton->setToolTip(QStringLiteral("Draw pile: %1").arg(drawCount));

    m_discardPileButton->setText(QString::number(discardCount));
    m_discardPileButton->setToolTip(QStringLiteral("Discard pile: %1").arg(discardCount));
}



void BattleView::onCardClicked(const QString &cardId)
{
    if (m_viewModel == nullptr || !m_viewModel->canSelectCards()) {
        return;
    }

    if (hasPotionTargeting()) {
        clearPotionTargeting();
    }

    CardWidget *clickedWidget = qobject_cast<CardWidget *>(sender());

    if (clickedWidget == nullptr) {
        return;
    }

    const int handIndex = clickedWidget->handIndex();

    if (handIndex < 0) {
        return;
    }

    const QVariantList hand = m_viewModel->handCards();

    if (handIndex >= hand.size()) {
        return;
    }
    const QVariantMap cardEntry = hand.at(handIndex).toMap();
    if (!isHandIndexAffordable(handIndex)) {
        return;
    }

    const QString targetType =
        cardEntry.value(QStringLiteral("targetType")).toString();

    const bool targetsEnemy = targetType == QStringLiteral("enemy");

    if (!targetsEnemy) {
        executeCardPlay(clickedWidget, cardId, handIndex, -1, targetType);
        return;
    }

    if (m_selectedCardWidget == clickedWidget) {
        clearCardSelection();
        return;
    }

    if (m_selectedCardWidget != nullptr) {
        m_selectedCardWidget->setSelected(false);
    }

    selectCard(clickedWidget, cardId, handIndex);
}

void BattleView::onPotionClicked()
{
    if (m_viewModel == nullptr || m_potionUseInFlight) {
        return;
    }

    auto *button = qobject_cast<QPushButton *>(sender());
    if (button == nullptr) {
        return;
    }

    const int slotIndex = m_potionSlots.indexOf(button);
    if (slotIndex < 0 || slotIndex >= m_potionSlotIds.size()) {
        return;
    }

    const QString potionId = m_potionSlotIds.at(slotIndex);
    if (potionId.isEmpty()) {
        return;
    }

    requestPotionUse(potionId, slotIndex, button->geometry().center());
}

void BattleView::onEndTurnClicked()
{
    if (m_viewModel == nullptr) {
        return;
    }

    if (hasPotionTargeting()) {
        clearPotionTargeting();
    }
    if (hasCardSelected()) {
        clearCardSelection();
    }

    try {

        m_viewModel->endPlayerTurn();

        updateFromViewModel();

    } catch (...) {

        QMessageBox::warning(this, QStringLiteral("Error"),

                             QStringLiteral("An unexpected error occurred while ending your turn."));

    }

}

