#pragma once

#include <QtCore/QObject>

#define BIND_PROPERTY(source, signalName, target, slotName) \
    QObject::connect(source, &std::remove_pointer<decltype(source)>::type::signalName, target, \
                     &std::remove_pointer<decltype(target)>::type::slotName)
