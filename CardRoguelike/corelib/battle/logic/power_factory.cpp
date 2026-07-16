#include "power_factory.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "battle/entities/actor.h"
#include "battle/entities/enemy.h"
#include "battle/entities/player.h"
#include "battle/entities/power.h"

namespace {

class GenericPower : public Power
{
public:
    GenericPower(Actor *owner, const QString &id, int value, int duration)
        : Power(owner, id, value, duration)
    {
    }

    QString name() const override
    {
        return powerId();
    }
};

class WeakPower : public Power
{
public:
    WeakPower(Actor *owner, int duration)
        : Power(owner, QStringLiteral("weak"), 1, duration)
    {
    }

    QString name() const override
    {
        return QStringLiteral("Weak");
    }

    int modifyOutgoingDamage(int damage) const override
    {
        return static_cast<int>(damage * 0.75);
    }
};

class VulnerablePower : public Power
{
public:
    VulnerablePower(Actor *owner, int duration)
        : Power(owner, QStringLiteral("vulnerable"), 1, duration)
    {
    }

    QString name() const override
    {
        return QStringLiteral("Vulnerable");
    }

    int modifyIncomingDamage(int damage) const override
    {
        return static_cast<int>(damage * 1.5);
    }
};

class FrailPower : public Power
{
public:
    FrailPower(Actor *owner, int duration)
        : Power(owner, QStringLiteral("frail"), 1, duration)
    {
    }

    QString name() const override
    {
        return QStringLiteral("Frail");
    }

    int modifyBlockGained(int block) const override
    {
        return static_cast<int>(block * 0.75);
    }
};

class RitualPower : public Power
{
public:
    RitualPower(Actor *owner, int value)
        : Power(owner, QStringLiteral("ritual"), value, -1)
    {
    }

    QString name() const override
    {
        return QStringLiteral("Ritual");
    }

    void onTurnEnd() override
    {
        if (owner() != nullptr) {
            owner()->addPower(PowerFactory::createPower(QStringLiteral("strength"), value(), -1));
        }
        Power::onTurnEnd();
    }
};

class MetallicizePower : public Power
{
public:
    MetallicizePower(Actor *owner, int value)
        : Power(owner, QStringLiteral("metallicize"), value, -1)
    {
    }

    QString name() const override
    {
        return QStringLiteral("Metallicize");
    }

    void onTurnEnd() override
    {
        if (owner() != nullptr) {
            owner()->addBlockRaw(value());
        }
        Power::onTurnEnd();
    }
};

class DemonFormPower : public Power
{
public:
    DemonFormPower(Actor *owner, int value)
        : Power(owner, QStringLiteral("demon_form"), value, -1)
    {
    }

    QString name() const override
    {
        return QStringLiteral("Demon Form");
    }

    void onTurnStart() override
    {
        if (owner() != nullptr) {
            owner()->addPower(PowerFactory::createPower(QStringLiteral("strength"), value(), -1));
        }
        Power::onTurnStart();
    }
};

class AngryPower : public Power
{
public:
    AngryPower(Actor *owner, int value)
        : Power(owner, QStringLiteral("angry"), value, -1)
    {
    }

    QString name() const override
    {
        return QStringLiteral("Angry");
    }

    void onDamageReceived(int damage, Actor *source) override
    {
        Q_UNUSED(source);
        if (damage > 0 && owner() != nullptr) {
            owner()->addPower(PowerFactory::createPower(QStringLiteral("strength"), value(), -1));
        }
    }
};

class ModeShiftPower : public Power
{
public:
    ModeShiftPower(Actor *owner, int threshold)
        : Power(owner, QStringLiteral("mode_shift"), threshold, -1)
    {
    }

    QString name() const override
    {
        return QStringLiteral("Mode Shift");
    }

    void onDamageReceived(int damage, Actor *source) override
    {
        Q_UNUSED(source);
        if (damage <= 0 || owner() == nullptr) {
            return;
        }

        auto *enemy = qobject_cast<Enemy *>(owner());
        if (enemy != nullptr) {
            enemy->notifyGuardianDamageTaken(damage);
        }
    }
};

class SharpHidePower : public Power
{
public:
    SharpHidePower(Actor *owner, int value)
        : Power(owner, QStringLiteral("sharp_hide"), value, -1)
    {
    }

    QString name() const override
    {
        return QStringLiteral("Sharp Hide");
    }

    void onPlayerPlayedAttack(Player *player) override
    {
        if (player == nullptr || value() <= 0) {
            return;
        }
        player->takeDirectDamage(value());
    }
};

class CurlUpPower : public Power
{
public:
    CurlUpPower(Actor *owner, int value)
        : Power(owner, QStringLiteral("curl_up"), value, -1)
        , m_triggered(false)
    {
    }

