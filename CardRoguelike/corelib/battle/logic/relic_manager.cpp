#include "relic_manager.h"

#include <memory>

#include "battle/entities/actor.h"
#include "battle/entities/player.h"
#include "battle/logic/battlefsm.h"
#include "battle/logic/effectresolver.h"
#include "systems/event_bus.h"

RelicManager::RelicManager(QObject *parent)
    : QObject(parent)
{
}

void RelicManager::clear()
{
    unregisterAll();
}

void RelicManager::unregisterAll()
{
    for (const QMetaObject::Connection &connection : m_connections) {
        QObject::disconnect(connection);
    }

    m_connections.clear();
    m_registeredRelics.clear();
    m_registeredIds.clear();
    m_battleFsm = nullptr;
    m_battleEnemies.clear();
    m_penNibAttackCount = 0;
}

void RelicManager::setBattleContext(BattleFSM *fsm, const QVector<Actor *> &enemies)
{
    m_battleFsm = fsm;
    m_battleEnemies = enemies;
}

void RelicManager::clearBattleContext()
{
    m_battleFsm = nullptr;
    m_battleEnemies.clear();
}

void RelicManager::registerRelic(const RelicData &relic, Actor *owner, BattleFSM *fsm)
{
    if (relic.id.isEmpty() || owner == nullptr || m_registeredIds.contains(relic.id)) {
        return;
    }

    if (fsm != nullptr) {
        m_battleFsm = fsm;
    }

    m_registeredRelics.append({relic, owner});
    m_registeredIds.insert(relic.id);

    switch (relic.trigger) {
    case RelicTrigger::OnPickup:
        applyRelicEffects(relic, owner, owner);
        break;

    case RelicTrigger::Passive:
    case RelicTrigger::OnEliteCombatStart:
        break;

    case RelicTrigger::OnBattleStart:
        // Persistent: fire once at the start of every combat (do not one-shot/re-arm).
        m_connections.append(connect(&EventBus::instance(), &EventBus::battleStarted, this,
                                     [this, relic, owner]() {
                                         applyRelicEffects(relic, owner, owner);
                                     }));
        break;

    case RelicTrigger::OnTurnStart:
        m_connections.append(connect(&EventBus::instance(), &EventBus::playerTurnStarted, this,
                                     [this, relic, owner]() {
                                         applyRelicEffects(relic, owner, owner);
                                     }));
        break;

    case RelicTrigger::OnCombatEnd:
        // Persistent: fire once at the end of every victorious combat (do not one-shot/re-arm).
        m_connections.append(connect(&EventBus::instance(), &EventBus::combatEnded, this,
                                     [this, relic, owner](bool playerWon) {
                                         if (playerWon) {
                                             applyRelicEffects(relic, owner, owner);
                                         }
                                     }));
        break;

    case RelicTrigger::OnPotionUse:
        m_connections.append(connect(&EventBus::instance(), &EventBus::potionUsed, this,
                                     [this, relic, owner]() {
                                         applyRelicEffects(relic, owner, owner);
                                     }));
        break;

    case RelicTrigger::OnAttackPlayed:
        m_connections.append(connect(&EventBus::instance(), &EventBus::playerPlayedAttack, this,
                                     [this, relic, owner](int attackCount) {
                                         Q_UNUSED(attackCount);
                                         if (relic.id == QStringLiteral("pen_nib")) {
                                             ++m_penNibAttackCount;
                                             if (m_penNibAttackCount % 10 == 0) {
                                                 if (auto *player = qobject_cast<Player *>(owner)) {
                                                     player->markPenNibDoubleDamage();
                                                 }
                                             }
                                         } else {
                                             applyRelicEffects(relic, owner, owner);
                                         }
                                     }));
        break;

    case RelicTrigger::OnFatalDamage: {
        // One-shot consume: capture Connection via shared_ptr so disconnect works.
        auto connection = std::make_shared<QMetaObject::Connection>();
        *connection = connect(&EventBus::instance(), &EventBus::fatalDamageReceived, this,
                              [this, connection, relic, owner](Actor *victim) {
                                  QObject::disconnect(*connection);
                                  if (victim != owner) {
                                      return;
                                  }

                                  applyRelicEffects(relic, owner, owner);
                                  consumeRelic(relic.id, owner);
                              });
        m_connections.append(*connection);
        break;
    }

    case RelicTrigger::OnDamageReceived:
        m_connections.append(connect(&EventBus::instance(), &EventBus::playerReceivedDamage, this,
                                     [this, relic, owner](int damage, Actor *source, Actor *player) {
                                         Q_UNUSED(damage);
                                         if (player != owner || source == nullptr) {
                                             return;
                                         }

                                         applyRelicEffects(relic, owner, source);
                                     }));
        break;

    case RelicTrigger::OnDamageDealt:
    case RelicTrigger::Unknown:
        break;
    }
}

bool RelicManager::isRegistered(const QString &relicId) const
{
    return m_registeredIds.contains(relicId);
}

QStringList RelicManager::registeredRelicIds() const
{
    return m_registeredIds.values();
}

void RelicManager::applyRelicEffects(const RelicData &relic, Actor *caster, Actor *target)
{
    EffectResolver::instance().execute(relic.effects, caster, target, m_battleEnemies, m_battleFsm);
}

void RelicManager::consumeRelic(const QString &relicId, Actor *owner)
{
    m_registeredIds.remove(relicId);
    for (int i = m_registeredRelics.size() - 1; i >= 0; --i) {
        if (m_registeredRelics.at(i).relic.id == relicId) {
            m_registeredRelics.removeAt(i);
        }
    }

    if (auto *player = qobject_cast<Player *>(owner)) {
        player->removeRelicId(relicId);
    }
}
