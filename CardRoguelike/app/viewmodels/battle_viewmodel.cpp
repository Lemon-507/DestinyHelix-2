#include "battle_viewmodel.h"

#include <QtCore/QDebug>

#include "battle/data/card_database.h"
#include "battle/data/relic_database.h"
#include "battle/entities/actor.h"
#include "battle/entities/card.h"
#include "battle/entities/enemy.h"
#include "battle/entities/player.h"
#include "battle/entities/power.h"
#include "battle/logic/battlefsm.h"
#include "battle/logic/card_text.h"
#include "battle/logic/relic_manager.h"
#include "game_manager.h"
#include "map/map_manager.h"
#include "potions/potion_database.h"
#include "systems/event_bus.h"
#include "utils/buff_catalog.h"

BattleViewModel::BattleViewModel(GameManager *gameManager, QObject *parent)
    : QObject(parent)
    , m_gameManager(gameManager)
{
    connectBattleFsm();
    connectPlayerSignals();
}

BattleFSM *BattleViewModel::battleFsm() const
{
    return m_gameManager != nullptr ? m_gameManager->battleFsm() : nullptr;
}

BattleViewModel::~BattleViewModel() = default;

int BattleViewModel::playerHp() const
{
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return 0;
    }
    return m_gameManager->player()->currentHp();
}

int BattleViewModel::playerMaxHp() const
{
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return 0;
    }
    return m_gameManager->player()->maxHp();
}

int BattleViewModel::playerBlock() const
{
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return 0;
    }
    return m_gameManager->player()->currentBlock();
}

Enemy *BattleViewModel::activeEnemy() const
{
    if (m_combatActive && battleFsm() != nullptr) {
        for (Enemy *enemy : battleFsm()->enemies()) {
            if (enemy != nullptr && enemy->currentHp() > 0) {
                return enemy;
            }
        }
        return nullptr;
    }
    if (m_gameManager == nullptr) {
        return nullptr;
    }
    return m_gameManager->activeEncounterEnemy();
}

Enemy *BattleViewModel::enemyAtIndex(int index) const
{
    if (!m_combatActive || index < 0) {
        return nullptr;
    }

    const QVector<Enemy *> enemies =
        battleFsm() != nullptr ? battleFsm()->enemies()
                               : (m_gameManager != nullptr ? m_gameManager->currentEncounterEnemies()
                                                           : QVector<Enemy *>{});
    if (index >= enemies.size()) {
        return nullptr;
    }

    Enemy *enemy = enemies.at(index);
    if (enemy == nullptr || enemy->currentHp() <= 0) {
        return nullptr;
    }
    return enemy;
}

int BattleViewModel::enemyHp() const
{
    Enemy *enemy = activeEnemy();
    return enemy != nullptr ? enemy->currentHp() : 0;
}

int BattleViewModel::enemyMaxHp() const
{
    Enemy *enemy = activeEnemy();
    return enemy != nullptr ? enemy->maxHp() : 0;
}

int BattleViewModel::enemyBlock() const
{
    Enemy *enemy = activeEnemy();
    return enemy != nullptr ? enemy->currentBlock() : 0;
}

QString BattleViewModel::enemyName() const
{
    Enemy *enemy = activeEnemy();
    if (enemy == nullptr || enemy->data() == nullptr) {
        return QString();
    }
    return enemy->data()->name;
}

int BattleViewModel::currentEnergy() const
{
    return battleFsm() != nullptr ? battleFsm()->currentEnergy() : 0;
}

int BattleViewModel::maxEnergy() const
{
    return battleFsm() != nullptr ? battleFsm()->maxEnergy() : 0;
}

QString BattleViewModel::enemyIntent() const
{
    return battleFsm() != nullptr ? battleFsm()->enemyIntent() : QString();
}

int BattleViewModel::enemyIntentValue() const
{
    return battleFsm() != nullptr ? battleFsm()->enemyIntentValue() : 0;
}

