#pragma once

extern Patcher* globalPatcher;
extern PatcherInstance* _PI;
using namespace h3;

struct alignas (4) Settings
{
	BOOL isEnabled;
	BOOL isHeld;
	int scanCode;
	int vKey;
	float height;
	float fcolorFill;
	float fcolorLoss;
	float fsaturation;
	
	H3String iniPath;
	H3String section;

	void reset();
	BOOL load();
	BOOL save();
	BOOL validateScanCode(eVKey scanCode) const noexcept;

};
struct DlgText
{
	LPCSTR hint;
	LPCSTR handlers;
	LPCSTR input;
	LPCSTR health;
	LPCSTR loss;
	LPCSTR height;
	LPCSTR density;
	LPCSTR dfltName;
	LPCSTR hotkey;
	LPCSTR wrong;
	LPCSTR enable;
	LPCSTR toggable;
	LPCSTR setHk;
	LPCSTR press;
	LPCSTR held;

};



struct SettingsDlg : public H3Dlg
{

	//Settings settings;

	H3DlgPcx* labelForHp = nullptr;
	H3DlgPcx* originalLabel = nullptr;

	struct HkHandler
	{
		H3DlgCustomButton* bttn = nullptr;
		H3DlgText* text = nullptr;
		H3DlgText* name = nullptr;

	} hk;


	DlgText* text = nullptr;
	BOOL needRedraw;
	//BOOL needKeyBoardListen;
	BOOL DialogProc(H3Msg& msg) override;

	H3String getVirtualKeyName(int vKey) const noexcept;


	static void __fastcall ColorFillScrollBarProc(INT32 id, H3BaseDlg* dlg);
	static void __fastcall ColorLossScrollBarProc(INT32 id, H3BaseDlg* dlg);
	static void __fastcall SaturationScrollBarProc(INT32 id, H3BaseDlg* dlg);
	static void __fastcall HeightScrollBarProc(INT32 id, H3BaseDlg* dlg);

	static void __fastcall ScrollBarGeneralProc(H3BaseDlg* dlg, INT32 scrollBarId, float& valuePtr);
	
	static  int __fastcall SettingsHotkeyCallback(H3Msg* msg) noexcept;

	static void HitPointsBarDraw(H3DlgItem* labelForHp)noexcept;
	static void LabelBarDraw(H3DlgItem* originalLabel) noexcept;
	SettingsDlg(int width, int height, Settings* incomingSettings, DlgText* text);
	virtual ~SettingsDlg();

};
class alignas (4) CombatHints
{

	static BOOL isInited;
	static CombatHints* combatHints;

	INT32 needRedraw = false;
	BOOL isEnabled = false;
	DlgText* dlgText = nullptr;

	static _LHF_(BeforebattleStackHintDraw);
	static _LHF_(BattleOptionsDlg);
	static _LHF_(BattleMgr_ProcessActionL);

	static  int __fastcall CombatOptionsCallback(H3Msg* msg) noexcept;

public:
	//static void Init();
	static void WindMgr_DrawColoredRect(int x, int y, int width, Settings* settings, bool lost = false)noexcept;

	static CombatHints& instance(PatcherInstance* _PI);

	CombatHints(CombatHints const&) = delete;        // Don't forget to disable copy
	void operator=(CombatHints const&) = delete;   // Don't forget to disable copy

private:
	CombatHints(PatcherInstance* _PI);
	CombatHints();
	~CombatHints();

};

