#include "death_view.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

namespace {

constexpr int kWindowWidth = 1920;
constexpr int kWindowHeight = 1080;

constexpr char kMenuButtonStyle[] =
    "QPushButton { color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
    "font-size: 16px; padding: 8px; min-width: 300px; min-height: 100px; }"
    "QPushButton:hover { border-color: #e94560; }";

} // namespace

DeathView::DeathView(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(kWindowWidth, kWindowHeight);
    setObjectName(QStringLiteral("gameplayScene"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("#gameplayScene { background-color: transparent; }"));
    setupLayout();
}

void DeathView::setupLayout()
{
    m_titleLabel = new QLabel(QStringLiteral("You died."), this);
    m_titleLabel->setGeometry(760, 300, 400, 100);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(QStringLiteral("color: #ffffff; background-color: transparent; font-size: 48px;"));

    m_menuButton = new QPushButton(QStringLiteral("Return to Menu"), this);
    m_menuButton->setGeometry(810, 500, 300, 100);
    m_menuButton->setStyleSheet(QString::fromLatin1(kMenuButtonStyle));
    connect(m_menuButton, &QPushButton::clicked, this, &DeathView::returnToMenuClicked);
}
