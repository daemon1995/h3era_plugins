#include "pch.h"
#include "RMG_ObjectsSettings.h"



//bool RMG_ObjectsSettings::WriteParametersToIni(const char* fileName,const RMGObjectInfo& objectInfo)
//{
//	constexpr int SIZE = 5;
//	constexpr const char* keyNames[SIZE] = { "enabled", "map", "zone", "value", "density" };
//	const int*  atr[SIZE] = { &objectInfo.enabled, &objectInfo.mapLimit, &objectInfo.zoneLimit,&objectInfo.value, &objectInfo.density };
//
//
//	const int zoneType = 0;
//
//	H3String sectionName = H3String::Format("%d_%d_%d", objectInfo.type, objectInfo.subtype, zoneType);// , obj.attributes->defName.String());
//
//
//	for (size_t i = 0; i < SIZE; i++)
//	{
//		if (!Era::WriteStrToIni(keyNames[i], std::to_string(*atr[i]).c_str(), sectionName.String(), fileName))
//			return false;
//	}
//
//
//	return true;
//
//}


//void RMG_ObjectsSettings::ReadParametersFromIni(const char* fileName, RMGObjectInfo& objectInfo)
//{
//
//	const int zoneType = 0;
//	constexpr int SIZE = 5;
//
//	constexpr const char* keyNames[SIZE] = { "enabled", "map", "zone", "value", "density" };
//
//	int* const atr[SIZE] = { &objectInfo.enabled, &objectInfo.mapLimit, &objectInfo.zoneLimit,&objectInfo.value, &objectInfo.density };
//	
//	H3String sectionName = H3String::Format("%d_%d_%d", objectInfo.type, objectInfo.subtype, zoneType);// , obj.attributes->defName.String());
//
//	for (size_t i = 0; i < SIZE; i++)
//	{
//		if (Era::ReadStrFromIni(keyNames[i], sectionName.String(), fileName, h3_TextBuffer))
//			// assign values from ini to object
//			*atr[i] = atoi(h3_TextBuffer);
//	}
//
//}

PluginText& PluginText::text()
{
	static PluginText instance;
	return instance;
}

PluginText::PluginText()
{
	Load();

}
void PluginText::Load()
{
	zoneLimit = EraJS::read("RMG.text.dlg.zoneLimit");
	mapLimit = EraJS::read("RMG.text.dlg.mapLimit");
	RMG = EraJS::read("RMG.text.dlg.RMG");
	iniError = EraJS::read("RMG.text.dlg.iniError");
}