QVariantList BattleViewModel::handCards() const
{
    QVariantList cards;
    if (battleFsm() == nullptr) {
        return cards;
    }
    for (const Card &card : battleFsm()->hand()) {
        QVariantMap entry;
        entry.insert(QStringLiteral("id"), card.id());
        entry.insert(QStringLiteral("name"), cardDisplayName(card.id()));
        entry.insert(QStringLiteral("cost"), card.cost());

        entry.insert(QStringLiteral("playable"), card.isPlayable());

        const int damageBonus = battleFsm()->cardDamageBonus(card.id());
        if (damageBonus != 0) {
            CardData adjusted = card.data();
            for (EffectDef &effect : adjusted.effects) {
                if (effect.type == QStringLiteral("deal_damage")) {
                    effect.value += damageBonus;
                }
            }
            entry.insert(QStringLiteral("description"), CardText::describeCard(Card(adjusted)));
        } else {
            entry.insert(QStringLiteral("description"), cardDescription(card));
        }

        QString targetType = QStringLiteral("enemy");
        if (card.data().target == TargetType::Self) {
            targetType = QStringLiteral("self");
        } else if (card.data().target == TargetType::AllEnemies) {
            targetType = QStringLiteral("allEnemies");
        }
        entry.insert(QStringLiteral("targetType"), targetType);
        cards.append(entry);
    }
    return cards;
}

bool BattleViewModel::combatActive() const
{
    return m_combatActive;
}

bool BattleViewModel::canSelectCards() const
{
    return m_combatActive && battleFsm() != nullptr
           && battleFsm()->battleState() == BattleState::PlayerTurn;
}

bool BattleViewModel::canAffordCard(int handIndex) const
{
    if (!canSelectCards() || battleFsm() == nullptr || handIndex < 0) {
        return false;
    }

    const QVector<Card> hand = battleFsm()->hand();
    if (handIndex >= hand.size()) {
        return false;
    }

    const Card &card = hand.at(handIndex);
    return card.isPlayable() && card.cost() <= currentEnergy();
}

int BattleViewModel::drawPileCount() const
{
    return battleFsm() != nullptr ? battleFsm()->drawPileCount() : 0;
}

int BattleViewModel::discardPileCount() const
{
    return battleFsm() != nullptr ? battleFsm()->discardPileCount() : 0;
}

int BattleViewModel::gold() const
{
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return 0;
    }
    return m_gameManager->player()->gold();
}

QVariantList BattleViewModel::relics() const
{
    QVariantList entries;
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return entries;
    }

    RelicDatabase::instance().load();
    const QStringList relicIds = m_gameManager->player()->relicIds();
    const int limit = qMin(12, relicIds.size());
    for (int i = 0; i < limit; ++i) {
        const RelicData relic = RelicDatabase::instance().getRelic(relicIds.at(i));
        QVariantMap entry;
        entry.insert(QStringLiteral("id"), relicIds.at(i));
        entry.insert(QStringLiteral("name"), relic.name.isEmpty() ? relicIds.at(i) : relic.name);
        entry.insert(QStringLiteral("abbreviation"),
                     textAbbreviation(relic.name, relicIds.at(i)));
        entry.insert(QStringLiteral("description"),
                     relic.description.isEmpty() ? relic.name : relic.description);
        entries.append(entry);
    }
    return entries;
}

QVariantList BattleViewModel::potions() const
{
    QVariantList entries;
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return entries;
    }

    PotionDatabase::instance().load();
    for (auto it = m_gameManager->player()->potionCounts().constBegin();
         it != m_gameManager->player()->potionCounts().constEnd(); ++it) {
        if (it.value() <= 0) {
            continue;
        }

        const PotionData potion = PotionDatabase::instance().getPotion(it.key());
        QVariantMap entry;
        entry.insert(QStringLiteral("id"), it.key());
        entry.insert(QStringLiteral("name"), potion.name.isEmpty() ? it.key() : potion.name);
        const QString letter = potion.name.isEmpty() ? it.key().left(1).toUpper()
                                                     : potion.name.left(1).toUpper();
        entry.insert(QStringLiteral("abbreviation"), letter);
        entry.insert(QStringLiteral("count"), it.value());
        entry.insert(QStringLiteral("canUse"), canUsePotion(it.key()));
        entries.append(entry);

        if (entries.size() >= 3) {
            break;
        }
    }
    return entries;
}

QVariantMap BattleViewModel::buffEntryFromPower(Power *power)
{
    QVariantMap entry;
    if (power == nullptr || power->isExpired()) {
        return entry;
    }

    BuffCatalog::instance().load();
    const QString id = power->powerId();
    const QString name = power->name();
    const int value = power->value();
    const int duration = power->duration();

    entry.insert(QStringLiteral("id"), id);
    entry.insert(QStringLiteral("name"), BuffCatalog::instance().displayName(id, name));
    entry.insert(QStringLiteral("abbreviation"), BuffCatalog::instance().abbreviation(id, name));
    entry.insert(QStringLiteral("description"),
                 BuffCatalog::instance().formatDescription(id, value));
    entry.insert(QStringLiteral("tooltip"),
                 BuffCatalog::instance().tooltipText(id, name, value, duration));
    entry.insert(QStringLiteral("value"), value);
    entry.insert(QStringLiteral("duration"), duration);
    return entry;
}

