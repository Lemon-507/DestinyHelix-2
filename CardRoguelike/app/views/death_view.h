#pragma once

#include <QtWidgets/QWidget>

class QLabel;
class QPushButton;

class DeathView : public QWidget
{
    Q_OBJECT

public:
    explicit DeathView(QWidget *parent = nullptr);

signals:
    void returnToMenuClicked();

private:
    void setupLayout();

    QLabel *m_titleLabel = nullptr;
    QPushButton *m_menuButton = nullptr;
};
