# Final Design Document: CardRoguelike

This document consolidates all confirmed design decisions, interface definitions, and JSON configuration examples. It is conflict-free and ready to guide development. The overall architecture follows the MVVM pattern, with core logic compiled into a static library `corelib`, and the UI layer using Qt Widgets.

------

## 1. Basic Implementation Plan

### 1.1 Overall Architecture

The game is driven by `QApplication`. The main window uses `QStackedWidget` to manage scenes: main menu, map, battle, settlement, etc. Core logic is compiled as a static library `corelib`, depending only on `QtCore` and C++17, with no UI coupling. Subsystems are decoupled through a global event bus `EventBus`, and card effects use the Command pattern.

### 1.2 Map System

#### Grid

- 11×11 grid with 121 nodes, stored as a 2D array containing `NodeType` and specific configuration.

#### Fixed Starting Points

- The four corners `(0,0)`, `(0,10)`, `(10,0)`, `(10,10)` and the midpoints of the four edges `(0,5)`, `(5,0)`, `(10,5)`, `(5,10)` — a total of 8 nodes — are fixed as empty nodes.
- After the game starts, the player manually clicks on one of the highlighted starting points in the UI. Once confirmed, the character is placed on that node.

#### Movement Rules

- Only orthogonal movement in four directions is allowed; no diagonal movement or jumping. All adjacent nodes are traversable; there are no physical barriers.

#### Node Types and Quantities

- Boss: 1 (fixed at the center `(5,5)`)
- Shop: 10
- Campfire: 16 (including 4 fixed Campfires adjacent to the Boss in the four orthogonal directions, i.e., `(4,5)`, `(6,5)`, `(5,4)`, `(5,6)`)
- Elite: 10
- Normal Enemy: 24
- Event: 20
- Empty: 40 (including 8 fixed starting points)
- Total: 121.

#### Map Generation

1. First, place the Boss (center) and 8 surrounding campfires, as well as the 8 fixed empty starting points.
2. The remaining pool contains 10 shops, 12 campfires (20-8), 10 elites, 20 normal enemies, 20 events, and 32 empty nodes (40-8). Shuffle and randomly fill the empty slots.
3. No reachability check is performed; all adjacent nodes are connected.

#### Explored Nodes

- After defeating an enemy or resolving an event, the node is marked `explored = true`. Upon subsequent entry, it is always treated as an empty node: step cost is that of an empty node (+1), and shrink damage is also calculated as for an empty node.

#### Victory Condition

- Winning the battle at the Boss node `(5,5)` results in victory, triggering a settlement screen displaying game statistics and returning to the main menu.

#### Death Handling

- Any situation where HP ≤ 0 (including death by shrink or battle defeat) triggers a unified death flow: a settlement screen is shown, then the main menu is loaded. There is no save file reloading; only a new game can be started.

### 1.3 Shrink Mechanism

#### Layer Definitions (from outer to inner)

- Layer 1 (outermost): 40 nodes, `max(|x-5|,|y-5|) == 5`
- Layer 2: 32 nodes, `max(...) == 4`
- Layer 3: 24 nodes, `max(...) == 3`
- Layer 4: 16 nodes, `max(...) == 2`
- Layer 5: 8 nodes, `max(...) == 1`
- Center: 1 node (Boss), `max == 0`

#### Step Accumulation

- Entering an empty node (including explored): `totalSteps += 1`
- Entering a non-empty node (battle, event, shop, campfire, etc.): `totalSteps += 3`

#### Shrink Trigger

- When `totalSteps >= nextShrinkStep` and `currentShrinkCount < 5`, shrink is triggered. A `while` loop ensures multiple triggers when thresholds are crossed:

  cpp

  ```
  while (totalSteps >= nextShrinkStep && currentShrinkCount < 5) {
      currentShrinkCount++;
      nextShrinkStep += 12;
      // Emit shrink event, update ShrinkDamagePower damage coefficient in combat
  }
  ```

  

- Shrink thresholds are successively 12, 24, 36, 48, 60. After reaching 60, no further shrinking occurs; the counter is never reset.

#### Shrink Zone Area

- With shrink count `n` (1~5), the shrink zone covers layer 1 through layer `n`, i.e., `nodeLayer(x, y) > 5 - n`.
- The Boss node `(5,5)` is permanently immune to the shrink zone.