QVariantList BattleViewModel::buildBuffEntries(const Actor *actor)
{
    QVariantList entries;
    if (actor == nullptr) {
        return entries;
    }

    for (QObject *child : actor->children()) {
        auto *power = qobject_cast<Power *>(child);
        if (power == nullptr || power->isExpired()) {
            continue;
        }

        const QVariantMap entry = buffEntryFromPower(power);
        if (!entry.isEmpty()) {
            entries.append(entry);
        }
    }
    return entries;
}

QVariantList BattleViewModel::playerBuffs() const
{
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return {};
    }

    return buildBuffEntries(m_gameManager->player());
}

bool BattleViewModel::playerBuffOverflow() const
{
    return playerBuffs().size() > 10;
}

QVariantList BattleViewModel::enemies() const
{
    QVariantList entries;
    if (!m_combatActive) {
        return entries;
    }

    const QVector<Enemy *> encounter =
        battleFsm() != nullptr ? battleFsm()->enemies()
                               : (m_gameManager != nullptr ? m_gameManager->currentEncounterEnemies()
                                                           : QVector<Enemy *>{});
    const int limit = qMin(encounter.size(), 5);
    const int buffMaxVisible = limit >= 4 ? 4 : 7;
    for (int i = 0; i < limit; ++i) {
        Enemy *enemy = encounter.at(i);
        if (enemy == nullptr) {
            continue;
        }

        const bool alive = enemy->currentHp() > 0;
        QVariantMap entry;
        entry.insert(QStringLiteral("slotIndex"), i);
        entry.insert(QStringLiteral("alive"), alive);
        entry.insert(QStringLiteral("id"), enemy->enemyId());
        entry.insert(QStringLiteral("name"), enemy->data() != nullptr ? enemy->data()->name : enemy->enemyId());
        entry.insert(QStringLiteral("hp"), enemy->currentHp());
        entry.insert(QStringLiteral("maxHp"), enemy->maxHp());
        entry.insert(QStringLiteral("block"), enemy->currentBlock());

        if (alive) {
            const EnemyIntentDef intent = enemy->peekNextIntent();
            const QString intentName = intent.displayText();
            entry.insert(QStringLiteral("intent"), intentName);
            entry.insert(QStringLiteral("intentValue"), intent.displayValue(enemy->strength()));
            entry.insert(QStringLiteral("intentAbbreviation"),
                         textAbbreviation(intentName, intent.type, 2));
            entry.insert(QStringLiteral("intentDescription"),
                         intentDescription(intent, enemy->strength()));
            entry.insert(QStringLiteral("defensiveMode"), enemy->isGuardianDefensiveMode());
            const QVariantList buffs = buildBuffEntries(enemy);
            entry.insert(QStringLiteral("buffs"), buffs);
            entry.insert(QStringLiteral("buffOverflow"), buffs.size() > buffMaxVisible);
            entry.insert(QStringLiteral("buffMaxVisible"), buffMaxVisible);
        } else {
            entry.insert(QStringLiteral("buffs"), QVariantList());
            entry.insert(QStringLiteral("buffOverflow"), false);
            entry.insert(QStringLiteral("buffMaxVisible"), buffMaxVisible);
        }

        entries.append(entry);
    }
    return entries;
}

GameContext BattleViewModel::currentPotionContext() const
{
    if (!m_combatActive) {
        return GameContext::NonCombat;
    }
    if (battleFsm() != nullptr && battleFsm()->battleState() == BattleState::PlayerTurn) {
        return GameContext::CombatPlayerTurn;
    }
    return GameContext::CombatAny;
}

bool BattleViewModel::canUsePotion(const QString &potionId) const
{
    if (m_gameManager == nullptr || m_gameManager->player() == nullptr) {
        return false;
    }
    if (m_gameManager->player()->potionCount(potionId) <= 0) {
        return false;
    }
    if (m_gameManager->player()->currentHp() <= 0) {
        return false;
    }

    PotionDatabase::instance().load();
    const PotionData potion = PotionDatabase::instance().getPotion(potionId);
    if (potion.id.isEmpty()) {
        return false;
    }

    const GameContext context = currentPotionContext();
    return PotionDatabase::instance().canUse(potion, context);
}

