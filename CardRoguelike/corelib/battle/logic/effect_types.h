#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>

// Effect type ids handled by EffectResolver. Card text and the JSON scan test
// use this list so a new type cannot land in data without being registered.
inline const QStringList &knownEffectTypes()
{
    static const QStringList types = {
        QStringLiteral("deal_damage"),
        QStringLiteral("deal_damage_per_block"),
        QStringLiteral("gain_block"),
        QStringLiteral("restore_hp"),
        QStringLiteral("restore_hp_percent"),
        QStringLiteral("revive_to_percent_hp"),
        QStringLiteral("gain_gold"),
        QStringLiteral("lose_hp"),
        QStringLiteral("lose_hp_percent"),
        QStringLiteral("gain_random_relic"),
        QStringLiteral("add_steps"),
        QStringLiteral("lose_gold"),
        QStringLiteral("lose_gold_random"),
        QStringLiteral("gain_potion"),
        QStringLiteral("gain_random_potion"),
        QStringLiteral("add_card"),
        QStringLiteral("remove_card"),
        QStringLiteral("upgrade_card"),
        QStringLiteral("transform_card"),
        QStringLiteral("upgrade_random_cards"),
        QStringLiteral("draw_cards"),
        QStringLiteral("draw_card"),
        QStringLiteral("gain_energy"),
        QStringLiteral("exhaust"),
        QStringLiteral("unplayable"),
        QStringLiteral("ethereal"),
        QStringLiteral("end_of_turn_damage"),
        QStringLiteral("increase_damage_per_play"),
        QStringLiteral("add_card_to_hand"),
        QStringLiteral("exhaust_non_attack_gain_block"),
        QStringLiteral("apply_power_to_all_enemies"),
        QStringLiteral("gain_strength"),
        QStringLiteral("max_hp_increase"),
        QStringLiteral("apply_power"),
        QStringLiteral("double_damage_every_10th_attack"),
        QStringLiteral("increase_vulnerable_multiplier"),
        QStringLiteral("explored_nodes_no_step_cost"),
        QStringLiteral("shrink_damage_cannot_kill"),
        QStringLiteral("elite_hp_multiplier"),
    };
    return types;
}

inline bool isKnownEffectType(const QString &type)
{
    return knownEffectTypes().contains(type);
}
