#include "battlefsm.h"

#include <QtCore/QDebug>
#include <QtCore/QMetaObject>

#include "battle/data/enemy_database.h"
#include "battle/entities/player.h"
#include "battle/entities/shrink_damage_power.h"
#include "battle/logic/effectresolver.h"
#include "battle/logic/power_factory.h"
#include "systems/event_bus.h"
#include "utils/rng.h"

BattleFSM::BattleFSM(QObject *parent)
    : QObject(parent)
{
}

int BattleFSM::currentEnergy() const
{
    return m_currentEnergy;
}

int BattleFSM::maxEnergy() const
{
    return m_maxEnergy;
}

QVector<Card> BattleFSM::hand() const
{
    return m_hand;
}

int BattleFSM::drawPileCount() const
{
    return m_drawPile.size();
}

int BattleFSM::discardPileCount() const
{
    return m_discardPile.size();
}

int BattleFSM::exhaustPileCount() const
{
    return m_exhaustPile.size();
}

int BattleFSM::cardDamageBonus(const QString &cardId) const
{
    return m_cardDamageBonus.value(cardId, 0);
}

BattleState BattleFSM::battleState() const
{
    return m_state;
}

QString BattleFSM::enemyIntent() const
{
    return m_enemyIntent;
}

int BattleFSM::enemyIntentValue() const
{
    return m_enemyIntentValue;
}

ShrinkDamagePower *BattleFSM::shrinkPower() const
{
    return m_shrinkPowerPtr;
}

QVector<Enemy *> BattleFSM::enemies() const
{
    return m_enemies;
}

void BattleFSM::adoptEncounter(std::vector<std::unique_ptr<Enemy>> enemies)
{
    m_encounterOwned = std::move(enemies);
}

void BattleFSM::releaseEncounter()
{
    if (m_shrinkConnection) {
        QObject::disconnect(m_shrinkConnection);
        m_shrinkConnection = {};
    }

    clearShrinkPower();
    m_enemies.clear();
    m_ownedEnemies.clear();
    m_encounterOwned.clear();
    m_enemy = nullptr;
    m_hand.clear();
    m_drawPile.clear();
    m_discardPile.clear();
    m_exhaustPile.clear();
    m_player = nullptr;
    m_state = BattleState::Idle;
    m_enemyIntent.clear();
    m_enemyIntentValue = 0;
}

void BattleFSM::startCombat(Player *player, const QVector<Enemy *> &enemies, int shrinkCount)
{
    if (player == nullptr || enemies.isEmpty()) {
        return;
    }

    m_ownedEnemies.clear();
    m_player = player;
    m_enemies = enemies;
    refreshEncounterPeers();
    m_enemy = activeEnemy();
    if (m_enemy == nullptr) {
        return;
    }
    m_turnNumber = 0;
    m_attackCardsPlayed = 0;

    if (m_shrinkConnection) {
        QObject::disconnect(m_shrinkConnection);
        m_shrinkConnection = {};
    }

    // Drop any leftover combat-only shrink buff before (re)applying for this fight.
    clearShrinkPower();

    m_hand.clear();
    m_discardPile.clear();
    m_exhaustPile.clear();
    m_cardDamageBonus.clear();
    m_deckReference = player->deck();
    m_drawPile = m_deckReference;
    shuffleDrawPile();

    m_maxEnergy = 3;
    m_currentEnergy = m_maxEnergy;
    m_state = BattleState::Idle;

    // shrinkCount > 0 means combat started on a node inside the shrink zone (caller must gate).
    applyShrinkPower(shrinkCount);

    m_shrinkConnection = QObject::connect(
        &EventBus::instance(),
        &EventBus::shrinkTriggered,
        this,
        [this](int newShrinkCount) {
            m_shrinkCount = newShrinkCount;
            if (m_shrinkActive && m_shrinkPowerPtr != nullptr) {
                m_shrinkPowerPtr->setDamage(newShrinkCount);
            }
        });

    EventBus::instance().triggerBattleStarted();
    updateEnemyIntentPreview();
    beginPlayerTurn();
}

