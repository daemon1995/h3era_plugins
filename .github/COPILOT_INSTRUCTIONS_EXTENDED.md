# Copilot Extended Instructions (RU) — h3era_plugins

Этот документ предназначен для настройки поведения GitHub Copilot (Ask / Edits / Chat / Agent) при работе с монорепозиторием плагинов Heroes of Might & Magic III ERA. Основные цели:
- Давать точные и контекстные подсказки по API Heroes III.
- Использовать справочную карту структур из каталога `headers/H3API_for_copilot`.
- Исключить генерацию кода, напрямую включающего файлы из `H3API_for_copilot`.
- Ускорить навигацию по внутренним типам, менеджерам и объектам карты.

> ВНИМАНИЕ: Каталог `headers/H3API_for_copilot` — ТОЛЬКО источник справочной информации для Copilot. Код плагинов должен включать лишь `framework.h` → `../headers/header.h`. Не предлагать пользователю добавлять прямые include на файлы `H3API_for_copilot`.

## 1. Базовые правила для Copilot
1. Всегда предлагай минималистичные include: только `#include "framework.h"` в *.cpp* файлах плагина.
2. Не дублируй `era.cpp` или внутренние реализации API — использовать общий механизм через solution.
3. При генерации примеров структур/методов указывай, что имена полей могут отличаться и нужно свериться с реальным заголовком в `headers/`.
4. Предлагай логирование / патчинг только через уже существующие объекты `Patcher`, `PatcherInstance`.
5. Если пользователь просит доступ к объекту игрового мира — сначала определи соответствующий модуль (Adventure Map, Managers, RMG, Towns, Skills и т.д.).
6. Не предлагать создавать собственные контейнеры вместо `H3Map<T>`, `H3FastMap<T>` — рекомендовать использование встроенных.

## 2. Шаблон нового плагина (для автогенерации)
```
MyNewPlugin/
  MyNewPlugin.vcxproj
  framework.h
  dllmain.cpp
  (опционально) FeatureLogic.{h,cpp}
```
`framework.h`:
```cpp
#pragma once
#define WIN32_LEAN_AND_MEAN
#include "../headers/header.h"
```
`dllmain.cpp` (минимум):
```cpp
#include "framework.h"
Patcher* globalPatcher = nullptr;
PatcherInstance* _PI = nullptr;
namespace dllText {
    constexpr char InstanceName[]    = "EraPlugin.MyNewPlugin.daemon_n";
    constexpr char PluginAuthor[]    = "daemon_n";
    constexpr char PluginVersion[]   = "1.0.0";
    constexpr char PluginDate[]      = __DATE__;
}
void StartPlugin() {
    globalPatcher = GetPatcher();
    _PI = globalPatcher->CreateInstance(dllText::InstanceName);
    Era::ConnectEra(GetModuleHandleA(nullptr), dllText::InstanceName);
    // TODO: хуки
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) StartPlugin();
    return TRUE;
}
```

## 3. Карта модулей API и их область применения
Ниже приведена расширенная систематизация файлов в `headers/H3API_for_copilot`. При формировании подсказок Copilot должен опираться на эти группы и предлагать корректный модуль.

