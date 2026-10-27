QT += widgets

CONFIG += c++17
TEMPLATE = app
TARGET = CardRoguelike

INCLUDEPATH += \
    ../corelib \
    . \
    views \
    viewmodels \
    widgets \
    utils

CONFIG(debug, debug|release) {
    LIBS += -L$$OUT_PWD/../corelib/debug -lcorelib
    PRE_TARGETDEPS += $$OUT_PWD/../corelib/debug/libcorelib.a
} else {
    LIBS += -L$$OUT_PWD/../corelib/release -lcorelib
    PRE_TARGETDEPS += $$OUT_PWD/../corelib/release/libcorelib.a
}

RESOURCES += app.qrc

HEADERS += \
    views/main_window.h \
    views/map_view.h \
    views/map_cell_item.h \
    views/battle_view.h \
    views/combat_animations.h \
    views/reward_view.h \
    views/death_view.h \
    views/event_view.h \
    views/shop_view.h \
    views/campfire_view.h \
    views/card_collection_view.h \
    viewmodels/map_viewmodel.h \
    viewmodels/battle_viewmodel.h \
    viewmodels/reward_viewmodel.h \
    viewmodels/event_viewmodel.h \
    viewmodels/shop_viewmodel.h \
    viewmodels/campfire_viewmodel.h \
    viewmodels/card_collection_viewmodel.h \
    widgets/card_widget.h \
    widgets/relic_bar_widget.h \
    utils/bind_property.h \
    utils/buff_catalog.h \
    utils/item_tooltips.h \
    utils/art_assets.h

SOURCES += \
    main.cpp \
    views/main_window.cpp \
    views/map_view.cpp \
    views/map_cell_item.cpp \
    views/battle_view.cpp \
    views/combat_animations.cpp \
    views/reward_view.cpp \
    views/death_view.cpp \
    views/event_view.cpp \
    views/shop_view.cpp \
    views/campfire_view.cpp \
    views/card_collection_view.cpp \
    viewmodels/map_viewmodel.cpp \
    viewmodels/battle_viewmodel.cpp \
    viewmodels/reward_viewmodel.cpp \
    viewmodels/event_viewmodel.cpp \
    viewmodels/shop_viewmodel.cpp \
    viewmodels/campfire_viewmodel.cpp \
    viewmodels/card_collection_viewmodel.cpp \
    widgets/card_widget.cpp \
    widgets/relic_bar_widget.cpp \
    utils/buff_catalog.cpp \
    utils/item_tooltips.cpp \
    utils/art_assets.cpp
