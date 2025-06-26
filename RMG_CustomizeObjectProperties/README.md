# RMG_CustomizeObjectProperties

Плагин для настройки свойств объектов в генераторе случайных карт Heroes of Might and Magic 3 ERA.

## Быстрая настройка для разработки

### 🚀 **Начальная настройка**

1. **Настройте пути в корне репозитория**:
   ```bash
   cd ".."
   copy Directory.Build.props.template Directory.Build.props.user
   # Отредактируйте Directory.Build.props.user под ваши пути к игре
   ```

2. **Откройте проект в Visual Studio 2022**
3. **Настройки отладки уже настроены** в `RMG_CustomizeObjectProperties.vcxproj.user`

### 🔧 **Отладка плагина**

1. **Установите брейкпоинты** в нужных местах кода
2. **Нажмите F5** или **Debug → Start Debugging**
3. **Игра запустится автоматически** с подключенным отладчиком
4. **Плагин загрузится** автоматически при старте игры

### 📁 **Файлы настроек (НЕ в Git)**

- `../Directory.Build.props.user` - ваши пути к игре и инструментам
- `RMG_CustomizeObjectProperties.vcxproj.user` - настройки отладки VS

### 🎯 **Структура проекта**

Этот проект расширяет функциональность объектов карты:
- **Заклинательные рынки** (Spell Markets) - `SpellMarketExtender.cpp`
- **Университеты** (Universities) - `UniversityExtender.cpp`
- **Колизей магов** (Colosseum of the Magi) - `ColosseumOfTheMagiExtender.cpp`
- **Банки существ** (Creature Banks) - `CreatureBanksExtender.cpp`
- **Беседки** (Gazebos) - `GazeboExtender.cpp`
- **Святилища** (Shrines) - `ShrinesExternder.cpp`
- **Склады** (Warehouses) - `WarehousesExtender.cpp`
- **Водопои** (Watering Places) - `WateringPlaceExtender.cpp`
- **Объекты WoG** - `WoGObjectsExtender.cpp`

### 🔨 **Сборка**

Проект использует общие настройки из корневого `Directory.Build.props` и автоматически:
- Компилируется с расширением `.era`
- Копируется в папку плагинов игры: `$(LocalGamePath)\Mods\WoG\eraplugins\`
- Создает map-файлы для отладки: `$(LocalToolsPath)\ExeMapCompiler\`

### 📖 **Дополнительная информация**

- Все настройки проекта наследуются из `../Directory.Build.props`
- Пользовательские настройки в `../Directory.Build.props.user`
- Подробнее о настройке VS: `../VS_USER_SETTINGS.md`