#### Determination Functions

cpp

```
int nodeLayer(int x, int y) {
    return std::max(std::abs(x - 5), std::abs(y - 5));
}
bool isInShrinkZone(int x, int y, int shrinkCount) {
    if (x == 5 && y == 5) return false;
    return nodeLayer(x, y) > (5 - shrinkCount);
}
```



#### Shrink Damage (n = currentShrinkCount)

- Entering an empty node (including explored) inside the shrink zone: immediately lose `n` HP.
- Entering an event/shop/campfire node inside the shrink zone: immediately lose `3n` HP.
- Entering a combat node inside the shrink zone: no damage on entry. After the battle starts, the player is given a `ShrinkDamagePower`: at the beginning of each player turn, lose `n` HP, ignoring block and all status effects, and can be lethal. This buff is removed after leaving combat. **If a shrink occurs during combat, the damage coefficient `n` of this buff is updated in real time.**
- Shrinking itself does not damage the player in place; only moving into a shrink zone node triggers entry damage.
- There are no means of immunizing against shrink damage. All shrink damage is direct life removal with no secondary confirmation.

#### Death Handling

- Same as described in 1.2.

### 1.4 Combat System

#### Turn-Based Rules

- Follows the *Slay the Spire* turn-based style: player turn begins → draw cards → play cards → end turn → enemy turn executes intent → loop.
- **Block is cleared at the end of the player's turn.**

#### Finite State Machine (BattleFSM)

- Manages combat flow; state transitions notify the UI via Qt signals.

#### Command Pattern

- Card effects use `ICommand` subclasses (`DamageCommand`, `ApplyPowerCommand`, etc.) and are executed in sequence by `EffectResolver`.

#### Enemy Intent

- Determined by AI at the start of the enemy turn, stored in the entity, and displayed in the UI with icon and value.

#### Shrink Buff

- If combat occurs within the shrink zone, the player is automatically given an undispellable `ShrinkDamagePower` at combat initialization. At the beginning of each player turn, the player loses `n` HP. If the shrink count increases mid-combat, the `n` value is updated.

#### Module Communication

- `GameManager` holds instances of each subsystem and decouples them through the global event bus `EventBus`. Relics and status effects listen and trigger via EventBus signals.

### 1.5 Map–Combat Integration

- Entering an enemy node switches to the battle scene.
- Upon victory, the node becomes explored and the player returns to the map; defeat triggers the death flow.
- Boss nodes force combat and cannot be canceled.
- The player cannot flee from battle.

### 1.6 Interaction Nodes

#### Campfire

- When staying at a campfire, the player can choose:
  - Recover 30% of maximum HP
  - Upgrade one chosen card (the player manually selects a card from their deck to upgrade)

#### Shop

- Spend gold to purchase cards or relics (specific item pools to be defined; extensible).

#### Events

- A random event script is triggered from the event database, selecting an event not yet completed. Example event "Trash Heap":

  json

  ```
  {
    "id": "trash_heap",
    "name": "Trash Heap",
    "description": "You pass by a pile of trash and glimpse a relic inside, but retrieving it will take some effort.",
    "options": [
      {
        "text": "Retrieve the relic",
        "effects": [
          { "type": "gain_random_relic", "value": 1 },
          { "type": "lose_hp", "value": 10 },
          { "type": "add_steps", "value": 2 }
        ]
      },
      {
        "text": "Leave",
        "effects": []
      }
    ]
  }
  ```

  

  Effects are executed in order. If HP loss causes death, subsequent effects are interrupted and the death flow starts.

#### Potions

- The player can use potions when the context matches. Potions are consumed upon use. Examples:

  - Health Potion:

    json

    ```
    {
      "id": "health_potion",
      "name": "Health Potion",
      "description": "Restore 20% of max HP (rounded down). In combat, can only be used during the player's turn. Outside combat, usable at any time.",
      "allowed_contexts": ["non_combat", "combat_player_turn"],
      "effects": [
        { "type": "restore_hp_percent", "value": 20, "target": "self" }
      ]
    }
    ```

    

  - Fire Potion:

    json

    ```
    {
      "id": "fire_potion",
      "name": "Fire Potion",
      "description": "Deal 20 damage to an enemy. Can only be used during the player's turn in combat.",
      "allowed_contexts": ["combat_player_turn"],
      "effects": [
        { "type": "deal_damage", "value": 20, "target": "selected_enemy" }
      ]
    }
    ```

    

  Potion effects are uniformly executed by `EffectResolver`. Potion inventories are stored as counts in the save file.

