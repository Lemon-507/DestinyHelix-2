#include "relic_bar_widget.h"

#include <QEvent>
#include <QtGui/QPixmap>
#include <QtWidgets/QLabel>

#include "utils/art_assets.h"

namespace {

constexpr char kFilledSlotStyle[] =
    "background-color: #16213e; border: 1px solid #0f3460; color: #e0e0e0; font-size: 10px;";

} // namespace

RelicBarWidget::RelicBarWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setFixedSize(1920, kRelicY + kRelicSize);

    QWidget *tooltipParent = window() != nullptr ? window() : this;
    m_tooltip = new QLabel(tooltipParent);
    m_tooltip->setWordWrap(true);
    m_tooltip->setAlignment(Qt::AlignCenter);
    m_tooltip->setStyleSheet(
        QStringLiteral("color: #ffffff; background-color: #16213e; border: 1px solid #0f3460; "
                       "padding: 6px 10px; font-size: 12px;"));
    m_tooltip->hide();

    m_slots.reserve(kMaxRelicSlots);
    for (int i = 0; i < kMaxRelicSlots; ++i) {
        auto *slot = new QLabel(this);
        slot->setObjectName(QStringLiteral("relicSlot"));
        slot->setFixedSize(kRelicSize, kRelicSize);
        slot->setAlignment(Qt::AlignCenter);
        slot->setStyleSheet(QString::fromLatin1(kFilledSlotStyle));
        slot->setAttribute(Qt::WA_Hover, true);
        slot->installEventFilter(this);
        slot->hide();
        m_slots.append(slot);
    }
}

void RelicBarWidget::setRelics(const QVariantList &relics)
{
    hideRelicTooltip();

    const int visibleCount = qMin(relics.size(), kMaxRelicSlots);
    for (int i = 0; i < m_slots.size(); ++i) {
        QLabel *slot = m_slots.at(i);
        if (i < visibleCount) {
            const QVariantMap entry = relics.at(i).toMap();
            const QString abbreviation = entry.value(QStringLiteral("abbreviation")).toString();
            const QString name = entry.value(QStringLiteral("name")).toString();
            const QString description = entry.value(QStringLiteral("description")).toString();
            const QString relicId = entry.value(QStringLiteral("id")).toString();

            ArtAssets::applyToLabel(slot, ArtAssets::relicIcon(relicId, QSize(kRelicSize, kRelicSize)),
                                    abbreviation);
            slot->setProperty("relicTooltip", description.isEmpty() ? name : description);
            slot->setGeometry(kRelicStartX + i * (kRelicSize + kRelicGap), kRelicY, kRelicSize, kRelicSize);
            slot->show();
            slot->raise();
        } else {
            slot->clear();
            slot->setPixmap(QPixmap());
            slot->setProperty("relicTooltip", QString());
            slot->hide();
        }
    }
}

bool RelicBarWidget::eventFilter(QObject *watched, QEvent *event)
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

    return QWidget::eventFilter(watched, event);
}

QRect RelicBarWidget::clampedTooltipGeometry(const QLabel *source, const QSize &tooltipSize) const
{
    QWidget *hostWindow = window();
    if (hostWindow == nullptr || source == nullptr || m_tooltip == nullptr) {
        return QRect();
    }

    const QRect windowRect = hostWindow->rect();
    const QPoint slotTopLeft = source->mapTo(hostWindow, QPoint(0, 0));
    const QPoint slotBottomLeft = source->mapTo(hostWindow, QPoint(0, source->height()));

    int x = slotTopLeft.x() + (source->width() - tooltipSize.width()) / 2;
    int y = slotBottomLeft.y() + kTooltipMargin;

    if (y + tooltipSize.height() > windowRect.bottom() - kTooltipMargin) {
        y = slotTopLeft.y() - tooltipSize.height() - kTooltipMargin;
    }

    if (x < windowRect.left() + kTooltipMargin) {
        x = windowRect.left() + kTooltipMargin;
    }
    if (x + tooltipSize.width() > windowRect.right() - kTooltipMargin) {
        x = windowRect.right() - kTooltipMargin - tooltipSize.width();
    }
    if (y < windowRect.top() + kTooltipMargin) {
        y = windowRect.top() + kTooltipMargin;
    }
    if (y + tooltipSize.height() > windowRect.bottom() - kTooltipMargin) {
        y = windowRect.bottom() - kTooltipMargin - tooltipSize.height();
    }

    return QRect(x, y, tooltipSize.width(), tooltipSize.height());
}

void RelicBarWidget::showRelicTooltip(QLabel *source, const QString &text)
{
    if (m_tooltip == nullptr || source == nullptr || text.isEmpty()) {
        return;
    }

    QWidget *hostWindow = window();
    if (hostWindow != nullptr && m_tooltip->parentWidget() != hostWindow) {
        m_tooltip->setParent(hostWindow);
    }

    m_tooltip->setText(text);
    m_tooltip->adjustSize();

    const QRect tooltipRect = clampedTooltipGeometry(source, m_tooltip->size());
    m_tooltip->setGeometry(tooltipRect);
    m_tooltip->show();
    m_tooltip->raise();
}

void RelicBarWidget::hideRelicTooltip()
{
    if (m_tooltip != nullptr) {
        m_tooltip->hide();
    }
}
