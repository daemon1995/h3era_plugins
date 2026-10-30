#pragma once
#include <array>
#include <map>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

using BOOL = int;
using DWORD = unsigned long;
using HMODULE = void *;
constexpr BOOL TRUE = 1;
constexpr BOOL FALSE = 0;
enum eSecondary { BALLISTICS = 10, ARTILLERY = 20, FIRST_AID = 27 };
enum eSecSkillLevel { NONE, BASIC, ADVANCED, EXPERT };
enum eResource { GOLD = 6 };

struct H3MapItem { int objectType = 141; int objectSubtype = 51; unsigned setup = 0; };
struct H3Hero
{
    int id = 7;
    int owner = 3;
    char name[13] = "Test hero";
    std::array<signed char, 28> secSkill{}, secSkillPosition{};
    int secSkillCount = 0;
    int movement = 1000;
    int recalculations = 0;
    void AddResource(int resource, int amount) const;
    void RecalculateMovement() { ++recalculations; }
};
struct H3PictureCategories
{
    int skill;
    int level;
    static H3PictureCategories SecSkill(eSecondary skill, eSecSkillLevel level) { return {skill, level}; }
};
struct H3PictureVector
{
    std::vector<H3PictureCategories> values;
    void Add(H3PictureCategories picture) { values.push_back(picture); }
    size_t Size() const { return values.size(); }
};
struct H3String
{
    std::string value;
    static H3String Format(const char *format, ...)
    {
        char buffer[128];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        return {buffer};
    }
    const char *String() const { return value.c_str(); }
};
struct H3Player {int currentHero=-1;};
struct FakeMain
{
    struct Player : H3Player { struct Resources { int gold = 3000; } playerResources; } players[8];
    H3Hero *GetHero(int) {return nullptr;}
};

namespace fake
{
static FakeMain main;
static H3Hero *hero;
static std::map<std::string,int> assoc;
static int warfare = 0;
static int option = 1;
static std::array<int, 1000> options{};
static int legacyHandlerRegistered = 0;
static bool modernDialogs = true;
static bool confirm = true;
static bool removalFails = false;
static int receivers = 0;
static int questions = 0;
static int clickedItem = 30721;
static int mask = 0;
static size_t nextChoice = 0;
static std::vector<int> choices;
static std::vector<std::vector<H3PictureCategories>> pages;
static std::vector<std::string> messages;
static std::string command;
static void __stdcall SetMask(int value) { mask = value; }

static void Reset(H3Hero &newHero)
{
    assoc.clear();
    main = {};
    newHero = {};
    hero = &newHero;
    warfare = 0;
    option = 1;
    options.fill(0);
    legacyHandlerRegistered = 0;
    modernDialogs = confirm = true;
    removalFails = false;
    receivers = questions = mask = 0;
    clickedItem = 30721;
    nextChoice = 0;
    choices.clear(); pages.clear(); messages.clear(); command.clear();
}
}
static FakeMain *P_Main = &fake::main;
inline void H3Hero::AddResource(int resource, int amount) const
{
    assert(resource == eResource::GOLD);
    fake::main.players[owner].playerResources.gold += amount;
}
static char h3_TextBuffer[2048];
namespace libc { using std::sprintf; }
static HMODULE GetModuleHandleA(const char *) { return fake::modernDialogs ? fake::hero : nullptr; }
static void *GetProcAddress(HMODULE, const char *) { return reinterpret_cast<void *>(&fake::SetMask); }

namespace Era
{
union TComplexDialogOpts
{
    struct { unsigned closeTimeoutMsec : 16; unsigned msgType : 4; unsigned alignment : 4; unsigned internal : 8; };
    int value = 0;
};
static void *GetRealAddr(void *address)
{
    if (reinterpret_cast<size_t>(address) == 0x2771920)
    {
        fake::options[109] = fake::option;
        return fake::options.data();
    }
    assert(false && "Unexpected direct state address");
    return nullptr;
}
struct TEvent {};
static std::map<std::string,void(__stdcall *)(TEvent *)> handlers;
static void RegisterHandler(void(__stdcall *handler)(TEvent *),const char *name){handlers[name]=handler;}
static int GetAssocVarIntValue(const char *name)
{
    if (std::strcmp(name,"wog_193_enabled")==0) return fake::warfare;
    const auto it=fake::assoc.find(name);return it==fake::assoc.end()?0:it->second;
}
static void SetAssocVarIntValue(const char *name,int value){fake::assoc[name]=value;}
static std::string tr(const char *key, const std::vector<std::string> &parameters)
{
    assert(parameters.size() % 2 == 0);
    std::string result = key;
    for (const auto &part : parameters) result += " " + part;
    return result;
}
static void ExecErmCmd(const char *) { assert(false && "Market of Time must not invoke ERM"); }

}
namespace EraJS
{
static void read(const std::string &, bool &found) { found = true; }
static int readInt(const char *key)
{
    return std::strcmp(key, "RMG.objectGeneration.63.52.optionId") == 0 ? 108 : 0;
}
}
struct H3WindowManager
{
    static H3WindowManager *Get() { static H3WindowManager instance; return &instance; }
    int ClickedItemID() const { return fake::clickedItem; }
};
#include "ObjectExtenderStubs.h"
#include "WoGTableStubs.h"
struct H3Messagebox
{
    static void Show(const char *text) { fake::messages.push_back(text); }
    static BOOL Choice(const char *text, H3PictureCategories)
    {
        ++fake::questions;
        fake::messages.push_back(text);
        return fake::confirm;
    }
    static void Display(const char *text, const H3PictureVector &pictures, int options)
    {
        assert(((options >> 16) & 15) == 10);
        assert(pictures.Size() > 0 && pictures.Size() <= (fake::modernDialogs ? 8u : 2u));
        if (fake::modernDialogs) assert(fake::mask == (1 << pictures.Size()) - 1);
        assert(fake::nextChoice < fake::choices.size());
        const int choice = fake::choices[fake::nextChoice++];
        fake::clickedItem = choice < 0 ? 30721 : 30729 + choice;
        fake::pages.push_back(pictures.values);
        fake::messages.push_back(text);
    }
};