### 3.1 Adventure Map (объекты приключенческой карты)
Каждый файл описывает структуру конкретного объекта или вспомогательные атрибуты. Применение: сканирование, модификация, логика появления/взаимодействия.
Список файлов (по алфавиту — повторения означают симметричный include в разных индексных проходах):
`H3GlobalObjectSettings.hpp`, `H3MapArtifact.hpp`, `H3MapBlackMarket.hpp`, `H3MapBoat.hpp`, `H3MapCampfire.hpp`, `H3MapCorpse.hpp`, `H3MapCreatureBank.hpp`, `H3MapEvent.hpp`, `H3MapFlotsam.hpp`, `H3MapFountainFortune.hpp`, `H3MapGarrison.hpp`, `H3MapGenerator.hpp`, `H3MapHero.hpp`, `H3MapItems.hpp`, `H3MapLeanTo.hpp`, `H3MapLearningStone.hpp`, `H3MapLighthouse.hpp`, `H3MapMagicShrine.hpp`, `H3MapMagicSpring.hpp`, `H3MapMine.hpp`, `H3MapMonolith.hpp`, `H3MapMonster.hpp`, `H3MapMysticGarden.hpp`, `H3MapObelisk.hpp`, `H3MapOceanBottle.hpp`, `H3MapPandorasBox.hpp`, `H3MapPrison.hpp`, `H3MapPyramid.hpp`, `H3MapQuestGuard.hpp`, `H3MapRefugeeCamp.hpp`, `H3MapResource.hpp`, `H3MapScholar.hpp`, `H3MapScroll.hpp`, `H3MapSeaChest.hpp`, `H3MapSeerHut.hpp`, `H3MapShipwreckSurvivor.hpp`, `H3MapShipyard.hpp`, `H3MapSign.hpp`, `H3MapTown.hpp`, `H3MapTreasureChest.hpp`, `H3MapTreeOfKnowledge.hpp`, `H3MapUniversity.hpp`, `H3MapWagon.hpp`, `H3MapWarriorsTomb.hpp`, `H3MapWaterMill.hpp`, `H3MapWindmill.hpp`, `H3MapWitchHut.hpp`, `H3ObjectAttributes.hpp`, `H3ObjectDetails.hpp`, `H3TileVision.hpp`.
Ключевые группы применения:
- Ресурсы / награды: Campfire, TreasureChest, Resource, SeaChest.
- Обучение / развитие: University, TreeOfKnowledge, LearningStone, WitchHut.
- Банки существ / охрана: CreatureBank, Monster, WarriorsTomb, Pyramid.
- Спец. сервисы: BlackMarket, SeerHut, MagicShrine, Shipyard, Lighthouse.
- Перемещение / взаимодействие: Boat, Monolith, Obelisk (квесты/навигация), QuestGuard.

### 3.2 RMG (Random Map Generator)
Генерация зон, шаблонов и объектов.
Файлы: `H3RmgTemplate.hpp`, `H3RmgRandomMapGenerator.hpp`, `H3RmgParameters.hpp`, `H3RmgObjectTown.hpp`, `H3RmgObjectSeer.hpp`, `H3RmgObjectProperties.hpp`, `H3RmgZone.hpp`, `H3RmgObjectMonster.hpp`, `H3RmgObject.hpp`, `H3RmgMovementCost.hpp`, `H3RmgMapItem.hpp`, `H3RmgMapInfo.hpp`, `H3RmgMap.hpp`, `H3RmgLocalMap.hpp`, `H3RmgGroundTileData.hpp`, `H3RmgGroundTile.hpp`, `H3RmgZoneGenerator.hpp`, `H3RmgZoneData.hpp`, `H3RmgZoneConnection.hpp`.
Использование:
- Тонкая настройка связей зон (ZoneConnection).
- Изменение стоимости перемещения (MovementCost / GroundTile).
- Подмена параметров шаблона (Parameters, Template).
- Кастомизация генерации объектов (Object*, ObjectProperties).

### 3.3 Skills (Primary & Secondary)
Файлы: `H3PrimarySkills.hpp`, `H3SecondarySkill.hpp`, `H3Diplomacy.hpp`.
Использование: изменение вычисления статов героя, нейтральных присоединений (Diplomacy), динамическая модификация вторичных навыков.

### 3.4 Towns (Города, экономика, события)
Файлы: `H3Town.hpp`, `H3SetupTown.hpp`, `H3TownCreatureTypes.hpp`, `H3TownDependencies.hpp`, `H3BuildingCosts.hpp`, `H3CastleEvent.hpp`, `H3OraclePuzzle.hpp`.
Применение:
- Экономика и стоимость построек (BuildingCosts / Dependencies).
- Состав существ (CreatureTypes).
- События города (CastleEvent).
- Пазл (OraclePuzzle).
- Инициализация городов при старте (SetupTown).

### 3.5 Artifacts
Агрегация: `H3Artifacts.hpp` включает `H3Artifact.hpp`, `H3ArtifactSetup.hpp`, `H3CombinationArtifacts.hpp`.
Применение: создание / проверка артефактов, бонусы комбинированных наборов.

### 3.6 Game Data
Агрегация: `H3GameData.hpp` — глобальные элементы состояния кампании / сценария.
Ключевые типы (кратко): `H3Main`, `H3MainSetup`, `H3MapInfo`, `H3ScenarioMapInformation`, `H3Resources`, `H3Spell`, `H3GlobalEvent`, `H3LossCondition`, `H3VictoryCondition`, `H3TurnTimer`, `H3GarrisonInterface`, `H3RandomDwelling`, `H3Network`.
Применение: чтение сценарных условий, экономических показателей, тайминга и сетевых атрибутов.