QString BattleViewModel::potionPrimaryTarget(const QString &potionId) const
{
    PotionDatabase::instance().load();
    const PotionData potion = PotionDatabase::instance().getPotion(potionId);
    if (potion.effects.isEmpty()) {
        return QStringLiteral("self");
    }
    const QString target = potion.effects.first().target;
    return target.isEmpty() ? QStringLiteral("self") : target;
}

void BattleViewModel::usePotion(const QString &potionId, int enemyIndex)
{
    if (m_gameManager == nullptr) {
        return;
    }

    const PlayResult result = m_gameManager->usePotion(potionId, enemyIndex);
    if (!result.ok) {
        qWarning() << "BattleViewModel: usePotion failed:" << result.reason;
    }
    refreshState();
}

void BattleViewModel::startCombat()
{
    if (m_gameManager == nullptr) {
        return;
    }

    m_gameManager->startCombat();
    if (battleFsm() == nullptr || battleFsm()->battleState() == BattleState::Idle) {
        return;
    }

    m_combatActive = true;
    connectPlayerSignals();
    connectEnemySignals();

    emit combatActiveChanged();
    refreshState();
}

void BattleViewModel::playCard(const QString &cardId, int enemyIndex, int handIndex)
{
    if (!m_combatActive || m_gameManager == nullptr || battleFsm() == nullptr) {
        return;
    }

    int indexToPlay = handIndex;
    if (indexToPlay < 0) {
        const QVector<Card> hand = battleFsm()->hand();
        for (int i = 0; i < hand.size(); ++i) {
            if (hand.at(i).id() == cardId) {
                indexToPlay = i;
                break;
            }
        }
    }

    const PlayResult result = m_gameManager->playCard(indexToPlay, enemyIndex);
    if (!result.ok) {
        qWarning() << "BattleViewModel: playCard failed:" << result.reason;
    }
    refreshState();
}

void BattleViewModel::endPlayerTurn()
{
    if (!m_combatActive || m_gameManager == nullptr) {
        return;
    }

    m_gameManager->endPlayerTurn();
    refreshState();
}

void BattleViewModel::refreshState()
{
    emit playerHpChanged();
    emit playerMaxHpChanged();
    emit playerBlockChanged();
    emit enemyHpChanged();
    emit enemyMaxHpChanged();
    emit enemyBlockChanged();
    emit enemyNameChanged();
    emit currentEnergyChanged();
    emit maxEnergyChanged();
    emit enemyIntentChanged();
    emit enemyIntentValueChanged();
    emit handCardsChanged();
    emit goldChanged();
    emit relicsChanged();
    emit potionsChanged();
    emit playerBuffsChanged();
    emit enemiesChanged();
    emit drawPileCountChanged();
    emit discardPileCountChanged();
    checkCombatResolution();
}

void BattleViewModel::connectEnemySignals()
{
    if (battleFsm() == nullptr) {
        return;
    }

    connect(battleFsm(), &BattleFSM::enemiesChanged, this, [this]() {
        connectEnemyHpSignals();
        refreshState();
    }, Qt::UniqueConnection);

    connectEnemyHpSignals();
}

void BattleViewModel::connectEnemyHpSignals()
{
    const QVector<Enemy *> enemies =
        battleFsm() != nullptr ? battleFsm()->enemies()
                               : (m_gameManager != nullptr ? m_gameManager->currentEncounterEnemies()
                                                           : QVector<Enemy *>{});
    for (Enemy *enemy : enemies) {
        if (enemy != nullptr) {
            connect(enemy, &Actor::hpChanged, this, &BattleViewModel::checkCombatResolution,
                    Qt::UniqueConnection);
            connect(enemy, &Enemy::guardianModeChanged, this,
                    [this](bool) {
                        if (battleFsm() != nullptr) {
                            battleFsm()->refreshEnemyIntentPreview();
                        }
                        refreshState();
                    },
                    Qt::UniqueConnection);
        }
    }
}

void BattleViewModel::checkCombatResolution()
{
    if (!m_combatActive) {
        return;
    }

    const QVector<Enemy *> enemies =
        battleFsm() != nullptr ? battleFsm()->enemies()
                               : (m_gameManager != nullptr ? m_gameManager->currentEncounterEnemies()
                                                           : QVector<Enemy *>{});
    if (enemies.isEmpty()) {
        return;
    }

    for (Enemy *enemy : enemies) {
        if (enemy != nullptr && enemy->currentHp() > 0) {
            return;
        }
    }

    finishCombat(true);
}

