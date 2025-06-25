# Heroes of Might and Magic 3 ERA Plugins

Monorepo of plugins for Heroes of Might and Magic 3 ERA.

> **📖 Русская версия**: See [README.md](README.md#russian) for Russian version

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

3. **Open in Visual Studio 2022**:
   ```bash
   start H3EraPlugins.sln
   ```

4. **Build all plugins**:
   - `Build -> Build Solution` (Ctrl+Shift+B)

### 🔧 **Developing New Plugin**

1. Create new folder for plugin
2. Add project to `H3EraPlugins.sln`
3. Build settings will be automatically inherited from `Directory.Build.props`
4. Use common headers from `headers/` folder

All plugins will be automatically copied to corresponding game folders according to settings in `Directory.Build.props.user`.

## 📚 **Additional Information**

- **Plugin format**: `.era` files (DLL with renamed extension)
- **Game API**: header files in `headers/` folder
- **Build system**: MSBuild with common settings
- **Compatibility**: Heroes of Might and Magic 3 ERA

## Contributing

When contributing to this repository:

1. **Follow the monorepo structure** - use common settings from `Directory.Build.props`
2. **Test with multiple plugins** - ensure your changes don't break other plugins
3. **Update documentation** - keep both Russian and English versions in sync
4. **Use shared headers** - leverage common API definitions from `headers/` folder

## License

This project contains plugins for Heroes of Might and Magic 3 ERA. Please respect the original game's license and ERA framework requirements.
