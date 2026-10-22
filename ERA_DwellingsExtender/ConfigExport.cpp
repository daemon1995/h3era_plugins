#include "pch.h"
#include "DwellingsExtender.h"
#include "../headers/Config/ExportProvider.h"
#include <set>

int __stdcall dwellings::DwellingsExtender::CollectConfig(void *, const era_config::Sink *sink)
{
    try
    {
        using namespace configProvider;
        auto &self = Get();
        if (!self.IsActive()) return era_config::Ok;
        Category category{sink, "dwellings", "Dwelling"};
        for (const auto *table : {&self.single, &self.multiple})
        {
            std::set<int> existing;
            if (const auto *game = H3Main::Get())
                for (const auto &object : game->mainSetup.objectLists[table->type]) existing.insert(object.subtype);
            for (int id = 0; id < int(table->names.size()); ++id)
            {
                const auto key = ObjectKey(table->type, id);
                bool configured = false;
                for (const char *field : {"name", "description", "creatures.0", "frame", "image"})
                    configured |= !EraJS::isEmpty((key + "." + field).c_str());
                if (id >= table->originalCount && !existing.count(id) && !configured) continue;
                EraJS::DwellingExportInfo info;
                if (!self.GetExportInfo(table->type, id, &info)) continue;
                Record record{category, Original(id, table->type == SINGLE_TYPE ? "dwellings1" : "dwellings4", 100)};
                record.identity = ObjectIdentity(table->type, id);
                if (info.name && std::strcmp(info.name, H3ObjectName::Get()[table->type])) Put(record.text, key + ".name", info.name);
                ObjectText(record, key);
                PutArray(record.data, key + ".creatures", info.creatures, info.slotCount);
                if (info.frameCount > 0)
                {
                    Put(record.data, key + ".frame", info.frame);
                    Put(record.data, "RMG.settings.dwellings.frames." + std::to_string(table->type), info.frameCount);
                }
                if (info.image && *info.image)
                {
                    Put(record.data, key + ".image", info.image);
                    Put(record.data, key + ".imageFrame", info.imageFrame);
                }

            }
        }
        return era_config::Ok;
    }
    catch (...) { return era_config::ProviderFailed; }
}
void RegisterDwellingConfigProvider()
{ configProvider::Registrar<dwellings::DwellingsExtender::CollectConfig>::Install("dwellings"); }
