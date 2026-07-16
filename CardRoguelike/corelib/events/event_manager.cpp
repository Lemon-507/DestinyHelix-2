#include "event_manager.h"

#include <QtCore/QDebug>
#include <QtCore/QVector>
#include <QtCore/QtGlobal>

#include "battle/data/card_database.h"
#include "battle/data/relic_database.h"
#include "battle/entities/card.h"
#include "battle/entities/player.h"
#include "map/map_manager.h"
#include "potions/potion_database.h"
#include "utils/rng.h"

EventManager::EventManager(QObject *parent)
    : QObject(parent)
{
}

bool EventManager::executeOption(const EventOption &option, Player *player, MapManager *mapManager,
                                 int selectedDeckIndex)
{
    if (player == nullptr) {
        return false;
    }

    for (const EffectDef &effect : option.effects) {
        if (effect.type == QStringLiteral("lose_hp")) {
            player->takeDamage(effect.value);
        } else if (effect.type == QStringLiteral("lose_hp_percent")) {
            const int damage = (player->maxHp() * effect.value) / 100;
            if (damage > 0) {
                player->takeDamage(damage);
            }
        } else if (effect.type == QStringLiteral("gain_random_relic")) {
            RelicDatabase::instance().load();
            const QString relicId = RelicDatabase::instance().randomRelicIdExcluding(player->relicIds());
            if (!relicId.isEmpty()) {
                player->addRelicId(relicId);
            }
        } else if (effect.type == QStringLiteral("add_steps")) {
            if (mapManager != nullptr) {
                mapManager->addSteps(effect.value);
            }
        } else if (effect.type == QStringLiteral("gain_gold")) {
            player->addGold(effect.value);
        } else if (effect.type == QStringLiteral("lose_gold")) {
            if (player->gold() < effect.value) {
                return false;
            }
            player->setGold(player->gold() - effect.value);
        } else if (effect.type == QStringLiteral("lose_gold_random")) {
            const int minGold = effect.value;
            const int maxGold = effect.duration >= minGold ? effect.duration : minGold;
            const int loss = RNG::get(minGold, maxGold);
            player->setGold(qMax(0, player->gold() - loss));
        } else if (effect.type == QStringLiteral("gain_potion")) {
            const QString potionId =
                effect.target.isEmpty() ? QStringLiteral("blood_potion") : effect.target;
            const int count = effect.value > 0 ? effect.value : 1;
            player->addPotion(potionId, count);
        } else if (effect.type == QStringLiteral("gain_random_potion")) {
            PotionDatabase::instance().load();
            const QStringList potionIds = PotionDatabase::instance().allPotionIds();
            if (!potionIds.isEmpty()) {
                const int count = effect.value > 0 ? effect.value : 1;
                for (int i = 0; i < count; ++i) {
                    const int index = RNG::get(0, potionIds.size() - 1);
                    player->addPotion(potionIds.at(index), 1);
                }
            }
        } else if (effect.type == QStringLiteral("restore_hp_percent")) {
            player->heal((player->maxHp() * effect.value) / 100);
        } else if (effect.type == QStringLiteral("max_hp_increase")) {
            player->setMaxHp(player->maxHp() + effect.value);
            player->heal(effect.value);
        } else if (effect.type == QStringLiteral("add_card")) {
            CardDatabase::instance().load();
            const QString cardId = effect.target.isEmpty() ? effect.cardId : effect.target;
            if (!cardId.isEmpty()) {
                const Card card = CardDatabase::instance().getCard(cardId);
                if (!card.id().isEmpty()) {
                    player->addCardToDeck(card);
                }
            }
        } else if (effect.type == QStringLiteral("remove_card")) {
            if (!player->removeCardAt(selectedDeckIndex)) {
                return false;
            }
        } else if (effect.type == QStringLiteral("upgrade_card")) {
            if (!player->upgradeCard(selectedDeckIndex)) {
                return false;
            }
        } else if (effect.type == QStringLiteral("transform_card")) {
            if (!player->transformCardAt(selectedDeckIndex)) {
                return false;
            }
        } else if (effect.type == QStringLiteral("upgrade_random_cards")) {
            CardDatabase::instance().load();
            QVector<int> upgradeable;
            for (int i = 0; i < player->deck().size(); ++i) {
                if (CardDatabase::instance().canUpgrade(player->deck().at(i).id())) {
                    upgradeable.append(i);
                }
            }
            int remaining = effect.value > 0 ? effect.value : 1;
            while (remaining > 0 && !upgradeable.isEmpty()) {
                const int pick = RNG::get(0, upgradeable.size() - 1);
                const int deckIndex = upgradeable.takeAt(pick);
                player->upgradeCard(deckIndex);
                // Indices after deckIndex shift? upgrade replaces in place — indices stay valid.
                --remaining;
            }
        } else {
            qWarning() << "EventManager: unsupported effect" << effect.type;
        }

        if (player->currentHp() <= 0) {
            return false;
        }
    }

    return true;
}

void EventManager::markEventCompleted(const QString &eventId)
{
    m_completedEvents.insert(eventId);
}

QSet<QString> EventManager::completedEvents() const
{
    return m_completedEvents;
}

void EventManager::setCompletedEvents(const QSet<QString> &completedEvents)
{
    m_completedEvents = completedEvents;
}
