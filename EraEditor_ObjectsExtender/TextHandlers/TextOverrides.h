#pragma once
#include "../framework.h"

namespace editorObjects
{
// Bind only editor-owned string slots. Numeric/resource configuration is startup data.
class TextOverrides
{
  public:
    static void Init();
    static void Apply(LPCSTR &slot, LPCSTR key);
    static void Forget(const void *base, size_t bytes);
};
}
