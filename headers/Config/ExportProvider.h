#pragma once
#include "../EraPluginsAPI/RuntimeServices.h"
#include "../EraPluginsAPI/ConfigExportAPI.h"
#include "../Objects/RmgSettingsExport.h"
#include <limits>
#include <string>
#include <type_traits>

namespace configProvider {
struct Failure {};
inline void Check(int result) { if (result != era_config::Ok) throw Failure{}; }
struct Category {
    const era_config::Sink *sink;
    const char *key, *fileStem;
    unsigned sequence = 0;
    Category(const era_config::Sink *sink, const char *key, const char *stem) : sink(sink), key(key), fileStem(stem) {
        if (!sink || sink->structSize < sizeof(*sink) || !sink->beginRecord || !sink->field) throw Failure{};
    }
};
struct Record;
struct Fields {
    Record *owner;
    unsigned section;
    void Set(const std::string &path, const era_config::Value &value) const;
};
struct Record {
    Category &category;
    bool original;
    std::string identity;
    Fields text{this, era_config::Text}, data{this, era_config::Data}, rmgDefaults{this, era_config::Defaults};
    era_config::RecordHandle handle = 0;
    explicit Record(Category &category, bool original = true) : category(category), original(original) {}
    Record(const Record &) = delete;
    Record &operator=(const Record &) = delete;
    void Set(unsigned section, const std::string &path, const era_config::Value &value) {
        const auto *sink = category.sink;
        if (!handle) {
            if (identity.empty()) identity = std::to_string(category.sequence);
            ++category.sequence;
            Check(sink->beginRecord(sink->context, category.key, category.fileStem, identity.c_str(), original, &handle));
        }
        Check(sink->field(sink->context, handle, section, path.c_str(), &value));
    }
};
inline void Fields::Set(const std::string &path, const era_config::Value &value) const { owner->Set(section, path, value); }
inline void StartArray(const Fields &root, const std::string &path) {
    era_config::Value value; value.type = era_config::Array; root.Set(path, value);
}
inline void Put(const Fields &root, const std::string &path, const std::string &input) {
    era_config::Value value; value.type = era_config::LocalText; value.string = input.c_str(); root.Set(path, value);
}
template<class T> void Put(const Fields &root, const std::string &path, const T &input) {
    era_config::Value value;
    if constexpr (std::is_convertible_v<T, const char *>) {
        if (!input) return;
        value.type = era_config::LocalText; value.string = input;
    } else if constexpr (std::is_array_v<T>) {
        StartArray(root, path);
        unsigned index = 0;
        for (const auto &entry : input) Put(root, path + "." + std::to_string(index++), entry);
        return;
    } else if constexpr (std::is_enum_v<T>) {
        Put(root, path, static_cast<std::underlying_type_t<T>>(input));
        return;
    } else if constexpr (std::is_floating_point_v<T>) {
        value.type = era_config::Real; value.real = input;
    } else if constexpr (std::is_unsigned_v<T>) {
        value.type = era_config::Unsigned; value.unsignedInteger = input;
    } else {
        value.type = era_config::Signed; value.integer = static_cast<std::int64_t>(input);
    }
    root.Set(path, value);
}
template<class T> void PutArray(const Fields &root, const std::string &path, const T *values, unsigned count) {
    StartArray(root, path);
    for (unsigned i = 0; i < count; ++i) Put(root, path + "." + std::to_string(i), values[i]);
}
inline void PutRmgOverrides(const Fields &root, const std::string &path, unsigned mask, const int *values) {
    constexpr const char *fields[] = {"enabled", "map", "zone", "value", "density"};
    for (int field = 0; field < rmgExport::FIELD_COUNT; ++field)
        if (mask & (1U << field)) Put(root, path + "." + fields[field], values[field]);
}
inline int OriginalLimit(const std::string &key, int fallback) {
    bool found = false;
    int value = EraJS::readInt("era.configExport.maximumId." + key, found);
    if (!found) value = EraJS::readInt("era.locale.dlg.maximumId." + key, found);
    return found && value >= -1 && value < (std::numeric_limits<int>::max)() ? value : fallback;
}
inline bool Original(int id, const char *key, int fallback) { return id <= OriginalLimit(key, fallback); }
inline std::string ObjectKey(int type, int subtype) { return "RMG.objectGeneration." + std::to_string(type) + "." + std::to_string(subtype); }
inline std::string ObjectIdentity(int type, int subtype) { return std::to_string(type) + ":" + std::to_string(subtype); }
inline void CopyText(const Fields &root, const std::string &key) {
    bool found; const char *value = EraJS::read(key, found); if (found) Put(root, key, value);
}
inline void CopyNumber(const Fields &root, const std::string &key) {
    bool found; const char *text = EraJS::read(key, found); long long value;
    if (found && EraJS::ParseNumber(text, value)) Put(root, key, value);
}
inline void ObjectText(Record &record, const std::string &key) {
    CopyText(record.text, key + ".name"); CopyText(record.text, key + ".description");
    for (const char *field : {"info", "hint", "visit", "visited", "cannotVisit", "victory", "plundered", "level"})
        CopyText(record.text, key + ".text." + field);
}
using Collector = int (__stdcall *)(void *, const era_config::Sink *);
template<Collector Collect> struct Registrar {
    static const char *&Id() { static const char *id = nullptr; return id; }
    static void __stdcall Register(Era::TEvent *) {
        const auto *api = era_services::Find<era_config::Api>(era_config::SERVICE);
        if (!api) return;
        era_config::Provider provider; provider.id = Id(); provider.collect = Collect;
        if (api->registerProvider(&provider) != era_config::Ok && Era::WriteLog)
            Era::WriteLog(Id(), "Configuration export", "Provider registration failed.");
    }
    static void Install(const char *id) {
        Id() = id; Register(nullptr);
        Era::RegisterHandler(Register, era_config::READY_EVENT);
        Era::RegisterHandler(Register, "OnAfterWog");
    }
};
}
