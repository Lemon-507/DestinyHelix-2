#pragma once

#include <QtCore/QString>

#include "battle/entities/card.h"

namespace CardText {

/// Human-readable text for a single effect, using the card's overall target for Gain vs Apply phrasing.
QString describeEffect(const EffectDef &effect, TargetType cardTarget = TargetType::Enemy);

/// Full card description: all effects joined with ". " (plus a trailing period when non-empty).
QString describeCard(const Card &card);

} // namespace CardText
