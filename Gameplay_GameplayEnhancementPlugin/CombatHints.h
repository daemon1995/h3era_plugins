#pragma once


using namespace h3;

namespace cmbhints
{
	struct Settings
	{
		BOOL isEnabled;
		BOOL isHeld;
		int scanCode;
		int vKey;
		union
		{

			struct
			{
				float fcolorFill;
				float fcolorLoss;
				float fsaturation;
				float height;

			};
			float values[4];
		};


		static constexpr LPCSTR iniPath = "Runtime/game_enhancement_mod.ini";
		static constexpr LPCSTR section = "CombatHints";

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
		LPCSTR warMachines;
		//DlgText();
	};



	struct SettingsDlg : public H3Dlg
	{

		//Settings settings;
	private:

		H3DlgPcx* labelForHp = nullptr;
		H3DlgPcx* originalLabel = nullptr;
		BOOL needRedraw;
		const DlgText* text = nullptr;

		struct HkHandler
		{
			H3DlgCustomButton* bttn = nullptr;
			H3DlgText* text = nullptr;
			H3DlgText* name = nullptr;

		} hk;


	public:
		SettingsDlg(int width, int height, Settings* incomingSettings, DlgText* text);
		virtual ~SettingsDlg();
	private:
		Settings* settings = nullptr;
		//BOOL needKeyBoardListen;
		BOOL DialogProc(H3Msg& msg) override;

		H3String getVirtualKeyName(int vKey) const noexcept;

	private:

		static void __fastcall ColorFillScrollBarProc(INT32 id, H3BaseDlg* dlg);
		static void __fastcall ColorLossScrollBarProc(INT32 id, H3BaseDlg* dlg);
		static void __fastcall SaturationScrollBarProc(INT32 id, H3BaseDlg* dlg);
		static void __fastcall HeightScrollBarProc(INT32 id, H3BaseDlg* dlg);

		static void __fastcall ScrollBarGeneralProc(H3BaseDlg* dlg, INT32 scrollBarId, float& valuePtr);

		static  int __fastcall SettingsHotkeyCallback(H3Msg* msg) noexcept;

		void HitPointsBarDraw() noexcept;
		void LabelBarDraw() noexcept;


	};

	class CombatHints : public IGamePatch
	{

		static CombatHints* instance;

		BOOL needRedraw = false;
		BOOL isEnabled = false;
	public:
		Settings settings;


	private:
		CombatHints();
		virtual void CreatePatches() noexcept final;

	private:
		static _LHF_(BeforeBattleStackHintDraw);
		//	static _LHF_(BattleOptionsDlg);
		static _LHF_(BattleMgr_ProcessActionL);
		static void __stdcall BattleOptionsDlg_Show(HiHook* h, H3BaseDlg* dlg);// 004682C0

	public:

		static  int __fastcall CombatOptionsCallback(H3Msg* msg) noexcept;

		//static void Init();
		static void WindMgr_DrawColoredRect(const int x, const int y, const  int width, const  int height, const  Settings* settings, const BOOL lost = false)noexcept;

		static CombatHints& Get();// (PatcherInstance* _PI);

	private:

	};
}