### 1.7 Initial Player Character

#### Ironclad

Strictly follows the *Slay the Spire* initial setup, with room for future character expansion.

**Character JSON Configuration** (`characters/ironclad.json`):

json

```
{
  "id": "ironclad",
  "name": "Ironclad",
  "description": "A melee fighter who excels at trading life for strength.",
  "base_stats": {
    "max_hp": 80,
    "current_hp": 80,
    "gold": 99
  },
  "initial_deck": [
    { "card_id": "strike", "count": 5 },
    { "card_id": "defend", "count": 4 },
    { "card_id": "bash", "count": 1 }
  ],
  "initial_relics": ["burning_blood"],
  "initial_potions": []
}
```



**Character Factory**:

cpp

```
class CharacterFactory {
public:
    static void registerCharacter(const QString& jsonPath);
    static std::optional<CharacterData> create(const QString& id);
private:
    static QHash<QString, CharacterData> s_database;
};
```



#### Relic "Burning Blood"

json

```
{
  "id": "burning_blood",
  "name": "Burning Blood",
  "description": "At the end of combat, heal 6 HP.",
  "trigger": "on_combat_end",
  "effects": [
    { "type": "restore_hp", "value": 6 }
  ],
  "max_stack": 1,
  "character_exclusive": "ironclad"
}
```



Requires adding the trigger `OnCombatEnd`, which emits an EventBus signal after combat victory or defeat.

### 1.8 Relic Trigger Architecture (EventBus Plan A)

All relics trigger by listening to `EventBus` signals. `BattleManager` registers connections based on the player's relics at combat initialization.

Example "Old Shield" relic:

json

```
{
  "id": "old_shield",
  "name": "Old Shield",
  "description": "On the first turn of combat, gain 10 Block.",
  "trigger": "on_battle_start",
  "effects": [
    { "type": "gain_block", "value": 10 }
  ],
  "max_stack": 1
}
```



Connection code:

cpp

```
connect(&EventBus::instance(), &EventBus::battleStarted,
        relic.data(), [relic, player]() {
    EffectResolver::instance().execute(relic->effects(), player, player);
}, Qt::SingleShotConnection);
```



------

## 2. Variable Definitions

### 2.1 Map and Shrink (`MapManager`)

cpp

```
struct MapNode {
    int x, y;
    NodeType type;   // Boss, Shop, Campfire, Elite, NormalEnemy, Event, Empty
    bool explored = false;
    // Pointer or ID to specific data
};

std::array<std::array<MapNode, 11>, 11> grid;
int playerX, playerY;
int totalSteps = 0;
int currentShrinkCount = 0;  // 0~5, also the shrink damage coefficient n
int nextShrinkStep = 12;
```



### 2.2 Combat Entity Attributes

cpp

```
struct ActorStats {
    int hp = 0, maxHp = 0;
    int block = 0;
    int strength = 0, dexterity = 0;
    std::vector<std::unique_ptr<Power>> powers;
};
```



### 2.3 Card Data

cpp

```
struct CardData {
    QString id;
    int baseCost;
    CardType type;      // Attack, Skill, Power
    CardRarity rarity;
    TargetType target;  // Enemy, AllEnemies, Self
    std::vector<EffectDef> effects;
};
```



### 2.4 Effect Definition

`EffectDef` is a general effect structure containing `type` (string), `value` (int), and optional `target`. All effects are executed by `EffectResolver`.

------

## 3. Project Directory Structure and Architecture Patterns

### 3.1 Directory Structure

text

```
CardRoguelike/
├── CardRoguelike.pro
├── corelib/
│   ├── corelib.pro
│   ├── battle/
│   │   ├── entities/       (Actor, Player, Enemy, Card, Relic, Power...)
│   │   ├── logic/          (BattleFSM, EffectResolver, AIController...)
│   │   └── data/           (CardDatabase, RelicDatabase, EnemyDatabase, CharacterFactory...)
│   ├── map/                (MapManager, MapGenerator, NodeType)
│   ├── events/             (EventManager, EventDatabase)
│   ├── potions/            (PotionDatabase)
│   ├── save/               (SaveManager)
│   ├── systems/            (EventBus, ICommand)
│   └── utils/              (RNG)
├── app/
│   ├── app.pro
│   ├── views/              (MainWindow, MapView, BattleView, CardWidget...)
│   ├── viewmodels/         (MapViewModel, BattleViewModel)
│   └── resources/          (qss, images...)
└── tests/
    ├── tests.pro
    └── unit/               (TestMechanics...)
```



