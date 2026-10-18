# Проверка сбоя VFS при запуске — 16.10.2026

Обнаружен дефект VFS 1.0.5.29, позволяющий воспроизвести `0xC0000005` точно на `vfs.dll + 0x2BB5C` без ERA и без ERA_SystemOptionsExtension. Условие воспроизведения — включённый DEP при выполнении переходника NtClose из страницы PAGE_READWRITE.

Это подтверждает существование конкретной причины с тем же адресом. Для окончательной привязки к сообщению с компьютера `F:\Homm 3 ERA` нужны тип нарушения доступа и защита памяти из его дампа. Журнал Windows их не содержит. Само зависание с высокой загрузкой CPU в проверочном процессе не воспроизводилось: обработчик диагностики фиксирует первый AV и завершает только свой процесс.

## Проверенные файлы

Локальная `D:\games\HoMM 3 ERA\vfs.dll`:

- версия `1.0.5.29`;
- PE TimeDateStamp `0x67BF2A4E`, совпадает с сообщением;
- SHA-256 `C54A548CEEC45623D593AF5BC905A6E9CF737BBBE811FB404BD88CB40AB50A25`;
- код сопоставлен с установленными исходниками `Tools\Era\Sources\Vfs 1.0.5.29` и `DebugMaps\Vfs.dbgmap`.

Локальная ERA — `3.9.31-port`. Это не доказывает, что на проблемном компьютере установлен тот же бинарник ERA. Версия и метка времени VFS также не заменяют сравнение SHA-256.

## Почему этот адрес указывает на DEP

В [VfsPatching.pas](<D:/games/HoMM 3 ERA/Tools/Era/Sources/Vfs 1.0.5.29/VfsPatching.pas:41>) объявлен статический массив PersistentMem размером 100 КБ. Карта символов размещает его в RVA `0x2BAF4..0x44AF3`, в секции `.bss`. Для проверенной DLL и текущих прологов Windows адрес `0x2BB5C` — начало сгенерированного переходника NtClose, смещение `0x68` внутри массива.

[WriteAtCode](<D:/games/HoMM 3 ERA/Tools/Era/Sources/Vfs 1.0.5.29/VfsPatching.pas:79>) временно выставляет PAGE_EXECUTE_READWRITE, записывает переходник и возвращает исходную защиту. Для PersistentMem исходная защита — PAGE_READWRITE (`0x04`). Вызов переходника при включённом DEP вызывает ошибку выполнения памяти.

[VfsHooks.InstallHooks](<D:/games/HoMM 3 ERA/Tools/Era/Sources/Vfs 1.0.5.29/VfsHooks.pas:579>) пробует отключить DEP вызовом SetProcessDEPPolicy(0), но при неудаче только пишет сообщение и продолжает установку перехватов. Постоянно включённый DEP этим вызовом отключить нельзя: [документация Microsoft](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setprocessdeppolicy).

## Изолированное воспроизведение

Проверочный x86-процесс загружает только исследуемую VFS и системные библиотеки. ERA, игровые плагины, карта и моды не загружаются. Изменения DEP и VirtualProtect действуют только в этом процессе. Установленные DLL и параметры Windows не изменяются.

| Режим | Результат |
| --- | --- |
| `dep-off` | RunVfs и открытие/закрытие файла проходят, код выхода 0 |
| `dep-on` | Инициализация VFS вызывает AV выполнения на `+0x2BB28`, NtOpenFile; код выхода 77 |
| `dep-before-close` | DEP включается после инициализации, непосредственно перед CloseHandle; AV точно на `+0x2BB5C`; код выхода 77 |
| `dep-on-executable-buffer` | В проверочном процессе массиву заранее разрешено выполнение; при включённом DEP инициализация и файл проходят, код выхода 0 |

Ключевая запись точного воспроизведения:

```text
Enable DEP immediately before CloseHandle=1
DEP query=1 flags=1
AV code=C0000005 instruction=0042BB5C vfsRva=0002BB5C access=8 target=0042BB5C protect=00000004
PersistentMem offset=68 bytes: FF 34 E4 C7 44 E4 04 6C BB 42 00 E9 DC 5B FF FF B8 0F 00 03 00 E9 3F 77
```

`access=8` означает попытку выполнения; `protect=4` — PAGE_READWRITE. Абсолютный адрес меняется с базой загрузки DLL, RVA остаётся тем же.

[Исходник проверки](<C:/dev/homm3/plugins/daemon_n/H3EraPlugins/build/ERA_SystemOptionsExtension/vfs-startup-review/VfsStartupProbe.cpp>), [сборка](<C:/dev/homm3/plugins/daemon_n/H3EraPlugins/build/ERA_SystemOptionsExtension/vfs-startup-review/build.cmd>), [точное воспроизведение](<C:/dev/homm3/plugins/daemon_n/H3EraPlugins/build/ERA_SystemOptionsExtension/vfs-startup-review/dep-before-close.txt>), [контроль с DEP](<C:/dev/homm3/plugins/daemon_n/H3EraPlugins/build/ERA_SystemOptionsExtension/vfs-startup-review/dep-on-executable-buffer.txt>). Диагностический каталог build игнорируется Git; это не компонент поставляемого плагина.

