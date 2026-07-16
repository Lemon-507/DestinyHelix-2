#include <QtTest>

#include "battle/data/enemy_database.h"
#include "battle/entities/enemy.h"
#include "battle/entities/player.h"
#include "battle/entities/status_powers.h"
#include "battle/logic/battlefsm.h"
#include "battle/logic/effectresolver.h"
#include "battle/logic/intention_sm.h"
#include "battle/logic/power_factory.h"
#include "character_factory.h"
#include "utils/rng.h"

namespace {

const EnemyData *requireEnemyData(const QString &id)
{
    EnemyDatabase::instance().load();
    const EnemyData *data = EnemyDatabase::instance().getEnemy(id);
    Q_ASSERT(data != nullptr);
    return data;
}

void loadEnemy(Enemy &enemy, const QString &id)
{
    enemy.loadFromData(*requireEnemyData(id));
}

} // namespace

class TestEnemies : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        CharacterFactory::initialize();
    }

    void test_enemy_database_loads_cultist()
    {
        const EnemyData *cultist = requireEnemyData(QStringLiteral("cultist"));
        QCOMPARE(cultist->name, QStringLiteral("Cultist"));
        QCOMPARE(cultist->type, QStringLiteral("normal"));
        QCOMPARE(cultist->intents.size(), 2);
        QCOMPARE(cultist->behavior.type, QStringLiteral("initial_then_loop"));
        QCOMPARE(cultist->behavior.loopSequence.size(), 1);
        QCOMPARE(cultist->behavior.loopSequence.at(0), 1);
    }

    void test_cultist_sequence_intents()
    {
        Enemy cultist;
        loadEnemy(cultist, QStringLiteral("cultist"));

        const EnemyIntentDef firstPeek = cultist.peekNextIntent();
        QCOMPARE(firstPeek.name, QStringLiteral("Incantation"));

        const EnemyIntentDef first = cultist.consumeNextIntent();
        QCOMPARE(first.name, QStringLiteral("Incantation"));

        const EnemyIntentDef secondPeek = cultist.peekNextIntent();
        QCOMPARE(secondPeek.name, QStringLiteral("Dark Strike"));

        const EnemyIntentDef second = cultist.consumeNextIntent();
        QCOMPARE(second.name, QStringLiteral("Dark Strike"));

        const EnemyIntentDef thirdPeek = cultist.peekNextIntent();
        QCOMPARE(thirdPeek.name, QStringLiteral("Dark Strike"));
    }

    void test_jaw_worm_probability_weights_loaded()
    {
        const EnemyData *data = requireEnemyData(QStringLiteral("jaw_worm"));
        QCOMPARE(data->behavior.probability.size(), 3);
        QVERIFY(data->behavior.probability.contains(0));
        QVERIFY(data->behavior.probability.contains(1));
        QVERIFY(data->behavior.probability.contains(2));
    }

    void test_jaw_worm_probability_restriction()
    {
        const EnemyData *data = requireEnemyData(QStringLiteral("jaw_worm"));
        QCOMPARE(data->behavior.type, QStringLiteral("initial_then_probability"));
        QCOMPARE(data->behavior.restrictions.size(), 3);

        IntentionStateMachine machine(*data, nullptr);

        int chompRepeats = 0;
        int bellowRepeats = 0;
        int lastIndex = -1;
        for (int i = 0; i < 200; ++i) {
            const EnemyIntentDef intent = machine.nextIntent();
            const int index = machine.lastIntentIndex();
            Q_UNUSED(intent);
            if (index == 0 && lastIndex == 0) {
                ++chompRepeats;
            }
            if (index == 1 && lastIndex == 1) {
                ++bellowRepeats;
            }
            lastIndex = index;
        }

        QCOMPARE(chompRepeats, 0);
        QCOMPARE(bellowRepeats, 0);
    }

    void test_jaw_worm_battlefsm_no_double_chomp()
    {
        Enemy jawWorm;
        loadEnemy(jawWorm, QStringLiteral("jaw_worm"));

        Player player;
        player.setMaxHp(200);
        player.healToFull();

        BattleFSM fsm;
        fsm.startCombat(&player, QVector<Enemy *>{&jawWorm});

        int chompRepeats = 0;
        int lastIndex = -1;
        for (int turn = 0; turn < 25; ++turn) {
            const EnemyIntentDef preview = jawWorm.peekNextIntent();
            const int index = preview.name == QStringLiteral("Chomp")
                                  ? 0
                                  : (preview.name == QStringLiteral("Bellow") ? 1 : 2);
            fsm.playerEndTurn();
            if (index == 0 && lastIndex == 0) {
                ++chompRepeats;
            }
            lastIndex = index;
        }

        QCOMPARE(chompRepeats, 0);
    }

    void test_jaw_worm_peek_consume_never_double_chomp()
    {
        Enemy jawWorm;
        loadEnemy(jawWorm, QStringLiteral("jaw_worm"));

        int chompRepeats = 0;
        int lastIndex = -1;
        for (int turn = 0; turn < 50; ++turn) {
            jawWorm.peekNextIntent();
            jawWorm.peekNextIntent();
            const EnemyIntentDef intent = jawWorm.consumeNextIntent();
            const int index = intent.name == QStringLiteral("Chomp")
                                  ? 0
                                  : (intent.name == QStringLiteral("Bellow") ? 1 : 2);
            if (index == 0 && lastIndex == 0) {
                ++chompRepeats;
            }
            lastIndex = index;
        }

        QCOMPARE(chompRepeats, 0);
    }

    void test_gremlin_nob_starts_with_bellow()
    {
        const EnemyData *data = requireEnemyData(QStringLiteral("gremlin_nob"));
        QCOMPARE(data->behavior.type, QStringLiteral("initial_then_probability"));

        Enemy nob;
        loadEnemy(nob, QStringLiteral("gremlin_nob"));

        const EnemyIntentDef first = nob.peekNextIntent();
        QCOMPARE(first.name, QStringLiteral("Bellow"));
    }

    void test_spike_slime_split_scheduled_at_half_hp()
    {
        const EnemyData *data = requireEnemyData(QStringLiteral("spike_slime_large"));
        QCOMPARE(data->behavior.splitIntentIndex, 2);

        Enemy slime;
        loadEnemy(slime, QStringLiteral("spike_slime_large"));
        const int targetHp = slime.maxHp() / 2;
        if (slime.currentHp() > targetHp) {
            slime.takeDirectDamage(slime.currentHp() - targetHp);
        }

        const EnemyIntentDef next = slime.peekNextIntent();
        QCOMPARE(next.type, QStringLiteral("split"));
    }

    void test_red_slaver_sequence_after_first_turn()
    {
        const EnemyData *data = requireEnemyData(QStringLiteral("red_slaver"));
        QCOMPARE(data->behavior.type, QStringLiteral("initial_then_sequence"));
        QCOMPARE(data->behavior.loopSequence, QVector<int>({1, 1, 0}));

        Enemy slaver;
        loadEnemy(slaver, QStringLiteral("red_slaver"));

        QCOMPARE(slaver.consumeNextIntent().name, QStringLiteral("Stab"));
        QCOMPARE(slaver.consumeNextIntent().name, QStringLiteral("Scrape"));
        QCOMPARE(slaver.consumeNextIntent().name, QStringLiteral("Scrape"));
        QCOMPARE(slaver.consumeNextIntent().name, QStringLiteral("Stab"));
    }

    void test_fungi_beast_rules_parsed()
    {
        const EnemyData *data = requireEnemyData(QStringLiteral("fungi_beast"));
        QCOMPARE(data->behavior.type, QStringLiteral("fungi_rules"));
        QVERIFY(data->behavior.restrictions.contains(QStringLiteral("cannot_growth_two_consecutive")));
        QVERIFY(data->behavior.restrictions.contains(QStringLiteral("cannot_attack_three_consecutive")));
    }

    void test_cultist_ritual_strength_in_combat()
    {
        Player player;
        player.setMaxHp(80);
        player.healToFull();

        Enemy cultist;
        loadEnemy(cultist, QStringLiteral("cultist"));

        BattleFSM fsm;
        fsm.startCombat(&player, QVector<Enemy *>{&cultist});

        QCOMPARE(cultist.strength(), 0);
        // Incantation applies Ritual 3; Ritual grants Strength at end of that turn.
        fsm.playerEndTurn();
        QCOMPARE(cultist.strength(), 3);
    }

    void test_all_alive_enemies_execute_intents()
    {
        Player player;
        player.setMaxHp(80);
        player.healToFull();

        Enemy left;
        Enemy right;
        loadEnemy(left, QStringLiteral("cultist"));
        loadEnemy(right, QStringLiteral("cultist"));

        BattleFSM fsm;
        fsm.startCombat(&player, QVector<Enemy *>{&left, &right});

        // Turn 1: both use Incantation → Ritual → Strength at endTurn.
        fsm.playerEndTurn();
        QCOMPARE(left.strength(), 3);
        QCOMPARE(right.strength(), 3);

        // Turn 2: both Dark Strike for 6 + 3 Strength = 9 each.
        const int hpBefore = player.currentHp();
        fsm.playerEndTurn();
        QCOMPARE(player.currentHp(), hpBefore - 18);
    }

    void test_dead_enemy_skipped_remaining_enemies_still_act()
    {
        Player player;
        player.setMaxHp(80);
        player.healToFull();

        Enemy left;
        Enemy right;
        loadEnemy(left, QStringLiteral("cultist"));
        loadEnemy(right, QStringLiteral("cultist"));

        BattleFSM fsm;
        fsm.startCombat(&player, QVector<Enemy *>{&left, &right});

        left.takeDirectDamage(left.currentHp());
        QCOMPARE(left.currentHp(), 0);

        fsm.playerEndTurn();
        QCOMPARE(left.strength(), 0);
        QCOMPARE(right.strength(), 3);
    }

    void test_green_louse_rolls_bite_and_curl_up_at_combat_start()
    {
        EnemyDatabase::instance().load();
        const EnemyData *templateData = requireEnemyData(QStringLiteral("green_louse"));
        QVERIFY(templateData->intents.size() >= 1);
        QVERIFY(templateData->intents.at(0).rollBaseValue);
        QCOMPARE(templateData->intents.at(0).baseValueMin, 5);
        QCOMPARE(templateData->intents.at(0).baseValueMax, 7);
        QVERIFY(templateData->abilities.size() >= 1);
        QVERIFY(templateData->abilities.at(0).rollValue);
        QCOMPARE(templateData->abilities.at(0).valueMin, 5);
        QCOMPARE(templateData->abilities.at(0).valueMax, 7);

        RNG::seed(12345);
        Enemy louse;
        loadEnemy(louse, QStringLiteral("green_louse"));

        const int bite = louse.data()->intents.at(0).baseValue;
        const int curlUp = louse.abilityValue(QStringLiteral("curl_up"));
        QVERIFY(bite >= 5 && bite <= 7);
        QVERIFY(curlUp >= 5 && curlUp <= 7);
        QVERIFY(!louse.data()->intents.at(0).rollBaseValue);
        QVERIFY(!louse.data()->abilities.at(0).rollValue);

        // Combat-start roll is cached for this instance.
        QCOMPARE(louse.data()->intents.at(0).baseValue, bite);
        QCOMPARE(louse.getPowerValue(QStringLiteral("curl_up")), curlUp);

        Player player;
        player.setMaxHp(80);
        player.healToFull();
        const int hpBefore = player.currentHp();
        DamageCommand(&louse, &player, bite).execute();
        QCOMPARE(player.currentHp(), hpBefore - bite);

        // Re-loading rolls again (seed advanced), still in range.
        Enemy another;
        loadEnemy(another, QStringLiteral("green_louse"));
        const int bite2 = another.data()->intents.at(0).baseValue;
        QVERIFY(bite2 >= 5 && bite2 <= 7);
    }

    void test_cultist_and_gremlin_nob_fixed_intent_values()
    {
        Enemy cultist;
        loadEnemy(cultist, QStringLiteral("cultist"));
        QCOMPARE(cultist.data()->intents.at(0).buffValue, 3);
        QCOMPARE(cultist.data()->intents.at(1).baseValue, 6);

        Enemy nob;
        loadEnemy(nob, QStringLiteral("gremlin_nob"));
        QCOMPARE(nob.abilityValue(QStringLiteral("rage")), 2);
        QCOMPARE(nob.getPowerValue(QStringLiteral("rage")), 2);
        QCOMPARE(nob.data()->intents.at(1).baseValue, 6);
        QCOMPARE(nob.data()->intents.at(1).debuffValue, 2);
        QCOMPARE(nob.data()->intents.at(2).baseValue, 14);
    }

    void test_vulnerable_increases_damage_taken()
    {
        Player player;
        player.setMaxHp(80);
        player.healToFull();
        player.addPower(createStatusPower(QStringLiteral("vulnerable"), &player, 2));

        Enemy enemy;
        loadEnemy(enemy, QStringLiteral("cultist"));

        const int hpBefore = player.currentHp();
        DamageCommand damage(&enemy, &player, 10);
        damage.execute();
        QCOMPARE(player.currentHp(), hpBefore - 15);
    }

    void test_weak_reduces_damage_dealt()
    {
        Player player;
        player.setMaxHp(80);
        player.healToFull();

        Enemy enemy;
        loadEnemy(enemy, QStringLiteral("cultist"));
        enemy.addPower(createStatusPower(QStringLiteral("weak"), &enemy, 1));

        const int hpBefore = player.currentHp();
        DamageCommand damage(&enemy, &player, 10);
        damage.execute();
        QCOMPARE(player.currentHp(), hpBefore - 7);
    }

    void test_guardian_behavior_parsed()
    {
        const EnemyData *data = requireEnemyData(QStringLiteral("the_guardian"));
        QCOMPARE(data->behavior.type, QStringLiteral("guardian_mode"));
        QCOMPARE(data->behavior.attackModeSequence, QVector<int>({0, 1, 2, 3}));
        QCOMPARE(data->behavior.defenseModeSequence, QVector<int>({4, 5, 6}));
        QCOMPARE(data->behavior.modeSwitchDamageThreshold, 30);
        QCOMPARE(data->behavior.thresholdIncreasePerSwitch, 10);
    }

    void test_guardian_starts_in_attack_mode()
    {
        Enemy guardian;
        loadEnemy(guardian, QStringLiteral("the_guardian"));

        QVERIFY(guardian.hasPower(QStringLiteral("mode_shift")));
        QVERIFY(!guardian.hasPower(QStringLiteral("sharp_hide")));
        QVERIFY(!guardian.isGuardianDefensiveMode());

        const EnemyIntentDef firstIntent = guardian.peekNextIntent();
        QCOMPARE(firstIntent.type, QStringLiteral("block"));
        QCOMPARE(firstIntent.blockValue, 9);
    }

    void test_guardian_mode_shift_at_damage_threshold()
    {
        Player player;
        player.setMaxHp(200);
        player.healToFull();

        Enemy guardian;
        loadEnemy(guardian, QStringLiteral("the_guardian"));

        guardian.takeDamage(29, &player);
        QVERIFY(!guardian.isGuardianDefensiveMode());
        QVERIFY(guardian.hasPower(QStringLiteral("mode_shift")));

        guardian.takeDamage(1, &player);
        QVERIFY(guardian.isGuardianDefensiveMode());
        QVERIFY(!guardian.hasPower(QStringLiteral("mode_shift")));
        // Sharp Hide is not granted on mode entry — only by Strengthen.
        QVERIFY(!guardian.hasPower(QStringLiteral("sharp_hide")));

        const EnemyIntentDef nextIntent = guardian.peekNextIntent();
        QCOMPARE(nextIntent.type, QStringLiteral("buff"));
        QCOMPARE(nextIntent.buffType, QStringLiteral("sharp_hide"));
        QCOMPARE(nextIntent.buffValue, 3);
    }

    void test_guardian_strengthen_applies_sharp_hide()
    {
        Player player;
        player.setMaxHp(200);
        player.healToFull();

        Enemy guardian;
        loadEnemy(guardian, QStringLiteral("the_guardian"));
        guardian.takeDamage(30, &player);
        QVERIFY(guardian.isGuardianDefensiveMode());
        QVERIFY(!guardian.hasPower(QStringLiteral("sharp_hide")));

        BattleFSM fsm;
        fsm.startCombat(&player, QVector<Enemy *>{&guardian});
        QVERIFY(!guardian.hasPower(QStringLiteral("sharp_hide")));

        fsm.playerEndTurn();

        QVERIFY(guardian.hasPower(QStringLiteral("sharp_hide")));
        QCOMPARE(guardian.getPowerValue(QStringLiteral("sharp_hide")), 3);
    }

    void test_guardian_sharp_hide_damages_player_on_attack()
    {
        Player player;
        player.setMaxHp(200);
        player.healToFull();

        Enemy guardian;
        loadEnemy(guardian, QStringLiteral("the_guardian"));
        guardian.takeDamage(30, &player);
        QVERIFY(!guardian.hasPower(QStringLiteral("sharp_hide")));

        // Apply Sharp Hide the same way Strengthen does (buff intent).
        const EnemyIntentDef strengthen = guardian.consumeNextIntent();
        QCOMPARE(strengthen.type, QStringLiteral("buff"));
        QCOMPARE(strengthen.buffType, QStringLiteral("sharp_hide"));
        guardian.addPower(
            PowerFactory::createPower(strengthen.buffType, strengthen.buffValue, -1));
        QVERIFY(guardian.hasPower(QStringLiteral("sharp_hide")));

        const int hpBefore = player.currentHp();
        guardian.notifyPlayerPlayedAttack(&player);
        QCOMPARE(player.currentHp(), hpBefore - 3);
    }

    void test_guardian_returns_to_attack_after_double_slash()
    {
        Player player;
        player.setMaxHp(200);
        player.healToFull();

        Enemy guardian;
        loadEnemy(guardian, QStringLiteral("the_guardian"));
        guardian.takeDamage(30, &player);

        QCOMPARE(guardian.consumeNextIntent().type, QStringLiteral("buff"));
        QCOMPARE(guardian.consumeNextIntent().type, QStringLiteral("attack"));
        QCOMPARE(guardian.consumeNextIntent().type, QStringLiteral("attack_multiple"));

        guardian.completeGuardianIntent(6);
        QVERIFY(!guardian.isGuardianDefensiveMode());
        QVERIFY(!guardian.hasPower(QStringLiteral("sharp_hide")));
        QVERIFY(guardian.hasPower(QStringLiteral("mode_shift")));
        QCOMPARE(guardian.guardianModeShiftThreshold(), 40);
        QCOMPARE(guardian.getPowerValue(QStringLiteral("mode_shift")), 40);

        const EnemyIntentDef whirlwind = guardian.peekNextIntent();
        QCOMPARE(whirlwind.type, QStringLiteral("attack_multiple"));
        QCOMPARE(whirlwind.baseValue, 5);
        QCOMPARE(whirlwind.times, 4);
    }

    void test_slime_boss_split_parsed()
    {
        const EnemyData *data = requireEnemyData(QStringLiteral("slime_boss"));
        QCOMPARE(data->behavior.splitIntentIndex, 3);
        QCOMPARE(data->intents.size(), 4);
        QCOMPARE(data->intents.at(3).type, QStringLiteral("split"));
        QCOMPARE(data->intents.at(3).splitInto,
                 QStringList({QStringLiteral("acid_slime_large"), QStringLiteral("spike_slime_large")}));
    }

    void test_slime_boss_forces_split_intent_at_half_hp()
    {
        Enemy boss;
        loadEnemy(boss, QStringLiteral("slime_boss"));
        QCOMPARE(boss.maxHp(), 140);

        boss.takeDirectDamage(boss.currentHp() - 70);
        QCOMPARE(boss.currentHp(), 70);

        const EnemyIntentDef next = boss.peekNextIntent();
        QCOMPARE(next.type, QStringLiteral("split"));
        QCOMPARE(next.splitInto.size(), 2);
    }

    void test_slime_boss_split_spawns_large_slimes()
    {
        Player player;
        player.setMaxHp(200);
        player.healToFull();

        Enemy boss;
        loadEnemy(boss, QStringLiteral("slime_boss"));
        boss.takeDirectDamage(boss.currentHp() - 70);

        BattleFSM fsm;
        fsm.startCombat(&player, QVector<Enemy *>{&boss});

        QCOMPARE(boss.peekNextIntent().type, QStringLiteral("split"));
        fsm.playerEndTurn();

        QCOMPARE(boss.currentHp(), 0);
        const QVector<Enemy *> enemies = fsm.enemies();
        QCOMPARE(enemies.size(), 2);
        QVERIFY(enemies.at(0) != nullptr);
        QVERIFY(enemies.at(1) != nullptr);
        QCOMPARE(enemies.at(0)->enemyId(), QStringLiteral("acid_slime_large"));
        QCOMPARE(enemies.at(1)->enemyId(), QStringLiteral("spike_slime_large"));
        QCOMPARE(enemies.at(0)->currentHp(), 70);
        QCOMPARE(enemies.at(1)->currentHp(), 70);
        QCOMPARE(enemies.at(0)->maxHp(), 70);
        QCOMPARE(enemies.at(1)->maxHp(), 70);
    }

    void test_spike_slime_large_split_spawns_mediums()
    {
        Player player;
        player.setMaxHp(200);
        player.healToFull();

        Enemy slime;
        loadEnemy(slime, QStringLiteral("spike_slime_large"));
        const int halfHp = slime.maxHp() / 2;
        slime.takeDirectDamage(slime.currentHp() - halfHp);

        BattleFSM fsm;
        fsm.startCombat(&player, QVector<Enemy *>{&slime});
        QCOMPARE(slime.peekNextIntent().type, QStringLiteral("split"));
        fsm.playerEndTurn();

        QCOMPARE(slime.currentHp(), 0);
        const QVector<Enemy *> enemies = fsm.enemies();
        QCOMPARE(enemies.size(), 2);
        QCOMPARE(enemies.at(0)->enemyId(), QStringLiteral("spike_slime_medium"));
        QCOMPARE(enemies.at(1)->enemyId(), QStringLiteral("spike_slime_medium"));
        QCOMPARE(enemies.at(0)->currentHp(), halfHp);
        QCOMPARE(enemies.at(1)->currentHp(), halfHp);
    }
};

int runEnemyTests(int argc, char *argv[])
{
    TestEnemies tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "test_enemies.moc"