### 3.2 Architecture Patterns

- **MVVM variant**: ViewModels expose `Q_PROPERTY` and notification signals to refresh Views.
- **Command pattern**: Card effects and map interactions implement `ICommand`.
- **Event bus**: `EventBus` singleton provides global signals for decoupling relics, status effects, and UI.

------

## 4. Error Handling and Return Specifications

- Movement legality: `MapManager::canMoveTo(x, y)` returns `bool`.
- Card play validation: `CardPlayer::canPlay(card, target)` returns `PlayResult{bool, QString}`.
- Numerical safety: HP, Block, etc. modifications use `qBound(min, val, max)`.
- Save file resilience: JSON saves contain no checksum; on read failure, show `QMessageBox::warning` and return to main menu.
- Defensive programming: Use `QPointer` or null checks where pointers may be null; nullable returns use `std::optional`.
- Logging: `qDebug()` / `qWarning()` / `qCritical()`; only `Critical` kept in release builds.

------

## 5. Technology Stack and Locked Core Libraries

- Language: C++17 (`std::optional`, `std::variant`, `if constexpr`, structured bindings)
- Compiler: MinGW-w64 64-bit, GCC 8.1.0+
- UI Framework: Qt 5.14.2 Widgets, QML disabled, use `QGraphicsView/QGraphicsScene`
- Serialization: `QJsonDocument` / `QJsonObject`
- Unit Testing: Qt Test framework
- Build System: qmake, `CONFIG += c++17`

------

## 6. Authentication and Security (Purely Offline)

- No network functionality.
- Saves are plain JSON, no encryption, no checksum.
- Debug detection is optional and does not affect normal gameplay.

------

## 7. Testing Strategy Boundaries

### Unit Tests (pure corelib, Qt Test)

- Shrink counter logic: step increase, cross-threshold shrink, damage coefficient
- Shrink damage logic: immediate damage and combat buff per-turn damage
- Explored node step cost and damage rules
- Damage calculation: block mitigation, strength influence
- AI intent generation
- Map generation legality
- Save serialization

### Integration Tests

- Simulate full combat flow, verify real-time shrink buff update
- Map node interaction: move from start, trigger shrink, check explored node behavior

### Not Tested

- UI rendering, animations, audio
- Resource file loading

------

## 8. Specific Syntactic Sugar and Boilerplate Code

### 8.1 Fluent Interface for Building Cards

cpp

```
Card strike = CardBuilder("Strike", CardType::Attack)
    .setCost(1)
    .setTarget(TargetType::SingleEnemy)
    .addEffect(EffectDef::damage(6))
    .build();
```



### 8.2 Event Bus Connection Macro

cpp

```
#define RELIC_TRIGGER(EventSignal, Method) \
    connect(&EventBus::instance(), &EventBus::EventSignal, this, &ThisClass::Method)
```



### 8.3 Simplified Property Binding

cpp

```
#define BIND_PROPERTY(view, setter, vmGetter, signal) \
    QObject::connect(vm, &ViewModel::signal, view, [vm, view](){ \
        view->setter(vm->vmGetter()); \
    });
```



### 8.4 JSON Safe Read Template

cpp

```
template<typename T>
std::optional<T> jsonValue(const QJsonObject& obj, const QString& key) {
    if (!obj.contains(key)) return std::nullopt;
    QJsonValue v = obj[key];
    if constexpr (std::is_same_v<T, int>) return v.toInt();
    else if constexpr (std::is_same_v<T, QString>) return v.toString();
    return std::nullopt;
}
```



### 8.5 Enum-String Conversion

Use `Q_NAMESPACE`, `Q_ENUM_NS`, and `QMetaEnum` to auto-generate mappings.

------

## 9. UI Component Style and Layout

### 9.1 Map View (MapView)