    QString name() const override
    {
        return QStringLiteral("Curl Up");
    }

    void onDamageReceived(int damage, Actor *source) override
    {
        Q_UNUSED(source);
        if (damage > 0 && !m_triggered && owner() != nullptr) {
            owner()->addBlockRaw(value());
            m_triggered = true;
        }
    }

private:
    bool m_triggered = false;
};

class SporeCloudPower : public Power
{
public:
    SporeCloudPower(Actor *owner, int value)
        : Power(owner, QStringLiteral("spore_cloud"), value, -1)
    {
    }

    QString name() const override
    {
        return QStringLiteral("Spore Cloud");
    }
};

int resolvedDuration(const BuffDefinition *definition, int value, int duration)
{
    if (duration >= 0) {
        return duration;
    }
    if (definition != nullptr && definition->stacking == QStringLiteral("duration")) {
        return value > 0 ? value : definition->durationDefault;
    }
    return -1;
}

std::unique_ptr<Power> buildPower(const QString &id, Actor *owner, int value, int duration)
{
    const BuffDefinition *definition = PowerFactory::instance().definition(id);
    const int resolved = resolvedDuration(definition, value, duration);

    if (id == QStringLiteral("weak")) {
        return std::make_unique<WeakPower>(owner, resolved);
    }
    if (id == QStringLiteral("vulnerable")) {
        return std::make_unique<VulnerablePower>(owner, resolved);
    }
    if (id == QStringLiteral("frail")) {
        return std::make_unique<FrailPower>(owner, resolved);
    }
    if (id == QStringLiteral("ritual")) {
        return std::make_unique<RitualPower>(owner, value > 0 ? value : 1);
    }
    if (id == QStringLiteral("metallicize")) {
        return std::make_unique<MetallicizePower>(owner, value > 0 ? value : 1);
    }
    if (id == QStringLiteral("demon_form")) {
        return std::make_unique<DemonFormPower>(owner, value > 0 ? value : 1);
    }
    if (id == QStringLiteral("angry")) {
        return std::make_unique<AngryPower>(owner, value > 0 ? value : 1);
    }
    if (id == QStringLiteral("curl_up")) {
        return std::make_unique<CurlUpPower>(owner, value > 0 ? value : 1);
    }
    if (id == QStringLiteral("spore_cloud")) {
        return std::make_unique<SporeCloudPower>(owner, value > 0 ? value : 1);
    }
    if (id == QStringLiteral("mode_shift")) {
        return std::make_unique<ModeShiftPower>(owner, value > 0 ? value : 30);
    }
    if (id == QStringLiteral("sharp_hide")) {
        return std::make_unique<SharpHidePower>(owner, value > 0 ? value : 3);
    }

    return std::make_unique<GenericPower>(owner, id, value > 0 ? value : 1, resolved);
}

} // namespace

PowerFactory &PowerFactory::instance()
{
    static PowerFactory factory;
    return factory;
}

void PowerFactory::load()
{
    if (m_loaded) {
        return;
    }

    Q_INIT_RESOURCE(resources);

    QFile file(QStringLiteral(":/resources/data/buffs.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QJsonArray buffs;
    if (document.isArray()) {
        buffs = document.array();
    } else {
        buffs = document.object().value(QStringLiteral("buffs")).toArray();
    }

    for (const QJsonValue &value : buffs) {
        const QJsonObject obj = value.toObject();
        BuffDefinition definition;
        definition.id = obj.value(QStringLiteral("id")).toString();
        definition.name = obj.value(QStringLiteral("name")).toString();
        definition.effect = obj.value(QStringLiteral("effect")).toString();
        definition.stacking = obj.value(QStringLiteral("stacking")).toString();
        definition.valueDefault = obj.value(QStringLiteral("value_default")).toInt(1);
        definition.durationDefault = obj.value(QStringLiteral("duration_default")).toInt(1);
        if (!definition.id.isEmpty()) {
            m_definitions.insert(definition.id, definition);
        }
    }

    m_loaded = true;
}

const BuffDefinition *PowerFactory::definition(const QString &id) const
{
    const_cast<PowerFactory *>(this)->load();
    const auto it = m_definitions.constFind(id);
    if (it == m_definitions.constEnd()) {
        return nullptr;
    }
    return &(*it);
}

std::unique_ptr<Power> PowerFactory::createPower(const QString &id, int value, int duration)
{
    instance().load();
    return buildPower(id, nullptr, value, duration);
}

