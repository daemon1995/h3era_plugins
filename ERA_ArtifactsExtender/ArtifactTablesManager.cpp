#include "pch.h"
#include <algorithm>

namespace artifacts
{
namespace
{
template <class T> DWORD Address(T *value) { return reinterpret_cast<DWORD>(value); }
void PatchPointers(std::initializer_list<DWORD> operands, DWORD value)
{
    for (DWORD operand : operands) _PI->WriteDword(operand, value);
}
}

bool ArtifactTablesManager::Compatible() const
{
    const int base = IntAt(0x717020);
    const DWORD bonuses = DwordAt(0x4E2D26);
    const DWORD end = DwordAt(0x4E2D3D);
    const DWORD eventBytes = DwordAt(0x49DD90);
    return CompatibleLayout(base, bonuses, end, eventBytes,
           Era::GetRealAddr(reinterpret_cast<void *>(0x7B6DA0)) == reinterpret_cast<void *>(0x7B6DA0),
           ByteAt(0x44CB31) == 0xBB && ByteAt(0x44CD1C) == 0x81 &&
           ByteAt(0x7324BA) == 0x81 && ByteAt(0x752D80) == 0x81);
}

bool ArtifactTablesManager::CompatibleLayout(int base, DWORD bonuses, DWORD end, DWORD eventBytes,
                                           bool unrelocated, bool supportedCode)
{
    // A prior relocator (Emerald, etc.) owns both its tables and their semantics.
    return base >= NATIVE_MAP_ARTIFACTS && base <= CAPACITY && unrelocated && supportedCode &&
           end > bonuses && (end - bonuses) % 4 == 0 &&
           (end - bonuses) / 4 <= NATIVE_COMBINATION_BITS &&
           eventBytes % sizeof(LPCSTR) == 0 &&
           eventBytes >= NATIVE_MAP_ARTIFACTS * sizeof(LPCSTR) &&
           eventBytes <= static_cast<DWORD>(base) * sizeof(LPCSTR);
}

void ArtifactTablesManager::Install(int artifactCount)
{
    baseCount = IntAt(0x717020);
    count = artifactCount;
    auto *oldSetups = H3ArtifactSetup::Get();
    auto *oldPrimary = reinterpret_cast<signed char *>(DwordAt(0x4E2D26));
    primarySpan = (DwordAt(0x4E2D3D) - Address(oldPrimary)) / 4;
    eventSpan = DwordAt(0x49DD90) / sizeof(LPCSTR);
    auto *oldBackup = *reinterpret_cast<H3ArtifactSetup **>(0x752D99);
    auto *oldTextVars = *reinterpret_cast<TextZVars **>(0x732675);
    auto *oldEvents = *reinterpret_cast<LPCSTR **>(0x49F51E);

    setups = new H3ArtifactSetup[CAPACITY];
    backup = new H3ArtifactSetup[CAPACITY];
    primary = new signed char[CAPACITY * 4]();
    textVars = new TextZVars[CAPACITY]();
    // Native ARTEVENT loader writes its first entry at [base + 4].
    auto *eventStorage = new LPCSTR[CAPACITY + 1]();
    events = eventStorage + 1;
    std::copy_n(oldSetups, baseCount, setups);
    std::copy_n(oldBackup, baseCount, backup);
    std::copy_n(oldPrimary, primarySpan * 4, primary);
    std::copy_n(oldTextVars, baseCount, textVars);
    std::copy_n(oldEvents, eventSpan, events);
    std::fill(events + eventSpan, events + CAPACITY, h3_NullString);

    const DWORD setup = Address(setups);
    PatchPointers({0x44CB32, 0x44CD1E, 0x44CD6C, 0x660B68, 0x754A42}, setup);
    PatchPointers({0x44CCDF}, setup + 0x1C);
    PatchPointers({0x44CCFA}, setup + 0x1D);
    PatchPointers({0x716F8D, 0x716F9E, 0x717146}, setup + 0x18);
    PatchPointers({0x716FAE, 0x716FBF, 0x717117}, setup + 0x14);
    // These bounds are table bounds, not bit-mask bounds.
    PatchPointers({0x716F7F, 0x717020, 0x7324BD, 0x732849}, count);

    const DWORD bonus = Address(primary);
    PatchPointers({0x4E2D26, 0x4E2DF1, 0x4E2E95, 0x4E2F7C}, bonus);
    // Preserve native combination enumeration while moving the entire bonus table.
    PatchPointers({0x4E2D3D, 0x4E2DD7, 0x4E2EAC, 0x4E2F41}, bonus + 4 * primarySpan);

    PatchPointers({0x751E40, 0x752D99, 0x752DD3, 0x7326FC}, Address(backup));
    PatchPointers({0x751EAD, 0x732795}, Address(backup) + 0x10);
    PatchPointers({0x752D83, 0x752DC0}, CAPACITY * sizeof(H3ArtifactSetup));
    PatchPointers({0x752DF1}, CAPACITY);
    // Correct displacement offset is +3 for every SIB instruction below.
    PatchPointers({0x75122F, 0x751C97, 0x751DE1, 0x752DFD, 0x732675, 0x7326A2}, Address(textVars));
    PatchPointers({0x7512C3, 0x751D2B, 0x751E4C, 0x752E0B, 0x73270D, 0x73273A},
                  Address(textVars) + 4);
    // Keep native WoG save/loader counts intact. Extra IDs have a named ERA section.

    PatchPointers({0x49F51E, 0x49F5DA, 0x49F2E7}, Address(events));
    PatchPointers({0x49DD9A}, Address(eventStorage));

    // The CRT constructor has already run: move existing 16-byte vector headers.
    auto *oldAI = reinterpret_cast<DWORD *>(DwordAt(0x4338DD));
    aiSpan = (DwordAt(0x433959) - Address(oldAI) - 8) / 16;
    aiVectors = new DWORD[CAPACITY * 4]();
    std::copy_n(oldAI, aiSpan * 4, aiVectors);
    const DWORD ai = Address(aiVectors);
    PatchPointers({0x432472, 0x43249D, 0x4338DD, 0x43412B}, ai);
    PatchPointers({0x433949, 0x434125, 0x434597}, ai + 8);
    // Combo evaluation still reads only the original five DWORD mask.
    PatchPointers({0x433959, 0x4339B3}, ai + aiSpan * 16 + 8);
    PatchPointers({0x4345E6}, ai + CAPACITY * 16 + 8);
    PatchPointers({0x43246B, 0x432496}, CAPACITY);

    Era::RedirectMemoryBlock(oldSetups, baseCount * sizeof(H3ArtifactSetup), setups);
    // WoG has direct references to its extended table in addition to the native one.
    if (oldSetups != reinterpret_cast<H3ArtifactSetup *>(0x7B6DA0))
        Era::RedirectMemoryBlock(reinterpret_cast<void *>(0x7B6DA0), baseCount * sizeof(H3ArtifactSetup), setups);
    Era::RedirectMemoryBlock(oldPrimary, primarySpan * 4, primary);
    Era::RedirectMemoryBlock(oldBackup, baseCount * sizeof(H3ArtifactSetup), backup);
    Era::RedirectMemoryBlock(oldTextVars, baseCount * sizeof(TextZVars), textVars);
    Era::RedirectMemoryBlock(oldEvents, eventSpan * sizeof(LPCSTR), events);
    Era::RedirectMemoryBlock(oldAI, aiSpan * 16, aiVectors);
    Log("Tables relocated: base=%d, count=%d, capacity=%d, combo primary span=%d, AI span=%d, event span=%d",
        baseCount, count, CAPACITY, primarySpan, aiSpan, eventSpan);
}

void ArtifactTablesManager::BuildAI(const std::array<ArtifactData, CAPACITY> &definitions)
{
    // Native stream: artifact ID, [property, arguments...], -1; final -100.
    // Preserve every original non-primary property, including special-artifact logic.
    const int widths[24] = {1,1,1,1,1,1,1,1,1,1,1,2,1,1,0,0,2,2,1,1,0,0,0,0};
    const int *source = reinterpret_cast<const int *>(DwordAt(0x43410F));
    std::array<bool, CAPACITY> seen{};
    std::vector<int> result;
    auto appendPrimary = [&](int id)
    {
        const auto *p = primary + id * 4;
        const int values[] = {p[0] + p[1], p[2], p[3]};
        for (int type = 0; type < 3; ++type)
            if (values[type]) { result.push_back(type); result.push_back(values[type]); }
    };
    // The known native stream is 2548 bytes including its terminator.
    const int *end = source + 637;
    while (source < end && *source >= 0)
    {
        const int id = *source++;
        if (id >= count) { Log("Invalid native AI artifact ID"); return; }
        seen[id] = true;
        result.push_back(id);
        const auto &changed = definitions[id].primaryChanged;
        const bool replace = changed[0] || changed[1] || changed[2] || changed[3];
        while (source < end && *source >= 0)
        {
            const int type = *source++;
            if (type > 23 || source + widths[type] > end) { Log("Invalid native AI stream"); return; }
            if (!replace || type > 2)
            {
                result.push_back(type);
                result.insert(result.end(), source, source + widths[type]);
            }
            source += widths[type];
        }
        if (source == end || *source++ != -1) { Log("Invalid native AI record end"); return; }
        if (replace) appendPrimary(id);
        result.push_back(-1);
    }
    if (source == end || *source != -100) { Log("Native AI terminator missing"); return; }
    for (int id = 0; id < count; ++id)
    {
        if (seen[id]) continue;
        const auto *p = primary + 4 * id;
        if (!(p[0] || p[1] || p[2] || p[3])) continue;
        result.push_back(id);
        appendPrimary(id);
        result.push_back(-1);
    }
    result.push_back(-100);
    aiProperties.swap(result); // Stable until process exit; published only once.
    PatchPointers({0x434107, 0x43410F}, Address(aiProperties.data()));
    // WinMain calls the native setup at 0x4EE021, before the all-TXT hook at
    // 0x4EE036. Rebuild its property objects now that JSON bonuses are final.
    CDECL_0(int, 0x434100);
}
} // namespace artifacts