void BattleFSM::beginPlayerTurn()
{
    if (m_player == nullptr || m_enemy == nullptr) {
        return;
    }

    if (m_player->currentHp() <= 0) {
        endCombat(false);
        return;
    }

    ++m_turnNumber;
    m_state = BattleState::PlayerTurnStart;
    m_player->beginTurn();

    if (m_player->currentHp() <= 0) {
        endCombat(false);
        return;
    }

    drawCardsToHand();
    m_currentEnergy = m_maxEnergy;

    EventBus::instance().triggerPlayerTurnStarted();
    m_state = BattleState::PlayerTurn;
    updateEnemyIntentPreview();
}

void BattleFSM::playerEndTurn()
{
    if (m_state != BattleState::PlayerTurn || m_player == nullptr) {
        return;
    }

    processEndOfTurnHandKeywords();
    if (m_state == BattleState::Ended) {
        return;
    }

    m_player->endTurn();
    // Project rule: player Block clears at end of player turn (before enemy acts).
    m_player->clearBlock();
    discardHand();

    EventBus::instance().triggerPlayerTurnEnded();
    enemyTurn();
}

void BattleFSM::enemyTurn()
{
    if (m_player == nullptr) {
        return;
    }

    m_state = BattleState::EnemyTurn;

    // Snapshot turn order so mid-turn splits don't act (or invalidate iterators).
    const QVector<Enemy *> turnOrder = m_enemies;
    for (Enemy *enemy : turnOrder) {
        if (m_player->currentHp() <= 0) {
            endCombat(false);
            return;
        }
        if (enemy == nullptr || enemy->currentHp() <= 0) {
            continue;
        }

        m_enemy = enemy;
        m_enemy->beginTurn();

        const EnemyIntentDef intent = m_enemy->consumeNextIntent();
        const int consumedIntentIndex = m_enemy->lastIntentIndex();
        m_enemyIntent = intent.displayText();
        m_enemyIntentValue = intent.displayValue(m_enemy->strength());
        executeEnemyIntent(intent);
        m_enemy->completeGuardianIntent(consumedIntentIndex);

        if (m_enemy->currentHp() > 0) {
            m_enemy->endTurn();
        }

        if (m_player->currentHp() <= 0) {
            endCombat(false);
            return;
        }
    }

    m_enemy = activeEnemy();
    if (m_enemy == nullptr) {
        endCombat(true);
        return;
    }

    updateEnemyIntentPreview();
    beginPlayerTurn();
}

void BattleFSM::executeEnemyIntent(const EnemyIntentDef &intent)
{
    if (m_player == nullptr || m_enemy == nullptr) {
        return;
    }

    const QString intentType = intent.type;
    if (intentType == QStringLiteral("attack") || intentType == QStringLiteral("attack_multiple")) {
        for (int i = 0; i < intent.times; ++i) {
            DamageCommand attackCommand(m_enemy, m_player, intent.baseValue);
            attackCommand.execute();
        }
    } else if (intentType == QStringLiteral("block")) {
        GainBlockCommand defendCommand(m_enemy, intent.blockValue);
        defendCommand.execute();
    } else if (intentType == QStringLiteral("buff")) {
        applyBuff(m_enemy, intent.buffType, intent.buffValue);
    } else if (intentType == QStringLiteral("buff_and_block")) {
        applyBuff(m_enemy, intent.buffType, intent.buffValue);
        GainBlockCommand blockCommand(m_enemy, intent.blockValue);
        blockCommand.execute();
    } else if (intentType == QStringLiteral("debuff")) {
        applyDebuff(m_player, intent.debuffType, intent.debuffValue);
    } else if (intentType == QStringLiteral("attack_and_block")) {
        GainBlockCommand blockCommand(m_enemy, intent.blockValue);
        blockCommand.execute();
        for (int i = 0; i < intent.times; ++i) {
            DamageCommand attackCommand(m_enemy, m_player, intent.baseValue);
            attackCommand.execute();
        }
    } else if (intentType == QStringLiteral("attack_and_debuff")) {
        for (int i = 0; i < intent.times; ++i) {
            DamageCommand attackCommand(m_enemy, m_player, intent.baseValue);
            attackCommand.execute();
        }
        applyDebuff(m_player, intent.debuffType, intent.debuffValue);
    } else if (intentType == QStringLiteral("none")) {
        // Intentionally no-op.
    } else if (intentType == QStringLiteral("add_cards_to_discard")
               || intentType == QStringLiteral("attack_and_add_cards")
               || intentType == QStringLiteral("attack_multiple_and_add_cards")) {
        if (intent.baseValue > 0) {
            for (int i = 0; i < intent.times; ++i) {
                DamageCommand attackCommand(m_enemy, m_player, intent.baseValue);
                attackCommand.execute();
            }
        }
        qWarning() << "BattleFSM: add_cards_to_discard not fully implemented for" << intent.cardId;
    } else if (intentType == QStringLiteral("split")) {
        executeSplitIntent(intent);
    } else {
        qWarning() << "BattleFSM: unsupported intent type" << intentType;
    }
}

