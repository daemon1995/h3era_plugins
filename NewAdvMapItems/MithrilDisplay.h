#pragma once
using namespace h3;
namespace ERI
{

	class ExtendedResourcesInfo
		:public IGamePatch
	{

		const INT m_dlgTextId;
		const char* defName;
		int m_resbarLastHintItemId=-1;
		H3DlgItem* advMapHintControl;
		H3DlgItem* kingdomHintcontrol;
		//bool lastHintIsMitril;


	private:
		ExtendedResourcesInfo(PatcherInstance* _pi);

	private:
		virtual void CreatePatches() override;

	private:
		const BOOL ShowMithrilRMCHint(H3Msg* msg, H3DlgItem* hintZone) noexcept;

	private:

		static BOOL8 BuildMithril(H3ResourceBarPanel* resourceBarPanel, int textItemWidth, bool buildFrame = true);
		static _LHF_(OnAdvMgrDlgResBarCreate);
		static _LHF_(OnKingdomOverviewDlgResBarCreate);
		static _LHF_(OnResourceBarDlgUpdate);
		static _LHF_(BeforeHotseatMsgBox);
		static _LHF_(OnAdvMgrDlgRightClick);
		static int __stdcall KingdomOverviewDlgProc(HiHook* h, H3BaseDlg* dlg, H3Msg* msg);
	
	public:
		const int LastHintItemId() const noexcept;
		void SetLastHintItemId(const int itemId) noexcept;
		const INT MitrilItemId() noexcept;
		static ExtendedResourcesInfo& Get();


	};
}