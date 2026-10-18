#pragma once
#include "framework.h"

namespace editorObjects
{
class ObjectDefinitions
{
    static std::vector<std::string> ReadProperties();
    static H3TextFile *__stdcall LoadObjectPrototypes(HiHook *h, LPCSTR fileName);

  public:
    static const std::vector<std::string> &Properties();
    static void Init();
};
} // namespace editorObjects
