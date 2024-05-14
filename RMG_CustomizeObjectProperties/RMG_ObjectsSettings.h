#pragma once


class MapObject;

struct RMG_Settings
{
	

	const char* m_iniPath;

	bool Save();
	

};

struct PluginText
{

	LPTSTR zoneLimit;
	LPTSTR mapLimit;
	LPTSTR RMG;
	LPTSTR iniError;

	static PluginText& text();
	PluginText();
};
struct RMG_ObjectsSettings : public RMG_Settings
{
	const char* defName;

	INT32 zoneLimit;
	INT32 mapLimit;
	BOOL enabled;

	//std::map<std::pair<int, int>, RMG_ObjectsSettings> settings;
	//bool Save(std::map<std::pair<int, int>, RMG_ObjectsSettings>& parameters);
	static void ReadParametersFromIni(const char* fileName, MapObject& mapObject);
	static bool WriteParametersToIni(const char* fileName, MapObject& mapObject);
};

struct RMG_CreatureBanksSettings : public RMG_ObjectsSettings
{


};