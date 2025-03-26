#pragma once
#include <Windows.h>
#include <string>
#include <vector>
class ExportManager
{
  public:
    struct ArtifactInfo
    {
        std::string name;
        std::string description;
        std::string event;
    };
    struct MonsterInfo
    {
        std::string singularName;
        std::string pluralName;
        std::string description;
    };
    static std::string LPCSTR_to_wstring(LPCSTR ansi_str);
    static void CreateArtifactsJson(const std::vector<ArtifactInfo> &artifacts);
    static void CreateObjectsJson(const std::vector<std::string> &objectNames,
                                  const std::vector<std::string> &dwellingNames);
    static void CreateMonstersJson(const std::vector<MonsterInfo> &monsters);
};
