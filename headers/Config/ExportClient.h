#pragma once
#include "../EraPluginsAPI/RuntimeServices.h"
#include "../EraPluginsAPI/ConfigExportAPI.h"
#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// JSON-free convenience wrappers. The service owns each snapshot and performs selection,
// counting, merging and serialization. Output callbacks are captured into caller-owned strings.
namespace configExport {
using Selection = era_config::Selection;
using Options = era_config::Options;
inline void Require(const era_config::Api *api, int result) {
    if (result == era_config::Ok) return;
    if (result == era_config::Unavailable) throw std::runtime_error("Configuration data is not ready yet");
    const char *error = api->lastError();
    throw std::runtime_error(error && *error ? error : "Configuration export API failed");
}
struct SnapshotOwner {
    const era_config::Api *api;
    era_config::SnapshotHandle handle = 0;
    SnapshotOwner() : api(era_services::Find<era_config::Api>(era_config::SERVICE)) {
        if (!api) throw std::runtime_error("Configuration export API is unavailable");
        Require(api, api->collectSnapshot(&handle));
    }
    ~SnapshotOwner() { api->releaseSnapshot(handle); }
    SnapshotOwner(const SnapshotOwner &) = delete;
    SnapshotOwner &operator=(const SnapshotOwner &) = delete;
};
struct Output { unsigned kind; std::string fileName, content; };
inline int __stdcall CaptureOutput(void *context, unsigned kind, const char *name, const char *content) {
    try { static_cast<std::vector<Output> *>(context)->push_back({kind, name, content}); return 1; }
    catch (...) { return 0; }
}
struct Category {
    std::shared_ptr<SnapshotOwner> owner;
    unsigned index;
    std::string key, fileStem;
    bool supportsData;
    unsigned recordCount;
    std::array<int, 4> Counts(const Selection &selection = {}, bool ignoreDefaults = true) const {
        std::array<int, 4> counts{};
        Require(owner->api, owner->api->counts(owner->handle, index, &selection, ignoreDefaults, counts.data()));
        return counts;
    }
    std::vector<Output> Outputs(const Selection &selection, const Options &options) const {
        std::vector<Output> outputs;
        era_config::OutputSink sink; sink.context = &outputs; sink.output = CaptureOutput;
        Require(owner->api, owner->api->outputs(owner->handle, index, &selection, &options, &sink));
        return outputs;
    }
};
inline std::vector<Category> Collect() {
    auto owner = std::make_shared<SnapshotOwner>();
    const unsigned count = owner->api->categoryCount(owner->handle);
    std::vector<Category> categories;
    categories.reserve(count);
    for (unsigned index = 0; index < count; ++index) {
        era_config::CategoryInfo info;
        Require(owner->api, owner->api->categoryInfo(owner->handle, index, &info));
        categories.push_back({owner, index, info.key, info.fileStem, info.supportsData != 0, info.recordCount});
    }
    return categories;
}
}
