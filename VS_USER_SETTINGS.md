# Настройка пользовательских параметров Visual Studio

**[🇷🇺 Русский](#user-settings-ru)** | **[🇺🇸 English](#user-settings-en)**

---

## <a id="user-settings-ru"></a>🇷🇺 Настройки для разработчика (русский)

### 🎯 **Где хранить локальные настройки**

Visual Studio позволяет сохранять пользовательские настройки в файлах, которые **НЕ попадают в Git**:

#### **1. Настройки путей и компиляции**
Файл: `Directory.Build.props.user` (в корне репозитория)

```xml
<?xml version="1.0" encoding="utf-8"?>
<Project>
  <PropertyGroup>
    <!-- Ваши локальные пути -->
    <LocalGamePath>D:\Games\HoMM3ERA</LocalGamePath>
    <LocalGameTestPath>D:\Games\HoMM3ERA_Test</LocalGamePath>
    <LocalToolsPath>D:\Tools</LocalToolsPath>
    
    <!-- Настройки компилятора -->
    <PlatformToolset>v143</PlatformToolset>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
    
    <!-- Дополнительные флаги для отладки -->
    <EnableClangTidyCodeAnalysis>false</EnableClangTidyCodeAnalysis>
    <RunCodeAnalysis>false</RunCodeAnalysis>
  </PropertyGroup>
</Project>
```

#### **2. Пользовательские настройки проекта**
Файл: `ProjectName.vcxproj.user` (создается автоматически)

В Visual Studio:
1. **Правой кнопкой на проект** → Properties
2. **Debugging** → настройте пути для отладки
3. **VC++ Directories** → дополнительные пути include/library
4. Настройки сохранятся в `.vcxproj.user`

#### **3. Настройки решения**
Файл: `H3EraPlugins.sln.docstates` (создается автоматически)
- Какие файлы открыты
- Позиция курсора
- Закладки

### 🔧 **Пример настройки отладки**

В Visual Studio для каждого проекта:

1. **Project Properties** → **Debugging**:
   ```
   Command: $(LocalGamePath)\Heroes3.exe
   Working Directory: $(LocalGamePath)
   Command Arguments: -plugin $(TargetPath)
   ```

2. **VC++ Directories** → **Include Directories**:
   ```
   $(LocalHeadersPath);$(IncludePath)
   ```

3. **Build Events** → **Post-Build Event**:
   ```
   xcopy "$(TargetPath)" "$(LocalGamePath)\Mods\WoG\eraplugins\" /Y
   ```

### 📁 **Файлы, которые НЕ попадают в Git**
- ✅ `Directory.Build.props.user` - пути и настройки компиляции
- ✅ `*.vcxproj.user` - настройки отладки и пользовательские свойства
- ✅ `*.sln.docstates` - состояние открытых файлов
- ✅ `.vs/` - кэш Visual Studio
- ✅ `Debug/`, `Release/` - результаты сборки

---

## <a id="user-settings-en"></a>🇺🇸 Developer Settings (English)

### 🎯 **Where to Store Local Settings**

Visual Studio allows saving user-specific settings in files that **DON'T go to Git**:

#### **1. Path and Compilation Settings**
File: `Directory.Build.props.user` (in repository root)

```xml
<?xml version="1.0" encoding="utf-8"?>
<Project>
  <PropertyGroup>
    <!-- Your local paths -->
    <LocalGamePath>D:\Games\HoMM3ERA</LocalGamePath>
    <LocalGameTestPath>D:\Games\HoMM3ERA_Test</LocalGameTestPath>
    <LocalToolsPath>D:\Tools</LocalToolsPath>
    
    <!-- Compiler settings -->
    <PlatformToolset>v143</PlatformToolset>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
    
    <!-- Additional debugging flags -->
    <EnableClangTidyCodeAnalysis>false</EnableClangTidyCodeAnalysis>
    <RunCodeAnalysis>false</RunCodeAnalysis>
  </PropertyGroup>
</Project>
```

#### **2. Project User Settings**
File: `ProjectName.vcxproj.user` (created automatically)

In Visual Studio:
1. **Right-click project** → Properties
2. **Debugging** → configure debugging paths
3. **VC++ Directories** → additional include/library paths
4. Settings saved to `.vcxproj.user`

#### **3. Solution Settings**
File: `H3EraPlugins.sln.docstates` (created automatically)
- Which files are open
- Cursor position
- Bookmarks

### 🔧 **Debugging Setup Example**

In Visual Studio for each project:

1. **Project Properties** → **Debugging**:
   ```
   Command: $(LocalGamePath)\Heroes3.exe
   Working Directory: $(LocalGamePath)
   Command Arguments: -plugin $(TargetPath)
   ```

2. **VC++ Directories** → **Include Directories**:
   ```
   $(LocalHeadersPath);$(IncludePath)
   ```

3. **Build Events** → **Post-Build Event**:
   ```
   xcopy "$(TargetPath)" "$(LocalGamePath)\Mods\WoG\eraplugins\" /Y
   ```

### 📁 **Files that DON'T go to Git**
- ✅ `Directory.Build.props.user` - paths and compilation settings
- ✅ `*.vcxproj.user` - debugging settings and user properties
- ✅ `*.sln.docstates` - open files state
- ✅ `.vs/` - Visual Studio cache
- ✅ `Debug/`, `Release/` - build results
