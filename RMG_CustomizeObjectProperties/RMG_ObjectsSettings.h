#pragma once



struct RMG_Settings
{


	const char* m_iniPath;


};

struct PluginText
{

	const char* zoneLimit;
	const char* mapLimit;
	const char* RMG;
	const char* iniError;

	static PluginText& text();
	void Load();
	PluginText();
};
struct RMG_ObjectsSettings : public RMG_Settings
{

	//std::map<std::pair<int, int>, RMG_ObjectsSettings> settings;
	//bool Save(std::map<std::pair<int, int>, RMG_ObjectsSettings>& parameters);
	//static void ReadParametersFromIni(const char* fileName, RMGObjectInfo &mapObject);
	//static bool WriteParametersToIni(const char* fileName, const RMGObjectInfo& mapObject);
};

