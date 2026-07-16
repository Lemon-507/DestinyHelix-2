#include "character_factory.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "battle/data/card_database.h"
#include "battle/data/enemy_database.h"
#include "battle/data/relic_database.h"
#include "battle/entities/player.h"
#include "events/event_database.h"
#include "potions/potion_database.h"

bool CharacterFactory::s_initialized = false;

void CharacterFactory::initialize()
{
    loadDatabases();
}

void CharacterFactory::loadDatabases()
{
    if (s_initialized) {
        return;
    }

    Q_INIT_RESOURCE(resources);

    CardDatabase::instance().load();
    EnemyDatabase::instance().load();
    RelicDatabase::instance().load();
    PotionDatabase::instance().load();
    EventDatabase::instance().load();
    s_initialized = true;
}

Player *CharacterFactory::createCharacter(const QString &id)
{
    loadDatabases();

    const QString resourcePath = QStringLiteral(":/resources/data/characters/%1.json").arg(id);
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return nullptr;
    }

    const QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    auto *player = new Player();

    const QJsonObject baseStats = obj.value(QStringLiteral("base_stats")).toObject();
    const int maxHp = baseStats.value(QStringLiteral("max_hp")).toInt(80);
    const int currentHp = baseStats.value(QStringLiteral("current_hp")).toInt(maxHp);
    const int gold = baseStats.value(QStringLiteral("gold")).toInt(0);

    player->setMaxHp(maxHp);
    player->healToFull();
    if (currentHp < maxHp) {
        player->takeDirectDamage(maxHp - currentHp);
    }
    player->setGold(gold);

    const QJsonArray deckEntries = obj.value(QStringLiteral("initial_deck")).toArray();
    for (const QJsonValue &entryValue : deckEntries) {
        const QJsonObject entry = entryValue.toObject();
        const QString cardId = entry.value(QStringLiteral("card_id")).toString();
        const int count = entry.value(QStringLiteral("count")).toInt(1);
        const Card cardTemplate = CardDatabase::instance().getCard(cardId);
        for (int i = 0; i < count; ++i) {
            player->addCardToDeck(cardTemplate);
        }
    }

    const QJsonArray relics = obj.value(QStringLiteral("initial_relics")).toArray();
    for (const QJsonValue &relicValue : relics) {
        player->addRelicId(relicValue.toString());
    }

    const QJsonArray potions = obj.value(QStringLiteral("initial_potions")).toArray();
    for (const QJsonValue &potionValue : potions) {
        player->addPotion(potionValue.toString());
    }

    return player;
}
