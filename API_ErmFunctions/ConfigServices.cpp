#include "framework.h"
#include "ConfigServices.h"
#include "../headers/EraPluginsAPI/RuntimeServices.h"
#include "../headers/EraPluginsAPI/ConfigExportAPI.h"
#include "../headers/Config/ExportSnapshot.h"
#include <map>
#include <memory>
#include <cmath>

namespace config_services {
namespace {
using namespace era_config;
using jsonExport::Json;
using jsonExport::Snapshot;
struct Entry {
    void *context;
    int (__stdcall *collect)(void *, const Sink *) = nullptr;
};
struct State {
    std::map<std::string, Entry> providers;
    std::map<SnapshotHandle, std::unique_ptr<Snapshot>> snapshots;
    SnapshotHandle nextSnapshot = 0;
    std::string error;
    bool collecting = false, ready = false;
};
State &Get() { static auto *state = new State; return *state; }
int Fail(const char *message, int result = ProviderFailed) { Get().error = message; return result; }
struct Guard {
    bool &value;
    explicit Guard(bool &value) : value(value) { value = true; }
    ~Guard() { value = false; }
};
int RegisterEntry(const char *id, const Entry &entry) {
    auto &state = Get();
    if (state.collecting) return Busy;
    const auto found = state.providers.find(id);
    if (found != state.providers.end())
        return found->second.context == entry.context && found->second.collect == entry.collect ? Ok : Invalid;
    state.providers.emplace(id, entry);
    return Ok;
}
int __stdcall Register(const Provider *provider) {
    if (!provider || provider->structSize < sizeof(*provider) || provider->version != 1 ||
        !provider->id || !*provider->id || !provider->collect) return Invalid;
    try { return RegisterEntry(provider->id, Entry{provider->context, provider->collect}); }
    catch (...) { return Fail("Cannot register export provider"); }
}
std::string Utf8Text(const char *text) {
    if (!*text) return {};
    const int size = static_cast<int>(std::strlen(text));
    const unsigned codePage = Era::GetCodePage();
    const int length = MultiByteToWideChar(codePage, 0, text, size, nullptr, 0);
    if (length <= 0) throw std::runtime_error("Cannot decode exported text");
    std::wstring wide(length, 0);
    if (MultiByteToWideChar(codePage, 0, text, size, &wide[0], length) != length)
        throw std::runtime_error("Cannot decode exported text");
    const int count = WideCharToMultiByte(CP_UTF8, 0, wide.data(), length, nullptr, 0, nullptr, nullptr);
    if (count <= 0) throw std::runtime_error("Cannot encode exported text");
    std::string result(count, 0);
    if (WideCharToMultiByte(CP_UTF8, 0, wide.data(), length, &result[0], count, nullptr, nullptr) != count)
        throw std::runtime_error("Cannot encode exported text");
    return result;
}
struct Pending {
    std::string category, stem, identity;
    int original;
    Json fields[3] = {Json::object(), Json::object(), Json::object()};
};
struct TypedContext { std::vector<Pending> records; std::string error; };
int TypedFail(TypedContext &context, const char *message, int result = Invalid) {
    context.error = message;
    return result;
}
int __stdcall BeginRecord(void *opaque, const char *category, const char *stem,
    const char *identity, int original, RecordHandle *handle) {
    auto &context = *static_cast<TypedContext *>(opaque);
    if (handle) *handle = 0;
    try {
        if (!handle || !jsonExport::SafeName(category) || !jsonExport::SafeName(stem) || !identity || !*identity)
            return TypedFail(context, "Invalid typed export identity");
        if (context.records.size() >= UINT32_MAX) return TypedFail(context, "Too many export records");
        context.records.push_back({category, stem, identity, original});
        *handle = static_cast<RecordHandle>(context.records.size());
        return Ok;
    } catch (const std::exception &error) { return TypedFail(context, error.what(), ProviderFailed); }
    catch (...) { return TypedFail(context, "Cannot create export record", ProviderFailed); }
}
int __stdcall Field(void *opaque, RecordHandle handle, unsigned section, const char *path, const Value *value) {
    auto &context = *static_cast<TypedContext *>(opaque);
    try {
        if (!handle || handle > context.records.size() || section > Defaults || !value || value->structSize < sizeof(*value) ||
            !path || !*path || *path == '.' || path[std::strlen(path) - 1] == '.' || std::strstr(path, ".."))
            return TypedFail(context, "Invalid typed export field");
        Json converted;
        switch (value->type) {
        case Signed: converted = value->integer; break;
        case Unsigned: converted = value->unsignedInteger; break;
        case Real:
            if (!std::isfinite(value->real)) return TypedFail(context, "Non-finite export number");
            converted = value->real; break;
        case Utf8: case LocalText:
            if (!value->string) return TypedFail(context, "Null export string");
            converted = value->type == LocalText ? Utf8Text(value->string) : std::string(value->string); break;
        case Array: converted = Json::array(); break;
        case Object: converted = Json::object(); break;
        case Boolean: converted = value->integer != 0; break;
        case Null: converted = nullptr; break;
        default: return TypedFail(context, "Unknown export value type");
        }
        jsonExport::At(context.records[handle - 1].fields[section], path) = std::move(converted);
        return Ok;
    } catch (const std::exception &error) { return TypedFail(context, error.what(), ProviderFailed); }
    catch (...) { return TypedFail(context, "Cannot build export field", ProviderFailed); }
}
bool CollectProvider(const Entry &entry, Snapshot &snapshot) {
    TypedContext context;
    Sink sink; sink.context = &context; sink.beginRecord = BeginRecord; sink.field = Field;
    if (entry.collect(entry.context, &sink) != Ok || !context.error.empty()) {
        snapshot.error = context.error.empty() ? "Export provider failed" : context.error;
        return false;
    }
    // Stage the entire provider without serializing and reparsing between DLLs.
    for (const auto &record : context.records)
        if (!jsonExport::AcceptObjects(snapshot, record.category.c_str(), record.stem.c_str(), record.identity.c_str(),
            record.original, record.fields[Text], record.fields[Data], record.fields[Defaults])) return false;
    return true;
}
int __stdcall CollectSnapshot(SnapshotHandle *output) {
    if (!output) return Invalid;
    *output = 0;
    auto &state = Get();
    if (!state.ready) return Unavailable;
    if (state.collecting) return Busy;
    Guard guard(state.collecting);
    try {
        auto snapshot = std::make_unique<Snapshot>();
        for (const auto &provider : state.providers) {
            const auto &entry = provider.second;
            const bool success = CollectProvider(entry, *snapshot);
            if (!success || !snapshot->error.empty())
                return Fail(snapshot->error.empty() ? "Export provider failed" : snapshot->error.c_str());
        }
        do { ++state.nextSnapshot; } while (!state.nextSnapshot || state.snapshots.count(state.nextSnapshot));
        state.snapshots.emplace(state.nextSnapshot, std::move(snapshot));
        *output = state.nextSnapshot;
        return Ok;
    } catch (const std::exception &error) { return Fail(error.what()); }
    catch (...) { return Fail("Cannot collect export snapshot"); }
}
Snapshot *FindSnapshot(SnapshotHandle handle) {
    auto &snapshots = Get().snapshots;
    const auto found = snapshots.find(handle);
    return found == snapshots.end() ? nullptr : found->second.get();
}
int __stdcall ReleaseSnapshot(SnapshotHandle handle) { return Get().snapshots.erase(handle) ? Ok : Invalid; }
unsigned __stdcall CategoryCount(SnapshotHandle handle) {
    const auto *snapshot = FindSnapshot(handle);
    return snapshot ? static_cast<unsigned>(snapshot->categories.size()) : 0;
}
int __stdcall CategoryInfoAt(SnapshotHandle handle, unsigned index, CategoryInfo *info) {
    const auto *snapshot = FindSnapshot(handle);
    if (!snapshot || index >= snapshot->categories.size() || !info || info->structSize < sizeof(*info)) return Invalid;
    const auto &category = snapshot->categories[index];
    info->key = category.key.c_str(); info->fileStem = category.fileStem.c_str();
    info->supportsData = category.supportsData; info->recordCount = static_cast<unsigned>(category.records.size());
    return Ok;
}
jsonExport::Selection Convert(const Selection &value) {
    return {value.original != 0, value.mods != 0, value.text != 0, value.data != 0};
}
int __stdcall Counts(SnapshotHandle handle, unsigned index, const Selection *selection, int ignoreDefaults, int *counts) {
    const auto *snapshot = FindSnapshot(handle);
    if (!snapshot || index >= snapshot->categories.size() || !selection || !counts) return Invalid;
    const auto values = snapshot->categories[index].Counts(Convert(*selection), ignoreDefaults != 0);
    std::copy(values.begin(), values.end(), counts);
    return Ok;
}
int __stdcall Outputs(SnapshotHandle handle, unsigned index, const Selection *selection,
    const Options *options, const OutputSink *sink) {
    const auto *snapshot = FindSnapshot(handle);
    if (!snapshot || index >= snapshot->categories.size() || !selection || !options ||
        !sink || sink->structSize < sizeof(*sink) || !sink->output) return Invalid;
    try {
        const auto outputs = snapshot->categories[index].Outputs(Convert(*selection),
            {options->mergeTextAndData != 0, options->ignoreRmgDefaults != 0});
        for (const auto &output : outputs) {
            const auto content = output.json.dump(4);
            if (!sink->output(sink->context, static_cast<unsigned>(output.kind),
                output.fileName.c_str(), content.c_str())) return Fail("Export output consumer failed");
        }
        return Ok;
    } catch (const std::exception &error) { return Fail(error.what()); }
    catch (...) { return Fail("Cannot serialize export output"); }
}
const char *__stdcall LastError() { return Get().error.c_str(); }
const Api api{sizeof(api), 1, Register, CollectSnapshot, ReleaseSnapshot,
    CategoryCount, CategoryInfoAt, Counts, Outputs, LastError};
void __stdcall Ready(Era::TEvent *) { Era::FireEvent(READY_EVENT, nullptr, 0); }
void __stdcall DataReady(Era::TEvent *) { Get().ready = true; Era::FireEvent(DATA_READY_EVENT, nullptr, 0); }
}
void Initialize() {
    if (era_services::Publish(SERVICE, &api)) {
        Era::RegisterHandler(Ready, "OnAfterWog");
        Era::RegisterHandler(DataReady, "OnAfterCreateWindow");
    }
}
}