void BattleFSM::executeSplitIntent(const EnemyIntentDef &intent)
{
    if (m_enemy == nullptr) {
        return;
    }

    if (intent.splitInto.isEmpty()) {
        qWarning() << "BattleFSM: split intent has empty into list for" << m_enemy->enemyId();
        return;
    }

    const int sharedHp = m_enemy->currentHp();
    const int insertAt = m_enemies.indexOf(m_enemy);
    if (insertAt < 0) {
        qWarning() << "BattleFSM: split parent not found in encounter list";
        return;
    }

    EnemyDatabase::instance().load();
    QVector<Enemy *> children;
    children.reserve(intent.splitInto.size());
    for (const QString &childId : intent.splitInto) {
        Enemy *child = EnemyDatabase::instance().createEnemy(childId, nullptr);
        if (child == nullptr) {
            qWarning() << "BattleFSM: failed to create split child" << childId;
            continue;
        }

        // JSON: children inherit the parent's current HP at the moment of the split.
        const int childHp = qMax(1, sharedHp);
        child->setMaxHp(childHp);
        child->healToFull();
        m_ownedEnemies.emplace_back(child);
        children.append(child);
    }

    // Remove the parent from combat (HP → 0) and insert children in its place.
    if (m_enemy->currentHp() > 0) {
        m_enemy->takeDirectDamage(m_enemy->currentHp());
    }

    m_enemies.removeAt(insertAt);
    for (int i = 0; i < children.size(); ++i) {
        m_enemies.insert(insertAt + i, children.at(i));
    }

    refreshEncounterPeers();
    m_enemy = activeEnemy();
    emit enemiesChanged();
}

void BattleFSM::refreshEncounterPeers()
{
    for (Enemy *enemy : m_enemies) {
        if (enemy != nullptr) {
            enemy->setEncounterPeers(&m_enemies);
        }
    }
}

Enemy *BattleFSM::activeEnemy() const
{
    for (Enemy *enemy : m_enemies) {
        if (enemy != nullptr && enemy->currentHp() > 0) {
            return enemy;
        }
    }
    return nullptr;
}

void BattleFSM::applyBuff(Actor *target, const QString &buffType, int value)
{
    if (target == nullptr || buffType.isEmpty() || value <= 0) {
        return;
    }

    target->addPower(PowerFactory::createPower(buffType, value, -1));
}

void BattleFSM::applyDebuff(Actor *target, const QString &debuffType, int value)
{
    if (target == nullptr || debuffType.isEmpty() || value <= 0) {
        return;
    }

    if (debuffType == QStringLiteral("weak_and_vulnerable")) {
        applyDebuff(target, QStringLiteral("weak"), value);
        applyDebuff(target, QStringLiteral("vulnerable"), value);
        return;
    }

    target->addPower(PowerFactory::createPower(debuffType, value, value));
}

void BattleFSM::updateEnemyIntentPreview()
{
    if (m_enemy == nullptr) {
        m_enemyIntent.clear();
        m_enemyIntentValue = 0;
        return;
    }

    const EnemyIntentDef intent = m_enemy->peekNextIntent();
    m_enemyIntent = intent.displayText();
    m_enemyIntentValue = intent.displayValue(m_enemy->strength());
}

