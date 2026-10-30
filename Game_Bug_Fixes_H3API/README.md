# Game Bug Fixes — порт на H3API

Проект использует общий `headers/header.h`, H3API и ERA API репозитория.
`Game bugs fixes.vcxproj` создаёт **Game bugs fixes extended.dll** для Win32.
Все установщики из `src/Plugin.cpp` вызываются при загрузке плагина; переключатель
`GBF_INSTALL_UNVERIFIED_PORTS` удалён, поскольку он не управлял этими вызовами.

Активные части GameLogic находятся в `src/Fixes/GameLogicCore.cpp` и
`src/Fixes/GameLogicRemaining.cpp`. Файл `src/Ports/GameLogic.incomplete.cpp`
сохранён как исторический черновик и исключён из компиляции. Каталог `legacy`
содержит исходники для сравнения и тоже не компилируется.

Проверка сборки подтверждает компиляцию и ABI-проверки `src/AbiLayoutChecks.cpp`.
Она не подтверждает поведение хуков в игре. Полная игровая проверка текущего
набора модулей с используемой версией ERA и другими плагинами ещё требуется.
Оригинальный Game Bug Fixes устанавливает пересекающиеся хуки: одновременно
загружать обе версии не следует.

Исправления размеров карт переданы XXL. Отдельного установщика MapSize в этом
проекте нет. Подробности и оставшиеся ограничения:

- [Статус модулей](docs/PORT_STATUS_REVIEW.md)
- [Аудит ABI и H3API](docs/H3API_DEEP_AUDIT.ru.md)
- [Перенос GameLogic](docs/GAMELOGIC_PORT_REVIEW.md)
- [Совместимость с XXL](docs/XXL_COMPATIBILITY.md)
- [Риски интеграции](docs/INTEGRATION_REVIEW.md)

Сборка: `Debug|Win32` или `Release|Win32`, toolset `v141_xp`, C++17, `/Zp1`.
Для проверочной сборки переопределяйте `PluginOutDir`, `OutDir` и `IntDir`:
обычные пути наследуются из общих настроек репозитория и могут вести в игру.

[Историческое описание переноса](docs/HISTORICAL_README.md) сохранено для
справки; его прежние инструкции по путям и переключателям не описывают текущую сборку.