## Что проверено в ERA_SystemOptionsExtension

- [DllMain и HooksInit](<C:/dev/homm3/plugins/daemon_n/H3EraPlugins/ERA_SystemOptionsExtension/dllmain.cpp:107>): регистрация ERA, установка хуков, чтение дополнительных опций и инициализация модулей. Плагин не создаёт фоновые потоки и не меняет DEP.
- [AdditionalConfig::Load](<C:/dev/homm3/plugins/daemon_n/H3EraPlugins/ERA_SystemOptionsExtension/ConfigManager.cpp:264>) читает ограниченный набор ключей heroes3.ini через ERA API. Обнаруженного цикла файлового опроса при запуске нет.
- [BattleSave](<C:/dev/homm3/plugins/daemon_n/H3EraPlugins/ERA_SystemOptionsExtension/BattleSave.cpp:1586>) на инициализации устанавливает хуки и регистрирует события. Работа с сохранением выполняется при событиях игры и боя, а не при первоначальной загрузке DLL.
- [RecoverOwnedFiles](<C:/dev/homm3/plugins/daemon_n/H3EraPlugins/ERA_SystemOptionsExtension/BattleSave.cpp:596>) уже проверяет точные имена. Прежняя подтверждённая взаимная блокировка с VFS watcher при переборе Games устранена. Это другой сценарий — загрузка игры, а не установленный пользователем этап первоначального запуска.
- Коллбэки событий используют `__stdcall`; строки API передаются как `const char*`; результаты логических функций ERA объявлены 32-битными. В установленной современной era.dll экспорт ReadStrFromIni (`RVA 0x1AD30C`) вызывает `System.@LStrFromPChar` (`RVA 0xBA10`) и заканчивается `ret 0x10`: у этого проверенного экспорта сохранены ANSI и stdcall с четырьмя аргументами. Экспорт WriteStrToIni также возвращает 32-битный результат и очищает 16 байт аргументов. Это ограниченная проверка бинарного интерфейса, не полный аудит всего порта ERA.

В обычном порядке инициализации ERA VFS запускается до загрузки `.era`-плагинов: [GameExt.Init](<D:/games/HoMM 3 ERA/Tools/Era/Sources/Era 3.9.16/gameext.pas:476>). Если процесс падает именно в RunVfs, код плагина настроек ещё не начал выполняться. Независимое воспроизведение подтверждает, что этот плагин не является обязательным условием обнаруженного AV.

## Связь с переносом ERA на современный Delphi

Из самого факта переноса нельзя вывести, кто включил DEP. Проверить следует launcher, другие DLL и настройки Exploit Protection для конкретного h3era.exe. DLL-флаг NXCOMPAT сам по себе не доказывает политику DEP всего процесса.

Отдельный риск переноса — изменение значения PChar на PWideChar. Публичные C API и границу со старой VFS нужно сохранять на PAnsiChar/AnsiChar, со старым stdcall, размерами логических результатов, упаковкой записей и явным владельцем выделенной памяти. [Документация Embarcadero](https://docwiki.embarcadero.com/RADStudio/Athens/en/Unicode_in_RAD_Studio). Нарушение этого интерфейса для данного адреса не подтверждено; проверенные INI-экспорты установленной ERA его сохраняют.

## Исправление и подтверждение на проблемной машине

Исправление следует делать в VFS: размещать сгенерированные переходники в выделенной исполняемой памяти. Надёжная реализация выделяет блок через VirtualAlloc, записывает код, переводит готовый блок в PAGE_EXECUTE_READ, вызывает FlushInstructionCache и только затем публикует переход в оригинальной функции. Временное изменение защиты оригинальных кодовых страниц остаётся отдельной операцией. Все результаты выделения, записи и смены защиты должны проверяться до установки перехвата.

Контрольный режим с PAGE_EXECUTE_READWRITE в проверочном процессе демонстрирует причину; это не готовый патч для поставки и не рекомендация отключать DEP в Windows.

Для окончательного подтверждения взять дамп аварии/зависшего процесса. Проверить ExceptionInformation[0] = 8, адрес VFS+0x2BB5C и PAGE_READWRITE у этой страницы, политику DEP, стек сбойного потока и сообщения VFS `Failed to turn DEP off`. Если остаётся именно высокая загрузка CPU, нужны стеки всех потоков: одно совпадение AV не объясняет, какой обработчик затем удерживает процесс активным.

Проверка выполнена без изменения кода плагина и установленных DLL. Пользовательское изменение версии плагина на 1.8.1 оставлено как было.
