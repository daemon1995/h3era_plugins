# Build settings and workflow

## Solutions and scope

The inspected sibling directory contains four solutions, with all referenced vcxproj files present:

| Solution | Entries | Role |
|---|---:|---|
| H3EraPlugins/H3EraPlugins.sln | 28 | Main ERA solution |
| H3EraPlugins/H3ObjectExtenders.sln | 6 | Object extenders and map editor |
| H3EraPlugins/Commissions/Commissions.sln | 28 | Commission plugins |
| H3SoDPlugins/H3SodPlugins.sln | 2 | Separate SoD plugins |

Counts include MapToDbgmap where referenced, and overlap between solutions. Choose projects from actual solution membership; some projects exist on disk without belonging to these solutions. Avoid historical copies under build, _build, Release/verification and tools/BattleAudioResearch unless that research is the task. Graphics_BikVideo has its own MASM `build.ps1`, not a vcxproj.

ERA solutions expose Debug/Release with platform `x86`; their native projects use `Win32`. Inspect SoD's solution configuration rather than assuming its labels match ERA. Direct ERA project builds use `/p:Platform=Win32`.

## ERA property chain

1. `Microsoft.Cpp.Default.props` discovers the nearest `Directory.Build.props`.
2. Repository props sets fallback `v143`, Windows SDK `10.0`, DynamicLibrary, `.era` via PluginTargetExt and the initial PluginOutDir. It imports `Directory.Build.props.user`.
3. Current local user props overrides PlatformToolset to `v141_xp`; game paths are `D:/games/HoMM 3 ERA` and `D:/games/HoMM 3 ERA test`. Templates suggesting v143 are not the effective local setting.
4. `ForceImportAfterCppProps` loads `Directory.Build.after.props` after Microsoft.Cpp.props. It sets OutDir from PluginOutDir, TargetExt from PluginTargetExt, static runtime `/MT` Release or `/MTd` Debug, and common compiler/linker settings.
5. `Directory.Build.targets` adds Release debug maps and the MapToDbgmap ProjectReference. `ForceImportAfterCppTargets` loads `Directory.Build.after.targets`, enforcing Release LinkTimeCodeGeneration.

Commission props sets PluginModName=Commissions and explicitly imports the parent props. MSBuild nearest-file discovery alone would otherwise hide parent defaults.

Release enables speed optimization, whole-program optimization, PDB, MAP, reference elimination and COMDAT folding. Debug disables optimization and enables runtime checks. Both add `/Zc:threadSafeInit-`. Preserve project-specific language standards; many explicitly set C++17, others do not.

## Output exceptions

- Ordinary ERA: `$(LocalGamePath)/Mods/$(PluginModName)/eraplugins/$(TargetName).era`, default mod WoG.
- Gameplay_GameplayEnhancementPlugin sets mod `Game Enhancement Mod` and has a .def export file.
- EraEditor_ObjectsExtender and EraEditor_JsonOverrides select `Mods/<mod>/EraEditor/` and `.dll` when default values still apply. Explicit PluginOutDir does not remove their extension logic.
- Interface_AlignClassName explicitly uses `.dll`.
- Graphics_LegendaryHeroes sets TargetName=`Legend_Heroes`.
- Utils_CombatEmulator selects `goldfish - Combat Emulator`; Utils_RandomCombatSimulator defines a deployment Copy target. Inspect that target before redirecting output.
- Combat_BattleHeroes/Verification contains Debug|Win32 console EXE projects EraSettings and CardOverlayLayout; do not treat them as game DLLs or build nonexistent Release configurations.

Release MAP and DBGMAP go to `GetFullPath(OutDir + '../DebugMaps/')`, not necessarily OutDir. Keep the corresponding PDB and maps with the binary when delivering a diagnostic build.

MapToDbgmap is a Release|Win32 console application, explicitly `v143`, C++17, `.exe`, with repository-local tools/MapToDbgmap/bin and obj. It evaluates to v143 while plugins evaluate to v141_xp. The reference removes global OutDir, IntDir, TargetName, TargetExt; it does NOT remove PlatformToolset, PluginOutDir or all other global settings. Therefore globally forcing PlatformToolset=v141_xp can also override the helper's intended toolset. Avoid blanket toolset overrides or disabling BuildProjectReferences as a routine fix. Its executable path is hard-coded in the root targets; redirecting the helper requires reviewing that consumer too.

## Inspection before a build

Locate MSBuild using `vswhere.exe` under the Visual Studio Installer directory rather than assuming a particular VS installation. The inspected machine has VS 2022 Community and toolsets v141_xp (under VC/v150), v143 (under VC/v170); SoD evaluated to v141. The normal shell's python.exe is an unusable WindowsApps alias; locate a real Python runtime if needed.

With the selected MSBuild executable and full project path in PowerShell variables:

```powershell
& $msbuild $project /nologo /p:Configuration=Release /p:Platform=Win32 /getProperty:PlatformToolset,VCTargetsPath,WindowsTargetPlatformVersion,TargetPath,OutDir,IntDir,PluginOutDir,ReleaseMapPath,ReleaseDbgMapPath,MapToDbgmapExe /getItem:ProjectReference
```

This evaluates properties without building; inspect XML targets separately for output paths and deployment actions. For inheritance problems, preprocess with `/pp:<file-in-task-work-directory>` and inspect effective item definitions such as Link.OutputFile and ClCompile.LanguageStandard as needed.

For verification builds, supply task-specific absolute paths with trailing separators, for example `PluginOutDir=<task>/bin/`, `OutDir=<task>/bin/` and `IntDir=<task>/obj/<project>/<configuration>/`. Reevaluate first; build with `/t:Build /m:1` and a file log in the task directory. A unique IntDir is necessary per project/configuration, especially for solution builds. Do not reuse one global IntDir across a solution.

Review hard-coded Link.OutputFile, import library, PDB, custom Copy/Exec targets and helper outputs: setting OutDir alone does not prove isolation. Compilation of a Release plugin may still build MapToDbgmap inside the repository. Do not claim all outputs are isolated unless all participating targets have been checked. Use the user's authorized deployment destination when deployment is requested; otherwise a verification build should not silently replace the live game plugin.

After a build, check exit status, produced binary and the expected Release debug map pair. A link success followed by DBGMAP conversion failure is a failed overall build; retain the actual diagnostic. Verify PE machine is x86 and expected exports when those affect the task. Check existing targeted tests only where relevant. In-game behavior remains unverified until exercised.

## SoD is separate

H3SoDPlugins has a different Directory.Build.props and no ERA after-props/after-targets chain. Its local Directory.Build.props.user explicitly selects v141, SDK 8.1 and `.dll`; Combat_ControlRandomEvents evaluation confirmed these values and the output under `D:/games/HoMM 3 ERA/_HD3_Data/Packs/Combat_ControlRandomEvents/`. Do not copy ERA's `.era`, mod directory or debug-map rules into it. It also imports user/global C++ property sheets; evaluate before changing settings.

## MASM path

Graphics_BikVideo/build.ps1 finds x86 ml.exe/link.exe and SDK libraries using vswhere. It accepts OutDir, builds `Video playback in BIK format.dll`, emits video.map and reports SHA256. It uses explicit DLL entry point and linker flags; use that script for this project rather than inventing a C++ project or replacing its entry point.
