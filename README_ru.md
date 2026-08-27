# Плагины Heroes of Might and Magic III ERA

[English](README.md) | **Русский**

Монорепозиторий C++-плагинов, общих заголовочных файлов и средств сборки для Heroes of Might and Magic III ERA.

Проекты изменяют работу 32-битного исполняемого файла игры, поэтому собираются **только для x86/Win32**. Репозиторий содержит общие плагины ERA, расширители объектов, заказные плагины, тестовые проекты и утилиту преобразования map-файлов в dbgmap.

## Требования

- Visual Studio 2022 с workload **Desktop development with C++**
- Windows 10 SDK
- Локальная установка Heroes III ERA для развёртывания и отладки

По умолчанию используется toolset Visual Studio 2022 `v143` и Windows SDK `10.0`. В solution доступны конфигурации `Debug|x86` и `Release|x86`; C++-проекты используют соответствующую платформу `Win32`.

### Совместимость с Windows XP

Плагины подразумевают возможность сборки и работы на **Windows XP**. Для получения совместимых с Windows XP бинарных файлов откройте Visual Studio Installer, выберите изменение установленной Visual Studio и добавьте компонент **C++ Windows XP support for VS 2017 (v141) tools**. Он предоставляет toolset `v141_xp` и по умолчанию не устанавливается.

После установки компонента укажите toolset в локальном `Directory.Build.props.user`:

```xml
<Project>
  <PropertyGroup>
    <PlatformToolset>v141_xp</PlatformToolset>
  </PropertyGroup>
</Project>
```

Без этого компонента Visual Studio сборка с `v141_xp` завершится ошибкой отсутствующего toolset.

## Структура репозитория

```text
├── headers/                         Общие заголовки API ERA, WoG и игры
├── Commissions/                     Заказные плагины и их solution
├── Objects_*/                       Плагины-расширители объектов
├── tools/MapToDbgmap/               Конвертер map-файлов Release-сборок
├── Directory.Build.props            Ранние общие настройки MSBuild
├── Directory.Build.after.props      Общие настройки компилятора и линкера
├── Directory.Build.targets          Создание .map/.dbgmap для Release
├── H3EraPlugins.sln                 Основной solution плагинов
└── H3ObjectExtenders.sln            Solution расширителей объектов
```

Каталоги плагинов названы по функциональности: `ERA_*`, `Gameplay_*`, `Interface_*`, `Objects_*`, `RMG_*` и другие. `Commissions/Commissions.sln` содержит заказные плагины. Проекты `Test_*` предназначены для разработки и тестирования, а не являются готовыми пользовательскими функциями.

## Локальная настройка

1. Клонируйте репозиторий:

   ```powershell
   git clone https://github.com/daemon1995/h3era_plugins.git
   cd h3era_plugins
   ```

2. Создайте локальный файл настроек, не отслеживаемый Git:

   ```powershell
   Copy-Item Directory.Build.props.template Directory.Build.props.user
   ```

3. Укажите пути к установкам ERA:

   ```xml
   <Project>
     <PropertyGroup>
       <LocalGamePath>C:\Games\Heroes3ERA</LocalGamePath>
       <LocalGameTestPath>C:\Games\Heroes3ERA_Test</LocalGameTestPath>
     </PropertyGroup>
   </Project>
   ```

4. Откройте `H3EraPlugins.sln` в Visual Studio 2022 и соберите `Debug|x86` или `Release|x86`.

`Directory.Build.props.user` исключён из Git. В репозитории есть резервные пути к игре, но они зависят от компьютера разработчика и обычно должны быть переопределены локально. Подробности находятся в [VS_USER_SETTINGS.md](VS_USER_SETTINGS.md).

## Результаты сборки

По умолчанию проекты плагинов являются динамическими библиотеками с расширением `.era`. Они записываются непосредственно в:

```text
$(LocalGamePath)\Mods\$(PluginModName)\eraplugins\
```

`PluginModName` по умолчанию равен `WoG`; проекты внутри `Commissions/` заменяют его на `Commissions`. Отдельные проекты могут переопределять расширение или другие общие настройки.

Release-сборки также создают linker-файлы `.map` и преобразуют их в `.dbgmap` в соседнем каталоге `DebugMaps`. Для Debug-сборок это преобразование не выполняется.

## Разработка

Проекты ниже корня репозитория автоматически наследуют общие настройки MSBuild. Новые проекты плагинов должны использовать общие заголовки из `headers/` и добавляться в подходящий solution.

- [CONTRIBUTING.md](CONTRIBUTING.md)
- [Руководство по разработке плагинов](PLUGIN_DEVELOPMENT_GUIDE.ru.md)
- [Рекомендации по solutions](SOLUTION_BEST_PRACTICES.md)
- [Документация RMG_CustomizeObjectProperties](RMG_CustomizeObjectProperties/README.md)
- [Краткое руководство по Object Extender API](RMG_CustomizeObjectProperties/QUICK_START.md)

В `.vscode` находятся вспомогательные задачи, однако часть старых задач отдельных проектов и локальных путей привязана к конкретному компьютеру. Основной документированный способ сборки — Visual Studio и solution-файлы.

## Документация встроенного API

Следующие README относятся к включённым исходникам H3API и описывают сторонние заголовки, а не систему сборки этого репозитория:

- [H3API README](headers/H3API/Readme.md)
- [H3API single-header README](headers/H3API/single_header/Readme.md)
- [Обзор структуры H3API](headers/H3API_for_copilot/Readme.md)

## Отказ от ответственности

Это неофициальный репозиторий фанатской разработки. Heroes of Might and Magic и связанные обозначения принадлежат соответствующим правообладателям.