void BattleFSM::refreshEnemyIntentPreview()
{
    updateEnemyIntentPreview();
}

void BattleFSM::concludeCombat(bool playerWon)
{
    endCombat(playerWon);
}

void BattleFSM::drawExtraCards(int count)
{
    for (int i = 0; i < count; ++i) {
        if (!drawCard()) {
            break;
        }
    }
}

void BattleFSM::gainEnergy(int amount)
{
    m_currentEnergy += amount;
    if (m_currentEnergy < 0) {
        m_currentEnergy = 0;
    }
}

void BattleFSM::addCardToHand(const Card &card)
{
    if (card.id().isEmpty()) {
        return;
    }
    m_hand.append(card);
}

void BattleFSM::exhaustNonAttackCardsGainBlock(int blockPerCard)
{
    if (m_player == nullptr || blockPerCard <= 0) {
        return;
    }

    int exhaustedCount = 0;
    QVector<Card> remaining;
    remaining.reserve(m_hand.size());
    for (const Card &card : m_hand) {
        if (card.data().type != CardType::Attack) {
            m_exhaustPile.append(card);
            ++exhaustedCount;
        } else {
            remaining.append(card);
        }
    }
    m_hand = remaining;

    if (exhaustedCount > 0) {
        m_player->gainBlock(exhaustedCount * blockPerCard);
    }
}

void BattleFSM::increaseCardDamageBonus(const QString &cardId, int amount)
{
    if (cardId.isEmpty() || amount == 0) {
        return;
    }
    m_cardDamageBonus[cardId] = m_cardDamageBonus.value(cardId, 0) + amount;
}

void BattleFSM::endCombat(bool playerWon)
{
    if (m_state == BattleState::Ended) {
        return;
    }

    m_state = BattleState::Ended;

    if (m_shrinkConnection) {
        QObject::disconnect(m_shrinkConnection);
        m_shrinkConnection = {};
    }

    // ShrinkDamagePower is combat-scoped; remove immediately on victory or defeat.
    clearShrinkPower();

    EventBus::instance().triggerCombatEnded(playerWon);
    emit combatEnded(playerWon);
}

PlayResult BattleFSM::playCard(Card card, Actor *target)
{
    if (m_state != BattleState::PlayerTurn || m_player == nullptr || m_enemy == nullptr) {
        return PlayResult::failure(QStringLiteral("Not the player turn"));
    }

    int handIndex = -1;
    for (int i = 0; i < m_hand.size(); ++i) {
        if (m_hand.at(i).matches(card)) {
            handIndex = i;
            break;
        }
    }

    if (handIndex < 0) {
        return PlayResult::failure(QStringLiteral("Card is not in hand"));
    }

    const Card &handCard = m_hand.at(handIndex);
    if (!handCard.isPlayable()) {
        return PlayResult::failure(QStringLiteral("Card cannot be played"));
    }

    if (m_currentEnergy < handCard.cost()) {
        return PlayResult::failure(QStringLiteral("Not enough energy"));
    }

    m_currentEnergy -= handCard.cost();

    if (handCard.data().type == CardType::Attack) {
        ++m_attackCardsPlayed;
        EventBus::instance().triggerPlayerPlayedAttack(m_attackCardsPlayed);
        for (Enemy *livingEnemy : m_enemies) {
            if (livingEnemy != nullptr && livingEnemy->currentHp() > 0) {
                livingEnemy->notifyPlayerPlayedAttack(m_player);
            }
        }
    }

    const bool exhaustPlayed = handCard.hasEffectType(QStringLiteral("exhaust"));
    const QString playedId = handCard.id();
    const QVector<EffectDef> effects = prepareCardEffects(handCard);
    const Card played = m_hand.takeAt(handIndex);

    Actor *effectTarget = target != nullptr ? target : m_enemy;
    EffectResolver::instance().execute(effects, m_player, effectTarget, livingEnemyActors(), this,
                                       playedId);

    if (exhaustPlayed) {
        m_exhaustPile.append(played);
    } else {
        m_discardPile.append(played);
    }

    if (m_enemy != nullptr && m_enemy->currentHp() <= 0) {
        m_enemy = activeEnemy();
        if (m_enemy == nullptr) {
            endCombat(true);
            return PlayResult::success();
        }
    }

    updateEnemyIntentPreview();
    return PlayResult::success();
}

