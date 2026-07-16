#pragma once



#include <QtWidgets/QFrame>



#include <functional>



class CardWidget : public QFrame

{

    Q_OBJECT



public:

    explicit CardWidget(QWidget *parent = nullptr);



    void setCardData(const QString &id, const QString &name, int cost, const QString &description,

                     const QString &rarity = QStringLiteral("basic"));

    QString cardId() const;

    int handIndex() const;

    void setHandIndex(int handIndex);

    bool isSelected() const;

    void setSelected(bool selected);

    void playUseAnimation(const std::function<void()> &onFinished);



signals:

    void clicked(const QString &cardId);



protected:

    void mousePressEvent(QMouseEvent *event) override;



private:

    QString m_cardId;

    int m_handIndex = -1;

    bool m_selected = false;

    bool m_animating = false;

    static QString defaultStyleSheet();

    static QString selectedStyleSheet();

};

