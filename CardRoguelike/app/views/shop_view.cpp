#include "shop_view.h"

#include <QtCore/QVariant>
#include <QtGui/QCursor>
#include <QtGui/QPixmap>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolTip>

#include "utils/art_assets.h"
#include "viewmodels/shop_viewmodel.h"

namespace {

constexpr int kWindowWidth = 1920;
constexpr int kWindowHeight = 1080;

constexpr int kCardCount = 7;
constexpr int kCardXs[kCardCount] = {200, 500, 800, 1100, 1400, 200, 500};
constexpr int kCardYs[kCardCount] = {180, 180, 180, 180, 180, 600, 600};
constexpr int kCardWidth = 210;
constexpr int kCardHeight = 350;

constexpr int kCostXs[kCardCount] = {280, 580, 880, 1180, 1480, 280, 580};
constexpr int kCostYs[kCardCount] = {550, 550, 550, 550, 550, 970, 970};
constexpr int kCostWidth = 51;
constexpr int kCostHeight = 30;

constexpr int kDeleteCardX = 1400;
constexpr int kDeleteCardY = 600;
constexpr int kDeleteCardCostX = 1480;
constexpr int kDeleteCardCostY = 960;

constexpr int kRelicCount = 3;
constexpr int kRelicXs[kRelicCount] = {830, 1000, 1170};
constexpr int kRelicY = 690;
constexpr int kRelicSize = 50;
constexpr int kRelicCostY = 750;

constexpr int kPotionCount = 3;
constexpr int kPotionXs[kPotionCount] = {830, 1000, 1170};
constexpr int kPotionY = 800;
constexpr int kPotionSize = 50;
constexpr int kPotionCostY = 870;

constexpr int kGoldLabelX = 1560;
constexpr int kGoldLabelY = 40;
constexpr int kGoldLabelWidth = 320;
constexpr int kGoldLabelHeight = 50;

constexpr int kLeaveX = 1740;
constexpr int kLeaveY = 1000;
constexpr int kLeaveWidth = 150;
constexpr int kLeaveHeight = 50;

constexpr char kSlotButtonStyle[] =
    "QPushButton { color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
    "font-size: 16px; }"
    "QPushButton:hover:enabled { border-color: #e94560; }"
    "QPushButton:disabled { color: #888888; }";

constexpr char kCostLabelStyle[] =
    "color: #e0e0e0; background-color: transparent; font-size: 12px;";

constexpr char kLeaveButtonStyle[] =
    "QPushButton { color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
    "font-size: 16px; }"
    "QPushButton:hover { border-color: #e94560; }";

} // namespace

