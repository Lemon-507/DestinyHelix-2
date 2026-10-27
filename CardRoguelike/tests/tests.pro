QT += testlib
QT -= gui

CONFIG += c++17 console testcase
TEMPLATE = app
TARGET = CardRoguelikeTests

INCLUDEPATH += ../corelib

CONFIG(debug, debug|release) {
    LIBS += -L$$OUT_PWD/../corelib/debug -lcorelib
    PRE_TARGETDEPS += $$OUT_PWD/../corelib/debug/libcorelib.a
} else {
    LIBS += -L$$OUT_PWD/../corelib/release -lcorelib
    PRE_TARGETDEPS += $$OUT_PWD/../corelib/release/libcorelib.a
}

SOURCES += \
    test_runner.cpp \
    unit/test_skeleton.cpp \
    unit/test_map_shrink.cpp \
    unit/test_combat.cpp \
    unit/test_nodes.cpp \
    unit/test_game_manager.cpp \
    unit/test_relics.cpp \
    integration/test_full_run.cpp \
    unit/test_enemies.cpp \
    unit/test_card_text.cpp
