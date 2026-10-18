#include "pch.h"
#include "ArtifactSave.h"
#include <algorithm>
#include <limits>

namespace artifacts
{

ArtifactsExtender &ArtifactsExtender::Get()
{
    // Process lifetime: patched engine pointers remain valid through native atexit.
    static auto *instance = new ArtifactsExtender;
    return *instance;
}

void ArtifactsExtender::Discover()
{
    tables.count = IntAt(0x717020);
    // Legacy mod-name arrays are declarations; the first mod in VFS priority wins.
    for (const auto &mod : modList::GetEraModList(1))
    {
        for (int index = 0; index < CAPACITY; ++index)
        {
            const std::string key = mod + ".artifacts." + std::to_string(index);
            bool found = false;
            const char *text = EraJS::read(key + ".id", found);
            if (!found) break;
            int id;
            if (!EraJS::ParseNumber(text, id) || id < 0 || id >= CAPACITY)
            {
                Log("Rejected artifact ID: %s.id = %s", key.c_str(), text);
                continue;
            }
            auto &definition = definitions[id];
            if (!definition.legacyKey.empty())
                Log("Duplicate artifact %d ignored: %s (using %s)", id, key.c_str(), definition.legacyKey.c_str());
            else
            {
                definition.legacyKey = key;
                definition.declared = true;
            }
            tables.count = (std::max)(tables.count, id + 1);
        }
    }
    for (int id = 0; id < CAPACITY; ++id)
    {
        if (ArtifactData::HasDefinition("era.artifacts." + std::to_string(id)))
        {
            definitions[id].declared = true;
            tables.count = (std::max)(tables.count, id + 1);
        }
    }
}

void ArtifactsExtender::Init()
{
    if (tables.setups) return;
    if (!tables.Compatible())
    {
        Log("Incompatible executable/table owner. Artifact extender was not installed.");
        return;
    }
    Discover();
    tables.Install(tables.count);
    _PI->WriteHiHook(0x4EE036, CDECL_, AfterReadAllTxtFiles);
    _PI->WriteHiHook(0x55C9C0, THISCALL_, LoadDef);
    _PI->WriteHiHook(0x4C9190, THISCALL_, RandomArtifact);
    _PI->WriteHiHook(0x714EA7, CDECL_, ArtDisabled);
    _PI->WriteHiHook(0x714F23, CDECL_, SetArtDisabled);
    _PI->WriteHiHook(0x4336C0, FASTCALL_, AIValue);
    _PI->WriteLoHook(0x4C07E6, MarkMapArtifactUsed);
    _PI->WriteHiHook(0x4C2110, THISCALL_, LoadMap);
    _PI->WriteHiHook(0x5063F0, THISCALL_, LoadObjects);
    for (DWORD address : {0x4C4BB4, 0x5701C1, 0x574AAF, 0x574FD1})
        _PI->WriteLoHook(address, DisableQuestArtifact);
    Era::RegisterHandler(OnSavegameWrite, "OnSavegameWrite");
    Era::RegisterHandler(OnBeforeLoadGame, "OnBeforeLoadGame");
    Era::RegisterHandler(OnSavegameRead, "OnSavegameRead");
    Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");
}

void ArtifactsExtender::AfterTextLoaded()
{
    if (ready) return;
    Log("Applying artifact JSON");
    for (int id = 0; id < tables.count; ++id)
    {
        auto &setup = tables.setups[id];
        if (id >= tables.baseCount)
        {
            setup = H3ArtifactSetup{};
            setup.disabled = !definitions[id].declared;
            if (definitions[id].declared)
                setup.name = Era::ToStaticStr(("Artifact #" + std::to_string(id)).c_str());
        }
        // Save the links produced by the actual combination table. JSON fields
        // processed by another plugin must not create unbacked combination links.
        definitions[id].Apply(setup, tables.primary + id * 4, tables.events[id], id);
        if (id >= tables.baseCount)
        {
            setup.comboArtifactId = eCombinationArtifacts::NONE;
            setup.partOfComboArtifactId = eCombinationArtifacts::NONE;
        }
        if (!tables.events[id] || !*tables.events[id])
            tables.events[id] = setup.name ? setup.name : h3_NullString;
        tables.backup[id] = setup;
    }
    tables.BuildAI(definitions);
    Log("Artifact AI properties rebuilt");
    ready = true;
    ResetExtraState();
    if (loadedObjectSetup) AddMapObjects(loadedObjectSetup);
    // The load hook also handles later resource reloads. This reference is balanced.
    if (auto *def = H3LoadedDef::Load("artifact.def"))
    {
        if (def->groups && def->groupsCount > 0 && def->groups[0])
            Log("Artifact DEF ready: %d frames", def->groups[0]->count);
        def->Dereference();
    }
    Log("JSON applied; active artifact range is 0..%d", tables.count - 1);
}

int __stdcall ArtifactsExtender::AfterReadAllTxtFiles(HiHook *hook)
{
    const int result = CDECL_0(int, hook->GetDefaultFunc());
    // This function returns a directory/status integer, not a TXT-load boolean.
    Get().AfterTextLoaded();
    return result;
}

H3LoadedDef *__stdcall ArtifactsExtender::LoadDef(HiHook *hook, LPCSTR name)
{
    const bool artifact = name && (!_stricmp(name, "artifact.def") || !_stricmp(name, "artifacts.def"));
    auto *def = THISCALL_1(H3LoadedDef *, hook->GetDefaultFunc(), name);
    if (def && artifact)
    {
        auto &self = Get();
        self.images.Apply(def, self.tables.count, self.definitions);
    }
    return def;
}

void ArtifactsExtender::ResetExtraState()
{
    mapDisabled.fill(0);
    mapUsed.fill(0);
    if (!ready) return;
    for (int id = tables.baseCount; id < CAPACITY; ++id)
    {
        tables.setups[id] = tables.backup[id];
        tables.textVars[id] = {};
    }
}

bool ArtifactsExtender::IsDisabled(H3Main *main, int id) const
{
    if (id < 0 || id >= tables.count) return true;
    return id < NATIVE_MAP_ARTIFACTS ? !main || main->artifactsAllowed[id] != 0 : mapDisabled[id] != 0;
}

unsigned char &ArtifactsExtender::Used(H3Main *main, int id)
{
    if (id < NATIVE_MAP_ARTIFACTS)
        return reinterpret_cast<unsigned char *>(main->randomArtifacts)[id];
    return mapUsed[id];
}

int __stdcall ArtifactsExtender::ArtDisabled(HiHook *hook, int id)
{
    auto &self = Get();
    if (id < 0 || id >= self.tables.count) return 1;
    if (id < NATIVE_MAP_ARTIFACTS) return CDECL_1(int, hook->GetDefaultFunc(), id);
    return self.mapDisabled[id] != 0;
}

void __stdcall ArtifactsExtender::SetArtDisabled(HiHook *hook, int id, int disabled)
{
    auto &self = Get();
    if (id < 0 || id >= self.tables.count) return;
    if (id < NATIVE_MAP_ARTIFACTS)
        CDECL_2(void, hook->GetDefaultFunc(), id, disabled);
    else self.mapDisabled[id] = disabled != 0;
}

int __stdcall ArtifactsExtender::RandomArtifact(HiHook *, H3Main *main, int level)
{
    auto &self = Get();
    if (!main) return -1;
    std::array<int, CAPACITY> candidates;
    auto eligible = [&](int id, int mask)
    {
        const auto &setup = self.tables.setups[id];
        return !setup.disabled && (static_cast<int>(setup.type) & mask) && !self.IsDisabled(main, id);
    };
    int mask = level;
    for (int pass = 0; pass < 2; ++pass)
    {
        int count = 0;
        for (int id = 0; id < self.tables.count; ++id)
            if (eligible(id, mask) && !self.Used(main, id)) candidates[count++] = id;
        if (!count)
        {
            for (int id = 0; id < self.tables.count; ++id)
                if (eligible(id, mask)) { self.Used(main, id) = 0; candidates[count++] = id; }
        }
        if (count)
        {
            const int id = candidates[H3Random::Rand(0, count - 1)];
            self.Used(main, id) = 1;
            return id;
        }
        mask = 30; // Native fallback to any normal artifact class.
    }
    // Native code loops forever if every class is disabled.
    Log("No eligible artifact for random class mask %d", level);
    return -1;
}

_LHF_(ArtifactsExtender::MarkMapArtifactUsed)
{
    auto &self = Get();
    const int id = c->eax;
    if (id >= 0 && id < self.tables.count)
        self.Used(reinterpret_cast<H3Main *>(c->ecx), id) = 1;
    c->return_address = 0x4C07EE;
    return NO_EXEC_DEFAULT;
}

int __stdcall ArtifactsExtender::AIValue(HiHook *hook, H3Hero *hero, int mode, int id, int subtype, int context)
{
    auto &self = Get();
    if (id < 0 || id >= self.tables.count || !hero) return 0;
    // EDX is a live input in the retail code (saved at 0x4336D1), in addition
    // to ECX and three stack arguments (ret 0Ch). Preserve it through the hook.
    const int original = FASTCALL_5(int, hook->GetDefaultFunc(), hero, mode, id, subtype, context);
    const auto &definition = self.definitions[id];
    const int extra = definition.hasAiValue ? definition.aiValue :
                      id >= self.tables.baseCount ? self.tables.setups[id].cost / 5 : 0;
    return static_cast<int>((std::min)(static_cast<long long>((std::numeric_limits<int>::max)()),
                                     static_cast<long long>(original) + extra));
}

int __stdcall ArtifactsExtender::LoadMap(HiHook *hook, H3Main *main, int file)
{
    Get().ResetExtraState();
    return THISCALL_2(int, hook->GetDefaultFunc(), main, file);
}

void __stdcall ArtifactsExtender::LoadObjects(HiHook *hook, H3MainSetup *setup)
{
    THISCALL_1(void, hook->GetDefaultFunc(), setup);
    auto &self = Get();
    self.loadedObjectSetup = setup;
    // The initial call precedes AfterTextLoaded; mapDef must be read first.
    if (self.ready) self.AddMapObjects(setup);
}

void ArtifactsExtender::AddMapObjects(H3MainSetup *setup)
{
    auto &objects = setup->objectLists[eObject::ARTIFACT];
    if (!objects.Count()) return;
    // Own a copy before native insertion can move the vector.
    H3ObjectAttributes prototype = objects[0];
    for (const auto &object : objects)
        if (object.subtype == 7) { prototype = object; break; }
    for (int id = tables.baseCount; id < tables.count; ++id)
    {
        if (!definitions[id].declared) continue;
        bool exists = false;
        for (const auto &object : objects)
            if (object.subtype == id) { exists = true; break; }
        if (exists) continue;
        H3ObjectAttributes object = prototype;
        object.subtype = id;
        object.defIndex = static_cast<UINT16>(-1);
        if (!definitions[id].mapDef.empty()) object.defName = definitions[id].mapDef.c_str();
        // The engine leaves spare vector storage unconstructed. H3API::Push
        // assigns into it, which makes H3String read an uninitialized pointer.
        setup->AddObjectAttribute(&object);
    }
    Log("Artifact map object descriptions initialized");
}

_LHF_(ArtifactsExtender::DisableQuestArtifact)
{
    const DWORD address = h->GetAddress();
    const int id = address == 0x4C4BB4 || address == 0x574AAF ? c->ecx :
                   address == 0x5701C1 ? c->edx : c->eax;
    if (id >= 0 && id < NATIVE_MAP_ARTIFACTS) return EXEC_DEFAULT;
    auto &self = Get();
    if (id >= 0 && id < self.tables.count) self.mapDisabled[id] = 1;
    c->return_address = address + 8;
    return NO_EXEC_DEFAULT;
}

_ERH_(ArtifactsExtender::OnBeforeLoadGame) { Get().ResetExtraState(); }

_ERH_(ArtifactsExtender::OnSavegameWrite)
{
    auto &self = Get();
    if (!self.ready) return;
    SaveHeader header;
    header.count = self.tables.count;
    Era::WriteSavegameSection(sizeof(header), &header, SAVE_SECTION);
    for (int id = NATIVE_MAP_ARTIFACTS; id < self.tables.count; ++id)
    {
        const auto &a = self.tables.setups[id];
        const auto &z = self.tables.textVars[id];
        SavedArtifact record = {a.cost, static_cast<int>(a.position), static_cast<int>(a.type),
                                static_cast<int>(a.comboArtifactId), static_cast<int>(a.partOfComboArtifactId),
                                z.name, z.description, static_cast<unsigned char>(a.disabled),
                                static_cast<unsigned char>(a.hasSpell), self.mapDisabled[id], self.mapUsed[id]};
        Era::WriteSavegameSection(sizeof(record), &record, SAVE_SECTION);
    }
}

_ERH_(ArtifactsExtender::OnSavegameRead)
{
    auto &self = Get();
    if (!self.ready) return;
    SaveHeader header;
    const int bytes = Era::ReadSavegameSection(sizeof(header), &header, SAVE_SECTION);
    if (!bytes) return; // Older saves keep the JSON defaults set before loading.
    if (bytes != sizeof(header) || header.magic != 0x58415245 || header.version != 1 ||
        header.count < NATIVE_MAP_ARTIFACTS || header.count > CAPACITY)
    {
        Log("Invalid extender save header");
        return;
    }
    std::vector<SavedArtifact> records(header.count - NATIVE_MAP_ARTIFACTS);
    for (size_t index = 0; index < records.size(); ++index)
        if (Era::ReadSavegameSection(sizeof(SavedArtifact), &records[index], SAVE_SECTION) != sizeof(SavedArtifact) ||
            !ValidSaved(records[index], index + NATIVE_MAP_ARTIFACTS >= static_cast<size_t>(self.tables.baseCount)))
        {
            Log("Invalid/truncated extender save section; no extra state applied");
            return;
        }
    if (header.count > static_cast<DWORD>(self.tables.count))
        Era::NotifyError("ERA_ArtifactsExtender: this save requires more artifact IDs. Restore its artifact mods.");
    for (int id = NATIVE_MAP_ARTIFACTS; id < static_cast<int>(header.count); ++id)
    {
        if (id >= self.tables.count) break;
        const auto &r = records[id - NATIVE_MAP_ARTIFACTS];
        self.mapDisabled[id] = r.mapDisabled;
        self.mapUsed[id] = r.mapUsed;
        if (id < self.tables.baseCount) continue; // Native WoG already restored these properties.
        auto &a = self.tables.setups[id];
        a.cost = r.cost;
        a.position = static_cast<eArtifactPositions>(r.position);
        a.type = static_cast<eArtifactType>(r.type);
        // New combinations remain deferred; only plain extra artifacts are persisted.
        a.comboArtifactId = eCombinationArtifacts::NONE;
        a.partOfComboArtifactId = eCombinationArtifacts::NONE;
        a.disabled = r.disabled;
        a.hasSpell = r.hasSpell;
        self.tables.textVars[id] = {r.nameVar, r.descriptionVar};
        if (r.nameVar) a.name = Era::z[r.nameVar];
        if (r.descriptionVar) a.description = Era::z[r.descriptionVar];
    }
}

_ERH_(ArtifactsExtender::OnAfterReloadLanguageData)
{
    auto &self = Get();
    if (!self.ready) return;
    for (int id = 0; id < self.tables.count; ++id)
    {
        auto &a = self.tables.setups[id];
        auto &back = self.tables.backup[id];
        const auto &z = self.tables.textVars[id];
        const bool name = !z.name && a.name == back.name;
        const bool description = !z.description && a.description == back.description;
        self.definitions[id].ReloadText(back, self.tables.events[id], true, true, id);
        if (name) a.name = back.name;
        if (description) a.description = back.description;
    }
}
} // namespace artifacts
