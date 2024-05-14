#pragma once
using namespace h3;
namespace Era
{
	//int* yVar = (int*)0xA48D7C; // 1..100
}


class MithrilDisplay
{

	const INT dlgTextId;
	const char* defName;
	const char* popupText;
	H3DlgItem* advMapHintControl;
	H3DlgItem* kingdomHintcontrol;
	static MithrilDisplay* mithrilBar;
	//bool lastHintIsMitril;

private:
	int getMithrilAmout(int playerId = P_Game->GetPlayerID());

	static BOOL8 buildMithril(H3ResourceBarPanel* resourceBarPanel, int textItemWidth, bool buildFrame = true);
	static MithrilDisplay* Get();

	static _LHF_(OnAdvMgrDlgResBarCreate);
	static _LHF_(OnKingdomOverviewDlgResBarCreate);
	static _LHF_(OnResourceBarDlgUpdate);
	static _LHF_(BeforeHotseatMsgBox);
	static _LHF_(OnAdvMgrDlgRightClick);

	static int __stdcall KingdomOverviewDlgProc(HiHook* h, H3BaseDlg* dlg, H3Msg* msg);
	bool showMithrilDescription(H3Msg* msg, H3DlgItem* hintZone);
public:
	MithrilDisplay(PatcherInstance* _PI, const char* hint);
	static const char* text()  noexcept;
	static void setHooks(PatcherInstance* pi);

};
