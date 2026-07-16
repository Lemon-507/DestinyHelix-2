#include <QtTest>
#include <QSignalSpy>

#include "battle/data/card_database.h"
#include "battle/data/enemy_database.h"
#include "battle/entities/card.h"
#include "battle/entities/enemy.h"
#include "battle/entities/player.h"
#include "battle/entities/shrink_damage_power.h"
#include "battle/logic/battlefsm.h"
#include "battle/logic/effectresolver.h"
#include "battle/logic/power_factory.h"
#include "character_factory.h"
#include "systems/event_bus.h"

namespace {

void prepareTestEnemy(Enemy &enemy, int hp = 40)
{
    EnemyDatabase::instance().load();
    const EnemyData *data = EnemyDatabase::instance().getEnemy(QStringLiteral("cultist"));
    if (data != nullptr) {
        enemy.loadFromData(*data);
    }
    enemy.setMaxHp(hp);
    enemy.healToFull();
}

QVector<Enemy *> enemyList(Enemy *enemy)
{
    return QVector<Enemy *>{enemy};
}

EffectDef makeEffect(const QString &type, int value = 0, const QString &cardId = QString())
{
    EffectDef effect;
    effect.type = type;
    effect.value = value;
    effect.cardId = cardId;
    return effect;
}

QVector<EffectDef> cardEffects(const Card &card)
{
    QVector<EffectDef> effects;
    effects.reserve(static_cast<int>(card.effects().size()));
    for (const EffectDef &effect : card.effects()) {
        effects.append(effect);
    }
    return effects;
}

} // namespace

class TestCombat : public QObject
{
    Q_OBJECT

private:
    Card findHandCardById(const BattleFSM &fsm, const QString &id) const
    {
        const QVector<Card> hand = fsm.hand();
        for (const Card &card : hand) {
            if (card.id() == id) {
                return card;
            }
        }
        return Card();
    }

    void populateDeck(Player &player) const
    {
        const Card strike = CardBuilder(QStringLiteral("strike"), CardType::Attack)
                                .setCost(1)
                                .addEffect(EffectDef::damage(6))
                                .build();
        const Card defend = CardBuilder(QStringLiteral("defend"), CardType::Skill)
                                .setCost(1)
                                .addEffect(EffectDef::gainBlock(5))
                                .build();
        const Card bash = CardBuilder(QStringLiteral("bash"), CardType::Attack)
                              .setCost(2)
                              .addEffect(EffectDef::damage(8))
                              .build();

        for (int i = 0; i < 5; ++i) {
            player.addCardToDeck(strike);
        }
        for (int i = 0; i < 4; ++i) {
            player.addCardToDeck(defend);
        }
        player.addCardToDeck(bash);
    }

private slots:
    void initTestCase()
    {
        CharacterFactory::initialize();
    }

