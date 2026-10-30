#pragma once
#include "framework.h"

namespace mrart
{
inline H3LoadedPcx16* ScaledPcx(LPCSTR name, int width, int height)
{
    if (!name || !*name || width <= 0 || height <= 0) return nullptr;
    H3Pcx16Loader source(name);
    if (!source.Get()) return nullptr;
    auto canvas = resized::H3LoadedPcx16Resized::Create(width, height);
    if (canvas)
        canvas->DrawResizedBicubic(source.Get(), source->width, source->height, 0, 0, width, height);
    return canvas;
}
inline H3DlgPcx16* CreateScaledPcx(H3Dlg& dlg, int x, int y, int width, int height, int id, LPCSTR name)
{
    auto canvas = ScaledPcx(name, width, height);
    if (!canvas) return nullptr;
    auto item = dlg.CreatePcx16(x, y, width, height, id, nullptr);
    if (item) item->SetPcx(canvas);
    else canvas->Dereference();
    return item;
}
inline void ReplaceScaledPcx(H3DlgPcx16* item, LPCSTR name)
{
    if (!item) return;
    auto canvas = ScaledPcx(name, item->GetWidth(), item->GetHeight());
    if (!canvas) return;
    if (auto old = item->GetPcx()) old->Dereference();
    item->SetPcx(canvas);
}
}
