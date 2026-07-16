#include "campfire_view.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#include "viewmodels/campfire_viewmodel.h"

namespace {

constexpr int kWindowWidth = 1920;
constexpr int kWindowHeight = 1080;

constexpr int kTitleX = 660;
constexpr int kTitleY = 200;
constexpr int kTitleWidth = 600;
constexpr int kTitleHeight = 100;

constexpr int kButtonX = 760;
constexpr int kButtonWidth = 400;
constexpr int kButtonHeight = 100;
constexpr int kRestY = 400;
constexpr int kUpgradeY = 550;

constexpr char kButtonStyle[] =
    "QPushButton { color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
    "font-size: 20px; padding: 8px; }"
    "QPushButton:hover:enabled { border-color: #e94560; }"
    "QPushButton:disabled { color: #666666; }";

} // namespace

CampfireView::CampfireView(CampfireViewModel *viewModel, QWidget *parent)
    : QWidget(parent)
    , m_viewModel(viewModel)
{
    setFixedSize(kWindowWidth, kWindowHeight);
    setObjectName(QStringLiteral("gameplayScene"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("#gameplayScene { background-color: transparent; }"));
    setupLayout();

    if (m_viewModel != nullptr) {
        connect(m_viewModel, &CampfireViewModel::canUpgradeChanged, this,
                &CampfireView::updateFromViewModel);
        connect(m_viewModel, &CampfireViewModel::restChosen, this, &CampfireView::restClicked);
        connect(m_viewModel, &CampfireViewModel::upgradeChosen, this, &CampfireView::upgradeClicked);
    }

    updateFromViewModel();
}

void CampfireView::setupLayout()
{
    m_titleLabel = new QLabel(QStringLiteral("Campfire"), this);
    m_titleLabel->setGeometry(kTitleX, kTitleY, kTitleWidth, kTitleHeight);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(
        QStringLiteral("color: #ffffff; background-color: transparent; font-size: 48px;"));

    m_restButton = new QPushButton(QStringLiteral("Rest (Heal 30%)"), this);
    m_restButton->setGeometry(kButtonX, kRestY, kButtonWidth, kButtonHeight);
    m_restButton->setStyleSheet(QString::fromLatin1(kButtonStyle));
    connect(m_restButton, &QPushButton::clicked, this, &CampfireView::onRestClicked);

    m_upgradeButton = new QPushButton(QStringLiteral("Forge (Upgrade a card)"), this);
    m_upgradeButton->setGeometry(kButtonX, kUpgradeY, kButtonWidth, kButtonHeight);
    m_upgradeButton->setStyleSheet(QString::fromLatin1(kButtonStyle));
    connect(m_upgradeButton, &QPushButton::clicked, this, &CampfireView::onUpgradeClicked);
}

void CampfireView::updateFromViewModel()
{
    if (m_upgradeButton == nullptr) {
        return;
    }
    const bool canUpgrade = m_viewModel != nullptr && m_viewModel->canUpgrade();
    m_upgradeButton->setEnabled(canUpgrade);
    m_upgradeButton->setToolTip(canUpgrade ? QString()
                                           : QStringLiteral("No cards in your deck can be upgraded."));
}

void CampfireView::onRestClicked()
{
    if (m_viewModel != nullptr) {
        m_viewModel->requestRest();
    } else {
        emit restClicked();
    }
}

void CampfireView::onUpgradeClicked()
{
    if (m_viewModel != nullptr) {
        m_viewModel->requestUpgrade();
    } else {
        emit upgradeClicked();
    }
}