QVector<Actor *> BattleFSM::livingEnemyActors() const
{
    QVector<Actor *> actors;
    for (Enemy *enemy : m_enemies) {
        if (enemy != nullptr && enemy->currentHp() > 0) {
            actors.append(enemy);
        }
    }
    return actors;
}

QVector<EffectDef> BattleFSM::prepareCardEffects(const Card &card) const
{
    QVector<EffectDef> effects = toEffectVector(card.effects());
    if (card.data().target != TargetType::AllEnemies) {
        return effects;
    }

    for (EffectDef &effect : effects) {
        if (effect.target.isEmpty() || effect.target == QStringLiteral("selected_enemy")
            || effect.target == QStringLiteral("SingleEnemy")) {
            if (effect.type == QStringLiteral("deal_damage")
                || effect.type == QStringLiteral("deal_damage_per_block")
                || effect.type == QStringLiteral("apply_power")) {
                effect.target = QStringLiteral("all_enemies");
            }
        }
    }
    return effects;
}

void BattleFSM::shuffleDrawPile()
{
    for (int i = m_drawPile.size() - 1; i > 0; --i) {
        const int j = RNG::get(0, i);
        m_drawPile.swapItemsAt(i, j);
    }
}

bool BattleFSM::drawCard()
{
    if (m_drawPile.isEmpty()) {
        if (m_discardPile.isEmpty()) {
            return false;
        }

        m_drawPile = m_discardPile;
        m_discardPile.clear();
        shuffleDrawPile();
    }

    m_hand.append(m_drawPile.takeLast());
    return true;
}

void BattleFSM::drawCardsToHand()
{
    while (m_hand.size() < 5) {
        if (!drawCard()) {
            break;
        }
    }
}

void BattleFSM::processEndOfTurnHandKeywords()
{
    if (m_player == nullptr) {
        return;
    }

    for (const Card &card : m_hand) {
        if (!card.hasEffectType(QStringLiteral("end_of_turn_damage"))) {
            continue;
        }
        const int damage = card.effectValue(QStringLiteral("end_of_turn_damage"));
        if (damage > 0) {
            m_player->takeDirectDamage(damage);
        }
    }

    if (m_player->currentHp() <= 0) {
        endCombat(false);
    }
}

void BattleFSM::discardHand()
{
    while (!m_hand.isEmpty()) {
        const Card card = m_hand.takeFirst();
        if (card.hasEffectType(QStringLiteral("ethereal"))) {
            m_exhaustPile.append(card);
        } else {
            m_discardPile.append(card);
        }
    }
}

void BattleFSM::clearShrinkPower()
{
    if (m_player != nullptr) {
        m_player->removePower(QStringLiteral("shrink_damage"));
    }
    m_shrinkPowerPtr = nullptr;
    m_shrinkActive = false;
    m_shrinkCount = 0;
}

void BattleFSM::applyShrinkPower(int shrinkCount)
{
    clearShrinkPower();

    if (shrinkCount <= 0 || m_player == nullptr) {
        return;
    }

    m_shrinkCount = shrinkCount;
    m_shrinkActive = true;

    auto shrinkPower = std::make_unique<ShrinkDamagePower>(m_player, shrinkCount);
    m_shrinkPowerPtr = shrinkPower.get();
    m_player->addPower(std::move(shrinkPower));
}

QVector<EffectDef> BattleFSM::toEffectVector(const std::vector<EffectDef> &effects)
{
    QVector<EffectDef> result;
    result.reserve(static_cast<int>(effects.size()));
    for (const EffectDef &effect : effects) {
        result.append(effect);
    }
    return result;
}
