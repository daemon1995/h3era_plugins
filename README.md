# Heroes of Might and Magic 3 ERA Plugins

**[🇷🇺 Русский](#russian)** | **[🇺🇸 English](#english)** | **[📄 English (separate file)](README.en.md)**

---

## <a id="russian"></a>🇷🇺 Русская версия

Монорепозиторий плагинов для Heroes of Might and Magic 3 ERA.

> **ℹ️ Для автоформатирования кода рекомендуется использовать расширение [clang-tools](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) для VS Code или аналог для вашей IDE. Форматирование кода производится автоматически по .clang-format.**

## Архитектура репозитория

Этот репозиторий использует **монорепо** структуру по следующим причинам:

### 🎯 **Почему монорепо?**
- **Общие заголовочные файлы**: все плагины используют большие header файлы с API игры (папка `headers/`)
- **Единая экосистема**: все плагины предназначены для одной игры - HoMM3 ERA
- **Общие настройки сборки**: унифицированная конфигурация для всех проектов
- **Синхронизация изменений**: изменения в API сразу отражаются на всех плагинах
- **Упрощенное управление зависимостями**: один набор библиотек и инструментов

### 📁 **Структура репозитория**
```
├── headers/                    # Общие заголовочные файлы API игры
├── Directory.Build.props       # Общие настройки сборки для всех проектов
├── Directory.Build.props.user  # Локальные настройки разработчика (не в Git)
├── H3EraPlugins.sln           # Общее решение для всех проектов
└── [Плагины]/
    ├── RMG_CustomizeObjectProperties/
    ├── Artifacts_AddingNewFeatures/
    ├── Combat_SwitchCreatureAction/
    ├── ERA_MultilingualSupport/
    └── ...
```

## Совместимость

Все проекты настроены для работы с **Visual Studio 2022 Community** и используют:
- Platform Toolset: v143 (по умолчанию для VS 2022)
- Windows SDK: 10.0 (автоматическое определение последней версии)
- VCProjectVersion: 17.0 (VS 2022)

## Настройка локального окружения

Все проекты используют общие файлы настроек для хранения путей компиляции и отладки.

### Первоначальная настройка

1. Скопируйте файл `Directory.Build.props.template` как `Directory.Build.props.user`
2. Отредактируйте пути под ваше локальное окружение в `Directory.Build.props.user`:

```xml
<PropertyGroup>
  <LocalGamePath>C:\YourPath\HoMM 3 ERA</LocalGamePath>
  <LocalGameTestPath>C:\YourPath\HoMM 3 ERA test</LocalGameTestPath>
  <LocalToolsPath>C:\YourPath\Tools</LocalToolsPath>
  <LocalHeadersPath>C:\YourPath\headers</LocalHeadersPath>
  
  <!-- Для VS 2022 по умолчанию используется v143 -->
  <PlatformToolset>v143</PlatformToolset>
  <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
</PropertyGroup>
```

### Структура настроек

- `Directory.Build.props` - общие настройки для всех проектов (включается в Git)
- `Directory.Build.props.user` - пользовательские настройки путей (исключается из Git)
- `Directory.Build.props.template` - шаблон для пользовательских настроек

### Плагины в репозитории

- **RMG_CustomizeObjectProperties** - настройка свойств объектов для генератора карт
- **Artifacts_AddingNewFeatures** - расширение функциональности артефактов  
- **Combat_SwitchCreatureAction** - переключение действий существ в бою
- **ERA_MultilingualSupport** - поддержка многоязычности
- **HeroDlg_AlignClassName** - выравнивание названий классов героев
- **GameplayEnhancementsPlugin** - улучшения геймплея
- **Graphics_TimeZonesSwitching** - переключение часовых поясов графики
- **HeroArtifactsDlg** - диалог артефактов героя
- **NewAdvMapItems** - новые предметы карты приключений
- **NewCombatDlgItems** - новые элементы диалога боя
- **NewMainMenuItems** - новые элементы главного меню
- И другие...

### 💡 **Преимущества монорепо для игровых плагинов**

1. **Единая версия API**: все плагины всегда совместимы друг с другом
2. **Атомарные изменения**: можно изменить API и все зависимые плагины одним коммитом
3. **Переиспользование кода**: общие утилиты и хелперы доступны всем плагинам
4. **Упрощенная сборка**: один клик собирает все плагины сразу
5. **Общее тестирование**: можно тестировать взаимодействие между плагинами

### Конфигурации сборки

- **Debug|Win32** - отладочная сборка для Win32
- **Release|Win32** - релизная сборка для Win32 
- **Debug|x64** - отладочная сборка для x64
- **Release|x64** - релизная сборка для x64

### Поддержка старых систем

Если нужна совместимость со старыми версиями Windows, можно использовать:
```xml
<PlatformToolset>v141_xp</PlatformToolset>
```

## Начало работы

### 🚀 **Быстрый старт**

1. **Клонируйте репозиторий**:
   ```bash
   git clone <repository-url>
   cd "plugins/daemon_n's"
   ```

2. **Настройте локальное окружение**:
   ```bash
   copy Directory.Build.props.template Directory.Build.props.user
   # Отредактируйте Directory.Build.props.user под ваши пути
   ```
   📖 **Подробнее**: [VS_USER_SETTINGS.md](VS_USER_SETTINGS.md)

3. **Откройте в Visual Studio 2022**:
   ```bash
   start H3EraPlugins.sln
   ```

4. **Соберите все плагины**:
   - `Build -> Build Solution` (Ctrl+Shift+B)

### 🔧 **Разработка нового плагина**

1. Создайте новую папку для плагина
2. Добавьте проект в `H3EraPlugins.sln` (📖 [подробнее](SOLUTION_BEST_PRACTICES.md))
3. Настройки сборки подхватятся автоматически из `Directory.Build.props`
4. Используйте общие заголовки из папки `headers/`

Все плагины будут автоматически скопированы в соответствующие папки игры согласно настройкам в `Directory.Build.props.user`.

### 🚀 **Работа с Visual Studio Code**

1. **Настройте окружение**:
   - Убедитесь, что установлен Visual Studio Code.
   - Установите расширение [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools).

2. **Откройте репозиторий в VS Code**:
   ```bash
   code .
   ```

3. **Сборка проекта**:
   - Откройте меню "Terminal -> Run Task".
   - Выберите задачу "Build Solution" для сборки всех плагинов.

4. **Отладка**:
   - Откройте меню "Run -> Start Debugging" (F5).
   - Убедитесь, что выбрана конфигурация "Launch Plugin".

5. **Редактирование кода**:
   - Используйте общие заголовочные файлы из папки `headers/`.
   - Форматируйте код с помощью `.clang-format`.

📖 **Подробнее**: [CONTRIBUTING.md](CONTRIBUTING.md)

## 📚 **Дополнительная информация**

- **Формат плагинов**: `.era` файлы (DLL с переименованным расширением)
- **API игры**: заголовочные файлы в папке `headers/`
- **Система сборки**: MSBuild с общими настройками
- **Совместимость**: Heroes of Might and Magic 3 ERA
- **Для разработчиков**: см. [CONTRIBUTING.md](CONTRIBUTING.md)

---

## <a id="english"></a>🇺🇸 English Version

Monorepo of plugins for Heroes of Might and Magic 3 ERA.

> **ℹ️ For automatic code formatting, it is recommended to use the [clang-tools](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) extension for VS Code or a similar tool for your IDE. Code is formatted automatically according to .clang-format.**

## Repository Architecture

This repository uses **monorepo** structure for the following reasons:

### 🎯 **Why Monorepo?**
- **Shared header files**: all plugins use large header files with game API (folder `headers/`)
- **Unified ecosystem**: all plugins are designed for one game - HoMM3 ERA
- **Common build settings**: unified configuration for all projects
- **Synchronized changes**: API changes are immediately reflected in all plugins
- **Simplified dependency management**: one set of libraries and tools

### 📁 **Repository Structure**
```
├── headers/                    # Common game API header files
├── Directory.Build.props       # Common build settings for all projects
├── Directory.Build.props.user  # Local developer settings (not in Git)
├── H3EraPlugins.sln           # Common solution for all projects
└── [Plugins]/
    ├── RMG_CustomizeObjectProperties/
    ├── Artifacts_AddingNewFeatures/
    ├── Combat_SwitchCreatureAction/
    ├── ERA_MultilingualSupport/
    └── ...
```

## Compatibility

All projects are configured to work with **Visual Studio 2022 Community** and use:
- Platform Toolset: v143 (default for VS 2022)
- Windows SDK: 10.0 (automatic detection of latest version)
- VCProjectVersion: 17.0 (VS 2022)

## Local Environment Setup

All projects use common configuration files to store compilation and debugging paths.

### Initial Setup

1. Copy file `Directory.Build.props.template` as `Directory.Build.props.user`
2. Edit paths for your local environment in `Directory.Build.props.user`:

```xml
<PropertyGroup>
  <LocalGamePath>C:\YourPath\HoMM 3 ERA</LocalGamePath>
  <LocalGameTestPath>C:\YourPath\HoMM 3 ERA test</LocalGameTestPath>
  <LocalToolsPath>C:\YourPath\Tools</LocalToolsPath>
  <LocalHeadersPath>C:\YourPath\headers</LocalHeadersPath>
  
  <!-- For VS 2022, v143 is used by default -->
  <PlatformToolset>v143</PlatformToolset>
  <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
</PropertyGroup>
```

### Settings Structure

- `Directory.Build.props` - common settings for all projects (included in Git)
- `Directory.Build.props.user` - user path settings (excluded from Git)
- `Directory.Build.props.template` - template for user settings

### Plugins in Repository

- **RMG_CustomizeObjectProperties** - customize object properties for map generator
- **Artifacts_AddingNewFeatures** - extend artifact functionality
- **Combat_SwitchCreatureAction** - switch creature actions in combat
- **ERA_MultilingualSupport** - multilingual support
- **HeroDlg_AlignClassName** - align hero class names
- **GameplayEnhancementsPlugin** - gameplay improvements
- **Graphics_TimeZonesSwitching** - graphics timezone switching
- **HeroArtifactsDlg** - hero artifacts dialog
- **NewAdvMapItems** - new adventure map items
- **NewCombatDlgItems** - new combat dialog items
- **NewMainMenuItems** - new main menu items
- And others...

### 💡 **Monorepo Benefits for Game Plugins**

1. **Unified API version**: all plugins are always compatible with each other
2. **Atomic changes**: can change API and all dependent plugins in one commit
3. **Code reuse**: common utilities and helpers available to all plugins
4. **Simplified build**: one click builds all plugins at once
5. **Common testing**: can test interactions between plugins

### Build Configurations

- **Debug|Win32** - debug build for Win32
- **Release|Win32** - release build for Win32
- **Debug|x64** - debug build for x64
- **Release|x64** - release build for x64

### Legacy System Support

If compatibility with older Windows versions is needed, you can use:
```xml
<PlatformToolset>v141_xp</PlatformToolset>
```

## Getting Started

### 🚀 **Quick Start**

1. **Clone repository**:
   ```bash
   git clone <repository-url>
   cd "plugins/daemon_n's"
   ```

2. **Setup local environment**:
   ```bash
   copy Directory.Build.props.template Directory.Build.props.user
   # Edit Directory.Build.props.user for your paths
   ```
   📖 **Details**: [VS_USER_SETTINGS.md](VS_USER_SETTINGS.md)

3. **Open in Visual Studio 2022**:
   ```bash
   start H3EraPlugins.sln
   ```

4. **Build all plugins**:
   - `Build -> Build Solution` (Ctrl+Shift+B)

### 🔧 **Developing New Plugin**

1. Create new folder for plugin
2. Add project to `H3EraPlugins.sln` (📖 [details](SOLUTION_BEST_PRACTICES.md))
3. Build settings will be automatically inherited from `Directory.Build.props`
4. Use common headers from `headers/` folder

All plugins will be automatically copied to corresponding game folders according to settings in `Directory.Build.props.user`.

### 🚀 **Работа с Visual Studio Code**

1. **Настройте окружение**:
   - Убедитесь, что установлен Visual Studio Code.
   - Установите расширение [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools).

2. **Откройте репозиторий в VS Code**:
   ```bash
   code .
   ```

3. **Сборка проекта**:
   - Откройте меню "Terminal -> Run Task".
   - Выберите задачу "Build Solution" для сборки всех плагинов.

4. **Отладка**:
   - Откройте меню "Run -> Start Debugging" (F5).
   - Убедитесь, что выбрана конфигурация "Launch Plugin".

5. **Редактирование кода**:
   - Используйте общие заголовочные файлы из папки `headers/`.
   - Форматируйте код с помощью `.clang-format`.

📖 **Подробнее**: [CONTRIBUTING.md](CONTRIBUTING.md)

## 📚 **Additional Information**

- **Plugin format**: `.era` files (DLL with renamed extension)
- **Game API**: header files in `headers/` folder
- **Build system**: MSBuild with common settings
- **Compatibility**: Heroes of Might and Magic 3 ERA
- **For developers**: see [CONTRIBUTING.md](CONTRIBUTING.md)