### 3.7 Managers
Агрегация: `H3Managers.hpp` — высокоуровневые менеджеры.
Типы: `H3AdventureManager`, `H3CombatManager`, `H3ExecutiveMgr`, `H3HiScoreManager`, `H3InputManager`, `H3MouseManager`, `H3RecruitManager`, `H3SoundManager`, `H3SwapManager`, `H3TownManager`, `H3WindowManager`.
Применение: внедрение логики в игровой цикл, UI, бой, рекрутинг.

### 3.8 Base / Containers / Core
(По образцу найденных символов)
- `H3Version` (версионные макросы — см. `H3Version.hpp`).
- Шаблонные контейнеры: `H3Map<T>`, `H3FastMap<T>` (в `H3Base/H3Core.hpp`).
- Аллокаторы: `H3AllocatorReference` (`H3Containers/H3Vector.hpp`).
Использование: безопасное хранение и быстрый доступ к игровым объектам.

### 3.9 Dialogs & UI
Файлы (выборочно): `H3DlgHintBar.hpp`, `H3DlgHighlightable.hpp` + другие элементы из `H3DialogControls/*` и агрегатор `H3Dialogs.hpp`.
Применение: расширение интерфейса, подсветка, вывод подсказок.

### 3.10 Utilities
`H3Utilities.hpp` — вспомогательные функции (строки, лог, ресурсные операции). При генерации кода предлагать использование утилит вместо самописных дублирующих решений.

### 3.11 Version
`H3Version.hpp` содержит макросы:
```
_H3API_VERSION_MAJOR_
_H3API_VERSION_MINOR_
_H3API_VERSION_BUILD_
_H3API_VERSION_
_H3API_VERSION_TEXT_
```
Использование: условная компиляция или лог предупреждений при несовместимости.

### 3.12 Combat & Spells (Бой и заклинания)
Модули боя и связанных с ними заклинаний делятся на:
- Низкоуровневые структуры поля боя: `H3CombatSquare.hpp`, `H3AdjacentSquares.hpp` — описание координат и соседства.
- Существа в бою: `H3CombatCreature.hpp` — состояние стека (количество, здоровье, эффекты, инициатива и пр.).
- Препятствия: `H3Obstacle.hpp`, `H3ObstacleInfo.hpp` — данные временных и постоянных объектов (стены, камни, фортификации).
- Секции стен: `H3WallSection.hpp` — состояние участков крепостной стены (целостность, разрушение катапультой).
- Цели катапульты: `H3ValidCatapultTargets.hpp` — набор валидных клеток/секций для атаки.
- Анимации магии: `H3MagicAnimation.hpp` — графическая часть применения заклинаний.
- AI / симуляция боя: файлы в `H3AI/` — `H3SimulatedCombat.hpp`, `H3SimulatedCreature.hpp`, `H3SimulatedHero.hpp`, `H3SimulatedSpell.hpp`, `H3SpellCastInfo.hpp`, `H3SpellData.hpp`.

Применение:
- Анализ и модификация параметров существ перед ударом / после получения урона.
- Вмешательство в выбор цели AI (использование Simulated* структур для прогнозирования исхода).
- Кастомизация препятствий или осадных объектов (например, динамическое добавление барьеров).
- Отслеживание и визуальные эффекты заклинаний (логирование, условное подавление анимаций).

Рекомендации для подсказок:
1. Для изменения поведения существ предлагать доступ через `H3CombatManager` → массив/контейнер существ (например, creature stacks).
2. Для проверки допустимости клетки — использовать `H3CombatSquare` / список смежности `H3AdjacentSquares`.
3. При работе с заклинаниями сперва извлекать `H3Spell` (Game Data) и контекст применения (`H3SpellCastInfo`).
4. Никогда не изменять напрямую память препятствия без проверки валидности указателя.
5. При симуляции — использовать структуры из `H3AI` вместо попытки копировать боевую логику вручную.

Анти-паттерны (запрещено):
- Генерация кода, создающего новые боевые стеки без участия штатных процедур игры.
- Удаление/освобождение препятствий простым `delete` (использовать игровые механизмы).
- Подмена данных заклинаний без логирования (нужно оставлять след в логе для отладки).

