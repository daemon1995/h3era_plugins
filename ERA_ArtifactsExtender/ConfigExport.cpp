#include "pch.h"
#include "../headers/Config/ExportProvider.h"
#include "../headers/Era/JsonOverridesSchema.hpp"

int __stdcall artifacts::ArtifactsExtender::CollectConfig(void *, const era_config::Sink *sink)
{
    try
    {
        using namespace configProvider;
        auto &self = Get();
        if (!self.ready) return era_config::Ok;
        Category category{sink, "artifacts", "Artifact"};
        for (int id = 0; id < self.tables.count; ++id)
        {
            if (id >= self.tables.baseCount && !self.definitions[id].declared) continue;
            Record record{category, Original(id, "artifacts", 170)};
            record.identity = std::to_string(id);
            const auto key = "era.artifacts." + std::to_string(id);
            EraJS::VisitArtifact(self.tables.setups[id], [&](const char *field, const auto &value) {
                if constexpr (std::is_convertible_v<decltype(value), const char *>) Put(record.text, key + "." + field, value);
                else Put(record.data, key + "." + field, value);
            });
            Put(record.text, key + ".event", self.tables.events[id]);
            const auto &definition = self.definitions[id];
            const char *skills[] = {"attack", "defence", "spellPower", "knowledge"};
            for (int skill = 0; skill < 4; ++skill)
                Put(record.data, key + ".primary." + skills[skill], int(self.tables.primary[id * 4 + skill]));
            if (definition.hasAiValue) Put(record.data, key + ".ai.value", definition.aiValue);
            if (!definition.image.empty())
            {
                Put(record.data, key + ".image", definition.image.c_str());
                Put(record.data, key + ".imageFrame", definition.imageFrame);
            }
            if (!definition.mapDef.empty()) Put(record.data, key + ".mapDef", definition.mapDef.c_str());

        }
        return era_config::Ok;
    }
    catch (...) { return era_config::ProviderFailed; }
}
void RegisterArtifactConfigProvider()
{ configProvider::Registrar<artifacts::ArtifactsExtender::CollectConfig>::Install("artifacts"); }
