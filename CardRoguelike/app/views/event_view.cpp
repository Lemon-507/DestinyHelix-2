#include "event_view.h"

#include <QtGui/QFont>
#include <QtGui/QPixmap>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGraphicsPixmapItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsTextItem>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#include "utils/art_assets.h"
#include "viewmodels/event_viewmodel.h"

namespace {

constexpr int kWindowWidth = 1920;
constexpr int kWindowHeight = 1080;

constexpr int kImageX = 100;
constexpr int kImageY = 170;
constexpr int kImageSize = 600;

constexpr int kNameX = 140;
constexpr int kNameY = 50;
constexpr int kNameWidth = 500;
constexpr int kNameHeight = 100;

constexpr int kDescX = 790;
constexpr int kDescY = 140;
constexpr int kDescWidth = 900;
constexpr int kDescHeight = 350;

constexpr int kOptionX = 790;
constexpr int kOptionWidth = 900;
constexpr int kOptionHeight = 50;
constexpr int kOptionYs[4] = {600, 700, 800, 900};
constexpr int kOptionCount = 4;

constexpr int kExitX = 790;
constexpr int kExitY = 1000;
constexpr int kExitWidth = 900;
constexpr int kExitHeight = 50;

constexpr char kOptionButtonStyle[] =
    "QPushButton { color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
    "font-size: 16px; padding: 6px 12px; text-align: left; }"
    "QPushButton:hover:enabled { border-color: #e94560; }"
    "QPushButton:disabled { color: #666666; }";

constexpr char kExitButtonStyle[] =
    "QPushButton { color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
    "font-size: 18px; padding: 6px 12px; }"
    "QPushButton:hover { border-color: #e94560; }";

} // namespace

