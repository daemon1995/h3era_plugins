#include "pch.h"
#include "ISettings.h"

ISettings::ISettings(const char* filePath, const char* sectionName)
	:filePath(filePath), sectionName(sectionName)
{
}
