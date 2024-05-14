#include "pch.h"
#include "RMG_ObjectsSettings.h"


bool RMG_Settings::Save()
{

	//H3Ini

	return Era::SaveIni(m_iniPath);

}
bool RMG_ObjectsSettings::WriteParametersToIni(const char* fileName, MapObject& obj)
{
	constexpr int SIZE = 3;
	constexpr const char* keyNames[SIZE] = { "enabled","map","zone" };
	const int* atr[SIZE] = { &obj.enabled, &obj.mapLimit, &obj.zoneLimit };

	sprintf(h3_TextBuffer, "%d_%d_%s", obj.attributes.type, obj.attributes.subtype, obj.attributes.defName.String());
	H3String sectionName = H3String::Format("%d_%d_%s", obj.attributes.type, obj.attributes.subtype, obj.attributes.defName.String());
	//H3Messagebox(H3String(sectionName));
	for (size_t i = 0; i < SIZE; i++)
	{
		if (!Era::WriteStrToIni(keyNames[i], std::to_string(*atr[i]).c_str(), sectionName.String(), fileName))
		return false;
	}

	
	return true;

}


void RMG_ObjectsSettings::ReadParametersFromIni(const char* fileName, MapObject& obj)
{
	
	constexpr int DEFAULT_LIMIT = 999;
	uint16_t readResult;
	H3String sectionName = H3String::Format("%d_%d_%s", obj.attributes.type, obj.attributes.subtype, obj.attributes.defName.String());


	BOOL enabled = true;
	UINT16 mapLimit = DEFAULT_LIMIT;
	UINT16 zoneLimit = DEFAULT_LIMIT;

//	sprintf(h3_TextBuffer, "%d_%d_%s", obj.attributes.type, obj.attributes.subtype, obj.attributes.defName.String());


	printf(h3_TextBuffer, "%d", enabled);
	Era::ReadStrFromIni("enabled", sectionName.String(), fileName, h3_TextBuffer);
	readResult = (atoi(h3_TextBuffer));
	if (enabled != readResult)
		enabled = readResult;

	printf(h3_TextBuffer, "%d", DEFAULT_LIMIT);
	Era::ReadStrFromIni("map", sectionName.String(), fileName, h3_TextBuffer);
	readResult = atoi(h3_TextBuffer);
	if (mapLimit != readResult && readResult< DEFAULT_LIMIT)
		mapLimit = readResult;

	printf(h3_TextBuffer, "%d", DEFAULT_LIMIT);
	Era::ReadStrFromIni("zone", sectionName.String(), fileName, h3_TextBuffer);
	readResult = atoi(h3_TextBuffer);
	if (zoneLimit != readResult && readResult < DEFAULT_LIMIT)
		zoneLimit = readResult;


	// assign values from ini to object
	obj.enabled = enabled;
	obj.mapLimit = mapLimit;
	obj.zoneLimit = zoneLimit;


}

PluginText& PluginText::text()
{
	static PluginText instance;
	return instance;
}

PluginText::PluginText()
{
	zoneLimit = EraJS::read("RMG.text.dlg.zoneLimit");
	mapLimit = EraJS::read("RMG.text.dlg.mapLimit");
	RMG = EraJS::read("RMG.text.dlg.RMG");
	iniError = EraJS::read("RMG.text.dlg.iniError");
}