    void test_player_turn_draw_and_energy()
    {
        Player player;
        populateDeck(player);
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy));

        QCOMPARE(fsm.hand().size(), 5);
        QCOMPARE(fsm.currentEnergy(), 3);
        QCOMPARE(fsm.maxEnergy(), 3);
        QCOMPARE(fsm.battleState(), BattleState::PlayerTurn);
    }

    void test_play_strike_damage()
    {
        Player player;
        populateDeck(player);
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy));

        const Card strike = findHandCardById(fsm, QStringLiteral("strike"));
        QVERIFY(!strike.id().isEmpty());

        const int handBefore = fsm.hand().size();
        fsm.playCard(strike, &enemy);

        QCOMPARE(enemy.currentHp(), 34);
        QCOMPARE(fsm.currentEnergy(), 2);
        QCOMPARE(fsm.hand().size(), handBefore - 1);
    }

    void test_block_reduces_damage()
    {
        Player player;
        populateDeck(player);
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);
        enemy.gainBlock(5);

        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy));

        const Card strike = findHandCardById(fsm, QStringLiteral("strike"));
        QVERIFY(!strike.id().isEmpty());

        fsm.playCard(strike, &enemy);

        QCOMPARE(enemy.currentBlock(), 0);
        QCOMPARE(enemy.currentHp(), 39);
    }

    void test_turn_end_clears_block()
    {
        Player player;
        populateDeck(player);
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy));

        player.gainBlock(10);
        QCOMPARE(player.currentBlock(), 10);

        fsm.playerEndTurn();
        QCOMPARE(player.currentBlock(), 0);
    }

    void test_shrink_buff_damage_ignores_block()
    {
        Player player;
        populateDeck(player);
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy), 2);

        player.gainBlock(20);
        QCOMPARE(player.currentBlock(), 20);
        QCOMPARE(player.currentHp(), 78);
    }

    void test_shrink_buff_dynamic_update()
    {
        Player player;
        populateDeck(player);
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);
        const EnemyData *cultistData = EnemyDatabase::instance().getEnemy(QStringLiteral("cultist"));
        if (cultistData != nullptr) {
            enemy.loadFromData(*cultistData);
        }

        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy), 1);
        QVERIFY(fsm.shrinkPower() != nullptr);
        QCOMPARE(fsm.shrinkPower()->damagePerTurn(), 1);
        QCOMPARE(player.currentHp(), 79);

        EventBus::instance().triggerShrinkTriggered(2);
        QCOMPARE(fsm.shrinkPower()->damagePerTurn(), 2);

        fsm.playerEndTurn();
        QCOMPARE(player.currentHp(), 77);
    }

    void test_shrink_buff_not_applied_outside_zone()
    {
        Player player;
        populateDeck(player);
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy), 0);

        QVERIFY(fsm.shrinkPower() == nullptr);
        QVERIFY(!player.hasPower(QStringLiteral("shrink_damage")));
        QCOMPARE(player.currentHp(), 80);
    }

    void test_shrink_buff_applied_inside_zone()
    {
        Player player;
        populateDeck(player);
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy), 3);

        QVERIFY(fsm.shrinkPower() != nullptr);
        QVERIFY(player.hasPower(QStringLiteral("shrink_damage")));
        QCOMPARE(fsm.shrinkPower()->damagePerTurn(), 3);
        QCOMPARE(player.currentHp(), 77);
    }

    void test_shrink_buff_removed_after_combat()
    {
        Player player;
        populateDeck(player);
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy, 6);

        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy), 2);
        QVERIFY(player.hasPower(QStringLiteral("shrink_damage")));
        QCOMPARE(player.currentHp(), 78);

        const Card strike = findHandCardById(fsm, QStringLiteral("strike"));
        QVERIFY(!strike.id().isEmpty());
        fsm.playCard(strike, &enemy);

        QCOMPARE(fsm.battleState(), BattleState::Ended);
        QVERIFY(!player.hasPower(QStringLiteral("shrink_damage")));
        QCOMPARE(fsm.shrinkPower(), nullptr);

        Enemy nextEnemy;
        prepareTestEnemy(nextEnemy);
        BattleFSM nextFsm;
        nextFsm.startCombat(&player, enemyList(&nextEnemy), 0);

        QVERIFY(nextFsm.shrinkPower() == nullptr);
        QVERIFY(!player.hasPower(QStringLiteral("shrink_damage")));
        QCOMPARE(player.currentHp(), 78);
    }

    void test_combat_victory()
    {
        Player player;
        populateDeck(player);
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy, 6);

        BattleFSM fsm;
        QSignalSpy endedSpy(&fsm, &BattleFSM::combatEnded);
        fsm.startCombat(&player, enemyList(&enemy));

        const Card strike = findHandCardById(fsm, QStringLiteral("strike"));
        QVERIFY(!strike.id().isEmpty());

        fsm.playCard(strike, &enemy);

        QCOMPARE(enemy.currentHp(), 0);
        QCOMPARE(endedSpy.count(), 1);
        QCOMPARE(endedSpy.at(0).at(0).toBool(), true);
        QCOMPARE(fsm.battleState(), BattleState::Ended);
    }

    void test_player_death_from_shrink()
    {
        Player player;
        populateDeck(player);
        Enemy enemy;
        player.setMaxHp(1);
        player.healToFull();
        prepareTestEnemy(enemy);

        BattleFSM fsm;
        QSignalSpy endedSpy(&fsm, &BattleFSM::combatEnded);
        fsm.startCombat(&player, enemyList(&enemy), 5);

        QCOMPARE(player.currentHp(), 0);
        QCOMPARE(endedSpy.count(), 1);
        QCOMPARE(endedSpy.at(0).at(0).toBool(), false);
        QCOMPARE(fsm.battleState(), BattleState::Ended);
        QVERIFY(!player.hasPower(QStringLiteral("shrink_damage")));
        QCOMPARE(fsm.shrinkPower(), nullptr);
    }

    void test_strength_increases_damage()
    {
        Player player;
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        player.addPower(PowerFactory::createPower(QStringLiteral("strength"), 2, -1));

        DamageCommand attack(&player, &enemy, 6);
        attack.execute();

        QCOMPARE(enemy.currentHp(), 32);
    }

    void test_weak_reduces_outgoing_damage()
    {
        Player player;
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        enemy.addPower(PowerFactory::createPower(QStringLiteral("weak"), 1, 1));

        DamageCommand attack(&enemy, &player, 8);
        attack.execute();

        QCOMPARE(player.currentHp(), 74);
    }

    void test_vulnerable_increases_damage_taken()
    {
        Player player;
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        enemy.addPower(PowerFactory::createPower(QStringLiteral("vulnerable"), 2, 2));

        DamageCommand attack(&player, &enemy, 6);
        attack.execute();

        QCOMPARE(enemy.currentHp(), 31);
    }

    void test_dexterity_and_frail_modify_block()
    {
        Player dexterityPlayer;
        dexterityPlayer.addPower(PowerFactory::createPower(QStringLiteral("dexterity"), 2, -1));
        dexterityPlayer.gainBlock(5);
        QCOMPARE(dexterityPlayer.currentBlock(), 7);

        Player frailPlayer;
        frailPlayer.addPower(PowerFactory::createPower(QStringLiteral("frail"), 1, 1));
        frailPlayer.gainBlock(8);
        QCOMPARE(frailPlayer.currentBlock(), 6);
    }

    void test_cultist_ritual_grants_strength()
    {
        Player player;
        populateDeck(player);
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy));

        QCOMPARE(enemy.strength(), 0);
        fsm.playerEndTurn();
        QCOMPARE(enemy.strength(), 3);
    }

    void test_lagavulin_metallicize_grants_block()
    {
        Player player;
        populateDeck(player);
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();

        EnemyDatabase::instance().load();
        const EnemyData *data = EnemyDatabase::instance().getEnemy(QStringLiteral("lagavulin"));
        if (data != nullptr) {
            enemy.loadFromData(*data);
        }
        enemy.setMaxHp(40);
        enemy.healToFull();

        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy));

        fsm.playerEndTurn();
        QVERIFY(enemy.currentBlock() >= 8);
    }

    void test_bash_applies_vulnerable_and_decays()
    {
        Player player;
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        QVector<EffectDef> effects;
        effects.append(EffectDef::applyPower(QStringLiteral("vulnerable"), 2, 2));
        EffectResolver::instance().execute(effects, &player, &enemy);

        QVERIFY(enemy.hasPower(QStringLiteral("vulnerable")));
        QCOMPARE(enemy.getPowerDuration(QStringLiteral("vulnerable")), 2);

        enemy.endTurn();
        QCOMPARE(enemy.getPowerDuration(QStringLiteral("vulnerable")), 1);

        enemy.endTurn();
        QVERIFY(!enemy.hasPower(QStringLiteral("vulnerable")));
    }

    void test_card_json_apply_power_durations_and_intensities()
    {
        CardDatabase::instance().load();
        PowerFactory::instance().load();

        Player player;
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy, 80);

        const Card bash = CardDatabase::instance().getCard(QStringLiteral("bash"));
        QVERIFY(!bash.id().isEmpty());
        EffectResolver::instance().execute(cardEffects(bash), &player, &enemy);
        QCOMPARE(enemy.getPowerDuration(QStringLiteral("vulnerable")), 2);

        Enemy bashPlusEnemy;
        prepareTestEnemy(bashPlusEnemy, 80);
        const Card bashPlus = CardDatabase::instance().getCard(QStringLiteral("bash+"));
        QVERIFY(!bashPlus.id().isEmpty());
        EffectResolver::instance().execute(cardEffects(bashPlus), &player, &bashPlusEnemy);
        QCOMPARE(bashPlusEnemy.getPowerDuration(QStringLiteral("vulnerable")), 3);

        Enemy uppercutEnemy;
        prepareTestEnemy(uppercutEnemy, 80);
        const Card uppercut = CardDatabase::instance().getCard(QStringLiteral("uppercut"));
        QVERIFY(!uppercut.id().isEmpty());
        EffectResolver::instance().execute(cardEffects(uppercut), &player, &uppercutEnemy);
        QCOMPARE(uppercutEnemy.getPowerDuration(QStringLiteral("weak")), 1);
        QCOMPARE(uppercutEnemy.getPowerDuration(QStringLiteral("vulnerable")), 1);

        Enemy uppercutPlusEnemy;
        prepareTestEnemy(uppercutPlusEnemy, 80);
        const Card uppercutPlus = CardDatabase::instance().getCard(QStringLiteral("uppercut+"));
        QVERIFY(!uppercutPlus.id().isEmpty());
        EffectResolver::instance().execute(cardEffects(uppercutPlus), &player, &uppercutPlusEnemy);
        QCOMPARE(uppercutPlusEnemy.getPowerDuration(QStringLiteral("weak")), 2);
        QCOMPARE(uppercutPlusEnemy.getPowerDuration(QStringLiteral("vulnerable")), 2);

        Enemy clotheslineEnemy;
        prepareTestEnemy(clotheslineEnemy, 80);
        const Card clothesline = CardDatabase::instance().getCard(QStringLiteral("clothesline"));
        QVERIFY(!clothesline.id().isEmpty());
        EffectResolver::instance().execute(cardEffects(clothesline), &player, &clotheslineEnemy);
        QCOMPARE(clotheslineEnemy.getPowerDuration(QStringLiteral("weak")), 2);

        Enemy shockwaveEnemy;
        prepareTestEnemy(shockwaveEnemy, 80);
        const Card shockwave = CardDatabase::instance().getCard(QStringLiteral("shockwave"));
        QVERIFY(!shockwave.id().isEmpty());
        QVector<EffectDef> shockwaveEffects = cardEffects(shockwave);
        for (EffectDef &effect : shockwaveEffects) {
            if (effect.type == QStringLiteral("apply_power")) {
                effect.target = QStringLiteral("all_enemies");
            }
        }
        EffectResolver::instance().execute(shockwaveEffects, &player, &shockwaveEnemy,
                                           QVector<Actor *>{&shockwaveEnemy});
        QCOMPARE(shockwaveEnemy.getPowerDuration(QStringLiteral("weak")), 3);
        QCOMPARE(shockwaveEnemy.getPowerDuration(QStringLiteral("vulnerable")), 3);

        Enemy shockwavePlusEnemy;
        prepareTestEnemy(shockwavePlusEnemy, 80);
        const Card shockwavePlus = CardDatabase::instance().getCard(QStringLiteral("shockwave+"));
        QVERIFY(!shockwavePlus.id().isEmpty());
        QVector<EffectDef> shockwavePlusEffects = cardEffects(shockwavePlus);
        for (EffectDef &effect : shockwavePlusEffects) {
            if (effect.type == QStringLiteral("apply_power")) {
                effect.target = QStringLiteral("all_enemies");
            }
        }
        EffectResolver::instance().execute(shockwavePlusEffects, &player, &shockwavePlusEnemy,
                                           QVector<Actor *>{&shockwavePlusEnemy});
        QCOMPARE(shockwavePlusEnemy.getPowerDuration(QStringLiteral("weak")), 5);
        QCOMPARE(shockwavePlusEnemy.getPowerDuration(QStringLiteral("vulnerable")), 5);

        // Intensity powers: value is stack amount, duration is permanent (-1).
        const Card inflame = CardDatabase::instance().getCard(QStringLiteral("inflame"));
        QVERIFY(!inflame.id().isEmpty());
        QVector<EffectDef> inflameEffects = cardEffects(inflame);
        for (EffectDef &effect : inflameEffects) {
            effect.target = QStringLiteral("self");
        }
        EffectResolver::instance().execute(inflameEffects, &player, &enemy);
        QCOMPARE(player.getPowerValue(QStringLiteral("strength")), 2);
        QCOMPARE(player.getPowerDuration(QStringLiteral("strength")), -1);

        const Card inflamePlus = CardDatabase::instance().getCard(QStringLiteral("inflame+"));
        QVERIFY(!inflamePlus.id().isEmpty());
        QVector<EffectDef> inflamePlusEffects = cardEffects(inflamePlus);
        for (EffectDef &effect : inflamePlusEffects) {
            effect.target = QStringLiteral("self");
        }
        EffectResolver::instance().execute(inflamePlusEffects, &player, &enemy);
        QCOMPARE(player.getPowerValue(QStringLiteral("strength")), 5); // 2 + 3 stacked
        QCOMPARE(player.getPowerDuration(QStringLiteral("strength")), -1);

        const Card metallicize = CardDatabase::instance().getCard(QStringLiteral("metallicize"));
        QVERIFY(!metallicize.id().isEmpty());
        QVector<EffectDef> metallicizeEffects = cardEffects(metallicize);
        for (EffectDef &effect : metallicizeEffects) {
            effect.target = QStringLiteral("self");
        }
        EffectResolver::instance().execute(metallicizeEffects, &player, &enemy);
        QCOMPARE(player.getPowerValue(QStringLiteral("metallicize")), 3);
        QCOMPARE(player.getPowerDuration(QStringLiteral("metallicize")), -1);
    }

    void test_reward_card_pool_excludes_basic_special_status()
    {
        CardDatabase::instance().load();
        const QStringList pool = CardDatabase::instance().rewardCardIds();
        QVERIFY(!pool.isEmpty());

        const QStringList banned = {
            QStringLiteral("strike"),
            QStringLiteral("defend"),
            QStringLiteral("bash"),
            QStringLiteral("slimed"),
            QStringLiteral("burn"),
            QStringLiteral("dazed"),
            QStringLiteral("wound"),
        };
        for (const QString &id : banned) {
            QVERIFY2(!pool.contains(id), qPrintable(id));
        }

        for (const QString &id : pool) {
            QVERIFY(!id.endsWith(QLatin1Char('+')));
            const Card card = CardDatabase::instance().getCard(id);
            QVERIFY(!card.id().isEmpty());
            const CardRarity rarity = card.data().rarity;
            QVERIFY(rarity == CardRarity::Common || rarity == CardRarity::Uncommon
                    || rarity == CardRarity::Rare);
        }

        QVERIFY(pool.contains(QStringLiteral("iron_wave"))
                || pool.contains(QStringLiteral("clothesline"))
                || pool.contains(QStringLiteral("inflame")));
    }

    void test_draw_card_and_gain_energy()
    {
        Player player;
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        const Card pommel = CardBuilder(QStringLiteral("pommel_strike"), CardType::Attack)
                                .setCost(1)
                                .addEffect(EffectDef::damage(9))
                                .addEffect(makeEffect(QStringLiteral("draw_card"), 1))
                                .build();
        const Card filler = CardBuilder(QStringLiteral("strike"), CardType::Attack)
                                .setCost(1)
                                .addEffect(EffectDef::damage(6))
                                .build();
        for (int i = 0; i < 8; ++i) {
            player.addCardToDeck(filler);
        }
        player.addCardToDeck(pommel);

        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy));

        Card handPommel = findHandCardById(fsm, QStringLiteral("pommel_strike"));
        if (handPommel.id().isEmpty()) {
            // Force it into hand for a deterministic case.
            fsm.addCardToHand(pommel);
            handPommel = pommel;
        }

        const int handBefore = fsm.hand().size();
        const int energyBefore = fsm.currentEnergy();
        fsm.playCard(handPommel, &enemy);

        QVERIFY(fsm.hand().size() >= handBefore); // spent one, drew one
        QCOMPARE(fsm.currentEnergy(), energyBefore - 1);

        EffectDef energyEffect;
        energyEffect.type = QStringLiteral("gain_energy");
        energyEffect.value = 2;
        EffectResolver::instance().execute(QVector<EffectDef>{energyEffect}, &player, &enemy, {},
                                           &fsm);
        QCOMPARE(fsm.currentEnergy(), energyBefore - 1 + 2);
    }

    void test_deal_damage_per_block()
    {
        Player player;
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy, 50);
        player.gainBlock(12);

        EffectDef effect;
        effect.type = QStringLiteral("deal_damage_per_block");
        EffectResolver::instance().execute(QVector<EffectDef>{effect}, &player, &enemy);

        QCOMPARE(enemy.currentHp(), 50 - 12);
    }

    void test_lose_hp_and_apply_power_from_json_fields()
    {
        Player player;
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        EffectDef loseHp;
        loseHp.type = QStringLiteral("lose_hp");
        loseHp.value = 3;
        EffectDef apply;
        apply.type = QStringLiteral("apply_power");
        apply.powerId = QStringLiteral("vulnerable");
        apply.value = 2;
        EffectResolver::instance().execute(QVector<EffectDef>{loseHp, apply}, &player, &enemy);

        QCOMPARE(player.currentHp(), 77);
        QVERIFY(enemy.hasPower(QStringLiteral("vulnerable")));
        QCOMPARE(enemy.getPowerDuration(QStringLiteral("vulnerable")), 2);
    }

    void test_exhaust_and_add_card_to_hand()
    {
        Player player;
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        CardDatabase::instance().load();
        EffectDef addWound;
        addWound.type = QStringLiteral("add_card_to_hand");
        addWound.cardId = QStringLiteral("wound");
        addWound.value = 2;

        BattleFSM fsm;
        const Card strike = CardBuilder(QStringLiteral("strike"), CardType::Attack)
                                .setCost(1)
                                .addEffect(EffectDef::damage(6))
                                .build();
        player.addCardToDeck(strike);
        player.addCardToDeck(strike);
        player.addCardToDeck(strike);
        player.addCardToDeck(strike);
        player.addCardToDeck(strike);
        fsm.startCombat(&player, enemyList(&enemy));

        const int handBefore = fsm.hand().size();
        EffectResolver::instance().execute(QVector<EffectDef>{addWound}, &player, &enemy, {}, &fsm);
        QCOMPARE(fsm.hand().size(), handBefore + 2);

        const Card seeingRed = CardBuilder(QStringLiteral("seeing_red"), CardType::Skill)
                                   .setCost(0)
                                   .addEffect(makeEffect(QStringLiteral("gain_energy"), 2))
                                   .addEffect(makeEffect(QStringLiteral("exhaust")))
                                   .build();
        fsm.addCardToHand(seeingRed);
        const int exhaustBefore = fsm.exhaustPileCount();
        const int energyBeforePlay = fsm.currentEnergy();
        fsm.playCard(seeingRed, &enemy);
        QCOMPARE(fsm.exhaustPileCount(), exhaustBefore + 1);
        QCOMPARE(fsm.currentEnergy(), energyBeforePlay + 2);    }

    void test_exhaust_non_attack_gain_block_and_rampage_scaling()
    {
        Player player;
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy, 80);

        const Card defend = CardBuilder(QStringLiteral("defend"), CardType::Skill)
                                .setCost(1)
                                .addEffect(EffectDef::gainBlock(5))
                                .build();
        const Card strike = CardBuilder(QStringLiteral("strike"), CardType::Attack)
                                .setCost(1)
                                .addEffect(EffectDef::damage(6))
                                .build();
        const Card secondWind =
            CardBuilder(QStringLiteral("second_wind"), CardType::Skill)
                .setCost(1)
                .addEffect(makeEffect(QStringLiteral("exhaust_non_attack_gain_block"), 5))
                .build();

        for (int i = 0; i < 5; ++i) {
            player.addCardToDeck(strike);
        }
        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy));
        fsm.addCardToHand(defend);
        fsm.addCardToHand(secondWind);

        const int handBefore = fsm.hand().size();
        fsm.playCard(secondWind, &enemy);
        QVERIFY(fsm.hand().size() < handBefore);
        QVERIFY(player.currentBlock() >= 5);
        QVERIFY(fsm.exhaustPileCount() >= 1);

        const Card rampage =
            CardBuilder(QStringLiteral("rampage"), CardType::Attack)
                .setCost(1)
                .addEffect(EffectDef::damage(8))
                .addEffect(makeEffect(QStringLiteral("increase_damage_per_play"), 5))
                .build();
        fsm.addCardToHand(rampage);
        const int hpBeforeFirst = enemy.currentHp();
        fsm.playCard(rampage, &enemy);
        const int firstHit = hpBeforeFirst - enemy.currentHp();
        QCOMPARE(firstHit, 8);

        fsm.addCardToHand(rampage);
        const int hpBeforeSecond = enemy.currentHp();
        fsm.playCard(rampage, &enemy);
        QCOMPARE(hpBeforeSecond - enemy.currentHp(), 13);
    }

    void test_unplayable_and_ethereal()
    {
        Player player;
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        const Card wound = CardBuilder(QStringLiteral("wound"), CardType::Skill)
                               .setCost(-1)
                               .addEffect(makeEffect(QStringLiteral("unplayable")))
                               .build();
        const Card dazed = CardBuilder(QStringLiteral("dazed"), CardType::Skill)
                               .setCost(-1)
                               .addEffect(makeEffect(QStringLiteral("unplayable")))
                               .addEffect(makeEffect(QStringLiteral("ethereal")))
                               .build();
        const Card strike = CardBuilder(QStringLiteral("strike"), CardType::Attack)
                                .setCost(1)
                                .addEffect(EffectDef::damage(6))
                                .build();
        for (int i = 0; i < 5; ++i) {
            player.addCardToDeck(strike);
        }

        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy));
        fsm.addCardToHand(wound);
        fsm.addCardToHand(dazed);

        const int handBefore = fsm.hand().size();
        fsm.playCard(wound, &enemy);
        QCOMPARE(fsm.hand().size(), handBefore);

        const int exhaustBefore = fsm.exhaustPileCount();
        fsm.playerEndTurn();
        QVERIFY(fsm.exhaustPileCount() >= exhaustBefore + 1);
    }

    void test_status_cards_from_database()
    {
        CardDatabase::instance().load();

        Player player;
        Enemy enemy;
        player.setMaxHp(80);
        player.healToFull();
        prepareTestEnemy(enemy);

        const Card strike = CardDatabase::instance().getCard(QStringLiteral("strike"));
        const Card fallbackStrike = CardBuilder(QStringLiteral("strike"), CardType::Attack)
                                        .setCost(1)
                                        .addEffect(EffectDef::damage(6))
                                        .build();
        const Card deckCard = strike.id().isEmpty() ? fallbackStrike : strike;
        for (int i = 0; i < 5; ++i) {
            player.addCardToDeck(deckCard);
        }

        const Card wound = CardDatabase::instance().getCard(QStringLiteral("wound"));
        const Card burn = CardDatabase::instance().getCard(QStringLiteral("burn"));
        const Card burnPlus = CardDatabase::instance().getCard(QStringLiteral("burn+"));
        const Card dazed = CardDatabase::instance().getCard(QStringLiteral("dazed"));
        const Card slimed = CardDatabase::instance().getCard(QStringLiteral("slimed"));

        QVERIFY(!wound.id().isEmpty());
        QVERIFY(!burn.id().isEmpty());
        QVERIFY(!dazed.id().isEmpty());
        QVERIFY(!slimed.id().isEmpty());
        QVERIFY(wound.hasEffectType(QStringLiteral("unplayable")));
        QVERIFY(burn.hasEffectType(QStringLiteral("unplayable")));
        QVERIFY(dazed.hasEffectType(QStringLiteral("unplayable")));
        QVERIFY(dazed.hasEffectType(QStringLiteral("ethereal")));
        QVERIFY(slimed.hasEffectType(QStringLiteral("exhaust")));
        QCOMPARE(slimed.cost(), 1);
        QCOMPARE(slimed.data().type, CardType::Skill);
        QCOMPARE(slimed.data().target, TargetType::Self);

        BattleFSM fsm;
        fsm.startCombat(&player, enemyList(&enemy));

        // --- Unplayable: Wound / Burn / Dazed must not leave the hand ---
        fsm.addCardToHand(wound);
        fsm.addCardToHand(burn);
        fsm.addCardToHand(dazed);
        const int handWithStatus = fsm.hand().size();
        const int energyBefore = fsm.currentEnergy();

        fsm.playCard(wound, &player);
        fsm.playCard(burn, &player);
        fsm.playCard(dazed, &player);
        QCOMPARE(fsm.hand().size(), handWithStatus);
        QCOMPARE(fsm.currentEnergy(), energyBefore);
        QCOMPARE(fsm.exhaustPileCount(), 0);

        // --- Slimed: playable, exhausts, no other side effects ---
        fsm.addCardToHand(slimed);
        const int hpBeforeSlimed = player.currentHp();
        const int exhaustBeforeSlimed = fsm.exhaustPileCount();
        const int handBeforeSlimed = fsm.hand().size();
        fsm.playCard(slimed, &player);
        QCOMPARE(fsm.hand().size(), handBeforeSlimed - 1);
        QCOMPARE(fsm.exhaustPileCount(), exhaustBeforeSlimed + 1);
        QCOMPARE(player.currentHp(), hpBeforeSlimed);
        QCOMPARE(fsm.currentEnergy(), energyBefore - 1);

        // --- End of turn: two Burns deal 2 each; Dazed exhausts (Ethereal) ---
        // Hand still has wound + burn + dazed from earlier; add a second burn.
        fsm.addCardToHand(burn);
        const int hpBeforeEnd = player.currentHp();
        const int exhaustBeforeEnd = fsm.exhaustPileCount();

        // Count Burns and Dazed currently in hand for assertions.
        int burnsInHand = 0;
        int dazedInHand = 0;
        int woundInHand = 0;
        for (const Card &card : fsm.hand()) {
            if (card.id() == QStringLiteral("burn") || card.id() == QStringLiteral("burn+")) {
                ++burnsInHand;
            } else if (card.id() == QStringLiteral("dazed")) {
                ++dazedInHand;
            } else if (card.id() == QStringLiteral("wound")) {
                ++woundInHand;
            }
        }
        QVERIFY(burnsInHand >= 2);
        QVERIFY(dazedInHand >= 1);
        QVERIFY(woundInHand >= 1);

        fsm.playerEndTurn();

        QCOMPARE(player.currentHp(), hpBeforeEnd - burnsInHand * 2);
        // Dazed is Ethereal → exhaust. Wound is not ethereal → must not inflate exhaust.
        QCOMPARE(fsm.exhaustPileCount(), exhaustBeforeEnd + dazedInHand);

        // --- Upgraded Burn deals 4 ---
        if (!burnPlus.id().isEmpty()) {
            Player player2;
            Enemy enemy2;
            player2.setMaxHp(80);
            player2.healToFull();
            prepareTestEnemy(enemy2);
            for (int i = 0; i < 5; ++i) {
                player2.addCardToDeck(deckCard);
            }

            BattleFSM fsm2;
            fsm2.startCombat(&player2, enemyList(&enemy2));
            fsm2.addCardToHand(burnPlus);
            const int handBeforePlay = fsm2.hand().size();
            fsm2.playCard(burnPlus, &player2);
            QCOMPARE(fsm2.hand().size(), handBeforePlay);

            const int hpBeforeBurnPlus = player2.currentHp();
            fsm2.playerEndTurn();
            QCOMPARE(player2.currentHp(), hpBeforeBurnPlus - 4);
        }
    }
};

int runCombatTests(int argc, char *argv[])
{
    TestCombat tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "test_combat.moc"
