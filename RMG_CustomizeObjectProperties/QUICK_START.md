# 🚀 Быстрая настройка RMG_CustomizeObjectProperties

## Для нового разработчика (5 минут)

### 1️⃣ **Настройка окружения**
```bash
# В корне репозитория
copy Directory.Build.props.template Directory.Build.props.user
notepad Directory.Build.props.user
```

Настройте в файле:
```xml
<LocalGamePath>D:\ВашПуть\HoMM 3 ERA</LocalGamePath>
<LocalGameTestPath>D:\ВашПуть\HoMM 3 ERA test</LocalGameTestPath>
<LocalToolsPath>D:\ВашПуть\Tools</LocalToolsPath>
```

### 2️⃣ **Открытие в Visual Studio**
```bash
# Откройте решение
start H3EraPlugins.sln
```

### 3️⃣ **Отладка плагина**
1. **Установите брейкпоинт** в коде плагина
2. **Правый клик на проект** `RMG_CustomizeObjectProperties` → **Set as StartUp Project**
3. **Нажмите F5** - игра запустится с отладчиком

### 4️⃣ **Проверка работы**
- Плагин скомпилируется в `.era` файл
- Автоматически скопируется в `$(LocalGamePath)\Mods\WoG\eraplugins\`
- Загрузится при запуске игры

## ✅ Что НЕ попадает в Git
- `Directory.Build.props.user` - ваши пути
- `*.vcxproj.user` - настройки отладки VS
- `Debug/`, `Release/` - результаты сборки

## 🆘 Проблемы?
1. **Игра не запускается** → проверьте `LocalGamePath` в `Directory.Build.props.user`
2. **Плагин не загружается** → проверьте путь копирования в Post-Build Event
3. **Ошибки компиляции** → убедитесь что `headers/` папка существует

## 📚 Подробная документация
- [VS_USER_SETTINGS.md](../VS_USER_SETTINGS.md) - детальная настройка Visual Studio
- [README.md](../README.md) - общая информация о репозитории
- [CONTRIBUTING.md](../CONTRIBUTING.md) - правила разработки
