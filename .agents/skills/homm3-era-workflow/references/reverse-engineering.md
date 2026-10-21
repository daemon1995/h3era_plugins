# Function research, hooks and ERM migration

## Available local evidence

Search the owning plugin, `headers/H3API`, `headers/Era`, `headers/WoG`, `headers/Objects` and `headers/EraPluginsAPI` first. Read actual declaration and implementation rather than using names from an unrelated H3API variant (`H3API_for_copilot` also exists). Research and tools/BattleAudioResearch contain prior analysis; its REPORT.ru.md explicitly distinguishes IDA 8.3 export analysis from runtime verification and warns that some IDA names are incorrect.

Sibling `C:/dev/homm3/plugins/daemon_n/homm3-decomp-decomp-complete-4.0` contains source reconstruction, config, docs and tools rather than a Visual Studio solution. Read its current AGENTS.md when working there. It targets English GOG Complete 4.0 (engine 3.2), HEROES3.EXE base 0x00400000; executable hashes are pinned in its README. This is not automatic proof of an ERA or HD executable's identical addresses.

## Evidence-driven investigation

Identify the exact module, executable version/hash, load base and whether a number is a file offset, RVA or VA. Reuse existing IDA exports, assembly and decompiled source where available. Confirm candidate identities against bytes, callers, parameter usage and structure offsets in the actual target binary. Generated names and pseudocode types are hypotheses until independently supported.

For hooks, establish the actual calling convention, argument locations, return handling, overwritten instruction boundaries, register/stack expectations and any existing patch at the site. Check the local patcher API and nearby working hooks. Preserve stack balance, structure packing, object lifetime and original-call semantics. Do not transfer addresses across retail, ERA, HD, editor or mod DLLs on the strength of a function name alone.

For ERM-to-C++ changes, identify the original triggers, arguments, persistent variables and load/save behavior. Map them to available ERA events/APIs and existing plugin architecture, then check the behavior affected by that mapping. Apply the repository's ERM syntax and disabling rules in SKILL.md to any retained scripts.

## Reconstructed retail code

Use this branch only for research or an explicitly requested reconstruction change. It is a separate toolchain: VC6 SP3 under Wine, with per-TU profiles in config/units.toml. Modern MSVC and clang are not its byte-matching verdict.

The repository documents `homm3 evidence <VA>` for collected Dreamcast/Mac/retail evidence and `homm3 build --fast <TU>` for targeted matching iteration. Verify its CLI/runtime availability before invoking; no homm3/Wine execution was tested while authoring this skill. Consult its AGENTS.md for current matching workflow and additional skills. Do not introduce routine full builds or fabricate an exact-match claim from pseudocode similarity. Preserve the boundary between using that code as evidence for an ERA plugin and actually modifying the reconstruction repository. The latter's publishing instructions do not authorize pushing unrelated plugin work.

## Integration boundary

No working Ghidra or IDA MCP connection was established in this audit. Do not claim direct access to either application's database or UI without a real available integration. Local exports and scripts may be sufficient; otherwise identify the specific missing evidence and use available tools. Keep research results tied to module/hash/address and state separately what the assembly proves, what reconstructed source suggests, and what remains a hypothesis.
