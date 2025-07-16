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
        ArtifactInfo()
        {
        }
        ArtifactInfo(const std::string &n, const std::string &d, const std::string &e)
			: name(n), description(d), event(e) {
		}
    };
    struct MonsterInfo
    {
        std::string singularName;
        std::string pluralName;
        std::string description;
    };
    //struct CreatureBankInfo
    //{
    //    std::string name;
    //    std::string visitText;
    //};
    struct DwellingInfo
    {
        std::string name;
        std::string description;
    };

    static std::string LPCSTR_to_wstring(LPCSTR ansi_str);
    static void WriteJsonFile(const std::string& filePath, nlohmann::json& j);
    static void WriteJsonFile(const std::string& filePath, nlohmann::ordered_json& j);

    static void CreateArtifactsJson(const std::vector<ArtifactInfo> &artifacts);
    static void CreateObjectsJson(const std::vector<std::string> &objectNames,
                                  const std::vector<std::string> &dwelling1Names,
                                  const std::vector<std::string> &dwelling4Names);

    static void CreateMonstersJson(const std::vector<MonsterInfo> &monsters);
    static void CreateCreatureBanksJson();

    static void CreateTownBuildingsJson(const std::vector<std::string> &standardBuildingNames,
                                        const std::vector<DwellingInfo> &dwellingInfos,
                                        const std::vector<DwellingInfo> &dwelling4Infos);
};
