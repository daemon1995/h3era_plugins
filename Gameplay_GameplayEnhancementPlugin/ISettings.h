#pragma once
#include "pch.h"
class ISettings
{
  public:
    const char *filePath;
    const char *sectionName;
    int scanCode = 0;
    int vKey = VK_MENU;
    LPCSTR settingsVersion = "1.0";

    ISettings(const char *filePath, const char *sectionName) : filePath(filePath), sectionName(sectionName)
    {
    }

  public:
    virtual void reset() = 0;
    virtual BOOL load() = 0;
    virtual BOOL save() = 0;
};
