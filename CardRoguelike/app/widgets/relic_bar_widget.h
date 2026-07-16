#pragma once

#include <QtCore/QVariant>
#include <QtWidgets/QWidget>

class QLabel;
class QEvent;

class RelicBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RelicBarWidget(QWidget *parent = nullptr);

    void setRelics(const QVariantList &relics);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void showRelicTooltip(QLabel *source, const QString &text);
    void hideRelicTooltip();
    QRect clampedTooltipGeometry(const QLabel *source, const QSize &tooltipSize) const;

    static constexpr int kMaxRelicSlots = 12;
    static constexpr int kRelicSize = 30;
    static constexpr int kRelicStartX = 20;
    static constexpr int kRelicY = 20;
    static constexpr int kRelicGap = 10;
    static constexpr int kTooltipMargin = 4;

    QVector<QLabel *> m_slots;
    QLabel *m_tooltip = nullptr;
};
