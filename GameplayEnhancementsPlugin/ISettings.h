#pragma once
class ISettings
{
  public:
    const char *filePath;
    const char *sectionName;
    int scanCode;
    int vKey;
    ISettings(const char *filePath, const char *sectionName);

  public:
    virtual void reset() = 0;
    virtual BOOL load() = 0;
    virtual BOOL save() = 0;
};
