
#ifndef HEADER_H_

#define HEADER_H_

#define DllImport extern "C" __declspec(dllimport)
#define DllExport extern "C" __declspec(dllexport)

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef H3API_SINGLE_HEADER
#define _H3API_PATCHER_X86_
#define H3API_SINGLE_HEADER

#include "H3API/single_header/H3API.hpp"

#endif // H3API_SINGLE_HEADER
// #include "H3API/include/patcher_x86.hpp"
#ifdef RESIZED_PCX
#include "DrawPcx16ResizedBicubic.h"
#endif // !1

#include "Era/era.h"
#include "Era/eraJson.hpp"
#ifdef ERA_MODLIST
#include "Era/eraModList.hpp"
#endif // ERA_MOD_LIST

#ifdef _WOG_
#include "WoG/NPC.h"
#include "WoG/WogClasses.h"
#endif // _WOG_

#ifdef NLOHMAN_JSON
#include "json.hpp"
#endif // !1

using namespace h3;
extern Patcher *globalPatcher;
extern PatcherInstance *_PI;

class IGamePatch
{

  protected:
    IGamePatch(PatcherInstance *pi) : _pi(pi)
    {
    }

    PatcherInstance *_pi = nullptr;

    BOOL m_isInited = false;
    BOOL m_isEnabled = false;

  public:
    void SetEnabled(bool state)
    {
        state ? _pi->ApplyAll() : _pi->UndoAll();
        m_isEnabled = state;
    }
    void Undo()
    {
        _pi->UndoAll();
    }

    // static IGamePatch * Get()  noexcept;
    virtual void CreatePatches() = 0;
};

class IPluginText
{

  private:
    // static std::vector< IPluginText*> texts;

  public:
    IPluginText()
    {
        // if (texts.empty())
        {
            // Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");
        }
        // texts.emplace_back(this);
        // Load();
    }

  public:
    // void __stdcall OnAfterReloadLanguageData(Era::TEvent* e)
    // static void __stdcall OnAfterReloadLanguageData(Era::TEvent* e);
    //{
    //	//for (auto&text: texts)
    //	{
    //	//	text->Load();
    //	}
    // }

  public:
    virtual void Load() = 0;
};

// std::vector< IPluginText*>IPluginText::texts;

#endif // !HEADER_H_
