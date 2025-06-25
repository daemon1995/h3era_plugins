# Contributing Guidelines | Руководство для разработчиков

**[🇷🇺 Русский](#contributing-ru)** | **[🇺🇸 English](#contributing-en)**

---

## <a id="contributing-ru"></a>🇷🇺 Для разработчиков (русский)

### 📋 **Требования**
- Visual Studio 2022 Community (или выше)
- Windows SDK 10.0
- Git

### 🚀 **Быстрый старт для разработчика**

1. **Форкните репозиторий** и клонируйте:
   ```bash
   git clone https://github.com/yourusername/heroes3era-plugins.git
   cd heroes3era-plugins
   ```

2. **Настройте окружение**:
   ```bash
   copy Directory.Build.props.template Directory.Build.props.user
   # Отредактируйте пути в Directory.Build.props.user
   ```

3. **Откройте проект**:
   ```bash
   start H3EraPlugins.sln
   ```

### 🔧 **Создание нового плагина**

1. **Создайте папку** для плагина: `YourPluginName/`
2. **Создайте .vcxproj файл** (используйте существующий как шаблон)
3. **Добавьте проект** в `H3EraPlugins.sln`
4. **Используйте общие настройки** - они подхватятся автоматически

### 📝 **Правила кодирования**

- **Используйте общие заголовки** из папки `headers/`
- **Следуйте стилю** существующего кода
- **Комментируйте код** на русском или английском
- **Тестируйте совместимость** с другими плагинами

### 🎯 **Структура плагина**
```
YourPluginName/
├── YourPluginName.vcxproj
├── YourPluginName.vcxproj.filters
├── dllmain.cpp
├── README.md
└── [source files]
```

---

## <a id="contributing-en"></a>🇺🇸 For Developers (English)

### 📋 **Requirements**
- Visual Studio 2022 Community (or higher)
- Windows SDK 10.0
- Git

### 🚀 **Developer Quick Start**

1. **Fork repository** and clone:
   ```bash
   git clone https://github.com/yourusername/heroes3era-plugins.git
   cd heroes3era-plugins
   ```

2. **Setup environment**:
   ```bash
   copy Directory.Build.props.template Directory.Build.props.user
   # Edit paths in Directory.Build.props.user
   ```

3. **Open project**:
   ```bash
   start H3EraPlugins.sln
   ```

### 🔧 **Creating New Plugin**

1. **Create folder** for plugin: `YourPluginName/`
2. **Create .vcxproj file** (use existing as template)
3. **Add project** to `H3EraPlugins.sln`
4. **Use common settings** - they will be inherited automatically

### 📝 **Coding Guidelines**

- **Use shared headers** from `headers/` folder
- **Follow existing code style**
- **Comment code** in Russian or English
- **Test compatibility** with other plugins

### 🎯 **Plugin Structure**
```
YourPluginName/
├── YourPluginName.vcxproj
├── YourPluginName.vcxproj.filters
├── dllmain.cpp
├── README.md
└── [source files]
```

---

## 🔄 **General Workflow | Общий процесс работы**

### Pull Request Process | Процесс Pull Request

1. **Create feature branch** | **Создайте ветку для функции**
   ```bash
   git checkout -b feature/your-plugin-name
   ```

2. **Make changes** | **Внесите изменения**
3. **Test thoroughly** | **Тщательно протестируйте**
4. **Update documentation** | **Обновите документацию**
5. **Create pull request** | **Создайте pull request**

### 📚 **Documentation | Документация**

- **Update both languages** | **Обновляйте оба языка**
- **Keep README.md and README.en.md in sync** | **Синхронизируйте README.md и README.en.md**
- **Document new features** | **Документируйте новые функции**

### 🧪 **Testing | Тестирование**

- **Test with multiple plugins** | **Тестируйте с несколькими плагинами**
- **Verify build configurations** | **Проверяйте конфигурации сборки**
- **Check game compatibility** | **Проверяйте совместимость с игрой**
