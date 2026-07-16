#include "card_widget.h"



#include <QtCore/QPropertyAnimation>

#include <QtGui/QMouseEvent>

#include <QtWidgets/QHBoxLayout>

#include <QtWidgets/QLabel>

#include <QtWidgets/QVBoxLayout>



CardWidget::CardWidget(QWidget *parent)

    : QFrame(parent)

{

    setFixedSize(120, 160);

    setCursor(Qt::PointingHandCursor);

    setObjectName(QStringLiteral("cardWidget"));



    auto *layout = new QVBoxLayout(this);

    layout->setContentsMargins(8, 8, 8, 8);

    layout->setSpacing(4);



    auto *header = new QHBoxLayout();

    auto *nameLabel = new QLabel(this);

    nameLabel->setObjectName(QStringLiteral("cardName"));

    nameLabel->setWordWrap(true);

    nameLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #fff;"));



    auto *costLabel = new QLabel(this);

    costLabel->setObjectName(QStringLiteral("cardCost"));

    costLabel->setAlignment(Qt::AlignCenter);

    costLabel->setFixedSize(24, 24);

    costLabel->setStyleSheet(

        QStringLiteral("background: #e94560; border-radius: 12px; color: #fff; font-weight: bold;"));



    header->addWidget(nameLabel, 1);

    header->addWidget(costLabel);



    auto *descLabel = new QLabel(this);

    descLabel->setObjectName(QStringLiteral("cardDesc"));

    descLabel->setWordWrap(true);

    descLabel->setAlignment(Qt::AlignTop);

    descLabel->setStyleSheet(QStringLiteral("color: #ccc; font-size: 11px;"));



    layout->addLayout(header);

    layout->addWidget(descLabel, 1);



    setStyleSheet(defaultStyleSheet());

}



QString CardWidget::defaultStyleSheet()
{
    return QStringLiteral(
        "CardWidget#cardWidget { background-color: #16213e; border: 2px solid #0f3460; border-radius: 8px; }"
        "CardWidget#cardWidget:hover { border-color: #e94560; }");
}

QString CardWidget::selectedStyleSheet()
{
    return QStringLiteral(
        "CardWidget#cardWidget { background-color: #16213e; border: 2px solid #4a90e2; border-radius: 8px; }");
}



void CardWidget::setCardData(const QString &id, const QString &name, int cost, const QString &description,

                             const QString &rarity)

{

    Q_UNUSED(rarity);

    m_cardId = id;

    m_selected = false;

    findChild<QLabel *>(QStringLiteral("cardName"))->setText(name);

    findChild<QLabel *>(QStringLiteral("cardCost"))->setText(QString::number(cost));

    findChild<QLabel *>(QStringLiteral("cardDesc"))->setText(description);

    setToolTip(description);

}



QString CardWidget::cardId() const

{

    return m_cardId;

}



int CardWidget::handIndex() const
{
    return m_handIndex;
}

void CardWidget::setHandIndex(int handIndex)
{
    m_handIndex = handIndex;
}

bool CardWidget::isSelected() const
{
    return m_selected;
}

void CardWidget::setSelected(bool selected)
{
    m_selected = selected;
    setStyleSheet(selected ? selectedStyleSheet() : defaultStyleSheet());
}



void CardWidget::playUseAnimation(const std::function<void()> &onFinished)

{

    if (m_animating) {

        return;

    }



    m_animating = true;

    setEnabled(false);



    const QRect start = geometry();

    auto *scaleAnim = new QPropertyAnimation(this, "geometry", this);

    scaleAnim->setDuration(180);

    scaleAnim->setStartValue(start);

    scaleAnim->setEndValue(start.adjusted(-10, -14, 10, 14));



    auto *opacityAnim = new QPropertyAnimation(this, "windowOpacity", this);

    opacityAnim->setDuration(180);

    opacityAnim->setStartValue(1.0);

    opacityAnim->setEndValue(0.0);



    connect(opacityAnim, &QPropertyAnimation::finished, this, [this, onFinished]() {

        m_animating = false;

        if (onFinished) {

            onFinished();

        }

    });



    scaleAnim->start(QAbstractAnimation::DeleteWhenStopped);

    opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);

}



void CardWidget::mousePressEvent(QMouseEvent *event)

{

    if (event->button() == Qt::LeftButton && !m_animating) {

        emit clicked(m_cardId);

    }

    QFrame::mousePressEvent(event);

}