ShopView::ShopView(ShopViewModel *viewModel, QWidget *parent)
    : QWidget(parent)
    , m_viewModel(viewModel)
{
    setFixedSize(kWindowWidth, kWindowHeight);
    setObjectName(QStringLiteral("gameplayScene"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("#gameplayScene { background-color: transparent; }"));
    setupLayout();

    if (m_viewModel != nullptr) {
        connect(m_viewModel, &ShopViewModel::shopDataChanged, this, &ShopView::updateFromViewModel);
        connect(m_viewModel, &ShopViewModel::goldChanged, this, &ShopView::updateFromViewModel);
        connect(m_viewModel, &ShopViewModel::leaveRequested, this, &ShopView::leaveClicked);
        connect(m_viewModel, &ShopViewModel::deleteCardRequested, this, &ShopView::deleteCardClicked);
        connect(m_viewModel, &ShopViewModel::purchaseDenied, this, &ShopView::onPurchaseDenied);
    }

    updateFromViewModel();
}

void ShopView::setupLayout()
{
    m_goldLabel = new QLabel(this);
    m_goldLabel->setObjectName(QStringLiteral("shopGoldLabel"));
    m_goldLabel->setGeometry(kGoldLabelX, kGoldLabelY, kGoldLabelWidth, kGoldLabelHeight);
    m_goldLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_goldLabel->setStyleSheet(
        QStringLiteral("color: #ffd700; background-color: transparent; font-size: 28px;"));

    m_cardButtons.reserve(kCardCount);
    m_cardCostLabels.reserve(kCardCount);
    for (int i = 0; i < kCardCount; ++i) {
        auto *cardButton = new QPushButton(QStringLiteral("Card"), this);
        cardButton->setObjectName(QStringLiteral("card%1").arg(i + 1));
        cardButton->setGeometry(kCardXs[i], kCardYs[i], kCardWidth, kCardHeight);
        cardButton->setStyleSheet(QString::fromLatin1(kSlotButtonStyle));
        cardButton->setProperty("slotIndex", i);
        connect(cardButton, &QPushButton::clicked, this, &ShopView::onCardClicked);
        m_cardButtons.append(cardButton);

        auto *costLabel = new QLabel(QStringLiteral("cost"), this);
        costLabel->setObjectName(QStringLiteral("cost%1").arg(i + 1));
        costLabel->setGeometry(kCostXs[i], kCostYs[i], kCostWidth, kCostHeight);
        costLabel->setAlignment(Qt::AlignCenter);
        costLabel->setStyleSheet(QString::fromLatin1(kCostLabelStyle));
        m_cardCostLabels.append(costLabel);
    }

    m_deleteCardButton = new QPushButton(QStringLiteral("Delete Card"), this);
    m_deleteCardButton->setObjectName(QStringLiteral("deletecard"));
    m_deleteCardButton->setGeometry(kDeleteCardX, kDeleteCardY, kCardWidth, kCardHeight);
    m_deleteCardButton->setStyleSheet(QString::fromLatin1(kSlotButtonStyle));
    m_deleteCardButton->setEnabled(false);
    connect(m_deleteCardButton, &QPushButton::clicked, this, &ShopView::onDeleteCardClicked);

    m_deleteCardCostLabel = new QLabel(QStringLiteral("cost"), this);
    m_deleteCardCostLabel->setObjectName(QStringLiteral("deletecardcost"));
    m_deleteCardCostLabel->setGeometry(kDeleteCardCostX, kDeleteCardCostY, kCostWidth, kCostHeight);
    m_deleteCardCostLabel->setAlignment(Qt::AlignCenter);
    m_deleteCardCostLabel->setStyleSheet(QString::fromLatin1(kCostLabelStyle));

    m_relicButtons.reserve(kRelicCount);
    m_relicCostLabels.reserve(kRelicCount);
    for (int i = 0; i < kRelicCount; ++i) {
        auto *relicButton = new QPushButton(QStringLiteral("Relic"), this);
        relicButton->setObjectName(QStringLiteral("relic%1").arg(i + 1));
        relicButton->setGeometry(kRelicXs[i], kRelicY, kRelicSize, kRelicSize);
        relicButton->setStyleSheet(QString::fromLatin1(kSlotButtonStyle));
        relicButton->setProperty("slotIndex", i);
        connect(relicButton, &QPushButton::clicked, this, &ShopView::onRelicClicked);
        m_relicButtons.append(relicButton);

        auto *costLabel = new QLabel(QStringLiteral("cost"), this);
        costLabel->setObjectName(QStringLiteral("reliccost%1").arg(i + 1));
        costLabel->setGeometry(kRelicXs[i], kRelicCostY, kCostWidth, kCostHeight);
        costLabel->setAlignment(Qt::AlignCenter);
        costLabel->setStyleSheet(QString::fromLatin1(kCostLabelStyle));
        m_relicCostLabels.append(costLabel);
    }

    m_potionButtons.reserve(kPotionCount);
    m_potionCostLabels.reserve(kPotionCount);
    for (int i = 0; i < kPotionCount; ++i) {
        auto *potionButton = new QPushButton(QStringLiteral("Potion"), this);
        potionButton->setObjectName(QStringLiteral("potion%1").arg(i + 1));
        potionButton->setGeometry(kPotionXs[i], kPotionY, kPotionSize, kPotionSize);
        potionButton->setStyleSheet(QString::fromLatin1(kSlotButtonStyle));
        potionButton->setProperty("slotIndex", i);
        connect(potionButton, &QPushButton::clicked, this, &ShopView::onPotionClicked);
        m_potionButtons.append(potionButton);

        auto *costLabel = new QLabel(QStringLiteral("cost"), this);
        costLabel->setObjectName(QStringLiteral("potioncost%1").arg(i + 1));
        costLabel->setGeometry(kPotionXs[i], kPotionCostY, kCostWidth, kCostHeight);
        costLabel->setAlignment(Qt::AlignCenter);
        costLabel->setStyleSheet(QString::fromLatin1(kCostLabelStyle));
        m_potionCostLabels.append(costLabel);
    }

    m_leaveButton = new QPushButton(QStringLiteral("Leave"), this);
    m_leaveButton->setObjectName(QStringLiteral("leaveShop"));
    m_leaveButton->setGeometry(kLeaveX, kLeaveY, kLeaveWidth, kLeaveHeight);
    m_leaveButton->setStyleSheet(QString::fromLatin1(kLeaveButtonStyle));
    connect(m_leaveButton, &QPushButton::clicked, this, &ShopView::onLeaveClicked);
}

void ShopView::updateFromViewModel()
{
    refreshGold();
    refreshCardSlots();
    refreshRelicSlots();
    refreshPotionSlots();
    refreshDeleteCard();
}

void ShopView::refreshGold()
{
    if (m_goldLabel == nullptr) {
        return;
    }

    const int gold = m_viewModel != nullptr ? m_viewModel->gold() : 0;
    m_goldLabel->setText(QStringLiteral("Gold: %1").arg(gold));
}

void ShopView::refreshCardSlots()
{
    if (m_viewModel == nullptr) {
        return;
    }

    const QVector<ShopViewModel::ShopSlot> &cardSlots = m_viewModel->cardSlots();
    for (int i = 0; i < m_cardButtons.size(); ++i) {
        QPushButton *button = m_cardButtons.at(i);
        QLabel *costLabel = m_cardCostLabels.at(i);
        if (i >= cardSlots.size() || cardSlots.at(i).sold) {
            button->setText(QStringLiteral("Sold"));
            button->setEnabled(false);
            button->setToolTip(QString());
            costLabel->setText(QStringLiteral("-"));
            costLabel->setToolTip(QString());
            continue;
        }

        const ShopViewModel::ShopSlot &slot = cardSlots.at(i);
        button->setText(slot.displayName);
        button->setToolTip(slot.tooltip);
        button->setEnabled(true);
        costLabel->setText(QStringLiteral("%1 Gold").arg(slot.price));
        costLabel->setToolTip(QStringLiteral("%1 Gold").arg(slot.price));
    }
}

void ShopView::refreshRelicSlots()
{
    if (m_viewModel == nullptr) {
        return;
    }

    const QVector<ShopViewModel::ShopSlot> &relicSlots = m_viewModel->relicSlots();
    for (int i = 0; i < m_relicButtons.size(); ++i) {
        QPushButton *button = m_relicButtons.at(i);
        QLabel *costLabel = m_relicCostLabels.at(i);
        if (i >= relicSlots.size() || relicSlots.at(i).sold) {
            ArtAssets::applyToButton(button, QPixmap(), QStringLiteral("Sold"));
            button->setEnabled(false);
            button->setToolTip(QString());
            costLabel->setText(QStringLiteral("-"));
            costLabel->setToolTip(QString());
            continue;
        }

        const ShopViewModel::ShopSlot &slot = relicSlots.at(i);
        ArtAssets::applyToButton(button,
                                 ArtAssets::relicIcon(slot.itemId, QSize(kRelicSize, kRelicSize)),
                                 slot.displayName);
        button->setToolTip(slot.tooltip);
        button->setEnabled(true);
        costLabel->setText(QStringLiteral("%1 Gold").arg(slot.price));
        costLabel->setToolTip(QStringLiteral("%1 Gold").arg(slot.price));
    }
}

void ShopView::refreshPotionSlots()
{
    if (m_viewModel == nullptr) {
        return;
    }

    const QVector<ShopViewModel::ShopSlot> &potionSlots = m_viewModel->potionSlots();
    const bool beltFull = m_viewModel->potionBeltFull();
    for (int i = 0; i < m_potionButtons.size(); ++i) {
        QPushButton *button = m_potionButtons.at(i);
        QLabel *costLabel = m_potionCostLabels.at(i);
        if (i >= potionSlots.size() || potionSlots.at(i).sold) {
            ArtAssets::applyToButton(button, QPixmap(), QStringLiteral("Sold"));
            button->setEnabled(false);
            button->setToolTip(QString());
            costLabel->setText(QStringLiteral("-"));
            costLabel->setToolTip(QString());
            continue;
        }

        const ShopViewModel::ShopSlot &slot = potionSlots.at(i);
        ArtAssets::applyToButton(button,
                                 ArtAssets::potionIcon(slot.itemId, QSize(kPotionSize, kPotionSize)),
                                 slot.displayName);
        if (beltFull) {
            button->setToolTip(slot.tooltip.isEmpty()
                                   ? QStringLiteral("Potion belt full")
                                   : QStringLiteral("Potion belt full\n\n%1").arg(slot.tooltip));
        } else {
            button->setToolTip(slot.tooltip);
        }
        button->setEnabled(!beltFull);
        costLabel->setText(QStringLiteral("%1 Gold").arg(slot.price));
        costLabel->setToolTip(QStringLiteral("%1 Gold").arg(slot.price));
    }
}

void ShopView::refreshDeleteCard()
{
    if (m_deleteCardButton == nullptr || m_deleteCardCostLabel == nullptr) {
        return;
    }

    if (m_viewModel == nullptr) {
        m_deleteCardButton->setEnabled(false);
        m_deleteCardButton->setText(QStringLiteral("Delete Card"));
        m_deleteCardCostLabel->setText(QStringLiteral("-"));
        m_deleteCardCostLabel->setVisible(true);
        return;
    }

    const int price = m_viewModel->cardDeletionPrice();
    m_deleteCardButton->setText(QStringLiteral("Delete Card"));
    m_deleteCardCostLabel->setText(QStringLiteral("%1 Gold").arg(price));
    m_deleteCardCostLabel->setToolTip(QStringLiteral("%1 Gold").arg(price));
    m_deleteCardCostLabel->setVisible(true);

    const bool enabled = m_viewModel->canDeleteCard();
    m_deleteCardButton->setEnabled(enabled);
    if (m_viewModel->hasDeletedThisVisit()) {
        m_deleteCardButton->setToolTip(QStringLiteral("Already used this shop visit"));
    } else if (!enabled) {
        m_deleteCardButton->setToolTip(QStringLiteral("Not enough gold"));
    } else {
        m_deleteCardButton->setToolTip(QStringLiteral("Remove one card from your deck"));
    }
}

void ShopView::onCardClicked()
{
    if (m_viewModel == nullptr) {
        return;
    }

    auto *button = qobject_cast<QPushButton *>(sender());
    if (button == nullptr) {
        return;
    }

    m_viewModel->purchaseCard(button->property("slotIndex").toInt());
}

void ShopView::onRelicClicked()
{
    if (m_viewModel == nullptr) {
        return;
    }

    auto *button = qobject_cast<QPushButton *>(sender());
    if (button == nullptr) {
        return;
    }

    m_viewModel->purchaseRelic(button->property("slotIndex").toInt());
}

void ShopView::onPotionClicked()
{
    if (m_viewModel == nullptr) {
        return;
    }

    auto *button = qobject_cast<QPushButton *>(sender());
    if (button == nullptr) {
        return;
    }

    m_viewModel->purchasePotion(button->property("slotIndex").toInt());
}

void ShopView::onDeleteCardClicked()
{
    if (m_viewModel != nullptr) {
        m_viewModel->requestDeleteCard();
    }
}

void ShopView::onLeaveClicked()
{
    if (m_viewModel != nullptr) {
        m_viewModel->requestLeave();
    }
}

void ShopView::onPurchaseDenied(const QString &reason)
{
    if (reason.isEmpty()) {
        return;
    }

    QToolTip::showText(QCursor::pos(), reason, this);
}
