QT += core
QT -= gui widgets

CONFIG += c++17 staticlib
TEMPLATE = lib
TARGET = corelib

INCLUDEPATH += \
    . \
    battle/entities \
    battle/logic \
    battle/data \
    map \
    events \
    potions \
    save \
    systems \
    utils

RESOURCES += resources.qrc

HEADERS += \
    map/node_type.h \
    map/map_manager.h \
    battle/entities/actor.h \
    battle/entities/power.h \
    battle/entities/card.h \
    battle/entities/player.h \
    battle/entities/enemy.h \
    battle/entities/relic.h \
    battle/entities/shrink_damage_power.h \
    battle/entities/status_powers.h \
    battle/logic/effectresolver.h \
    battle/logic/power_factory.h \
    battle/logic/battlefsm.h \
    battle/logic/intention_sm.h \
    battle/logic/relic_manager.h \
    battle/logic/card_text.h \
    battle/data/card_database.h \
    battle/data/enemy_database.h \
    battle/data/enemy_pool.h \
    battle/data/relic_database.h \
    events/event_database.h \
    events/event_manager.h \
    potions/potion_database.h \
    character_factory.h \
    game_manager.h \
    systems/event_bus.h \
    save/game_state.h \
    save/save_manager.h \
    utils/rng.h \
    utils/json_utils.h

SOURCES += \
    map/map_manager.cpp \
    battle/entities/actor.cpp \
    battle/entities/power.cpp \
    battle/entities/card.cpp \
    battle/entities/player.cpp \
    battle/entities/enemy.cpp \
    battle/entities/relic.cpp \
    battle/entities/shrink_damage_power.cpp \
    battle/entities/status_powers.cpp \
    battle/logic/effectresolver.cpp \
    battle/logic/power_factory.cpp \
    battle/logic/battlefsm.cpp \
    battle/logic/intention_sm.cpp \
    battle/logic/relic_manager.cpp \
    battle/logic/card_text.cpp \
    battle/data/card_database.cpp \
    battle/data/enemy_database.cpp \
    battle/data/enemy_pool.cpp \
    battle/data/relic_database.cpp \
    events/event_database.cpp \
    events/event_manager.cpp \
    potions/potion_database.cpp \
    character_factory.cpp \
    game_manager.cpp \
    systems/event_bus.cpp \
    save/save_manager.cpp \
    utils/rng.cpp
