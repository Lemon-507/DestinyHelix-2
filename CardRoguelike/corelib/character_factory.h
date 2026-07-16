#pragma once

#include <QtCore/QString>

class Player;

class CharacterFactory
{
public:
    static void initialize();
    static Player *createCharacter(const QString &id);

private:
    static void loadDatabases();
    static bool s_initialized;
};
