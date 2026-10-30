#pragma once
#include <algorithm>
#include <array>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <functional>
#include <cassert>
#include <cstdio>
#include <cstdint>
using BOOL=int; using BOOL8=bool; using INT=int; using LPCSTR=const char *;
constexpr BOOL TRUE=1,FALSE=0;
enum class eTextColor { REGULAR,GOLD,HIGHLIGHT,SILVER }; enum class eTextAlignment { MIDDLE_LEFT,MIDDLE_CENTER,MIDDLE_RIGHT };
enum class eVKey { H3VK_ENTER,H3VK_ESCAPE }; enum class eMsgFlag { NONE }; enum eResource { GOLD=6 }; enum eArtifact : int {};
namespace NH3Dlg { namespace Text { static const char *SMALL="small",*MEDIUM="medium",*BIG="big"; }
namespace Assets { static const char *ARTIFACT_DEF="artifact.def",*RESOURCE_DEF="resource.def",*DLGBOX="dlgbox.def",*DIBOXBACK="diboxbck.pcx"; } }
struct H3ARGB888 { unsigned value=0; }; struct H3RGB565 { H3RGB565() {} H3RGB565(int,int,int) {} H3RGB565(H3ARGB888) {} };
struct H3BitMode { static int Get() { return 4; } };
struct H3LoadedPcx16
{
 int width,height; std::vector<H3ARGB888> pixels;
 static H3LoadedPcx16 *Create(int w,int h) { return new H3LoadedPcx16{w,h,std::vector<H3ARGB888>(w*h)}; }
 void Destroy() { delete this; }
 void FillRectangle(int,int,int,int,int,int,int) {} void DrawFrame(int,int,int,int,int,int,int) {}
 void DarkenArea(int,int,int,int,int) {} void FrameRegion(int,int,int,int,BOOL,int,BOOL) {}
 void SimpleFrameRegion(int,int,int,int) {} void SinkArea(int,int,int,int) {}
 H3ARGB888 GetPixel(int x,int y) { return pixels.at(y*width+x); }
 H3ARGB888 *GetPixel888(int x,int y) { return &pixels.at(y*width+x); }
 H3RGB565 *GetPixel565(int,int) { static H3RGB565 pixel; return &pixel; }
};
struct H3DefFrame { int width=128,height=64,frameWidth=128,frameHeight=64,marginLeft=0,marginTop=0; };
struct H3LoadedDef
{
 struct Group { int count=256; } group;
 Group *one=&group; Group **groups=&one; int groupsCount=1,widthDEF=128,heightDEF=26; H3DefFrame frame;
 H3DefFrame *GetGroupFrame(int,int) { return &frame; }
 void DrawToPcx16(int,int,int,int,int,int,H3LoadedPcx16*,int,int) {}
};
struct H3DefLoader { H3LoadedDef resource; H3DefLoader(const char*) {} H3LoadedDef *Get() { return &resource; } H3LoadedDef *operator->() { return Get(); } };
struct H3PcxLoader
{
 struct Pcx { int width=128,height=128; void DrawToPcx16(int,int,int,int,H3LoadedPcx16*,int,int,BOOL) {} } resource;
 H3PcxLoader(const char*) {} Pcx *Get() { return &resource; } Pcx *operator->() { return Get(); }
};
struct H3Msg {};
struct H3DlgItem
{
 int id; bool visible=true,active=true,shaded=false; std::string hint;
 H3DlgItem(int id) : id(id) {} virtual ~H3DlgItem() {}
 int GetID() const { return id; } const char *GetHint() const { return hint.c_str(); }
 void ShowActivate() { visible=active=true; } void HideDeactivate() { visible=active=false; }
 void DeActivate() { active=false; } void Activate() { active=true; } void Shade() { shaded=true; } void UnShade() { shaded=false; }
 void SetHints(const char *text,const char*,BOOL) { hint=text?text:h3_NullString; }
};
struct H3DlgText : H3DlgItem
{
 std::string text; H3DlgText(int id,const char *text):H3DlgItem(id),text(text?text:h3_NullString) {}
 void SetText(const char *value) { text=value?value:h3_NullString; }
};
struct H3DlgDefButton : H3DlgText
{
 std::string defName; int frame=0,clickFrame=1; std::vector<eVKey> hotkeys;
 H3DlgDefButton(int id,const char *def,const char *text=h3_NullString):H3DlgText(id,text),defName(def) {}
 static H3DlgDefButton *Create(int,int,int id,const char *def,int,int,BOOL,int) { return new H3DlgDefButton(id,def); }
 void SetFrame(int value) { frame=value; } void SetClickFrame(int value) { clickFrame=value; }
 void AddHotkey(eVKey key) { hotkeys.push_back(key); }
};
struct H3DlgCaptionButton : H3DlgDefButton
{
 H3DlgCaptionButton(int id,const char *def,const char *text):H3DlgDefButton(id,def,text) {}
 static H3DlgCaptionButton *Create(int,int,int id,const char *def,const char *text,const char*,int,int,BOOL,int,eTextColor)
 {return new H3DlgCaptionButton(id,def,text);}
};
struct H3DlgPcx16 : H3DlgItem
{
 H3LoadedPcx16 *surface=nullptr; H3DlgPcx16(int id):H3DlgItem(id) {}
 void SetPcx(H3LoadedPcx16 *value) { surface=value; }
};
struct H3DlgDef : H3DlgItem
{
 H3LoadedDef def; int frame=0; H3DlgDef(int id):H3DlgItem(id) {}
 H3LoadedDef *GetDef() { return &def; } void SetFrame(int value) { frame=value; }
 static H3DlgDef *Create(int,int,int id,const char*,int frame,int) {auto *p=new H3DlgDef(id);p->frame=frame;return p;}
};
struct H3DlgFrame : H3DlgItem
{
 H3DlgFrame(int id):H3DlgItem(id) {}
 static H3DlgFrame *Create(int,int,int,int,int id,H3RGB565) { return new H3DlgFrame(id); }
};
struct H3DlgHintBar { std::string text; void ShowMessage(const char *value) { text=value?value:h3_NullString; } };
struct H3GameWidth { static int Get() { return 800; } }; struct H3GameHeight { static int Get() { return 600; } };
struct H3Dlg
{
 int flags=0x12; bool stopped=false; std::vector<std::unique_ptr<H3DlgItem>> items; H3DlgHintBar hint;
 H3Dlg(int,int,int,int,BOOL,BOOL) {} virtual ~H3Dlg() {}
 virtual BOOL OnLeftClick(INT,H3Msg&) { return TRUE; } virtual BOOL OnRightClick(H3DlgItem*) { return TRUE; }
 virtual BOOL OnMouseHover(H3DlgItem*) { return TRUE; } virtual BOOL OnMouseWheel(INT) { return TRUE; }
 virtual BOOL OnKeyPress(eVKey,eMsgFlag) { return TRUE; }
 int GetX() { return 10; } int GetY() { return 20; } void Stop() { stopped=true; } void Start() {} void Redraw() {}
 H3DlgItem *AddItem(H3DlgItem *item) { items.emplace_back(item); return item; }
 void AddBackground(H3LoadedPcx16*) {}
 H3DlgPcx16 *CreatePcx16(int,int,int,int,int id,const char*) { auto *p=new H3DlgPcx16(id); AddItem(p); return p; }
 H3DlgText *CreateText(int,int,int,int,const char *text,const char*,eTextColor,int id,eTextAlignment)
 { auto *p=new H3DlgText(id,text); AddItem(p); return p; }
 H3DlgDef *CreateDef(int,int,int id,const char*,int frame) { auto *p=new H3DlgDef(id); p->frame=frame; AddItem(p); return p; }
 void CreateHint(int,int,int,int) {} H3DlgHintBar *GetHintBar() { return &hint; }
};
template<typename T> struct GameVector : std::vector<T> { unsigned Count() const { return unsigned(this->size()); } };
struct H3ObjectAttributes { int type=141,subtype=52; struct Name { const char *String() const { return "ZArtif1.def"; } } defName; };
struct H3MapItem { struct Drawing { unsigned sprite=0; }; GameVector<Drawing> objectDrawing; };
struct H3Position {};
struct H3Hero { char name[13]="Test hero"; };
struct FakeMain { struct Setup { GameVector<H3ObjectAttributes> objectAttributes; } mainSetup; };
static FakeMain fakeMain; static FakeMain *P_Main=&fakeMain;
struct H3ArtifactCount { static int Get() { return 256; } };
struct H3Artifact
{
 int id,modifier; H3Artifact(int id,int modifier):id(id),modifier(modifier) {}
 const char *GetName() const { static std::string name; name="Artifact "+std::to_string(id); return name.c_str(); }
};
struct H3String { std::string value; const char *String() const { return value.c_str(); } };
namespace fakeui { static bool confirm=false; static int descriptions=0; static std::function<void()> onConfirm;
inline void Describe(const H3Artifact*,H3String *text) { ++descriptions; text->value="Description"; } }
#define THISCALL_2(type,address,a,b) fakeui::Describe(a,b)
struct H3PictureCategories { static H3PictureCategories Artifact(eArtifact) { return {}; } };
struct H3Messagebox
{
 static void Show(const char*) {} static void RMB(const char*,H3PictureCategories) {}
 static BOOL Choice(const char*) { if (fakeui::onConfirm) fakeui::onConfirm(); return fakeui::confirm; }
};
struct FakePatcher { template<typename T> T VarGetValue(const char*,T fallback) { return fallback; } };
static FakePatcher fakePatcher; static FakePatcher *globalPatcher=&fakePatcher;
struct WindowManager { int resultItemID=123; }; static WindowManager window; static WindowManager *P_WindowManager=&window;
struct SoundManager { void ClickSound() {} }; static SoundManager sound; static SoundManager *P_SoundMgr=&sound;
static const char *P_ResourceName[]={"Wood","Mercury","Ore","Sulfur","Crystal","Gems","Gold"};
namespace Era
{
inline std::string tr(const char *key,const std::vector<std::string> &parameters={})
{ std::string s=key; for (auto &p:parameters) s+=" "+p; return s; }
}

struct H3CombatCreature; struct H3Army; struct H3Point; struct H3PictureVector;
