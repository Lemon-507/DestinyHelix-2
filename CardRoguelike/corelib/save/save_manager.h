#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include "save/game_state.h"

class SaveManager : public QObject
{
    Q_OBJECT

public:
    explicit SaveManager(QObject *parent = nullptr);

    void saveGame(const QString &path, const GameState &state);
    bool loadGame(const QString &path, GameState &state);
};
