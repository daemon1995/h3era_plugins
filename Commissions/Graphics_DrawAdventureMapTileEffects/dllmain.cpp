// dllmain.cpp : Определяет точку входа для приложения DLL.
#define _H3API_PLUGINS_
#include "framework.h"

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;

namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}

struct
{
    RECT m_mapView;
    INT currentFrameToDraw = 0;
    INT framesAmount = 0;
    H3LoadedDef *rainDef = nullptr;

} settings;

_LHF_(AdvMgr_BeforeObjectsDraw)
{

    if (++settings.currentFrameToDraw >= settings.framesAmount)
    {
        settings.currentFrameToDraw = 0;
    }

    return EXEC_DEFAULT;
}

void __stdcall AdvMgr_TileObjectDraw(HiHook *h, H3AdventureManager *adv, int mapX, int mapY, int mapZ, int screenX,
                                     int screenY)
{

    THISCALL_6(void, h->GetDefaultFunc(), adv, mapX, mapY, mapZ, screenX, screenY);
    const int mapSize = *P_MapSize;
    if (mapX >= 0 && mapY >= 0 && mapX < mapSize && mapY < mapSize && settings.framesAmount)
    {
        auto mapItem = P_Game->GetMapItem(H3Position(mapX, mapY, mapZ));
        if (mapItem && mapItem->land == eTerrain::ROCK)
        {
            return;
        }

        constexpr int TILE_WIDTH = 32;
        constexpr int TEMP_PCX_WIDTH = TILE_WIDTH;
        constexpr int TEMP_PCX_HEIGHT = TILE_WIDTH;

        auto drawBuffer = P_WindowManager->GetDrawBuffer();

        constexpr int objectWidth = 1 * TILE_WIDTH;
        const int outOfWidthBorder = (TEMP_PCX_WIDTH - objectWidth) >> 1;

        int destPcxX = screenX * TILE_WIDTH + adv->screenDrawOffset.x; // -outOfWidthBorder;

        const int additionalYOffset = 0; // instance->settings.drawObjectHint[currentItem->objectType].yOffset;

        int destPcxY = screenY * TILE_WIDTH + adv->screenDrawOffset.y; // -TEMP_PCX_HEIGHT + additionalYOffset;

        int drawWidth = TEMP_PCX_WIDTH;
        int drawHeight = TEMP_PCX_WIDTH;
        // adjust left border draw

        const auto &m_mapView = settings.m_mapView;

        UINT srcX = 0;

        if (destPcxX < m_mapView.left)
        {
            srcX = m_mapView.left - destPcxX;
            destPcxX = m_mapView.left;
            drawWidth -= srcX;
        }
        if (destPcxX + TEMP_PCX_WIDTH - m_mapView.left > m_mapView.right)
            drawWidth = m_mapView.right + m_mapView.left - destPcxX;

        UINT srcY = 0;
        if (destPcxY < m_mapView.top)
        {
            srcY = m_mapView.top - destPcxY;
            destPcxY = m_mapView.top;
            drawHeight -= srcY;
        }
        if (destPcxY + TEMP_PCX_HEIGHT - m_mapView.top > m_mapView.bottom)
            drawHeight = m_mapView.top + m_mapView.bottom - destPcxY;

        settings.rainDef->DrawToPcx16(0, settings.currentFrameToDraw, srcX, srcY, drawWidth, drawHeight, drawBuffer,
                                      destPcxX, destPcxY);
    }
}

static _LHF_(HooksInit)
{
    auto &rect = settings.m_mapView;
    rect.left = 8;                                   // right panel
    rect.top = 8;                                    // bottom panel
    rect.right = H3GameWidth::Get() - (800 - 592);   // right panel
    rect.bottom = H3GameHeight::Get() - (600 - 544); // bottom panel

    settings.rainDef = H3LoadedDef::Load("zrain00.def");
    settings.framesAmount = settings.rainDef->groups[0]->count;

    _PI->WriteLoHook(0x040F5AB, AdvMgr_BeforeObjectsDraw);
    _PI->WriteHiHook(0x040F5D7, THISCALL_, AdvMgr_TileObjectDraw);

    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool initialized = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!initialized)
        {
            initialized = true;
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::instanceName);
            Era::ConnectEra(hModule, dllText::instanceName);
            _PI->WriteLoHook(0x4EEAF2, HooksInit);
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