- `QGraphicsView/QGraphicsScene` draws a light gray grid.
- Node icons: empty node floor, shop coin bag, event question mark, campfire flame, monster silhouette, Boss large icon.
- Player position highlighted with a glow; movable neighbors blink as hints.
- Shrink zone overlay: semi-transparent red/purple mask that deepens with shrink layers (visual only, no popup).
- Step info displayed in a corner.

### 9.2 Battle View (BattleView)

- Hand cards arc at the bottom, enlarge and lift on selection.
- Card rendering: rounded rectangle, cost orb, name, description, rarity border color.
- Character status: HP bar and Block custom `QProgressBar`, buff icons below characters with stack numbers.
- Enemy intent: icon and value displayed overhead.
- Energy orb: current energy shown in center bottom.

### 9.3 Global Style

- Dark theme QSS file for consistent fonts and button style.

------

## 10. Concrete Unit Test Code

The following tests cover shrink counter, explored nodes, combat shrink buff with block clearing, and death by shrink, using Qt Test.

cpp

```
#include <QtTest>
#include "map/mapmanager.h"
#include "battle/logic/damagecalculator.h"

class TestGameMechanics : public QObject {
    Q_OBJECT

private slots:
    void testStepCounterAndShrink() {
        MapManager map;
        map.generateMap(12345);
        QCOMPARE(map.totalSteps(), 0);
        QCOMPARE(map.nextShrinkStep(), 12);
        QCOMPARE(map.currentShrinkCount(), 0);

        // Empty node +1
        map.addSteps(1); QCOMPARE(map.totalSteps(), 1);
        map.addSteps(1); QCOMPARE(map.totalSteps(), 2);
        map.addSteps(1); QCOMPARE(map.totalSteps(), 3);

        // Non-empty node +3, cumulative to 6
        map.addSteps(3); QCOMPARE(map.totalSteps(), 6);
        map.addSteps(3); QCOMPARE(map.totalSteps(), 9);
        map.addSteps(3); QCOMPARE(map.totalSteps(), 12);
        QCOMPARE(map.currentShrinkCount(), 1);
        QCOMPARE(map.nextShrinkStep(), 24);
    }

    void testExploredNodeTreatedAsEmpty() {
        MapManager map;
        map.generateMap(100);
        // Set a node as explored, originally NormalEnemy
        map.setNodeExplored(3, 4);
        int prevSteps = map.totalSteps();
        // Enter explored node, step cost +1
        map.enterNode(3, 4);
        QCOMPARE(map.totalSteps(), prevSteps + 1);
        // Shrink damage calculated as Empty node
        QCOMPARE(map.shrinkDamageForNode(3, 4), MapManager::shrinkDamage(NodeType::Empty, map.currentShrinkCount()));
    }

    void testBattleShrinkBuffWithBlockClearing() {
        Actor player;
        player.setMaxHp(80);
        player.healToFull();
        int shrinkCount = 2; // n=2

        // Player receives undispellable ShrinkDamagePower
        auto shrinkBuff = new ShrinkDamagePower(&player, shrinkCount);
        player.addPower(shrinkBuff);

        // Simulate turn start (triggers all Power onTurnStart)
        player.beginTurn();
        QCOMPARE(player.currentHp(), 78); // 80-2, block ignored

        // Player gains block
        player.gainBlock(10);
        QCOMPARE(player.currentBlock(), 10);

        // End turn: block cleared
        player.endTurn();
        QCOMPARE(player.currentBlock(), 0);

        // Next turn start, damage again
        player.beginTurn();
        QCOMPARE(player.currentHp(), 76); // 78-2, block already cleared
    }

    void testDeathByShrinkDamage() {
        Actor player;
        player.setMaxHp(1);
        player.healToFull();
        MapManager map;
        map.setCurrentShrinkCount(5); // n=5

        // Entering shrink empty node causes instant death
        QVERIFY(player.currentHp() == 1);
        map.applyShrinkDamageOnEnter(player, NodeType::Empty);
        QVERIFY(player.currentHp() <= 0);
        QVERIFY(map.isPlayerDead() == true);
    }
};

QTEST_APPLESS_MAIN(TestGameMechanics)
#include "test_mechanics.moc"
```



**Note**: The `Actor` class must provide `beginTurn()` (triggering all Power on-turn-start effects) and `endTurn()` (clearing block and triggering on-turn-end effects). The shrink buff implements damage in `onTurnStart`.