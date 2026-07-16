#include "reward_view.h"

#include <QtCore/QVariant>
#include <QtGui/QFont>
#include <QtGui/QFontDatabase>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#include "viewmodels/reward_viewmodel.h"

namespace {

constexpr int kWindowWidth = 1920;
constexpr int kWindowHeight = 1080;

constexpr int kTitleX = 760;
constexpr int kTitleY = 80;
constexpr int kTitleWidth = 421;
constexpr int kTitleHeight = 91;

constexpr int kRewardX = 660;
constexpr int kRewardWidth = 600;
constexpr int kRewardHeight = 100;
constexpr int kRewardSlotYs[4] = {250, 400, 550, 700};
constexpr int kRewardSlotCount = 4;

constexpr int kBackButtonX = 810;
constexpr int kBackButtonY = 860;
constexpr int kBackButtonWidth = 300;
constexpr int kBackButtonHeight = 100;

constexpr char kRewardButtonStyle[] =
    "QPushButton { color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
    "font-size: 18px; padding: 8px; }"
    "QPushButton:hover:enabled { border-color: #e94560; }"
    "QPushButton:disabled { color: #666666; }";

constexpr char kBackButtonStyle[] =
    "QPushButton { color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
    "font-size: 16px; padding: 8px; }"
    "QPushButton:hover { border-color: #e94560; }";

QFont academicoFont(int pointSize)
{
    QFont font(QStringLiteral("Academico"));
    font.setPointSize(pointSize);
    if (!QFontDatabase().hasFamily(QStringLiteral("Academico"))) {
        font = QFont();
        font.setPointSize(pointSize);
    }
    return font;
}

} // namespace

RewardView::RewardView(RewardViewModel *viewModel, QWidget *parent)
    : QWidget(parent)
    , m_viewModel(viewModel)
{
    setFixedSize(kWindowWidth, kWindowHeight);
    setObjectName(QStringLiteral("gameplayScene"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("#gameplayScene { background-color: transparent; }"));
    setupLayout();

    if (m_viewModel != nullptr) {
        connect(m_viewModel, &RewardViewModel::offersChanged, this, &RewardView::updateFromViewModel);
    }

    updateFromViewModel();
}

void RewardView::setupLayout()
{
    m_titleLabel = new QLabel(QStringLiteral("You win!"), this);
    m_titleLabel->setGeometry(kTitleX, kTitleY, kTitleWidth, kTitleHeight);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(
        QStringLiteral("color: #ffffff; background-color: transparent; font-size: 48pt;"));
    m_titleLabel->setFont(academicoFont(48));

    m_rewardSlots.reserve(kRewardSlotCount);
    for (int i = 0; i < kRewardSlotCount; ++i) {
        auto *slot = new QPushButton(this);
        slot->setGeometry(kRewardX, kRewardSlotYs[i], kRewardWidth, kRewardHeight);
        slot->setStyleSheet(QString::fromLatin1(kRewardButtonStyle));
        slot->setProperty("rewardType", QVariant(-1));
        connect(slot, &QPushButton::clicked, this, &RewardView::onRewardSlotClicked);
        slot->hide();
        m_rewardSlots.append(slot);
    }

    m_backButton = new QPushButton(QStringLiteral("Back to map"), this);
    m_backButton->setGeometry(kBackButtonX, kBackButtonY, kBackButtonWidth, kBackButtonHeight);
    m_backButton->setStyleSheet(QString::fromLatin1(kBackButtonStyle));
    m_backButton->setFont(academicoFont(16));
    connect(m_backButton, &QPushButton::clicked, this, &RewardView::backToMapClicked);
}

void RewardView::populateRewardSlots()
{
    if (m_viewModel == nullptr) {
        return;
    }

    const QVector<RewardViewModel::RewardOffer> &offers = m_viewModel->offers();

    QVector<RewardViewModel::RewardType> activeRewards;
    activeRewards.reserve(kRewardSlotCount);
    for (const RewardViewModel::RewardOffer &offer : offers) {
        if (offer.granted && !offer.claimed) {
            activeRewards.append(offer.type);
        }
    }

    for (int slotIndex = 0; slotIndex < m_rewardSlots.size(); ++slotIndex) {
        QPushButton *slot = m_rewardSlots.at(slotIndex);
        if (slotIndex < activeRewards.size()) {
            const RewardViewModel::RewardType type = activeRewards.at(slotIndex);
            const int offerIndex = static_cast<int>(type);
            if (offerIndex < 0 || offerIndex >= offers.size()) {
                slot->hide();
                slot->setToolTip(QString());
                slot->setProperty("rewardType", QVariant(-1));
                continue;
            }

            const RewardViewModel::RewardOffer &offer = offers.at(offerIndex);
            slot->setText(offer.buttonText);
            slot->setToolTip(offer.tooltip);
            slot->setProperty("rewardType", QVariant(offerIndex));
            slot->setEnabled(true);
            slot->show();
        } else {
            slot->hide();
            slot->setToolTip(QString());
            slot->setProperty("rewardType", QVariant(-1));
        }
    }
}

void RewardView::updateFromViewModel()
{
    populateRewardSlots();
}

void RewardView::onRewardSlotClicked()
{
    if (m_viewModel == nullptr) {
        return;
    }

    auto *slot = qobject_cast<QPushButton *>(sender());
    if (slot == nullptr) {
        return;
    }

    const int typeIndex = slot->property("rewardType").toInt();
    if (typeIndex < 0 || typeIndex > static_cast<int>(RewardViewModel::RewardType::Relic)) {
        return;
    }

    const auto type = static_cast<RewardViewModel::RewardType>(typeIndex);
    if (m_viewModel->claimReward(type)) {
        populateRewardSlots();
    }
}
