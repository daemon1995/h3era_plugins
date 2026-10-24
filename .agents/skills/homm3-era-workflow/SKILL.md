---
name: homm3-era-workflow
description: Develop and build the user's local H3EraPlugins and H3SoDPlugins C++ projects, investigate Heroes III functions and hooks, and transfer ERM logic into ERA plugins. Use for these repositories and their game binaries, not unrelated C++ projects.
---

# Heroes III plugin workflow

Default repository: `C:/dev/homm3/plugins/daemon_n/H3EraPlugins`. Adjacent repositories: `H3SoDPlugins` and `homm3-decomp-decomp-complete-4.0`. Revalidate paths if the working checkout differs. Read the applicable current AGENTS.md before editing. Snapshot references are evidence from 2026-10-21, not permanent configuration.

## Choose the relevant workflow

- For compiling, changing projects, diagnosing build failures, or delivering a binary, read [build.md](references/build.md). It explains the actual property import chain and separate toolchains.
- For identifying a game function, interpreting pseudocode, fixing hooks, or porting ERM, read [reverse-engineering.md](references/reverse-engineering.md).
- To locate a project or inspect unusual output settings, search [inventory.md](references/inventory.md). [inventory.json](references/inventory.json) contains the full static leaf settings, import paths, target actions and conditions. Load only relevant records. Research and backup projects are not necessarily active solution members.
- [evaluated-projects.json](references/evaluated-projects.json) records read-only Release|Win32 evaluation of the 60 unique native projects referenced by the four solutions; all returned exit code zero. It is a snapshot of effective paths/toolsets, not proof of compilation.

## Local invariants

## User's working preferences

- Create Git commits and push changes only when the user explicitly requests those actions for the current work. Do not commit or push automatically after edits, fixes, builds or version bumps, and do not carry authorization from an earlier completed request into subsequent changes. Otherwise leave changes in the working tree for review.
- Do not use computer-use, desktop/browser UI automation, or the computer-use skill for these projects. Work through source files, command-line tools, scripts, available APIs and existing exports. If evidence is accessible only through a GUI, explain the specific missing evidence instead of switching to computer-use.
- Work promptly and keep changes focused. Follow the owning plugin's existing coding style, naming, initialization, hooks, file organization and API conventions. Prefer the smallest clear implementation; avoid unrelated refactors, speculative frameworks and unnecessary planning or documentation.
- Do not create or run tests for every minor edit. Use targeted verification when the user requests it or a concrete behavior change, failure or regression risk warrants it. Batch related edits before checking; do not repeatedly rerun successful checks without a new reason. A relevant compilation is distinct from adding a test suite; do not require whole-solution builds for small plugin changes.
- Keep all newly created tests, test source, harnesses, fixtures, test generators and test outputs out of Git staging, commits and pushes. Put necessary temporary checks outside the repository or in a verified ignored task directory. Before staging production changes, inspect the intended file list. Do not delete or untrack pre-existing tests as part of this preference. If the user explicitly requests a versioned test change, follow that request.
- Reuse existing ERA/H3API and plugin APIs where suitable. When the task needs functionality shared across plugins or callable from ERM, expose a small reusable API in the repository's established style, with the necessary declarations, exports and implementation. Keep ABI and lifetime contracts clear; avoid inventing an API for a one-off internal operation without a concrete consumer or need.

## Implementation constraints

Preserve the 32-bit game ABI, exported names, calling conventions and structure layout. Use the existing headers and plugin initialization patterns instead of recreating declarations. `headers/header.h` routes H3API, patcher_x86, ERA and optional APIs via macros; inspect the owning project's pch/framework and defines before selecting includes.

Do not infer effective settings from README templates: the actual user props and evaluated MSBuild configuration take precedence. Keep project-specific DLL names, mod destinations and extension differences. A `.era` output is a native plugin binary; editor plugins and some game plugins deliberately use `.dll`.

When editing ERM, current repository rules require `active[source]`, not `active[(source)]`, inside local array indexes. To disable a receiver/trigger/instruction, break its leading marker: `!!VR` → `*!VR`, `!?FU` → `*?FU`, `!#VA` → `*#VA`; merely prepending `*` leaves the original marker intact. Preserve the file's actual encoding, especially Russian game text, rather than globally converting files.

Report the exact project/configuration/toolset and artifact location when building. Distinguish static investigation, successful compilation, and in-game verification. State which of these actually occurred. Choose targeted existing checks that exercise the changed behavior; whole-solution builds and game launches are task-dependent.
