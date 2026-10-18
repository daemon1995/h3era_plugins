#pragma once
#include "framework.h"

namespace editorObjects
{
class ObjectText
{
    static LPCSTR NativeTypeName(int type) noexcept;
    static _LHF_(PaletteHint);
    static _LHF_(MapHint);

  public:
    static void Init();
};
} // namespace editorObjects