void BattleViewModel::finishCombat(bool playerWon)
{
    if (!m_combatActive) {
        return;
    }

    m_combatActive = false;

    if (m_gameManager != nullptr) {
        m_gameManager->finishCombat(playerWon);
    }
    emit combatActiveChanged();
    emit combatEnded(playerWon);
}

void BattleViewModel::connectPlayerSignals()
{
    if (m_gameManager == nullptr) {
        return;
    }

    Player *player = m_gameManager->player();
    if (player == nullptr) {
        return;
    }

    connect(player, &Actor::hpChanged, this, &BattleViewModel::refreshState, Qt::UniqueConnection);
}

void BattleViewModel::connectBattleFsm()
{
    if (battleFsm() == nullptr) {
        return;
    }

    connect(battleFsm(), &BattleFSM::combatEnded, this, [this](bool playerWon) {
        finishCombat(playerWon);
    });
}

QString BattleViewModel::cardDescription(const Card &card)
{
    return CardText::describeCard(card);
}

QString BattleViewModel::cardDisplayName(const QString &id)
{
    CardDatabase::instance().load();
    return CardDatabase::instance().displayName(id);
}

QString BattleViewModel::textAbbreviation(const QString &name, const QString &fallbackId, int maxLen)
{
    if (name.isEmpty()) {
        return fallbackId.left(maxLen).toUpper();
    }

    const QStringList words = name.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (words.size() >= 2) {
        QString abbr;
        for (const QString &word : words) {
            if (!word.isEmpty()) {
                abbr.append(word.at(0).toUpper());
            }
            if (abbr.size() >= maxLen) {
                break;
            }
        }
        return abbr.left(maxLen);
    }

    return name.left(maxLen).toUpper();
}

QString BattleViewModel::intentDescription(const EnemyIntentDef &intent, int ownerStrength)
{
    const QString name = intent.displayText();
    const int value = intent.displayValue(ownerStrength);
    const QString &type = intent.type;

    if (type == QStringLiteral("attack")) {
        return QStringLiteral("%1: Deal %2 damage.").arg(name).arg(value);
    }
    if (type == QStringLiteral("attack_multiple") || type == QStringLiteral("attack_multiple_scaling")) {
        const int perHit = intent.baseValue + ownerStrength;
        return QStringLiteral("%1: Deal %2 damage %3 times.")
            .arg(name)
            .arg(perHit)
            .arg(intent.times);
    }
    if (type == QStringLiteral("block")) {
        return QStringLiteral("%1: Gain %2 Block.").arg(name).arg(value);
    }
    if (type == QStringLiteral("buff")) {
        if (!intent.buffType.isEmpty()) {
            return QStringLiteral("%1: Gain %2 %3.")
                .arg(name)
                .arg(intent.buffValue)
                .arg(intent.buffType);
        }
        return QStringLiteral("%1: Gain %2.").arg(name).arg(value);
    }
    if (type == QStringLiteral("buff_and_block")) {
        return QStringLiteral("%1: Gain %2 %3 and %4 Block.")
            .arg(name)
            .arg(intent.buffValue)
            .arg(intent.buffType)
            .arg(intent.blockValue);
    }
    if (type == QStringLiteral("debuff")) {
        return QStringLiteral("%1: Apply %2 %3.")
            .arg(name)
            .arg(intent.debuffValue)
            .arg(intent.debuffType);
    }
    if (type == QStringLiteral("attack_and_block")) {
        return QStringLiteral("%1: Gain %2 Block and deal %3 damage.")
            .arg(name)
            .arg(intent.blockValue)
            .arg(value);
    }
    if (type == QStringLiteral("attack_and_debuff")) {
        return QStringLiteral("%1: Deal %2 damage and apply %3 %4.")
            .arg(name)
            .arg(value)
            .arg(intent.debuffValue)
            .arg(intent.debuffType);
    }
    if (type == QStringLiteral("none")) {
        return name;
    }
    if (type == QStringLiteral("split")) {
        return QStringLiteral("%1: Split into smaller foes.").arg(name);
    }
    if (value > 0) {
        return QStringLiteral("%1: %2.").arg(name).arg(value);
    }
    return name;
}