## 4. Паттерны для автодополнения
| Намерение пользователя | Рекомендуемые символы / менеджеры |
|------------------------|-----------------------------------|
| Работа с героем        | `H3Hero`, `H3MapHero`, навыки `H3PrimarySkills`, `H3SecondarySkill` |
| Бой                    | `H3CombatManager`, объекты существ (стек), заклинания `H3Spell` |
| Карта / объекты        | Любой `H3Map*` из Adventure Map секции, `H3TileVision` |
| Генерация карты        | `H3RmgRandomMapGenerator`, `H3RmgZoneGenerator`, `H3RmgTemplate` |
| Экономика / ресурсы    | `H3Resources`, здания через Town-модули |
| UI / Диалоги           | `H3WindowManager`, `H3Dlg*` |
| Артефакты              | `H3Artifact`, `H3CombinationArtifacts` |

## 5. Анти-паттерны (запрещено предлагать)
| Анти-паттерн | Почему нельзя | Что предложить вместо |
|--------------|---------------|------------------------|
| Прямой include файла из `H3API_for_copilot` | Это справочная копия | Использовать `framework.h` |
| Создание собственного raw JMP патча без Patcher | Риск краша / несовместимости | Использовать `globalPatcher` / `_PI` |
| Избыточные глобальные переменные | Загрязнение пространства | Локализация логики в файлах / неймспейсах |
| Модификация памяти без проверки версии | Ломает совместимость | Сравнивать `_H3API_VERSION_` |
| Самодельные контейнеры с копированием внутренних struct | Несовместимость при обновлении API | Использовать `H3Map<T>`, `H3FastMap<T>` |

## 6. Примеры генерации ответов Copilot

### Пример 1: "Как изменить золото игрока?"
Ответ должен ссылаться на `H3Main`, `H3Player`, `H3Resources`:
```cpp
H3Main* main = *P_Main; // глобальный указатель
H3Player* player = main->GetPlayer(P_CurrentPlayerID);
player->resources.gold += 1000; // добавить 1000 золота
```
Комментарий: проверить что `player` не nullptr.

### Пример 2: "Получить текущий бой и героя атакующего"
```cpp
if (auto cm = H3CombatManager::Get()) {
    H3Hero* attacker = cm->hero[0];
    if (attacker) {
        // работа с навыками
    }
}
```

### Пример 3: "Сканировать объекты рядом с героем"
Указать использование `H3MapHero` + обращения к координатам и `H3TileVision`.

## 7. Проверки перед выдачей ответа
Copilot должен ментально пройти чек:
- Используются ли только допустимые include? (Да: `framework.h`)
- Нет ли ссылок на файлы `H3API_for_copilot`? (Должно быть НЕТ)
- Используются правильные модули по назначению? (Напр., бой → CombatManager)
- Есть ли оговорка о возможной разнице в полях? (При доступе к внутренним структурам желательно)

## 8. Краткий глоссарий ключевых сущностей
| Символ | Смысл |
|--------|-------|
| `H3Main` | Центральное состояние карты / сценария |
| `H3Hero` | Герой: навыки, инвентарь, параметры |
| `H3Town` | Город: гарнизон, постройки, экономика |
| `H3CombatManager` | Контекст текущей битвы |
| `H3RmgRandomMapGenerator` | Генератор случайной карты |
| `H3Artifact` | Артефакт и его характеристики |
| `H3Resources` | Ресурсы игрока |
| `H3WindowManager` | Управление окнами/диалогами |
| `H3Dlg*` | UI контролы |
| `H3Map*` | Объекты карты (префикс) |

## 9. Дополнительные рекомендации для Copilot
- При сомнении: предложи пользователю найти точное поле в соответствующем заголове.
- Для перехвата поведения всегда предлагай патч через предоставленный патчер (не вручную ASM).
- Сохраняй ответы короткими в коде и богатыми комментариями там, где есть риск misuse.

## 10. Напоминание
Каталог `headers/H3API_for_copilot` НЕ включается в будущие плагины. Любая подсказка Copilot, нарушающая это правило, должна быть скорректирована.

---
Эти инструкции могут расширяться. При добавлении новых модулей API — обновить карту в разделе 3.
