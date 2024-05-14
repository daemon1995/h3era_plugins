#pragma once
#include "..\..\headers\header.h"
#include "CombatHints.h"

struct alignas(4) Settings
{
	BOOL isEnabled;
	BOOL isHeld;
	int vKey;

	float height;
	float fcolorFill;
	float fcolorLoss;
	float fsaturation;

	H3Ini ini;// = nullptr;
	H3String iniPath;
	void reset();
	BOOL load();
	BOOL save();
	BOOL validateScanCode(eVKey scanCode) const noexcept;

};


struct DlgText
{
	LPCSTR hint;
	LPCSTR input;
	LPCSTR health;
	LPCSTR loss;
	LPCSTR height;
	LPCSTR density;
	LPCSTR default;
	LPCSTR hotkey;
	LPCSTR wrong;
	LPCSTR enable;
	LPCSTR toggable;
	LPCSTR held;

};
struct SettingsDlg : public H3Dlg
{

	//Settings settings;

	H3DlgPcx* labelForHp = nullptr;
	H3DlgPcx* originalLabel = nullptr;
	H3DlgCustomButton* hotkeyChoiceCallbackBttn = nullptr;
	H3DlgText* hotkeyName = nullptr;
	DlgText* text = nullptr;
	BOOL needRedraw;
	//BOOL needKeyBoardListen;
	BOOL DialogProc(H3Msg& msg) override;

	H3String getVirtualKeyName(int vKey) const noexcept;

	static void __fastcall ColorFillScrollBarProc(INT32 id, H3BaseDlg* dlg);
	static void __fastcall ColorLossScrollBarProc(INT32 id, H3BaseDlg* dlg);
	static void __fastcall SaturationScrollBarProc(INT32 id, H3BaseDlg* dlg);
	static void __fastcall HeightScrollBarProc(INT32 id, H3BaseDlg* dlg);

	static void __fastcall ScrollBarGeneralProc(H3BaseDlg* dlg, INT32 scrollBarId, INT32 txtId, float& valuePtr);

	static  int __fastcall SettingsHotkeyCallback(H3Msg* msg) noexcept;


	static void HitPointsBarDraw(H3DlgItem* labelForHp)noexcept;
	static void LabelBarDraw(H3DlgItem* originalLabel) noexcept;
	SettingsDlg(int width, int height, Settings* incomingSettings, DlgText* text);
	virtual ~SettingsDlg();

};

