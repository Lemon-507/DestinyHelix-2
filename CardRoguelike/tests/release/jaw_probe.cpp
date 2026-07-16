#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include "battle/data/enemy_database.h"
#include "battle/entities/enemy.h"
#include "character_factory.h"
#include "utils/rng.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    CharacterFactory::initialize();
    EnemyDatabase::instance().load();
    const EnemyData* data = EnemyDatabase::instance().getEnemy("jaw_worm");
    qDebug() << "type" << data->behavior.type << "restrictions" << data->behavior.restrictions << "prob" << data->behavior.probability;
    Enemy worm;
    worm.loadFromData(*data);
    RNG::seed(42);
    int chompRepeats = 0;
    int last = -1;
    for (int turn = 0; turn < 50; ++turn) {
        worm.peekNextIntent();
        worm.peekNextIntent();
        const EnemyIntentDef intent = worm.consumeNextIntent();
        const int idx = (intent.name == "Chomp") ? 0 : (intent.name == "Bellow" ? 1 : 2);
        if (idx == 0 && last == 0) ++chompRepeats;
        qDebug() << "turn" << turn << intent.name << "idx" << idx;
        last = idx;
    }
    qDebug() << "chompRepeats" << chompRepeats;
    return chompRepeats > 0 ? 1 : 0;
}