EventView::EventView(EventViewModel *viewModel, QWidget *parent)
    : QWidget(parent)
    , m_viewModel(viewModel)
{
    setFixedSize(kWindowWidth, kWindowHeight);
    setObjectName(QStringLiteral("gameplayScene"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("#gameplayScene { background-color: transparent; }"));
    setupLayout();

    if (m_viewModel != nullptr) {
        connect(m_viewModel, &EventViewModel::eventDataChanged, this, &EventView::updateFromViewModel);
        connect(m_viewModel, &EventViewModel::optionSelectedChanged, this,
                &EventView::updateFromViewModel);
        connect(m_viewModel, &EventViewModel::exitRequested, this, &EventView::exitClicked);
    }

    updateFromViewModel();
}

void EventView::setupLayout()
{
    m_eventImage = new QGraphicsView(this);
    m_eventImage->setObjectName(QStringLiteral("eventimage"));
    m_eventImage->setGeometry(kImageX, kImageY, kImageSize, kImageSize);
    m_eventImage->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_eventImage->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_eventImage->setFrameShape(QFrame::Box);
    m_eventImage->setStyleSheet(
        QStringLiteral("background-color: #16213e; border: 1px solid #0f3460;"));

    auto *scene = new QGraphicsScene(m_eventImage);
    scene->setSceneRect(0, 0, kImageSize, kImageSize);
    m_eventImage->setScene(scene);

    auto *placeholder = scene->addText(QStringLiteral("Event Image"));
    placeholder->setObjectName(QStringLiteral("eventImagePlaceholder"));
    placeholder->setDefaultTextColor(Qt::white);
    QFont imageFont = placeholder->font();
    imageFont.setPointSize(24);
    placeholder->setFont(imageFont);
    const QRectF textRect = placeholder->boundingRect();
    placeholder->setPos((kImageSize - textRect.width()) / 2.0,
                        (kImageSize - textRect.height()) / 2.0);

    m_eventName = new QLabel(this);
    m_eventName->setObjectName(QStringLiteral("eventname"));
    m_eventName->setGeometry(kNameX, kNameY, kNameWidth, kNameHeight);
    m_eventName->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_eventName->setWordWrap(true);
    m_eventName->setStyleSheet(
        QStringLiteral("color: #ffffff; background-color: transparent; font-size: 28px;"));

    m_eventDescription = new QLabel(this);
    m_eventDescription->setObjectName(QStringLiteral("eventdescription"));
    m_eventDescription->setGeometry(kDescX, kDescY, kDescWidth, kDescHeight);
    m_eventDescription->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_eventDescription->setWordWrap(true);
    QFont descriptionFont = m_eventDescription->font();
    descriptionFont.setPixelSize(20);
    m_eventDescription->setFont(descriptionFont);
    m_eventDescription->setStyleSheet(
        QStringLiteral("color: #e0e0e0; background-color: transparent; font-size: 20px;"));

    m_optionButtons.reserve(kOptionCount);
    for (int i = 0; i < kOptionCount; ++i) {
        auto *button = new QPushButton(this);
        button->setObjectName(QStringLiteral("option%1").arg(i + 1));
        button->setGeometry(kOptionX, kOptionYs[i], kOptionWidth, kOptionHeight);
        button->setStyleSheet(QString::fromLatin1(kOptionButtonStyle));
        button->setProperty("optionIndex", i);
        connect(button, &QPushButton::clicked, this, &EventView::onOptionClicked);
        button->hide();
        m_optionButtons.append(button);
    }

    m_exitButton = new QPushButton(QStringLiteral("Exit"), this);
    m_exitButton->setObjectName(QStringLiteral("exit"));
    m_exitButton->setGeometry(kExitX, kExitY, kExitWidth, kExitHeight);
    m_exitButton->setStyleSheet(QString::fromLatin1(kExitButtonStyle));
    connect(m_exitButton, &QPushButton::clicked, this, &EventView::onExitClicked);
    m_exitButton->hide();
}

void EventView::updateFromViewModel()
{
    if (m_viewModel == nullptr) {
        return;
    }

    m_eventName->setText(m_viewModel->eventName());
    m_eventDescription->setText(m_viewModel->eventDescription());
    refreshEventImage();
    refreshOptionButtons();
    refreshExitVisibility();
}

void EventView::refreshEventImage()
{
    if (m_eventImage == nullptr || m_eventImage->scene() == nullptr) {
        return;
    }

    QGraphicsScene *scene = m_eventImage->scene();
    QList<QGraphicsItem *> items = scene->items();
    for (QGraphicsItem *item : items) {
        if (item->data(0).toString() == QStringLiteral("eventArt")) {
            scene->removeItem(item);
            delete item;
        }
    }

    QGraphicsTextItem *placeholder = nullptr;
    for (QGraphicsItem *item : scene->items()) {
        auto *textItem = qgraphicsitem_cast<QGraphicsTextItem *>(item);
        if (textItem != nullptr && textItem->toPlainText() == QStringLiteral("Event Image")) {
            placeholder = textItem;
            break;
        }
    }

    const QString eventId = m_viewModel != nullptr ? m_viewModel->eventData().id : QString();
    const QPixmap art = ArtAssets::eventIllustration(eventId, QSize(kImageSize, kImageSize));
    if (!art.isNull()) {
        auto *pixmapItem = scene->addPixmap(art);
        pixmapItem->setData(0, QStringLiteral("eventArt"));
        const QRectF bounds = pixmapItem->boundingRect();
        pixmapItem->setPos((kImageSize - bounds.width()) / 2.0,
                           (kImageSize - bounds.height()) / 2.0);
        if (placeholder != nullptr) {
            placeholder->hide();
        }
    } else if (placeholder != nullptr) {
        placeholder->show();
    }
}

void EventView::refreshOptionButtons()
{
    if (m_viewModel == nullptr) {
        return;
    }

    const bool selected = m_viewModel->optionSelected();
    const int optionCount = m_viewModel->optionCount();

    for (int i = 0; i < m_optionButtons.size(); ++i) {
        QPushButton *button = m_optionButtons.at(i);
        if (selected || i >= optionCount) {
            button->hide();
            button->setEnabled(false);
            continue;
        }

        button->setText(m_viewModel->optionButtonText(i));
        const bool available = m_viewModel->isOptionAvailable(i);
        button->setEnabled(available);
        button->show();
    }
}

void EventView::refreshExitVisibility()
{
    if (m_exitButton == nullptr || m_viewModel == nullptr) {
        return;
    }

    if (m_viewModel->optionSelected()) {
        m_exitButton->show();
    } else {
        m_exitButton->hide();
    }
}

void EventView::onOptionClicked()
{
    if (m_viewModel == nullptr || m_viewModel->optionSelected()) {
        return;
    }

    auto *button = qobject_cast<QPushButton *>(sender());
    if (button == nullptr) {
        return;
    }

    const int index = button->property("optionIndex").toInt();
    if (!m_viewModel->isOptionAvailable(index)) {
        return;
    }
    m_viewModel->selectOption(index);
}

void EventView::onExitClicked()
{
    if (m_viewModel == nullptr) {
        return;
    }
    m_viewModel->requestExit();
}
