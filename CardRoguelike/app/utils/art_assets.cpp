#include "art_assets.h"

#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QtGlobal>
#include <QtGui/QIcon>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

namespace ArtAssets {
namespace {

QHash<QString, QPixmap> g_cache;
QSet<QString> g_warnedMissing;

QString resourcePathForEnemy(const QString &enemyId, bool defensiveMode = false)
{
    if (defensiveMode && enemyId == QStringLiteral("the_guardian")) {
        return QStringLiteral(":/art/enemies/bosses/The-guardian-def-pretty.png");
    }

    static const QHash<QString, QString> kMap = {
        {QStringLiteral("cultist"), QStringLiteral(":/art/enemies/normals/Cultist-pretty.png")},
        {QStringLiteral("jaw_worm"), QStringLiteral(":/art/enemies/normals/Jaw-worm-pretty.png")},
        {QStringLiteral("spike_slime_large"),
         QStringLiteral(":/art/enemies/normals/Spike-slime-l-pretty.png")},
        {QStringLiteral("spike_slime_medium"),
         QStringLiteral(":/art/enemies/normals/Spike_Slime_M.png")},
        {QStringLiteral("spike_slime_small"),
         QStringLiteral(":/art/enemies/normals/Spike_Slime_S.png")},
        {QStringLiteral("acid_slime_large"),
         QStringLiteral(":/art/enemies/normals/Acid-slime-l-pretty.png")},
        {QStringLiteral("acid_slime_medium"),
         QStringLiteral(":/art/enemies/normals/Acid-slime-m-pretty.png")},
        {QStringLiteral("acid_slime_small"),
         QStringLiteral(":/art/enemies/normals/Acid-slime-s-pretty.png")},
        {QStringLiteral("fat_gremlin"), QStringLiteral(":/art/enemies/normals/Fat-gremlin-pretty.png")},
        {QStringLiteral("mad_gremlin"), QStringLiteral(":/art/enemies/normals/Mad-gremlin-pretty.png")},
        {QStringLiteral("shield_gremlin"), QStringLiteral(":/art/enemies/normals/Shield_Gremlin.png")},
        {QStringLiteral("sneaky_gremlin"),
         QStringLiteral(":/art/enemies/normals/Sneaky-gremlin-pretty.png")},
        {QStringLiteral("gremlin_wizard"),
         QStringLiteral(":/art/enemies/normals/Wizard-gremlin-pretty.png")},
        {QStringLiteral("red_louse"), QStringLiteral(":/art/enemies/normals/Louse-red-pretty.png")},
        {QStringLiteral("green_louse"), QStringLiteral(":/art/enemies/normals/Louse-green-pretty.png")},
        {QStringLiteral("red_slaver"), QStringLiteral(":/art/enemies/normals/Slaver-red-pretty.png")},
        {QStringLiteral("blue_slaver"), QStringLiteral(":/art/enemies/normals/Slaver-blue-pretty.png")},
        {QStringLiteral("fungi_beast"), QStringLiteral(":/art/enemies/normals/Fungi-beast-pretty.png")},
        {QStringLiteral("looter"), QStringLiteral(":/art/enemies/normals/Looter-pretty.png")},
        {QStringLiteral("gremlin_nob"), QStringLiteral(":/art/enemies/elites/Gremlin-nob-pretty.png")},
        {QStringLiteral("lagavulin"), QStringLiteral(":/art/enemies/elites/Lagavulin-awake-pretty.png")},
        {QStringLiteral("sentry"), QStringLiteral(":/art/enemies/elites/Sentry-pretty.png")},
        {QStringLiteral("hexaghost"), QStringLiteral(":/art/enemies/bosses/Hexaghust.png")},
        {QStringLiteral("the_guardian"),
         QStringLiteral(":/art/enemies/bosses/The-guardian-atk-pretty.png")},
        {QStringLiteral("slime_boss"), QStringLiteral(":/art/enemies/bosses/Slime-boss-pretty.png")},
    };
    return kMap.value(enemyId);
}

QString resourcePathForEvent(const QString &eventId)
{
    static const QHash<QString, QString> kMap = {
        {QStringLiteral("priest"), QStringLiteral(":/art/events/priest.png")},
        {QStringLiteral("shining_light"), QStringLiteral(":/art/events/Shining_Light.png")},
        {QStringLiteral("slime_world"), QStringLiteral(":/art/events/Event_-_World_of_Goop.png")},
        {QStringLiteral("living_wall"), QStringLiteral(":/art/events/Event_-_Living_Wall.png")},
        {QStringLiteral("wandering_merchant"), QStringLiteral(":/art/events/wandering_merchant.png")},
        {QStringLiteral("suspicious_chest"), QStringLiteral(":/art/events/Chest.png")},
        {QStringLiteral("big_fish"), QStringLiteral(":/art/events/Event_-_Big_Fish.png")},
    };
    return kMap.value(eventId);
}

QString resourcePathForMapLetter(const QString &letter)
{
    static const QHash<QString, QString> kMap = {
        {QStringLiteral("S"), QStringLiteral(":/art/map/ShopMapIcon.png")},
        {QStringLiteral("F"), QStringLiteral(":/art/map/RestMapIcon.png")},
        {QStringLiteral("E"), QStringLiteral(":/art/map/EliteMapIcon.png")},
        {QStringLiteral("N"), QStringLiteral(":/art/map/NormalMapIcon.png")},
        {QStringLiteral("?"), QStringLiteral(":/art/map/EventMapIcon.png")},
        // Generic boss fallback; prefer mapBossIcon(bossId) for the center node.
        {QStringLiteral("B"), QStringLiteral(":/art/map/Hexaghosticon.png")},
    };
    return kMap.value(letter);
}

QString resourcePathForMapBoss(const QString &bossEnemyId)
{
    static const QHash<QString, QString> kMap = {
        {QStringLiteral("hexaghost"), QStringLiteral(":/art/map/Hexaghosticon.png")},
        {QStringLiteral("the_guardian"), QStringLiteral(":/art/map/Guardianicon.png")},
        {QStringLiteral("slime_boss"), QStringLiteral(":/art/map/Slimebossicon.png")},
    };

    const QString mapped = kMap.value(bossEnemyId);
    if (!mapped.isEmpty()) {
        return mapped;
    }

    // Convention fallback for newly added bosses: enemy_<boss_id>.png
    if (!bossEnemyId.isEmpty()) {
        return QStringLiteral(":/art/map/enemy_%1.png").arg(bossEnemyId);
    }
    return QString();
}

QString resourcePathForPotion(const QString &potionId)
{
    static const QHash<QString, QString> kMap = {
        {QStringLiteral("ancient_potion"), QStringLiteral(":/art/potions/Ancient_Potion.png")},
        {QStringLiteral("block_potion"), QStringLiteral(":/art/potions/Block_Potion.png")},
        {QStringLiteral("blood_potion"), QStringLiteral(":/art/potions/Blood_Potion.png")},
        {QStringLiteral("cultist_potion"), QStringLiteral(":/art/potions/CultistPotion.png")},
        {QStringLiteral("dexterity_potion"), QStringLiteral(":/art/potions/Dexterity_Potion.png")},
        {QStringLiteral("energy_potion"), QStringLiteral(":/art/potions/Energy_Potion.png")},
        {QStringLiteral("explosive_potion"), QStringLiteral(":/art/potions/Explosive_Potion.png")},
        {QStringLiteral("fear_potion"), QStringLiteral(":/art/potions/Fear_Potion.png")},
        {QStringLiteral("fire_potion"), QStringLiteral(":/art/potions/Fire_Potion.png")},
        {QStringLiteral("fruit_juice"), QStringLiteral(":/art/potions/Fruit_Juice.png")},
        {QStringLiteral("strength_potion"), QStringLiteral(":/art/potions/Strength_Potion.png")},
        {QStringLiteral("swift_potion"), QStringLiteral(":/art/potions/Swift_Potion.png")},
        {QStringLiteral("weakness_potion"), QStringLiteral(":/art/potions/Weak_Potion.png")},
    };
    return kMap.value(potionId);
}

QString resourcePathForRelic(const QString &relicId)
{
    static const QHash<QString, QString> kMap = {
        {QStringLiteral("burning_blood"), QStringLiteral(":/art/relics/72px-BurningBlood.png")},
        {QStringLiteral("marble_bag"), QStringLiteral(":/art/relics/72px-Marbles.png")},
        {QStringLiteral("vajra"), QStringLiteral(":/art/relics/72px-Vajra.png")},
        {QStringLiteral("toy_ornithopter"), QStringLiteral(":/art/relics/72px-Ornithopter.png")},
        {QStringLiteral("prepared_backpack"), QStringLiteral(":/art/relics/72px-Bagofprep.png")},
        {QStringLiteral("insect_preserved_specimen"), QStringLiteral(":/art/relics/72px-Insect.png")},
        {QStringLiteral("smooth_stone"), QStringLiteral(":/art/relics/72px-Smooth_stone.png")},
        {QStringLiteral("pen_nib"), QStringLiteral(":/art/relics/72px-PenNib.png")},
        {QStringLiteral("paper_frog"), QStringLiteral(":/art/relics/72px-PaperFrog.png")},
        {QStringLiteral("old_coin"), QStringLiteral(":/art/relics/72px-OldCoin.png")},
        {QStringLiteral("hiking_boots"), QStringLiteral(":/art/relics/hiking_boots.png")},
        {QStringLiteral("fate"), QStringLiteral(":/art/relics/72px-fate.png")},
        {QStringLiteral("lizard_tail"), QStringLiteral(":/art/relics/72px-LizardTail.png")},
    };
    return kMap.value(relicId);
}

void warnMissing(const QString &kind, const QString &id, const QString &path)
{
    const QString key = kind + QLatin1Char(':') + id + QLatin1Char(':') + path;
    if (g_warnedMissing.contains(key)) {
        return;
    }
    g_warnedMissing.insert(key);
    if (path.isEmpty()) {
        qWarning("ArtAssets: no image mapping for %s \"%s\"", qPrintable(kind), qPrintable(id));
    } else {
        qWarning("ArtAssets: missing image for %s \"%s\" (%s)", qPrintable(kind), qPrintable(id),
                 qPrintable(path));
    }
}

QPixmap loadMapped(const QString &kind, const QString &id, const QString &path, const QSize &size)
{
    if (id.isEmpty()) {
        return {};
    }
    if (path.isEmpty()) {
        warnMissing(kind, id, path);
        return {};
    }

    const QPixmap pixmap = load(path);
    if (pixmap.isNull()) {
        warnMissing(kind, id, path);
        return {};
    }
    return scaled(pixmap, size);
}

} // namespace

QPixmap load(const QString &resourcePath)
{
    if (resourcePath.isEmpty()) {
        return {};
    }

    const auto it = g_cache.constFind(resourcePath);
    if (it != g_cache.cend()) {
        return it.value();
    }

    QPixmap pixmap(resourcePath);
    if (!pixmap.isNull()) {
        g_cache.insert(resourcePath, pixmap);
    }
    return pixmap;
}

QPixmap scaled(const QPixmap &source, const QSize &size)
{
    if (source.isNull() || !size.isValid() || size.isEmpty()) {
        return source;
    }
    return source.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap playerPortrait(const QSize &size)
{
    const QString path = QStringLiteral(":/art/character/ironclad.png");
    const QPixmap pixmap = load(path);
    if (pixmap.isNull()) {
        warnMissing(QStringLiteral("player"), QStringLiteral("ironclad"), path);
        return {};
    }
    return scaled(pixmap, size);
}

QPixmap enemyPortrait(const QString &enemyId, const QSize &size, bool defensiveMode)
{
    return loadMapped(QStringLiteral("enemy"), enemyId, resourcePathForEnemy(enemyId, defensiveMode),
                      size);
}

QPixmap eventIllustration(const QString &eventId, const QSize &size)
{
    return loadMapped(QStringLiteral("event"), eventId, resourcePathForEvent(eventId), size);
}

QPixmap mapNodeIcon(const QString &nodeLetter, const QSize &size)
{
    if (nodeLetter.isEmpty()) {
        return {};
    }
    const QString path = resourcePathForMapLetter(nodeLetter);
    if (path.isEmpty()) {
        return {};
    }
    return loadMapped(QStringLiteral("map"), nodeLetter, path, size);
}

QPixmap mapBossIcon(const QString &bossEnemyId, const QSize &size)
{
    const QString fallbackPath = QStringLiteral(":/art/map/Hexaghosticon.png");

    if (bossEnemyId.isEmpty()) {
        return loadMapped(QStringLiteral("map_boss"), QStringLiteral("hexaghost"), fallbackPath, size);
    }

    const QString path = resourcePathForMapBoss(bossEnemyId);
    const QPixmap pixmap = loadMapped(QStringLiteral("map_boss"), bossEnemyId, path, size);
    if (!pixmap.isNull()) {
        return pixmap;
    }

    // Missing art for this boss ID — keep Hexaghost as the generic placeholder.
    return loadMapped(QStringLiteral("map_boss"), QStringLiteral("hexaghost"), fallbackPath, size);
}

QPixmap potionIcon(const QString &potionId, const QSize &size)
{
    return loadMapped(QStringLiteral("potion"), potionId, resourcePathForPotion(potionId), size);
}

QPixmap relicIcon(const QString &relicId, const QSize &size)
{
    return loadMapped(QStringLiteral("relic"), relicId, resourcePathForRelic(relicId), size);
}

bool applyToLabel(QLabel *label, const QPixmap &pixmap, const QString &fallbackText)
{
    if (label == nullptr) {
        return false;
    }

    if (!pixmap.isNull()) {
        label->setPixmap(pixmap);
        label->setText(QString());
        return true;
    }

    label->setPixmap(QPixmap());
    if (!fallbackText.isNull()) {
        label->setText(fallbackText);
    }
    return false;
}

bool applyToButton(QPushButton *button, const QPixmap &pixmap, const QString &fallbackText)
{
    if (button == nullptr) {
        return false;
    }

    if (!pixmap.isNull()) {
        button->setIcon(QIcon(pixmap));
        button->setIconSize(button->size());
        button->setText(QString());
        return true;
    }

    button->setIcon(QIcon());
    if (!fallbackText.isNull()) {
        button->setText(fallbackText);
    }
    return false;
}

void preloadStaticMapIcons()
{
    const QStringList letters = {QStringLiteral("S"), QStringLiteral("F"), QStringLiteral("E"),
                                 QStringLiteral("N"), QStringLiteral("?"), QStringLiteral("B")};
    for (const QString &letter : letters) {
        const QString path = resourcePathForMapLetter(letter);
        if (!path.isEmpty()) {
            load(path);
        }
    }

    const QStringList bossIds = {QStringLiteral("hexaghost"), QStringLiteral("the_guardian"),
                                 QStringLiteral("slime_boss")};
    for (const QString &bossId : bossIds) {
        const QString path = resourcePathForMapBoss(bossId);
        if (!path.isEmpty()) {
            load(path);
        }
    }
}

} // namespace ArtAssets
