#pragma once

#include <QtCore/QObject>

namespace Map {

Q_NAMESPACE

enum class NodeType {
    Boss,
    Shop,
    Campfire,
    Elite,
    NormalEnemy,
    Event,
    Empty
};

Q_ENUM_NS(NodeType)

} // namespace Map

using NodeType = Map::NodeType;
