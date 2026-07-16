#include "card_collection_view.h"

#include <QtCore/QVariant>
#include <QtGui/QShowEvent>
#include <QtWidgets/QPushButton>

#include "viewmodels/card_collection_viewmodel.h"

namespace {

constexpr int kWindowWidth = 1920;
constexpr int kWindowHeight = 1080;

constexpr int kCardWidth = 100;
constexpr int kCardHeight = 160;
constexpr int kCardStartX = 40;
constexpr int kCardStepX = 140;
constexpr int kColumns = 13; // x = 40 + i*140, i in [0,12] → max 1720 (fits 1920)
constexpr int kRows = 5;
constexpr int kRowYs[kRows] = {80, 280, 480, 680, 880};
constexpr int kSlotCount = kRows * kColumns;

constexpr int kExitX = 1730;
constexpr int kExitY = 910;
constexpr int kExitWidth = 141;
constexpr int kExitHeight = 91;

constexpr char kCardButtonStyle[] =
    "QPushButton { color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
    "font-size: 11px; padding: 4px; }"
    "QPushButton:hover:enabled { border-color: #e94560; }"
    "QPushButton:disabled { color: #666666; }";

constexpr char kExitButtonStyle[] =
    "QPushButton { color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
    "font-size: 16px; padding: 8px; }"
    "QPushButton:hover { border-color: #e94560; }";

} // namespace

CardCollectionView::CardCollectionView(CardCollectionViewModel *viewModel, QWidget *parent)
    : QWidget(parent)
    , m_viewModel(viewModel)
{
    setFixedSize(kWindowWidth, kWindowHeight);
    setObjectName(QStringLiteral("gameplayScene"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("#gameplayScene { background-color: transparent; }"));
    setupLayout();

    if (m_viewModel != nullptr) {
        connect(m_viewModel, &CardCollectionViewModel::cardsChanged, this,
                &CardCollectionView::updateFromViewModel);
        connect(m_viewModel, &CardCollectionViewModel::exitRequested, this,
                &CardCollectionView::exitClicked);
        connect(m_viewModel, &CardCollectionViewModel::selectionCancelled, this,
                &CardCollectionView::selectionCancelled);
    }

    updateFromViewModel();
}

void CardCollectionView::startSelectionMode(const QString &actionType)
{
    m_selectionMode = true;
    if (m_viewModel != nullptr) {
        m_viewModel->setSelectionAction(actionType);
    } else {
        populateSlots();
    }
    show();
}

void CardCollectionView::clearSelectionMode()
{
    m_selectionMode = false;
    if (m_viewModel != nullptr) {
        m_viewModel->clearSelectionMode();
    } else {
        populateSlots();
    }
}

void CardCollectionView::setupLayout()
{
    m_cardSlots.reserve(kSlotCount);
    for (int row = 0; row < kRows; ++row) {
        for (int col = 0; col < kColumns; ++col) {
            auto *slot = new QPushButton(this);
            const int x = kCardStartX + col * kCardStepX;
            const int y = kRowYs[row];
            slot->setGeometry(x, y, kCardWidth, kCardHeight);
            slot->setStyleSheet(QString::fromLatin1(kCardButtonStyle));
            slot->setVisible(false);
            slot->setProperty("deckIndex", QVariant(-1));
            slot->setProperty("cardId", QVariant(QString()));
            connect(slot, &QPushButton::clicked, this, &CardCollectionView::onCardSlotClicked);
            m_cardSlots.append(slot);
        }
    }

    m_exitButton = new QPushButton(QStringLiteral("Exit"), this);
    m_exitButton->setGeometry(kExitX, kExitY, kExitWidth, kExitHeight);
    m_exitButton->setStyleSheet(QString::fromLatin1(kExitButtonStyle));
    m_exitButton->raise();
    connect(m_exitButton, &QPushButton::clicked, this, &CardCollectionView::onExitClicked);
}

void CardCollectionView::populateSlots()
{
    const int cardCount = m_viewModel != nullptr ? m_viewModel->cards().size() : 0;
    const bool selectionMode =
        m_selectionMode || (m_viewModel != nullptr && m_viewModel->selectionMode());

    for (int i = 0; i < m_cardSlots.size(); ++i) {
        QPushButton *slot = m_cardSlots.at(i);
        if (m_viewModel != nullptr && i < cardCount) {
            const CardCollectionViewModel::CardEntry &entry = m_viewModel->cards().at(i);
            slot->setText(entry.displayName);
            slot->setToolTip(entry.tooltip);
            slot->setProperty("deckIndex", entry.deckIndex);
            slot->setProperty("cardId", entry.id);
            slot->setEnabled(!selectionMode || entry.selectable);
            slot->setVisible(true);
        } else {
            slot->setText(QString());
            slot->setToolTip(QString());
            slot->setProperty("deckIndex", QVariant(-1));
            slot->setProperty("cardId", QVariant(QString()));
            slot->setEnabled(false);
            slot->setVisible(false);
        }
    }

    if (m_exitButton != nullptr) {
        m_exitButton->raise();
    }
}

void CardCollectionView::updateFromViewModel()
{
    populateSlots();
}

void CardCollectionView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (m_viewModel != nullptr) {
        m_viewModel->refresh();
    } else {
        populateSlots();
    }
}

void CardCollectionView::onExitClicked()
{
    if (m_viewModel != nullptr) {
        m_viewModel->requestExit();
    } else if (m_selectionMode) {
        emit selectionCancelled();
    } else {
        emit exitClicked();
    }
}

void CardCollectionView::onCardSlotClicked()
{
    const bool selectionMode =
        m_selectionMode || (m_viewModel != nullptr && m_viewModel->selectionMode());
    if (!selectionMode) {
        return;
    }

    auto *slot = qobject_cast<QPushButton *>(sender());
    if (slot == nullptr || !slot->isEnabled()) {
        return;
    }

    const int deckIndex = slot->property("deckIndex").toInt();
    const QString cardId = slot->property("cardId").toString();
    if (deckIndex < 0 || cardId.isEmpty()) {
        return;
    }

    emit cardSelected(cardId, deckIndex);
}
