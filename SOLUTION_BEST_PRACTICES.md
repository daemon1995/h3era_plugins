# 📋 Solution Files в Монорепо - Лучшие практики

## 🎯 **Правильный подход для монорепо HoMM3 ERA плагинов**

### ✅ **ИСПОЛЬЗУЙТЕ - Один основной solution**
```
H3EraPlugins.sln  ← ГЛАВНЫЙ solution для всех плагинов
```

**Преимущества:**
- 🚀 **Одна команда Build Solution** собирает все плагины
- 🔄 **Общие зависимости** между проектами видны
- 🎯 **Простая навигация** между плагинами в одном окне VS
- 📊 **Batch Build** - сборка нескольких конфигураций сразу
- 🐛 **Единая отладка** - можно отлаживать взаимодействие плагинов

### ❌ **НЕ СОЗДАВАЙТЕ - Отдельные solution файлы**
```
Plugin1/Plugin1.sln      ← Лишнее!
Plugin2/Plugin2.sln      ← Лишнее!
Plugin3/Plugin3.sln      ← Лишнее!
```

**Проблемы:**
- 😵 **Фрагментация** - нужно открывать много окон VS
- 🐌 **Медленная разработка** - переключение между проектами
- 🔗 **Потеря связей** между плагинами
- 🧹 **Лишние файлы** в репозитории

## 🔧 **Как добавить новый плагин в основной solution**

### **В Visual Studio:**
1. **Откройте** `H3EraPlugins.sln`
2. **Solution Explorer** → **Add** → **Existing Project**
3. **Выберите** `YourPlugin/YourPlugin.vcxproj`
4. **Сохраните** solution (Ctrl+S)

### **Вручную в .sln файле:**
```sln
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "YourPluginName", "YourPluginFolder\YourPlugin.vcxproj", "{GUID}"
EndProject
```

## 🧹 **Очистка лишних solution файлов**

Текущие лишние `.sln` файлы в репозитории:
- `assemblyMainPLugin/assemblyMainPLugin.sln` ❌
- `desktopApp/desktopApp.sln` ❌
- `NewAdvMapItems/newAdvMapItems.sln` ❌
- `NH3API_Plugins/NH3API_Plugins.sln` ❌
- `Legendary Heroes/Legendary Heroes.sln` ❌

**Рекомендация:** Удалить их и использовать только `H3EraPlugins.sln`

## 📁 **Обновленный .gitignore**

```gitignore
# Лишние solution файлы в подпапках
*/*.sln
!H3EraPlugins.sln  ← Исключение для главного solution
```

## 🎯 **Результат правильной структуры**

```
📁 HoMM3_ERA_Plugins/
├── H3EraPlugins.sln                    ← ЕДИНСТВЕННЫЙ solution
├── Directory.Build.props               ← Общие настройки
├── Directory.Build.props.user          ← Ваши локальные настройки
├── RMG_CustomizeObjectProperties/
│   ├── RMG_CustomizeObjectProperties.vcxproj
│   └── *.cpp, *.h файлы
├── ERA_MultilingualSupport/
│   ├── ERA_MultilingualSupport.vcxproj
│   └── *.cpp, *.h файлы
└── OtherPlugin/
    ├── OtherPlugin.vcxproj
    └── *.cpp, *.h файлы
```

## 🚀 **Команды для работы**

```bash
# Открыть все плагины сразу
start H3EraPlugins.sln

# Собрать все плагины
# В VS: Build → Build Solution (Ctrl+Shift+B)

# Собрать конкретный плагин
# В VS: правый клик на проект → Build

# Batch Build (несколько конфигураций)
# В VS: Build → Batch Build
```

**Вывод:** Используйте ТОЛЬКО `H3EraPlugins.sln` для всех плагинов! 🎯
