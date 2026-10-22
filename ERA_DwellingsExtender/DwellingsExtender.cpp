#include "DwellingsExtender.h"
#include "../headers/Era/GameJsonTables.hpp"
#include "../headers/Era/ObjectText.hpp"
#include "../headers/EraPluginsAPI/ObjectDataAPI.h"

#pragma comment(linker, "/EXPORT:GetDwellingExportInfo=_GetDwellingExportInfo@12")
DllExport BOOL __stdcall GetDwellingExportInfo(int type, int subtype, EraJS::DwellingExportInfo *info)
{
    return dwellings::DwellingsExtender::Get().GetExportInfo(type, subtype, info);
}

#pragma comment(linker, "/EXPORT:InitializeDwellings=_InitializeDwellings@8")
DllExport BOOL __stdcall InitializeDwellings(const INT16 *limits, unsigned count)
{
    if (!limits || count < h3::limits::OBJECTS) return FALSE;
    auto &instance = dwellings::DwellingsExtender::Get();
    instance.Initialize(limits);
    return instance.IsActive();
}

namespace dwellings
{
namespace
{
constexpr LPCSTR SAVE_SECTION = "RMG_CustomizeObjectProperties.Dwellings";
constexpr UINT32 SAVE_VERSION = 1;
constexpr UINT32 MAX_SAVED_DWELLINGS = 252 * 252 * 2;

int CreatureCount() noexcept
{
    return IntAt(0x4A1657);
}
int ReadInt(int type, int subtype, LPCSTR field, bool &found)
{
    return EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d.%s", type, subtype, field).String(), found);
}

} // namespace

DwellingsExtender::DwellingsExtender()
    : _pi(globalPatcher->CreateInstance("EraPlugin.DwellingsExtender.daemon_n"))
{
}

DwellingsExtender &DwellingsExtender::Get()
{
    static auto *instance = new DwellingsExtender;
    return *instance;
}

void DwellingsExtender::Install()
{
    if (installed) return;
    installed = true;
    static const era_objects::DwellingApi api{sizeof(api), 1, GetDwellingExportInfo, InitializeDwellings};
    era_services::Publish(era_objects::DWELLINGS, &api);
    _pi->WriteHiHook(0x4EE01C, THISCALL_, AfterLoadObjects);
}

void __stdcall DwellingsExtender::AfterLoadObjects(HiHook *h, const H3MainSetup *setup)
{
    // RMG merges its definitions while the default object loader runs.
    THISCALL_1(void, h->GetDefaultFunc(), setup);
    std::array<INT16, h3::limits::OBJECTS> limits;
    limits.fill(-1);
    for (int type : {SINGLE_TYPE, MULTIPLE_TYPE})
        for (const auto &object : setup->objectLists[type])
            if (object_config::IsObjectSubtype(object.subtype))
                limits[type] = (std::max)(limits[type], static_cast<INT16>(object.subtype));
    Get().Initialize(limits.data());
}

DwellingsExtender::Table *DwellingsExtender::FindTable(int type) noexcept
{
    return const_cast<Table *>(static_cast<const DwellingsExtender *>(this)->FindTable(type));
}
const DwellingsExtender::Table *DwellingsExtender::FindTable(int type) const noexcept
{
    switch (type)
    {
    case eObject::CREATURE_GENERATOR1:
        return &single;
    case eObject::CREATURE_GENERATOR4:
        return &multiple;
    default:
        return nullptr;
    }
}

const int *DwellingsExtender::FindCreatures(int type, int subtype) const noexcept
{
    const auto *table = FindTable(type);
    if (!table || subtype < 0 || size_t(subtype) >= table->names.size())
        return nullptr;
    return &table->creatures[subtype * table->slotCount];
}

BOOL DwellingsExtender::GetExportInfo(int type, int subtype, EraJS::DwellingExportInfo *info) const noexcept
{
    const auto *table = FindTable(type);
    if (!m_isInited || !info || info->structSize < offsetof(EraJS::DwellingExportInfo, image) || !table ||
        subtype < 0 || size_t(subtype) >= table->names.size())
        return FALSE;
    EraJS::DwellingExportInfo result;
    result.structSize = info->structSize;
    result.slotCount = table->slotCount;
    std::copy_n(&table->creatures[subtype * table->slotCount], table->slotCount, result.creatures);
    result.frame = table->frames[subtype];
    result.frameCount = table->frameCount;
    result.name = table->names[subtype];
    result.image = table->portraits[subtype].image.c_str();
    result.imageFrame = table->portraits[subtype].imageFrame;
    if (portraits.Ready())
    {
        result.overviewFrame = table->overviewFrames[subtype];
        result.overviewFrameCount = portraits.layout.Count();
    }
    // Older locale-manager DLLs still receive exactly their original record.
    libc::memcpy(info, &result, (std::min)(size_t(info->structSize), sizeof(result)));
    return TRUE;
}

void DwellingsExtender::LoadTable(Table &table, int maxSubtype)
{
    int nameCount = 0;
    const int *originalCreatures = nullptr;
    const LPCSTR *originalNames = nullptr;
    // Read the operands after WoG/other startup patches, rather than assuming SoD's 80/2.
    switch (table.type)
    {
    case eObject::CREATURE_GENERATOR1:
        nameCount = EraJS::GameTables::Dwelling1Capacity();
        originalCreatures = reinterpret_cast<const int *>(DwordAt(0x534CEA));
        originalNames = H3DwellingNames1::Get();
        table.originalCount = (std::max)(nameCount, IntAt(0x5032BC) + 1);
        table.originalCount = (std::max)(table.originalCount, IntAt(0x539C77));
        break;
    case eObject::CREATURE_GENERATOR4:
        nameCount = EraJS::GameTables::Dwelling4Capacity();
        originalCreatures = reinterpret_cast<const int *>(DwordAt(0x4B85B7));
        originalNames = H3DwellingNames4::Get();
        table.originalCount = nameCount;
        break;
    default:
        return;
    }
    // Preserve native entries and extend to the largest loaded object subtype.
    const int size = (std::max)(table.originalCount, maxSubtype + 1);
    table.creatures.assign(size * table.slotCount, -1);
    table.names.assign(size, H3ObjectName::Get()[table.type]);
    table.frames.assign(size, 0);
    table.portraits.resize(size);

    std::copy(originalCreatures, originalCreatures + table.originalCount * table.slotCount, table.creatures.begin());
    std::copy(originalNames, originalNames + nameCount, table.names.begin());
    table.originalNames = table.names;
    table.publishedNames = table.names;

    bool found = false;
    const int frames = EraJS::readInt(
        H3String::Format("RMG.settings.dwellings.frames.%d", table.type).String(), found);
    if (found && frames > 0 && frames <= size)
        table.frameCount = frames;

    for (int subtype = 0; subtype < size; ++subtype)
    {
        CreatureSlots creatures = EMPTY_CREATURES;
        for (int slot = 0; slot < table.slotCount; ++slot)
            creatures[slot] = table.creatures[subtype * table.slotCount + slot];
        ReadCreatures(creatures, table.slotCount, CreatureCount(), [&](int slot, int &creature) {
                creature = EraJS::readInt(H3String::Format("RMG.objectGeneration.%d.%d.creatures.%d",
                                                          table.type, subtype, slot).String(), found);
                return found;
            });
        for (int slot = 0; slot < table.slotCount; ++slot)
            table.creatures[subtype * table.slotCount + slot] = creatures[slot];

        // This is a SOURCE frame within the type's original block, not the
        // runtime ID used to group the kingdom overview.
        if (subtype < table.frameCount)
            table.frames[subtype] = subtype;
        const int frame = ReadInt(table.type, subtype, "frame", found);
        if (found && frame >= 0 && frame < table.frameCount)
            table.frames[subtype] = frame;
        auto &portrait = table.portraits[subtype];
        const char *image = EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.image", table.type, subtype).String(), found);
        if (found && image) portrait.image = image;
        const int imageFrame = ReadInt(table.type, subtype, "imageFrame", found);
        if (found && imageFrame >= 0 && imageFrame <= 65535) portrait.imageFrame = imageFrame;
    }
}

void DwellingsExtender::PreparePortraits(H3LoadedDef *resource)
{
    const int sourceSpecial = single.frameCount + multiple.frameCount;
    const int prefix = portraits.UseResource(resource, (std::max)(originalSpecialFrame + 6, sourceSpecial + 6));
    portraits.layout.Init(prefix);
    for (auto *table : {&single, &multiple})
    {
        const int nativeOffset = table == &single ? 0 : single.nativeFrameCount;
        const int sourceOffset = table == &single ? 0 : single.frameCount;
        table->overviewFrames.resize(table->frames.size());
        for (size_t subtype = 0; subtype < table->frames.size(); ++subtype)
        {
            auto &source = table->portraits[subtype];
            source.frame = sourceOffset + table->frames[subtype];
            const int nativeFrame = int(subtype) < table->nativeFrameCount ? nativeOffset + int(subtype) : -1;
            table->overviewFrames[subtype] = portraits.layout.Add(table->type, int(subtype), nativeFrame, source);
        }
    }
    // Legacy custom block counts describe an existing source DEF. Keep native
    // special-object IDs stable while reading their pictures from that layout.
    const int offsets[] = {0, 1, 2, 3, 5};
    for (size_t i = 0; i < specialFrames.size(); ++i)
        portraits.SetSpecialSource(specialFrames[i], sourceSpecial + offsets[i]);
    portraits.Finish();
}

void DwellingsExtender::ReloadNames()
{
    for (auto *table : {&single, &multiple})
        for (size_t subtype = 0; subtype < table->names.size(); ++subtype)
        {
            // Preserve a refreshed native translation supplied by another text handler.
            if (table->names[subtype] != table->publishedNames[subtype])
                table->originalNames[subtype] = table->names[subtype];
            LPCSTR name = EraJS::ReadObjectName(table->type, static_cast<int>(subtype));
            table->names[subtype] = table->originalNames[subtype];
            if (name)
                table->names[subtype] = name;
            table->publishedNames[subtype] = table->names[subtype];
        }
}

void DwellingsExtender::Initialize(const INT16 *maxSubtypes)
{
    if (m_isInited)
        return;
    // Dwellings.dll replaces this instruction with a JMP and owns the same tables/hooks.
    // Leave the other extender in charge, without a dialog or overlapping hooks.
    if (ByteAt(0x521A7F) != 0x83 || ByteAt(0x521A80) != 0xF8)
        return;
    single.type = eObject::CREATURE_GENERATOR1;
    single.slotCount = 1;
    // CMP EAX,imm8: the immediate is one byte, not a DWORD.
    single.frameCount = ByteAt(0x521A81);
    single.nativeFrameCount = single.frameCount;
    multiple.type = eObject::CREATURE_GENERATOR4;
    multiple.slotCount = 4;
    originalSpecialFrame = IntAt(0x520D1D);
    multiple.frameCount = originalSpecialFrame - single.frameCount;
    multiple.nativeFrameCount = multiple.frameCount;
    specialFrames = {{IntAt(0x520D1D), IntAt(0x520EFF), IntAt(0x520F58), IntAt(0x520CAA), IntAt(0x52103D)}};
    LoadTable(single, maxSubtypes[single.type]);
    LoadTable(multiple, maxSubtypes[multiple.type]);
    ReloadNames();
    // Native DEF palettes need video channel masks, unavailable during object loading.
    // Prepare portraits in Kingdom_LoadDef after the screen format is initialized.
    CreatePatches();
}

LPCSTR DwellingsExtender::GetObjectName(int type, int subtype) const noexcept
{
    if (subtype < 0)
        return h3_NullString;
    const auto *table = FindTable(type);
    if (!table)
        return h3_NullString;
    if (size_t(subtype) < table->names.size())
        return table->names[subtype];
    // Before initialization (or after a conflicting extender was detected), use native names.
    if (!m_isInited)
    {
        switch (type)
        {
        case eObject::CREATURE_GENERATOR1:
            if (subtype < EraJS::GameTables::Dwelling1Capacity())
                return P_DwellingNames1[subtype];
            break;
        case eObject::CREATURE_GENERATOR4:
            if (subtype < EraJS::GameTables::Dwelling4Capacity())
                return P_DwellingNames4[subtype];
            break;
        }
    }
    return H3ObjectName::Get()[type];
}

int __stdcall DwellingsExtender::Rmg_GetDwellingValue(HiHook *h, const H3RmgObjectGenerator *generator,
                                                    const H3RmgZoneGenerator *zone,
                                                    const H3RmgRandomMapGenerator *rmg)
{
    const auto *creatures = Get().FindCreatures(generator->type, generator->subtype);
    if (!creatures)
        return -1;

    switch (generator->type)
    {
    case eObject::CREATURE_GENERATOR1:
    {
        const int creature = creatures[0];
        if (!IsCreature(creature, CreatureCount()))
            return -1;
        auto &aiValue = P_CreatureInformation[creature].aiValue;
        const int storedValue = aiValue;
        // RMG publishes its current settings into the generator itself.
        // Native dwelling generators need not populate their generic value field.
        // Only RMG's adapter publishes an override there; standalone keeps native AI.
        if (GetModuleHandleA("RMG_CustomizeObjectProperties.era") && generator->value >= 0)
            aiValue = generator->value;
        const int value = THISCALL_3(int, h->GetDefaultFunc(), generator, zone, rmg);
        aiValue = storedValue;
        return value;
    }
    case eObject::CREATURE_GENERATOR4:
    {
        int result = -1;
        const int totalTowns = rmg->townsCount;
        for (int slot = 0; slot < 4; ++slot)
        {
            const int creature = creatures[slot];
            if (!IsCreature(creature, CreatureCount()))
                continue;
            const auto &info = P_CreatureInformation[creature];
            const int town = info.town;
            // Preserve the existing RMG restriction on the first slot's town.
            if (slot == 0 && town != zone->townType2)
                return -1;
            if (!info.aiValue)
                continue;
            int value = info.aiValue * info.grow;
            if (totalTowns && town != eTown::NEUTRAL)
            {
                const int matchingTowns = rmg->townsCountByType[town];
                if (matchingTowns)
                    value += value * matchingTowns / totalTowns;
                result += value + ((totalTowns * info.aiValue) >> 1);
            }
            if (town == eTown::NEUTRAL)
                result += value;
        }
        return result >> 2;
    }
    default:
        return -1;
    }
}

int DwellingsExtender::SubtypeAtPosition(const H3Generator &dwelling) noexcept
{
    const auto *game = P_Game->Get();
    if (dwelling.x >= game->mainSetup.mapSize || dwelling.y >= game->mainSetup.mapSize ||
        dwelling.z >= (game->mainSetup.hasUnderground ? 2 : 1))
        return static_cast<UINT8>(dwelling.subtype);
    const auto *item = P_AdventureManager->GetMapItem(dwelling.x, dwelling.y, dwelling.z);
    // A hero standing on a dwelling retains the tile's original subtype.
    if (item)
        return static_cast<UINT16>(item->objectSubtype);
    return static_cast<UINT8>(dwelling.subtype);
}

_LHF_(DwellingsExtender::Map_ReadDwellingSubtype)
{
    const int subtype = IntAt(c->edx + c->ecx * 4 + 0x3C);
    Get().pendingSubtype = subtype;
    c->ecx = subtype;
    ByteAt(c->ebp - 0x67) = static_cast<BYTE>(subtype);
    c->return_address = 0x4FF3FC;
    return NO_EXEC_DEFAULT;
}
_LHF_(DwellingsExtender::Rmg_ReadDwellingSubtype)
{
    Get().pendingSubtype = IntAt(c->ebp - 8);
    return EXEC_DEFAULT;
}
_LHF_(DwellingsExtender::Erm_ReadDwellingSubtype)
{
    Get().pendingSubtype = IntAt(c->ebp + 0x18);
    return EXEC_DEFAULT;
}
_LHF_(DwellingsExtender::Dwelling_SelectCreatures)
{
    auto &instance = Get();
    const auto *dwelling = c->Esi<H3Generator *>();
    int subtype = instance.pendingSubtype;
    if (subtype < 0)
        subtype = SubtypeAtPosition(*dwelling);
    instance.pendingSubtype = -1;
    const auto *table = instance.FindTable(dwelling->type);
    if (!table || subtype < 0 || size_t(subtype) >= table->names.size())
        return EXEC_DEFAULT;
    // Skip both signed-byte subtype reads; resume before the game's growth/guards setup.
    c->ecx = reinterpret_cast<int>(&table->creatures[subtype * table->slotCount]);
    c->eax = table->slotCount;
    c->return_address = 0x4B85C0;
    return NO_EXEC_DEFAULT;
}
_LHF_(DwellingsExtender::Adventure_GetDwellingName)
{
    switch (h->GetAddress())
    {
    case 0x40B504:
        c->return_address = 0x40B50B;
        break;
    case 0x40B546:
        c->return_address = 0x40B54D;
        break;
    default:
        return EXEC_DEFAULT;
    }
    c->eax = SubtypeAtPosition(*c->Eax<H3Generator *>());
    // The hook also displaces CMP ECX,-1. Resume after it, with its branch condition.
    c->flags.ZF = c->ecx == -1;
    return NO_EXEC_DEFAULT;
}
_LHF_(DwellingsExtender::Adventure_GetDwellingClickName)
{
    switch (h->GetAddress())
    {
    case 0x413E59:
        c->return_address = 0x413E5E;
        break;
    case 0x413E9F:
        c->return_address = 0x413EA4;
        break;
    default:
        return EXEC_DEFAULT;
    }
    const auto *dwelling = reinterpret_cast<H3Generator *>(c->ecx + c->eax);
    c->eax = SubtypeAtPosition(*dwelling);
    return NO_EXEC_DEFAULT;
}
_LHF_(DwellingsExtender::Kingdom_GetDwellingFrame)
{
    const auto *dwelling = reinterpret_cast<H3Generator *>(c->ecx + c->edi);
    const auto &instance = Get();
    const auto *table = instance.FindTable(dwelling->type);
    const int subtype = SubtypeAtPosition(*dwelling);
    c->edx = 0;
    if (instance.portraits.Ready() && table && subtype >= 0 && size_t(subtype) < table->overviewFrames.size())
        c->edx = table->overviewFrames[subtype];
    // A LoHook preserves TEST EAX,EAX at 520DB1, needed by the following JNZ.
    return NO_EXEC_DEFAULT;
}
_LHF_(DwellingsExtender::Kingdom_GetDwellingHint)
{
    const auto &instance = Get();
    const int frame = c->eax;
    if (const auto *object = instance.portraits.layout.Find(frame))
    {
        c->edi = reinterpret_cast<int>(instance.GetObjectName(object->type, object->subtype));
        c->return_address = 0x521C31;
        return NO_EXEC_DEFAULT;
    }
    // The original switch still expects SoD's 82/83/84/85/87, even in WoG.
    c->eax = VanillaSpecialHint(frame, instance.specialFrames);
    c->return_address = 0x521AC1;
    return NO_EXEC_DEFAULT;
}

H3LoadedDef *__stdcall DwellingsExtender::Kingdom_LoadDef(HiHook *h, LPCSTR name)
{
    auto *def = THISCALL_1(H3LoadedDef *, h->GetDefaultFunc(), name);
    auto &instance = Get();
    // The native constructor loads the atlas after the video channel masks are ready.
    const char *overviewName = reinterpret_cast<const char *>(DwordAt(0x52025D));
    if (def && instance.m_isInited && DwellingPortraits::IsResource(name) && !_stricmp(name, overviewName) &&
        instance.portraits.GraphicsReady())
    {
        if (!instance.portraits.Ready()) instance.PreparePortraits(def);
        else if (instance.portraits.UsesResource(name)) instance.portraits.Apply(def);
    }
    return def;
}
_LHF_(DwellingsExtender::Kingdom_CompareSpecialFrame)
{
    int operand = 0;
    int equalAddress = 0;
    switch (h->GetAddress())
    {
    case 0x520CE9:
        operand = 0x520D1D;
        equalAddress = 0x520CF8;
        c->return_address = 0x520CEE;
        break;
    case 0x520ECF:
        operand = 0x520EFF;
        equalAddress = 0x520EDE;
        c->return_address = 0x520ED4;
        break;
    case 0x520F2C:
        operand = 0x520F58;
        equalAddress = 0x520F3B;
        c->return_address = 0x520F31;
        break;
    case 0x520C76:
        operand = 0x520CAA;
        equalAddress = 0x520C85;
        c->return_address = 0x520C7B;
        break;
    case 0x520FF9:
        operand = 0x52103D;
        equalAddress = 0x521008;
        c->return_address = 0x520FFE;
        break;
    default:
        return EXEC_DEFAULT;
    }
    if (IntAt(c->eax) == IntAt(operand))
        c->return_address = equalAddress;
    return NO_EXEC_DEFAULT;
}

void DwellingsExtender::CreatePatches()
{
    const DWORD names1 = reinterpret_cast<DWORD>(single.names.data());
    const DWORD names4 = reinterpret_cast<DWORD>(multiple.names.data());
    const DWORD creatures1 = reinterpret_cast<DWORD>(single.creatures.data());
    const DWORD creatures4 = reinterpret_cast<DWORD>(multiple.creatures.data());
    for (const int address : {0x40B521, 0x413E78, 0x4A1553, 0x40B517, 0x413E6D})
        _pi->WriteDword(address, names1);
    for (const int address : {0x40B559, 0x40B579, 0x413EB3, 0x413ED3, 0x4A155F})
        _pi->WriteDword(address, names4);
    for (const int address : {0x418754, 0x4B85A3, 0x5032D8, 0x521A87, 0x534CEA})
        _pi->WriteDword(address, creatures1);
    _pi->WriteDword(0x4B85B7, creatures4);
    // Reverse lookup when resolving a random dwelling's creature to a subtype.
    _pi->WriteDword(0x5032BC, int(single.names.size()) - 1);
    _pi->WriteDword(0x5032C8, creatures1 + (single.names.size() - 1) * sizeof(int));

    // Preserve WoG's native generator count before the host creates generators.
    if (IntAt(0x539C77) == 80)
        _pi->WriteDword(0x539C77, 101);
    _pi->WriteHiHook(0x534CE0, THISCALL_, Rmg_GetDwellingValue);

    _pi->WriteLoHook(0x4FF3F5, Map_ReadDwellingSubtype);
    _pi->WriteLoHook(0x5033B1, Rmg_ReadDwellingSubtype);
    _pi->WriteLoHook(0x71343D, Erm_ReadDwellingSubtype);
    _pi->WriteLoHook(0x4B859A, Dwelling_SelectCreatures);
    _pi->WriteLoHook(0x40B504, Adventure_GetDwellingName);
    _pi->WriteLoHook(0x40B546, Adventure_GetDwellingName);
    _pi->WriteLoHook(0x413E59, Adventure_GetDwellingClickName);
    _pi->WriteLoHook(0x413E9F, Adventure_GetDwellingClickName);
    _pi->WriteLoHook(0x520DB3, Kingdom_GetDwellingFrame);
    _pi->WriteLoHook(0x521A7F, Kingdom_GetDwellingHint);
    _pi->WriteHiHook(0x55C9C0, THISCALL_, Kingdom_LoadDef);
    // SoD overwrites the dwelling's type with 17 here. WoG already NOPs it;
    // preserve type 20 on both paths before resolving the full map subtype.
    _pi->WriteHexPatch(0x520DA0, "90 90 90");

    // Special-object IDs are never shifted when new dwelling portraits are added.
    for (const int address : {0x520CE9, 0x520ECF, 0x520F2C, 0x520C76, 0x520FF9})
        _pi->WriteLoHook(address, Kingdom_CompareSpecialFrame);

    Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");
    Era::RegisterHandler(OnSavegameWrite, "OnSavegameWrite");
    Era::RegisterHandler(OnSavegameRead, "OnSavegameRead");
    Era::RegisterHandler(OnAfterLoadGame, "OnAfterLoadGame");
    Era::RegisterHandler(OnGameLeave, "OnGameLeave");
    m_isInited = TRUE;
    static const era_services::Ownership owner;
    era_services::Publish(era_services::DWELLING_OWNER, &owner);
}

void __stdcall DwellingsExtender::OnAfterReloadLanguageData(Era::TEvent *)
{
    Get().ReloadNames();
}
void __stdcall DwellingsExtender::OnSavegameWrite(Era::TEvent *)
{
    static_assert(sizeof(SavedDwelling) == 20, "Dwelling save format must use 32-bit IDs.");
    const auto &dwellings = P_Game->dwellings;
    std::vector<SavedDwelling> records;
    records.reserve(dwellings.Size());
    for (const auto &dwelling : dwellings)
    {
        SavedDwelling record{H3Position::Pack(dwelling.x, dwelling.y, dwelling.z), EMPTY_CREATURES};
        std::copy(dwelling.creatureTypes, dwelling.creatureTypes + 4, record.creatures.begin());
        records.push_back(record);
    }
    UINT32 header[] = {SAVE_VERSION, UINT32(records.size())};
    Era::WriteSavegameSection(sizeof(header), header, SAVE_SECTION);
    if (!records.empty())
        Era::WriteSavegameSection(int(records.size() * sizeof(SavedDwelling)), records.data(), SAVE_SECTION);
}
void __stdcall DwellingsExtender::OnSavegameRead(Era::TEvent *)
{
    auto &instance = Get();
    instance.loadedDwellings.clear();
    instance.restoreLoadedDwellings = false;
    UINT32 header[2]{};
    if (Era::ReadSavegameSection(sizeof(header), header, SAVE_SECTION) != sizeof(header) ||
        header[0] != SAVE_VERSION || header[1] > MAX_SAVED_DWELLINGS)
        return;
    instance.loadedDwellings.resize(header[1]);
    const int bytes = int(instance.loadedDwellings.size() * sizeof(SavedDwelling));
    if (bytes && Era::ReadSavegameSection(bytes, instance.loadedDwellings.data(), SAVE_SECTION) != bytes)
    {
        instance.loadedDwellings.clear();
        return;
    }
    instance.restoreLoadedDwellings = true;
}
void __stdcall DwellingsExtender::OnAfterLoadGame(Era::TEvent *)
{
    auto &instance = Get();
    auto &dwellings = P_Game->dwellings;
    if (instance.restoreLoadedDwellings && instance.loadedDwellings.size() == dwellings.Size())
    {
        bool valid = true;
        for (size_t i = 0; i < dwellings.Size(); ++i)
        {
            const auto &dwelling = dwellings[i];
            const auto &record = instance.loadedDwellings[i];
            valid = valid && IsValidSavedDwelling(record, H3Position::Pack(dwelling.x, dwelling.y, dwelling.z),
                                                  CreatureCount());
        }
        if (valid)
            for (size_t i = 0; i < dwellings.Size(); ++i)
                std::copy(instance.loadedDwellings[i].creatures.begin(), instance.loadedDwellings[i].creatures.end(),
                          dwellings[i].creatureTypes);
    }
    instance.loadedDwellings.clear();
    instance.restoreLoadedDwellings = false;
    instance.pendingSubtype = -1;
}
void __stdcall DwellingsExtender::OnGameLeave(Era::TEvent *)
{
    auto &instance = Get();
    instance.loadedDwellings.clear();
    instance.restoreLoadedDwellings = false;
    instance.pendingSubtype = -1;
}
} // namespace dwellings
